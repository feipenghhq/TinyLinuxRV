#include "cpu/csr.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "log.h"

// ----------------------------------------------
// Helper variable and macro
// ----------------------------------------------

enum {
    // Machine Trap Setup (MRW)
    CSR_MSTATUS    = 0x300,
    CSR_MISA       = 0x301,
    CSR_MEDELEG    = 0x302,
    CSR_MIDELEG    = 0x303,
    CSR_MIE        = 0x304,
    CSR_MTVEC      = 0x305,
    CSR_MCOUNTEREN = 0x306,
    // Machine Trap Handling (MRW)
    CSR_MSCRATCH = 0x340,
    CSR_MEPC     = 0x341,
    CSR_MCAUSE   = 0x342,
    CSR_MTVAL    = 0x343,
    CSR_MIP      = 0x344,
    // Machine Information Registers (MRO)
    CSR_MVENDORID = 0xF11,
    CSR_MARCHID   = 0xF12,
    CSR_MIMPID    = 0xF13,
    CSR_MHARTID   = 0xF14,
}

csr_addr;

#define CSR_DECODE_RW(addr, name) \
    case addr:                    \
        csr_reg = &csr->name;     \
        break;

#define CSR_DECODE_RO(addr, name) \
    case addr:                    \
        read_only = true;         \
        csr_reg   = &csr->name;   \
        break;

static inline uint64_t csr_field_get(uint64_t *csr, int pos, int width) {
    uint64_t mask = (1U << width) - 1;
    return (*csr >> pos) & mask;
}

static inline void csr_field_set(uint64_t *csr, int pos, int width, uint64_t value) {
    uint64_t mask          = (1U << width) - 1;
    uint64_t mask_shifted  = mask << pos;
    uint64_t value_shifted = value << pos;

    *csr = (*csr & ~mask_shifted) | value_shifted;
}

// Note: Other special case not implemented
// - mepc: mepc[0] is always read as 0

// ----------------------------------------------
// Local helper function
// ----------------------------------------------

static void misa_default(csr_t *csr) {
    csr->misa = 0;
    // MXL field need to be set to 2 to indicate 64 bit ISA
    csr_field_set(&csr->misa, 62, 2, 2);
    csr_field_set(&csr->misa, 0, 1, 1);  // A
    csr_field_set(&csr->misa, 8, 1, 1);  // I
    csr_field_set(&csr->misa, 12, 1, 1); // M
}

// ----------------------------------------------
// Main function
// ----------------------------------------------

void csr_init(csr_t *csr) {
    memset(csr, 0, sizeof(*csr));
    // -- MISA --
    // MXL field need to be set to 2 to indicate 64 bit ISA
    misa_default(csr);
    // -- MSTATUS --
    // MPP field should be set to 3 to indicate starting at Machine Mode.
}

