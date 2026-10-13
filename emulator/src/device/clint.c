#define _POSIX_C_SOURCE 200809L

#include "clint.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "utils/log.h"

long get_time(void) {
    struct timespec tp;

    if (clock_gettime(CLOCK_MONOTONIC, &tp) != 0) {
        LOG_ERROR("CLINT: Failed to get time from host");
    }
    return tp.tv_sec * 1000000000 + tp.tv_nsec;
}

void clint_reset(clint_t *clint) {
    clint->MSIP         = false;
    clint->MTIP         = false;
    clint->reg.msip     = 0;
    clint->reg.mtime    = 0;
    clint->reg.mtimecmp = UINT64_MAX;
}

void clint_init(clint_t *clint, uint64_t base) {
    clint->base = base;
    clint_reset(clint);
    clint->prev_time_ns = get_time();
}

int clint_write(clint_t *clint, uint64_t addr, size_t size, const void *data) {
    uint64_t offset = addr - clint->base;

    switch (offset) {
    case 0: { // msip
        // size should be 4 for msip
        if (size != 4) {
            LOG_ERROR("Clint msip only support 4 byte access. Received %zu byte", size);
            return -1;
        }
        uint32_t value;
        memcpy(&value, data, 4);
        clint->reg.msip = value & 0x1;
        return 0;
    }
    case 0x4000: { // mtimecmp
        // size should be 8 for mtimecmp
        if (size != 8) {
            LOG_ERROR("Clint mtimecmp only support 8 byte access. Received %zu byte", size);
            return -1;
        }
        memcpy(&clint->reg.mtimecmp, data, 8);
        return 0;
    }
    case 0xBFF8: { // mtime
        // size should be 8 for mtime
        if (size != 8) {
            LOG_ERROR("Clint mtime only support 8 byte access. Received %zu byte", size);
            return -1;
        }
        memcpy(&clint->reg.mtime, data, 8);
        return 0;
    }
    default: {
        LOG_ERROR("Unsupported write address in clint: %lx", addr);
        return -1;
    }
    }
    return 0;
}

int clint_read(clint_t *clint, uint64_t addr, size_t size, void *data) {
    uint64_t offset = addr - clint->base;

    switch (offset) {
    case 0: { // msip
        // size should be 4 for msip
        if (size != 4) {
            LOG_ERROR("Clint msip only support 4 byte access. Received %zu byte", size);
            return -1;
        }
        memcpy(data, &clint->reg.msip, 4);
        return 0;
    }
    case 0x4000: { // mtimecmp
        // size should be 4 or 8 for mtimecmp
        if (size == 8) {
            memcpy(data, &clint->reg.mtimecmp, 8);
        } else if (size == 4) {
            memcpy(data, &clint->reg.mtimecmp, 4);
        } else {
            LOG_ERROR("Clint mtimecmp only support 4 or 8 byte access. Received %zu byte", size);
            return -1;
        }
        return 0;
    }
    case 0x4004: { // mtimecmp[63:32]
        // size should be 4
        uint32_t mtimecmph = (uint32_t)(clint->reg.mtimecmp >> 32);
        if (size == 4) {
            memcpy(data, &mtimecmph, 4);
        } else {
            LOG_ERROR("Clint mtimecmp only support 4 or 8 byte access. Received %zu byte", size);
            return -1;
        }
        return 0;
    }
    case 0xBFF8: { // mtime
        // size should be 4 or 8 byte for mtime
        if (size == 8) {
            memcpy(data, &clint->reg.mtime, 8);
        } else if (size == 4) {
            memcpy(data, &clint->reg.mtime, 4);
        } else {
            LOG_ERROR("Clint mtime only support 4 or 8 byte access. Received %zu byte", size);
            return -1;
        }
        return 0;
    }
    case 0xBFFC: { // mtime[63:32]
        // size should be 4
        uint32_t mtimeh = (uint32_t)(clint->reg.mtime >> 32);
        if (size == 4) {
            memcpy(data, &mtimeh, 4);
        } else {
            LOG_ERROR("Clint mtime only support 4 or 8 byte access. Received %zu byte", size);
            return -1;
        }
        return 0;
    }
    default: {
        LOG_ERROR("Unsupported read address in clint: %lx", addr);
        return -1;
    }
    }
    return 0;
}

/**
 * We assume the clock for mtime is 100MHz
 */
void clint_mtime_tick(clint_t *clint) {
    long time_ns    = get_time();
    long elapsed_ns = time_ns - clint->prev_time_ns;
    long ticks      = elapsed_ns / 10;
    clint->reg.mtime += (uint64_t)ticks;
    // Advance only by whole ticks, preserving leftover nanoseconds for the next update.
    clint->prev_time_ns += ticks * 10;
}

bool clint_irq_level(clint_t *clint) {
    clint->MSIP = false;
    clint->MTIP = false;
    if (clint->reg.msip == 1) {
        clint->MSIP = true;
    }
    if (clint->reg.mtime >= clint->reg.mtimecmp) {
        clint->MTIP = true;
    }
    return clint->MSIP || clint->MTIP;
}

bool clint_msip(clint_t *clint) {
    return clint->MSIP;
}

bool clint_mtip(clint_t *clint) {
    return clint->MTIP;
}

uint64_t clint_mtime(clint_t *clint) {
    return clint->reg.mtime;
}
