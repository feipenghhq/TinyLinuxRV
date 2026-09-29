#include "mmu.h"

#include <stdbool.h>
#include <stdint.h>

#include "bus/bus.h"
#include "cpu/riscv.h"

//---------------------------------------------------------
// Useful data type
//---------------------------------------------------------

typedef struct {
    uint16_t page_offset;
    uint16_t vpn[3];
} Sv39_va_s;

typedef struct {
    uint16_t page_offset;
    uint64_t ppn[3];
} Sv39_pa_s;

typedef struct {
    bool     v;
    bool     r;
    bool     w;
    bool     x;
    bool     u;
    bool     g;
    bool     a;
    bool     d;
    uint8_t  rsw;
    uint64_t ppn;
    uint32_t ppnx[3];
    uint8_t  pbmt;
    bool     n;
} Sv39_pte_s;

typedef enum {
    PTE_INVALID,
    PTE_LEAF,
    PTE_NONLEAF,
    PTE_PAGE_FAULT,
} pte_check;

typedef enum {
    WALKER_VALID,
    WALKER_ACCESS_FAULT,
    WALKER_PAGE_FAULT,
} walker_result;

//---------------------------------------------------------
// Related Macro
//---------------------------------------------------------

#define PAGESIZE 4096
#define PPN_MASK 0xFFFFFFFFFFF

#define LEVELS_SV39  3
#define PTESIZE_SV39 8

#define PAGE_OFFSET_MASK 0xFFF
#define SV39_VPNx_MASK   0x1FF
#define SV39_PPN0_MASK   0x1FF
#define SV39_PPN1_MASK   0x1FF
#define SV39_PPN2_MASK   0x3FFFFFF

// Sv39 virtual address
#define SV39_VA_VPN0_OFFSET 12
#define SV39_VA_VPN1_OFFSET 21
#define SV39_VA_VPN2_OFFSET 30

// Sv39 physical address
#define SV39_PA_PPN0_OFFSET 12
#define SV39_PA_PPN1_OFFSET 21
#define SV39_PA_PPN2_OFFSET 30

// Sv39 page table entry
#define PTE_V_OFFSET    0
#define PTE_R_OFFSET    1
#define PTE_W_OFFSET    2
#define PTE_X_OFFSET    3
#define PTE_U_OFFSET    4
#define PTE_G_OFFSET    5
#define PTE_A_OFFSET    6
#define PTE_D_OFFSET    7
#define PTE_RSW_OFFSET  8
#define PTE_PPN_OFFSET  10
#define PTE_PPN0_OFFSET 10
#define PTE_PPN1_OFFSET 19
#define PTE_PPN2_OFFSET 28
#define PTE_PBMT_OFFSET 61
#define PTE_N_OFFSET    63

// CSR related
#define MSTATUS_MPRV_OFFSET 17
#define MSTATUS_SUM_OFFSET  18
#define MSTATUS_MXR_OFFSET  19
#define MSTATUS_MPP_OFFSET  11

#define SATP_MODE_OFFSET 60

//---------------------------------------------------------
// Local helper function
//---------------------------------------------------------

static inline bool parse_sv39_va(uint64_t va, Sv39_va_s *sv39_va) {

    // Instruction fetch addresses and load and store effective addresses,
    // which are 64 bits, must have bits 63–39 all equal to bit 38, or else a page-fault exception will occur.
    uint64_t bit63_39  = va >> 39;
    uint64_t bit38     = (va >> 38) & 0x1;
    bool     bit_equal = true;
    if (bit38 == 0) {
        bit_equal = (bit63_39 == 0);
    } else {
        bit_equal = (bit63_39 == 0x1FFFFFF);
    }
    if (!bit_equal) {
        return false;
    }

    sv39_va->page_offset = va & PAGE_OFFSET_MASK;

    sv39_va->vpn[0] = (va >> SV39_VA_VPN0_OFFSET) & SV39_VPNx_MASK;
    sv39_va->vpn[1] = (va >> SV39_VA_VPN1_OFFSET) & SV39_VPNx_MASK;
    sv39_va->vpn[2] = (va >> SV39_VA_VPN2_OFFSET) & SV39_VPNx_MASK;

    return true;
}

