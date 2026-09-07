#include "clint.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utils/log.h"

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
        // size should be 8 for mtimecmp
        if (size != 8) {
            LOG_ERROR("Clint mtimecmp only support 8 byte access. Received %zu byte", size);
            return -1;
        }
        memcpy(data, &clint->reg.mtimecmp, 8);
        return 0;
    }
    case 0xBFF8: { // mtime
        // size should be 8 for mtime
        if (size != 8) {
            LOG_ERROR("Clint mtime only support 8 byte access. Received %zu byte", size);
            return -1;
        }
        memcpy(data, &clint->reg.mtime, 8);
        return 0;
    }
    default: {
        LOG_ERROR("Unsupported read address in clint: %lx", addr);
        return -1;
    }
    }
    return 0;
}

void clint_mtime_tick(clint_t *clint) {
    clint->reg.mtime++;
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
