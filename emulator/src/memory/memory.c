#include "memory.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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



/**
 * Initialize a memory block with a given size and base address.
 * The memory block is allocated and initialized to zero.
 */
int memory_init(memory_t *memory) {
    memory->size = RAM_SIZE;
    memory->base = RAM_BASE;
    memory->end  = RAM_BASE + RAM_SIZE;
    memory->data = calloc(RAM_SIZE, sizeof(uint8_t));
    if (memory->data == NULL) {
        LOG_ERROR("Can't allocate memory data");
        return -1;
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
Read from memory. Supporting different size:
Supported sizes are 1, 2, 4, and 8 bytes.
*/
int memory_cpu_read(const memory_t *memory, uint64_t addr, size_t size, void *data) {
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
Store data to memory. Supporting different size:
Supported sizes are 1, 2, 4, and 8 bytes.
*/
int memory_cpu_write(memory_t *memory, uint64_t addr, size_t size, const void *data) {
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
