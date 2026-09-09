#include "bus/bus.h"

#include <stddef.h>
#include <stdint.h>

#include "device/syscon.h"
#include "device/uart16550.h"
#include "device/clint.h"
#include "device/plic.h"
#include "memory/memory.h"
#include "utils/address_range.h"
#include "utils/log.h"

// ----------------------------------------------
// bus access dispatch
// ----------------------------------------------

#define DEVICE(name) bus->devices->name

int bus_read(bus_t *bus, uint64_t addr, size_t size, void *data) {
    if        (check_addr_range(addr, size, DEVICE(syscon).base, DEVICE(syscon).size)) {
        return syscon_read(DEVICE(syscon).device, addr, size, data);
    } else if (check_addr_range(addr, size, DEVICE(uart0).base, DEVICE(uart0).size)) {
        return uart16550_read(DEVICE(uart0).device, addr, size, data);
    } else if (check_addr_range(addr, size, DEVICE(clint).base, DEVICE(clint).size)) {
        return clint_read(DEVICE(clint).device, addr, size, data);
    } else if (check_addr_range(addr, size, DEVICE(plic).base, DEVICE(plic).size)) {
        return plic_read(DEVICE(plic).device, addr, size, data);
    } else if (check_addr_range(addr, size, bus->memory->base, bus->memory->size)) {
        return ram_read(bus->memory, addr, size, data);
    } else {
        LOG_ERROR("Invalid address in bus read: %lx", addr);
        return -1;
    }
}

int bus_write(bus_t *bus, uint64_t addr, size_t size, const void *data) {
    if (check_addr_range(addr, size, DEVICE(syscon).base, DEVICE(syscon).size)) {
        return syscon_write(DEVICE(syscon).device, addr, size, data);
    } else if (check_addr_range(addr, size, DEVICE(uart0).base, DEVICE(uart0).size)) {
        return uart16550_write(DEVICE(uart0).device, addr, size, data);
    } else if (check_addr_range(addr, size, DEVICE(clint).base, DEVICE(clint).size)) {
        return clint_write(DEVICE(clint).device, addr, size, data);
    } else if (check_addr_range(addr, size, DEVICE(plic).base, DEVICE(plic).size)) {
        return plic_write(DEVICE(plic).device, addr, size, data);
    } else if (check_addr_range(addr, size, bus->memory->base, bus->memory->size)) {
        return ram_write(bus->memory, addr, size, data);
    } else {
        LOG_ERROR("Invalid address in bus write: %lx", addr);
        return -1;
    }
}
