#include "device.h"

#include <stddef.h>
#include <stdlib.h>

#include "addrmap.h"
#include "device/clint.h"
#include "device/syscon.h"
#include "device/uart16550.h"
#include "device/plic.h"
#include "utils/log.h"

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

    INIT_DEVICE(virtio, VirtIO);

    INIT_DEVICE(syscon, Syscon);
    INIT_DEVICE(uart0, UART0);
    INIT_DEVICE(clint, CLINT);
    INIT_DEVICE(plic, PLIC);

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

    // init clint
    dev->clint.device = malloc(sizeof(clint_t));
    if (dev->clint.device == NULL) {
        LOG_ERROR("Failed to initialize clint device.");
        return -1;
    }
    clint_init(dev->clint.device, CLINT_BASE);

    // init plic
    dev->plic.device = malloc(sizeof(plic_t));
    if (dev->plic.device == NULL) {
        LOG_ERROR("Failed to initialize plic device.");
        return -1;
    }
    plic_init(dev->plic.device, PLIC_BASE);

    return 0;
}

void device_reset(dev_list_t *dev) {
    syscon_reset(dev->syscon.device);
    uart16550_reset(dev->uart0.device);
    clint_reset(dev->clint.device);
    plic_reset(dev->plic.device);
}

void device_free(dev_list_t *dev) {
    free(dev->syscon.device);
    dev->syscon.device = NULL;
    free(dev->uart0.device);
    dev->uart0.device = NULL;
    free(dev->clint.device);
    dev->clint.device = NULL;
    free(dev->plic.device);
    dev->plic.device = NULL;
}

/**
 *  Update device
 */
int device_update(dev_list_t *dev) {
    int result;

    result = uart16550_poll_input(dev->uart0.device);
    if (result != 0)
        return result;

    clint_mtime_tick(dev->clint.device);

    return 0;
}

/**
 * Call device interrupt function to get device interrupt updated
 * and then send the interrupt to plic
 */
void device_irq_level(dev_list_t *dev) {
    bool irq[PLIC_MAX_INTERRUPT] = {false};

    // clint has timer and software interrupt
    clint_irq_level(dev->clint.device);

    // get device interrupt level
    irq[10] = uart16550_irq_level(dev->uart0.device);

    plic_irq_update(dev->plic.device, irq);
}
