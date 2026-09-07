#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>

#include "syscon.h"
#include "uart16550.h"

typedef struct {
    void    *device;
    uint64_t base;
    uint64_t size;
} device_t;

typedef struct {
    device_t bootROM;
    device_t syscon;
    device_t aclint;
    device_t plic;
    device_t uart0;
    device_t virtio;
} dev_list_t;

int  device_init(dev_list_t *dev);
void device_reset(dev_list_t *dev);
void device_free(dev_list_t *dev);
int  device_poll_input(dev_list_t *dev);
void device_irq_level(dev_list_t *dev);

#endif
