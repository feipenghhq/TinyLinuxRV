#include "bus/bus.h"

#include <stddef.h>
#include <stdint.h>

#include "device/syscon.h"
#include "device/uart16550.h"
#include "memory/memory.h"
#include "utils/address_range.h"
#include "utils/log.h"

// ----------------------------------------------
// bus access dispatch
// ----------------------------------------------

int bus_read(bus_t *bus, uint64_t addr, size_t size, void *data) {
    if (check_addr_range(addr, size, bus->devices->syscon.base, bus->devices->syscon.size)) {
        return syscon_read(bus->devices->syscon.device, addr, size, data);
    } else if (check_addr_range(addr, size, bus->devices->uart0.base, bus->devices->uart0.size)) {
        return uart16550_read(bus->devices->uart0.device, addr, size, data);
    } else if (check_addr_range(addr, size, bus->memory->base, bus->memory->size)) {
        return ram_read(bus->memory, addr, size, data);
    } else {
        LOG_ERROR("Invalid address in bus read: %lx", addr);
        return -1;
    }
}

int bus_write(bus_t *bus, uint64_t addr, size_t size, const void *data) {
    if (check_addr_range(addr, size, bus->devices->syscon.base, bus->devices->syscon.size)) {
        return syscon_write(bus->devices->syscon.device, addr, size, data);
    } else if (check_addr_range(addr, size, bus->devices->uart0.base, bus->devices->uart0.size)) {
        return uart16550_write(bus->devices->uart0.device, addr, size, data);
    } else if (check_addr_range(addr, size, bus->memory->base, bus->memory->size)) {
        return ram_write(bus->memory, addr, size, data);
    } else {
        LOG_ERROR("Invalid address in bus write: %lx", addr);
        return -1;
    }
}