int csr_access(csr_t *csr, int addr, int op, const uint64_t value, uint64_t *rdata, bool read_csr, bool write_csr) {
    uint64_t *csr_reg       = NULL;
    bool      read_only     = false;
    bool      imp_read_only = false;

    switch (addr) {
        // Machine Trap Setup (MRW)
        CSR_DECODE_RW(CSR_MSTATUS, mstatus)
        CSR_DECODE_RW(CSR_MISA, misa)
        CSR_DECODE_RW(CSR_MEDELEG, medeleg)
        CSR_DECODE_RW(CSR_MIDELEG, mideleg)
        CSR_DECODE_RW(CSR_MIE, mie)
        CSR_DECODE_RW(CSR_MTVEC, mtvec)
        // Machine Trap Handling (MRW)
        CSR_DECODE_RW(CSR_MCOUNTEREN, mcounteren)
        CSR_DECODE_RW(CSR_MSCRATCH, mscratch)
        CSR_DECODE_RW(CSR_MEPC, mepc)
        CSR_DECODE_RW(CSR_MCAUSE, mcause)
        CSR_DECODE_RW(CSR_MTVAL, mtval)
        CSR_DECODE_RW(CSR_MIP, mip)
        // Machine Information Registers (MRO)
        CSR_DECODE_RO(CSR_MVENDORID, mvendorid)
        CSR_DECODE_RO(CSR_MARCHID, marchid)
        CSR_DECODE_RO(CSR_MIMPID, mimpid)
        CSR_DECODE_RO(CSR_MHARTID, mhartid)
    default: {
        LOG_ERROR("CPU: Access imp CSR register: %x", addr);
        return 1;
    }
    }

    // write to read only CSR. Should cause illegal instruction
    if (csr_reg && write_csr && read_only) {
        return 1;
    }

    // process read operation first
    if (csr_reg && read_csr) {
        switch (addr) {
        // special case for MSTATUS
        // - MPP field always return 3 as we only implemented M mode right now
        case CSR_MSTATUS:
            *rdata = *csr_reg | 0x1800;
            break;
        //case CSR_MIP:
        //    *rdata = *csr_reg;
        //    break;
        default:
            *rdata = *csr_reg;
            break;
        }
    }

    // In our implementation, we make these CSR read only to Software
    switch (addr) {
    case CSR_MISA:
    case CSR_MIP:
        imp_read_only = true;
    }

    // process write operation
    if (csr_reg && write_csr && !imp_read_only) {
        switch (op) {
        // swap the csr and the value
        case CSR_OP_RW: {
            *csr_reg = value;
            break;
        }
        // set the corresponding bit
        case CSR_OP_RS: {
            *csr_reg |= value;
            break;
        }
        // clear the corresponding bit
        case CSR_OP_RC: {
            *csr_reg &= ~value;
            break;
        }
        }
    }

    return 0;
}

uint64_t trap_enter(csr_t *csr, uint64_t cause, uint64_t mtval, uint64_t pc) {
    uint64_t trap_vec_base;
    uint64_t trap_vec_mode;
    uint64_t trap_vec;
    uint64_t mie;
    uint64_t interrupt;

    // update mstatus
    // Currently only support machine mode
    // - MPIE is updated with MIE
    // - MIE is updated to 0
    // - MPP is updated to the privilege mode before the trap happens
    mie = csr_field_get(&csr->mstatus, 3, 1);
    csr_field_set(&csr->mstatus, 7, 1, mie);
    csr_field_set(&csr->mstatus, 3, 1, 0);
    csr_field_set(&csr->mstatus, 11, 2, 3);

    // update mtval
    csr->mtval = mtval;

    // update mcause
    csr->mcause = cause;

    // update mepc
    // since we only support only IALIGN=32, the two low bits (mepc[1:0]) are always zero.
    csr->mepc = pc & ~UINT64_C(0x3);

    // return the trap vector
    interrupt     = csr_field_get(&csr->mcause, 63, 1);
    trap_vec_base = csr->mtvec & ~UINT64_C(0x3);
    trap_vec_mode = csr->mtvec & 0x3;
    if (trap_vec_mode == 1 && interrupt) {
        trap_vec = trap_vec_base + 4 * (uint64_t)cause;
    } else {
        trap_vec = trap_vec_base;
    }
    return trap_vec;
}

uint64_t trap_exit(csr_t *csr) {
    uint64_t mpie;

    // update mstatus
    // - restore mie and set mpie to 1
    mpie = csr_field_get(&csr->mstatus, 7, 1);
    csr_field_set(&csr->mstatus, 3, 1, mpie);
    csr_field_set(&csr->mstatus, 7, 1, 1);
    // - set MPP to 0. For now set it to 3 as we only support M mode
    csr_field_set(&csr->mstatus, 11, 2, 3);

    // return mepc
    return csr->mepc & ~UINT64_C(0x3);
}

bool is_trap_enable(csr_t *csr, interrupt_code_t id, int mode) {
    assert(mode == 1 || mode == 3);

    bool mstatus_mie = csr_field_get(&csr->mstatus, mode, 1);
    bool mie_enable  = csr_field_get(&csr->mie, (int)id, 1);
    return mstatus_mie & mie_enable;
}

void csr_interrupt_update(csr_t *csr, bool eip, bool sip, bool tip) {
    // update the MIP CSR register

    // We currently only support Machine mode
    csr_field_set(&csr->mip, INT_MEIP, 1, eip);
    csr_field_set(&csr->mip, INT_MSIP, 1, sip);
    csr_field_set(&csr->mip, INT_MTIP, 1, tip);
}
