#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

static inline void mmio_write8(uint64_t addr, uint8_t data) {
    *((volatile uint8_t *)addr) = data;
}

static inline uint8_t mmio_read8(uint64_t addr) {
    return *((volatile uint8_t *)addr);
}

#define REG_FIELD_GET(reg, mask, offset) (((reg) & (mask)) >> (offset))

#endif
