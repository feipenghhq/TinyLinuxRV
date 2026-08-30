#include "device.h"

#include <stddef.h>
#include <stdlib.h>

#include "addrmap.h"
#include "log.h"
#include "syscon.h"
#include "uart16550.h"

// ----------------------------------------------
// Helper Macro
// ----------------------------------------------

#define INIT_DEVICE(name, NAME)         \
    do {                                \
        dev->name.base   = NAME##_BASE; \
        dev->name.size   = NAME##_SIZE; \
        dev->name.end    = NAME##_END;  \
        dev->name.device = NULL;        \
    } while (0)

int device_init(dev_list_t *dev) {
    // Place holder for unimp devices
    INIT_DEVICE(bootROM, BootROM);
    INIT_DEVICE(aclint, ACLINT);
    INIT_DEVICE(plic, PLIC);

    INIT_DEVICE(virtio, VirtIO);

    // init syscon
    INIT_DEVICE(syscon, Syscon);
    dev->syscon.device = malloc(sizeof(syscon_t));
    if (dev->syscon.device == NULL || syscon_init(dev->syscon.device, Syscon_BASE) != 0) {
        LOG_ERROR("Failed to initialize syscon device.");
        return -1;
    }

    // init uart0
    INIT_DEVICE(uart0, UART0);
    dev->uart0.device = malloc(sizeof(uart16550_t));
    if (dev->uart0.device == NULL || uart16550_init(dev->uart0.device, UART0_BASE) != 0) {
        LOG_ERROR("Failed to initialize uart0 device.");
        return -1;
    }
    return 0;
}

int device_reset(dev_list_t *dev) {
    if (syscon_reset(dev->syscon.device) != 0)
        return -1;
    if (uart16550_reset(dev->uart0.device) != 0)
        return -1;
    return 0;
}

int device_free(dev_list_t *dev) {
    free(dev->syscon.device);
    dev->syscon.device = NULL;
    free(dev->uart0.device);
    dev->uart0.device = NULL;
    return 0;
}
