#include "syscon.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "addrmap.h"
#include "device/syscon.h"
#include "log.h"

// global variable
bool poweroff_requested = false;
bool reboot_requested   = false;

int syscon_init(syscon_t *syscon) {
    syscon->reg.sys_ctrl    = 0;
    syscon->reg.reset_cause = RESET_CAUSE_POWER_ON;
    poweroff_requested      = false;
    reboot_requested        = false;
    return 0;
}

int syscon_reset(syscon_t *syscon) {
    syscon->reg.sys_ctrl    = 0;
    poweroff_requested      = false;
    reboot_requested        = false;
    return 0;
}

int syscon_write(syscon_t *syscon, uint64_t addr, size_t size, const void *data) {
    uint64_t offset = addr - Syscon_BASE;
    if (size != 4) {
        LOG_ERROR("Syscon only support 4 byte access. Received %zu byte", size);
        return -1;
    }
    switch (offset) {
    case (0): {
        syscon->reg.sys_ctrl = *((uint32_t *)data);
        switch (syscon->reg.sys_ctrl) {
        case 1: {
            poweroff_requested = true;
            break;
        }
        case 2: {
            reboot_requested = true;
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
    uint64_t offset = addr - Syscon_BASE;
    if (size != 4) {
        LOG_ERROR("Syscon only support 4 byte access. Received %zu byte", size);
        return -1;
    }
    switch (offset) {
    case (4): {
        *((uint32_t *)data) = syscon->reg.reset_cause;
        break;
    }
    default: {
        LOG_ERROR("Unsupported read address in syscon: %lx", addr);
        return -1;
    }
    }
    return 0;
}
