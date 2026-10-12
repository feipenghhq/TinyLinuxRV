#ifndef TLB_H
#define TLB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/bus.h"
#include "cpu/mmu.h"
#include "cpu/riscv.h"

#define USE_TLB

// Need to be power of 2
#define TLB_SIZE 256
#define TLB_MASK (TLB_SIZE - 1)

typedef struct {
    bool       valid;
    pte_flag_t flag;
    uint16_t   asid;
    uint64_t   tag;
    uint64_t   ppn;
} tlb_entry_t;

typedef struct {
    tlb_entry_t tlb[TLB_SIZE];

    // statistic
    uint64_t hit;
    uint64_t miss;
} tlb_t;

int  tlb_init(tlb_t *tlb);
void tlb_void(tlb_t *tlb);

mmu_translation_type tlb_lookup(tlb_t *tlb, uint64_t va, uint64_t *pa, bus_t *bus, uint64_t satp, uint64_t mstatus,
                                mmu_access_mode mode, priv_mode_t priv);

void tlb_stats_report(const tlb_t *tlb, const char *name);

#endif
