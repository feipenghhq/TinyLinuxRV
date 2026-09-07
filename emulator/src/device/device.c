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
        dev->name.device = NULL;        \
    } while (0)

int device_init(dev_list_t *dev) {
    // Place holder for unimp devices
    INIT_DEVICE(bootROM, BootROM);
    INIT_DEVICE(aclint, ACLINT);
    INIT_DEVICE(plic, PLIC);
    INIT_DEVICE(virtio, VirtIO);
    INIT_DEVICE(syscon, Syscon);
    INIT_DEVICE(uart0, UART0);

    // init syscon
    dev->syscon.device = malloc(sizeof(syscon_t));
    if (dev->syscon.device == NULL) {
        LOG_ERROR("Failed to initialize syscon device.");
        return -1;
    }
    syscon_init(dev->syscon.device, Syscon_BASE);

    // init uart0
    dev->uart0.device = malloc(sizeof(uart16550_t));
    if (dev->uart0.device == NULL) {
        LOG_ERROR("Failed to initialize uart0 device.");
        return -1;
    }
    uart16550_init(dev->uart0.device, UART0_BASE);

    return 0;
}

void device_reset(dev_list_t *dev) {
    syscon_reset(dev->syscon.device);
    uart16550_reset(dev->uart0.device);
}

void device_free(dev_list_t *dev) {
    free(dev->syscon.device);
    dev->syscon.device = NULL;
    free(dev->uart0.device);
    dev->uart0.device = NULL;
}

/**
 *  call device poll function to get device updated
 */
int device_poll_input(dev_list_t *dev) {
    int result;
    result = uart16550_poll_input(dev->uart0.device);
    if (result != 0)
        return result;
    return 0;
}

/**
 *  call device interrupt function to get device interrupt updated
 */
void device_irq_level(dev_list_t *dev) {
    uart16550_irq_level(dev->uart0.device);
}
