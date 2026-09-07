#ifndef BUS_H
#define BUS_H

#include <stddef.h>
#include <stdint.h>

#include "device/device.h"
#include "memory/memory.h"

typedef struct {
    memory_t   *memory;
    dev_list_t *devices;
} bus_t;

int bus_read(bus_t *bus, uint64_t addr, size_t size, void *data);
int bus_write(bus_t *bus, uint64_t addr, size_t size, const void *data);

#endif
