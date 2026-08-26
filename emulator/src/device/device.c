#include "device.h"

#include <stddef.h>

#include "addrmap.h"

// ----------------------------------------------
// Helper Macro
// ----------------------------------------------

#define INIT_DEVICE(name, NAME)          \
    do {                                 \
        dev->name.base   = NAME##_BASE; \
        dev->name.size   = NAME##_SIZE; \
        dev->name.end    = NAME##_END;  \
        dev->name.device = NULL;        \
    } while (0)

/**
 * Initialize all the devices in the emulator.
 */

int device_init(dev_list_t *dev) {
    INIT_DEVICE(bootROM, BootROM);
    INIT_DEVICE(reset, Reset);
    INIT_DEVICE(aclint, ACLINT);
    INIT_DEVICE(plic, PLIC);
    INIT_DEVICE(uart0, UART0);
    INIT_DEVICE(virtio, VirtIO);

    return 0;
}
