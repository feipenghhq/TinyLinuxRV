#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>

#include "syscon.h"

typedef struct {
    void    *device;
    uint64_t size;
    uint64_t base;
    uint64_t end;
} device_t;

typedef struct {
    device_t bootROM;
    device_t syscon;
    device_t aclint;
    device_t plic;
    device_t uart0;
    device_t virtio;
} dev_list_t;

int device_init(dev_list_t *dev);
int device_reset(dev_list_t *dev);
int device_free(dev_list_t *dev);

#endif
