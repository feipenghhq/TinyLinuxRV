#include "memory.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "log.h"

// ----------------------------------------------
// Helper Function
// ----------------------------------------------

static inline bool check_addr_range(const memory_t *memory, uint64_t addr, size_t size) {
    // check address range
    if (addr < memory->base || addr > memory->end || size > memory->end - addr) { // out of range
        LOG_ERROR("Address out of memory range: %lx", addr);
        return false;
    }
    return true;
}

// ----------------------------------------------
// Memory initialize
// ----------------------------------------------

/**
 * Initialize a memory block with a given size and base address.
 * The memory block is allocated and initialized to zero.
 * If poison_ram is set the ram content to non zero value (0xA5).
 */
int memory_init(memory_t *memory, bool poison_ram) {
    memory->size = RAM_SIZE;
    memory->base = RAM_BASE;
    memory->end  = RAM_BASE + RAM_SIZE;
    memory->data = calloc(RAM_SIZE, sizeof(uint8_t));
    if (memory->data == NULL) {
        LOG_ERROR("Can't allocate memory data");
        return -1;
    }
    if (poison_ram) {
        memset(memory->data, 0xA5, RAM_SIZE * sizeof(uint8_t));
    }
    LOG_INFO("Initialize Memory done");
    return 0;
}

/**
 * Free the memory block
 */
void memory_free(memory_t *memory) {
    free(memory->data);
    memory->data = NULL;
}

// ----------------------------------------------
// CPU access interface
// ----------------------------------------------

// Check whether the memory access meets size, alignment, and range requirements.
static inline bool cpu_access_check(const memory_t *memory, uint64_t addr, size_t size) {
    // check size
    if (size != 1 && size != 2 && size != 4 && size != 8) {
        LOG_ERROR("Unsupported memory access size: %zu", size);
        return false;
    }
    // check alignment
    if ((addr & (size - 1)) != 0) {
        LOG_ERROR("Access %ld-bit data at unaligned address: %lx", size * 8, addr);
        return false;
    }
    // check address range
    return check_addr_range(memory, addr, size);
}

/**
Read from ram. Supporting different size:
Supported sizes are 1, 2, 4, and 8 bytes.
*/
int ram_read(const memory_t *memory, uint64_t addr, size_t size, void *data) {
    uint64_t offset;
    if (!cpu_access_check(memory, addr, size)) {
        return -1;
    }
    // RISC-V memory is little-endian.
    // memcpy preserves byte order, so this currently assumes a little-endian host.
    offset = addr - memory->base;
    memcpy(data, &memory->data[offset], size);
    return 0;
}

/**
Store data to ram. Supporting different size:
Supported sizes are 1, 2, 4, and 8 bytes.
*/
int ram_write(memory_t *memory, uint64_t addr, size_t size, const void *data) {
    uint64_t offset;
    if (!cpu_access_check(memory, addr, size)) {
        return -1;
    }
    // RISC-V memory is little-endian.
    // memcpy preserves byte order, so this currently assumes a little-endian host.
    offset = addr - memory->base;
    memcpy(&memory->data[offset], data, size);
    return 0;
}

// ----------------------------------------------
// Host access interface
// ----------------------------------------------

/**
 * The memory_set() function fill host memory with a constant byte
 */
void *memory_set(memory_t *memory, uint64_t start_addr, int c, size_t n) {

    // check address range
    if (!check_addr_range(memory, start_addr, n)) {
        return NULL;
    }
    uint64_t offset = start_addr - memory->base;
    return memset(&memory->data[offset], c, n);
}

// ----------------------------------------------
// CPU access dispatch
// ----------------------------------------------
// Placeholder dispatcher before the device is implemented
#define DISPATCH_ERR(dev, op)                                         \
    do {                                                              \
        if (addr >= devs->dev.base && addr <= devs->dev.end - size) { \
            LOG_ERROR("Unsupported devices: %s", #dev);               \
            LOG_ERROR("Address: %lx", addr);                          \
            return -1;                                                \
        }                                                             \
    } while (0)

#define DISPATCH(dev, op)                                             \
    do {                                                              \
        if (addr >= devs->dev.base && addr <= devs->dev.end - size) { \
            return dev##_##op(devs->dev.device, addr, size, data);        \
        }                                                             \
    } while (0)

// CPU read dispatch
int memory_cpu_read(const memory_t *memory, dev_list_t *devs, uint64_t addr, size_t size, void *data) {
    DISPATCH_ERR(bootROM, read);
    DISPATCH_ERR(aclint, read);
    DISPATCH_ERR(plic, read);
    DISPATCH_ERR(uart0, read);
    DISPATCH_ERR(virtio, read);

    DISPATCH(syscon, read);
    return ram_read(memory, addr, size, data);
}

// CPU write dispatch
int memory_cpu_write(memory_t *memory, dev_list_t *devs, uint64_t addr, size_t size, const void *data) {
    DISPATCH_ERR(bootROM, write);
    DISPATCH_ERR(aclint, write);
    DISPATCH_ERR(plic, write);
    DISPATCH_ERR(uart0, write);
    DISPATCH_ERR(virtio, write);

    DISPATCH(syscon, write);
    return ram_write(memory, addr, size, data);
}
