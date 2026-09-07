#include "clint.h"

#include <stdint.h>

#include "clint_reg.h"
#include "mmio.h"

void clint_set_msip(uint64_t base) {
    mmio_write32(base + CLINT_MSIP_OFFSET, 0x1);
}

void clint_clear_msip(uint64_t base) {
    mmio_write32(base + CLINT_MSIP_OFFSET, 0x0);
}

void clint_set_mtime(uint64_t base, uint64_t value) {
    mmio_write64(base + CLINT_MTIME_OFFSET, value);
}

uint64_t clint_read_mtime(uint64_t base) {
    return mmio_read64(base + CLINT_MTIME_OFFSET);
}

void clint_set_mtimecmp(uint64_t base, uint64_t value) {
    mmio_write64(base + CLINT_MTIMECMP_OFFSET, value);
}

uint64_t clint_read_mtimecmp(uint64_t base) {
    return mmio_read64(base + CLINT_MTIMECMP_OFFSET);
}