static inline void parse_sv39_pte(uint64_t pte, Sv39_pte_s *sv39_pte) {
    sv39_pte->v    = (pte >> PTE_V_OFFSET) & 0x1;
    sv39_pte->r    = (pte >> PTE_R_OFFSET) & 0x1;
    sv39_pte->w    = (pte >> PTE_W_OFFSET) & 0x1;
    sv39_pte->x    = (pte >> PTE_X_OFFSET) & 0x1;
    sv39_pte->u    = (pte >> PTE_U_OFFSET) & 0x1;
    sv39_pte->g    = (pte >> PTE_G_OFFSET) & 0x1;
    sv39_pte->a    = (pte >> PTE_A_OFFSET) & 0x1;
    sv39_pte->d    = (pte >> PTE_D_OFFSET) & 0x1;
    sv39_pte->rsw  = (pte >> PTE_RSW_OFFSET) & 0x3;
    sv39_pte->pbmt = (pte >> PTE_PBMT_OFFSET) & 0x3;
    sv39_pte->n    = (pte >> PTE_N_OFFSET) & 0x1;
    sv39_pte->ppn  = (pte >> PTE_PPN_OFFSET) & PPN_MASK;

    sv39_pte->ppnx[0] = (pte >> PTE_PPN0_OFFSET) & SV39_PPN0_MASK;
    sv39_pte->ppnx[1] = (pte >> PTE_PPN1_OFFSET) & SV39_PPN1_MASK;
    sv39_pte->ppnx[2] = (pte >> PTE_PPN2_OFFSET) & SV39_PPN2_MASK;
}

//---------------------------------------------------------
// MMU function
//---------------------------------------------------------

/**
 * Walk page table
 * A direct translation of the Virtual Address Translation Process from the RISC-V Spec
 */
static walker_result page_table_walk_sv39(uint64_t va, uint64_t *pa, bus_t *bus, uint64_t satp, uint64_t sstatus,
                                          mmu_access_mode mode, priv_mode_t priv) {

    uint64_t   satp_ppn;
    uint64_t   sstatus_sum;
    uint64_t   sstatus_mxr;
    uint64_t   pte_base_addr;
    uint64_t   pte_addr, pte;
    Sv39_pte_s pte_s;
    Sv39_pa_s  pa_s;
    Sv39_va_s  va_s;
    int        level, xwr;

    // extract field from CSR
    satp_ppn    = satp & PPN_MASK;
    sstatus_sum = (sstatus >> MSTATUS_SUM_OFFSET) & 0x1;
    sstatus_mxr = (sstatus >> MSTATUS_MXR_OFFSET) & 0x1;

    // parse va
    if (!parse_sv39_va(va, &va_s)) {
        return WALKER_PAGE_FAULT;
    }

    // step 1
    pte_base_addr = satp_ppn * PAGESIZE;
    level         = LEVELS_SV39 - 1;

    // step 2
step_2:
    pte_addr = pte_base_addr + va_s.vpn[level] * PTESIZE_SV39;
    if (bus_read(bus, pte_addr, 8, &pte) != 0) {
        return WALKER_ACCESS_FAULT;
    }

    // step 3
    parse_sv39_pte(pte, &pte_s);
    xwr = pte_s.x << 2 | pte_s.w << 1 | pte_s.r;
    if (!pte_s.v || (!pte_s.r && pte_s.w) || (xwr == 2 || xwr == 6)) {
        return WALKER_PAGE_FAULT;
    }

    // step 4
    if (!(pte_s.r || pte_s.x)) {
        if (level == 0) {
            return WALKER_PAGE_FAULT;
        } else {
            level--;
            pte_base_addr = pte_s.ppn * PAGESIZE;
            goto step_2;
        }
    }

    // step 5
    for (int i = 0; i < level; i++) {
        // check for misaligned superpage
        if (pte_s.ppnx[i] != 0) {
            return WALKER_PAGE_FAULT;
        }
    }

    // step 6 + step 7 + step 8
    // Check privilege
    // U-mode software may only access the page when U=1
    if (priv == PRIV_U && !pte_s.u) {
        return WALKER_PAGE_FAULT;
    }
    // S-mode software may access page when U = 1 if sstatus.sum bit is set
    if (priv == PRIV_S && pte_s.u && !sstatus_sum) {
        return WALKER_PAGE_FAULT;
    }
    // S-mode software may NOT execute code on pages with U = 1
    if (priv == PRIV_S && pte_s.u && mode == MMU_EXECUTE) {
        return WALKER_PAGE_FAULT;
    }
    // Check access mode
    switch (xwr) {
    case 1: // read only
        if (mode == MMU_EXECUTE || mode == MMU_WRITE)
            return WALKER_PAGE_FAULT;
        break;

    case 3: // read-write
        if (mode == MMU_EXECUTE)
            return WALKER_PAGE_FAULT;
        break;
    case 4: // execute-only
        if (mode == MMU_WRITE)
            return WALKER_PAGE_FAULT;
        // When MXR=0, only loads from pages marked readable (R=1 in Sv32 page table entry) will succeed. When
        // MXR=1, loads from pages marked either readable or executable (R=1 or X=1) will succeed.
        if (mode == MMU_READ && !sstatus_mxr)
            return WALKER_PAGE_FAULT;
        break;
    case 5: // read-execute
        if (mode == MMU_WRITE)
            return WALKER_PAGE_FAULT;
        break;
    }

    // step 9
    // Implement Svade behavior
    if (!pte_s.a) {
        return WALKER_PAGE_FAULT;
    }
    if (mode == MMU_WRITE && !pte_s.d) {
        return WALKER_PAGE_FAULT;
    }

    // step 10
    // translation is successful !!! :)
    pa_s.page_offset = va_s.page_offset;
    if (level > 0) {
        for (int i = 0; i < level; i++)
            pa_s.ppn[i] = va_s.vpn[i];
    }
    for (int i = level; i < LEVELS_SV39; i++)
        pa_s.ppn[i] = pte_s.ppnx[i];

    *pa = pa_s.page_offset | (pa_s.ppn[0] << SV39_PA_PPN0_OFFSET) | (pa_s.ppn[1] << SV39_PA_PPN1_OFFSET) |
          (pa_s.ppn[2] << SV39_PA_PPN2_OFFSET);

    return WALKER_VALID;
}

