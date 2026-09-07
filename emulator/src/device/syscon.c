#include "syscon.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "addrmap.h"
#include "device/syscon.h"
#include "log.h"

// global variable

void syscon_init(syscon_t *syscon, uint64_t base) {
    syscon->reg.sys_ctrl    = 0;
    syscon->reg.reset_cause = RESET_CAUSE_POWER_ON;
    syscon->base            = base;
    syscon->poweroff        = false;
    syscon->reboot          = false;
}

void syscon_reset(syscon_t *syscon) {
    syscon->reg.sys_ctrl = 0;
    syscon->poweroff     = false;
    syscon->reboot       = false;
}

int syscon_write(syscon_t *syscon, uint64_t addr, size_t size, const void *data) {
    uint32_t value;
    uint64_t offset = addr - syscon->base;

    if (size != 4) {
        LOG_ERROR("Syscon only support 4 byte access. Received %zu byte", size);
        return -1;
    }
    memcpy(&value, data, size);

    switch (offset) {
    case (0): {
        syscon->reg.sys_ctrl = value;
        switch (syscon->reg.sys_ctrl) {
        case 1: {
            syscon->poweroff = true;
            break;
        }
        case 2: {
            syscon->reboot          = true;
            syscon->reg.reset_cause = RESET_CAUSE_REBOOT;
            break;
        }
        default: {
            // FIXME: Do we need to inform CPU about this?
            LOG_ERROR("Unsupported syscon command: %d", syscon->reg.sys_ctrl);
            break;
        }
        }

        break;
    }
    default: {
        LOG_ERROR("Unsupported write address in syscon: %lx", addr);
        return -1;
    }
    }
    return 0;
}

int syscon_read(syscon_t *syscon, uint64_t addr, size_t size, void *data) {
    uint32_t value;
    uint64_t offset = addr - syscon->base;
    if (size != 4) {
        LOG_ERROR("Syscon only support 4 byte access. Received %zu byte", size);
        return -1;
    }
    switch (offset) {
    case (4): {
        value = syscon->reg.reset_cause;
        break;
    }
    default: {
        LOG_ERROR("Unsupported read address in syscon: %lx", addr);
        return -1;
    }
    }
    memcpy(data, &value, size);
    return 0;
}

bool syscon_reboot_requested(const syscon_t *syscon) {
    return syscon->reboot;
}

bool syscon_poweroff_requested(const syscon_t *syscon) {
    return syscon->poweroff;
}
