#ifndef MMU_H
#define MMU_H

#include <stdint.h>

#include "bus/bus.h"
#include "cpu/riscv.h"

typedef enum {
    MMU_READ,
    MMU_WRITE,
    MMU_AMO,
    MMU_EXECUTE,
} mmu_access_mode;

typedef enum {
    MMU_VALID                  = 0,
    MMU_INST_ACCESS_FAULT      = 1,
    MMU_LOAD_ACCESS_FAULT      = 5,
    MMU_STORE_AMO_ACCESS_FAULT = 7,
    MMU_INST_PAGE_FAULT        = 12,
    MMU_LOAD_PAGE_FAULT        = 13,
    MMU_STORE_AMO_PAGE_FAULT   = 15,
} mmu_translation_type;

mmu_translation_type mmu_translation(uint64_t va, uint64_t *pa, bus_t *bus, uint64_t satp, uint64_t mstatus,
                                     mmu_access_mode mode, priv_mode_t priv);

#endif
