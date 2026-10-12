/**
 * TLB: For simplicity and speed (in emulator), we use direct map TLB
 */

#include "tlb.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bus/bus.h"
#include "cpu/mmu.h"
#include "cpu/riscv.h"
#include "log.h"

#define ASID_OFFSET 44
#define ASID_WIDTH  16
#define ASID_MASK   ((1 << ASID_WIDTH) - 1)

#define SATP_MODE_OFFSET 60

/**
 * void all the TLB entries
 */
void tlb_void(tlb_t *tlb) {
    memset(tlb->tlb, 0, sizeof(tlb_entry_t) * TLB_SIZE);
}

/**
 * Initialize tlb
 */
int tlb_init(tlb_t *tlb) {
    tlb_void(tlb);
    return 0;
}

/**
 * Look up TLB: This implement a direct mapped TLB
 */
mmu_translation_type tlb_lookup(tlb_t *tlb, uint64_t va, uint64_t *pa, bus_t *bus, uint64_t satp, uint64_t mstatus,
                                mmu_access_mode mode, priv_mode_t priv) {

#ifdef USE_TLB

    mmu_translation_type result;
    pte_flag_t           flag;

    uint16_t asid   = (satp >> ASID_OFFSET) & ASID_MASK;
    uint64_t vpn    = va >> 12;
    uint64_t offset = va & 0xFFF;
    uint64_t index  = vpn & TLB_MASK;

    if (bypass_address_translation(satp, mstatus, mode, priv)) {
        *pa = va;
        return MMU_VALID;
    }

    if (tlb->tlb[index].valid) {
        if (tlb->tlb[index].asid == asid && tlb->tlb[index].tag == vpn) {
            *pa = (tlb->tlb[index].ppn << 12) | offset;
            // check basic permission
            if (!pte_permission_check(tlb->tlb[index].flag, mstatus, mode, priv)) {
                switch (mode) {
                case MMU_EXECUTE:
                    return MMU_INST_PAGE_FAULT;
                case MMU_READ:
                    return MMU_LOAD_PAGE_FAULT;
                case MMU_WRITE:
                    return MMU_STORE_AMO_PAGE_FAULT;
                case MMU_AMO:
                    return MMU_STORE_AMO_PAGE_FAULT;
                }
            }
            tlb->hit++;
            return MMU_VALID;
        }
    }

    tlb->miss++;
    // No matching entry found, need to walk page table and add it to TLB
    result = mmu_translation(va, pa, bus, satp, mstatus, mode, priv, &flag);
    if (result != MMU_VALID) {
        return result;
    }

    // Add the translation result back to the TLB
    tlb->tlb[index].asid  = asid;
    tlb->tlb[index].tag   = vpn;
    tlb->tlb[index].ppn   = *pa >> 12;
    tlb->tlb[index].valid = true;
    tlb->tlb[index].flag  = flag;

    return MMU_VALID;

#else

    return mmu_translation(va, pa, bus, satp, mstatus, mode, priv, NULL);

#endif
}

void tlb_stats_report(const tlb_t *tlb, const char *name) {

    uint64_t total    = tlb->hit + tlb->miss;
    double   hit_rate = total ? 100.0 * (double)tlb->hit / (double)total : 0.0;

    LOG_INFO("%s", name);
    LOG_INFO("    Hits:     %ld", tlb->hit);
    LOG_INFO("    Misses:   %ld", tlb->miss);
    LOG_INFO("    Hit Rate: %.2f%%", hit_rate);
}