/**
 * Translate the virtual address to physical addres
 */
mmu_translation_type mmu_translation(uint64_t va, uint64_t *pa, bus_t *bus, uint64_t satp, uint64_t mstatus,
                                     mmu_access_mode mode, priv_mode_t priv) {

    uint64_t satp_mode    = (satp >> SATP_MODE_OFFSET) & 0xF;
    uint64_t mstatus_mprv = (mstatus >> MSTATUS_MPRV_OFFSET) & 0x1;
    uint64_t mstatus_mpp  = (mstatus >> MSTATUS_MPP_OFFSET) & 0x3;

    // CSR insure only Bare and Sv39 are enabled.
    if (satp_mode == 0) {
        *pa = va;
        return MMU_VALID;
    }

    // else Sv39 mode

    // Instruction fetch in Machine mode does not translate the address
    if (priv == PRIV_M && mode == MMU_EXECUTE) {
        *pa = va;
        return MMU_VALID;
    }

    // Load/Store in Machine mode used effective privilege
    priv_mode_t effective_priv = priv;
    if (mstatus_mprv) {
        effective_priv = (priv_mode_t)mstatus_mpp;
    }

    if (effective_priv == PRIV_M) {
        *pa = va;
        return MMU_VALID;
    }

    // For now we always need to walk page table as we don't have any TLB :(
    walker_result result = page_table_walk_sv39(va, pa, bus, satp, mstatus, mode, effective_priv);

    if (result == WALKER_VALID) {
        return MMU_VALID;
    } else if (result == WALKER_ACCESS_FAULT) {
        switch (mode) {
        case MMU_EXECUTE:
            return MMU_INST_ACCESS_FAULT;
        case MMU_READ:
            return MMU_LOAD_ACCESS_FAULT;
        case MMU_WRITE:
            return MMU_STORE_AMO_ACCESS_FAULT;
        }
    } else {
        switch (mode) {
        case MMU_EXECUTE:
            return MMU_INST_PAGE_FAULT;
        case MMU_READ:
            return MMU_LOAD_PAGE_FAULT;
        case MMU_WRITE:
            return MMU_STORE_AMO_PAGE_FAULT;
        }
    }
    return MMU_VALID;
}
