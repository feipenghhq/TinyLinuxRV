#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

static inline void mmio_write8(uint64_t addr, uint8_t data) {
    *((volatile uint8_t *)addr) = data;
}

static inline uint8_t mmio_read8(uint64_t addr) {
    return *((volatile uint8_t *)addr);
}

static inline void mmio_write32(uint64_t addr, uint32_t data) {
    *((volatile uint32_t *)addr) = data;
}

static inline uint32_t mmio_read32(uint64_t addr) {
    return *((volatile uint32_t *)addr);
}

static inline void mmio_write64(uint64_t addr, uint64_t data) {
    *((volatile uint64_t *)addr) = data;
}

static inline uint64_t mmio_read64(uint64_t addr) {
    return *((volatile uint64_t *)addr);
}

#define REG_FIELD_GET(reg, mask, offset) (((reg) & (mask)) >> (offset))

#endif
