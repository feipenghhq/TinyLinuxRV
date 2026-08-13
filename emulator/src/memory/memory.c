#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "memory.h"
#include "log.h"


/**
 * Initialize memory
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
 * Free memory
 */
void memory_free(memory_t *memory) {
    free(memory->data);
    memory->data = NULL;
}

/**
 * Load binary (.bin) file to the memory. Will only read what the memory can hold.
 * Error out if the file is too large.
 */
int memory_load_binary(memory_t *memory, const char *bin) {
    FILE *fp = fopen(bin, "rb");
    char extra;

    if (fp == NULL) {
        LOG_ERROR("Can't open %s: %s", bin, strerror(errno));
        return -1;
    }
    size_t count = fread(memory->data, 1, (size_t) memory->size, fp);
    if (ferror(fp)) {
        LOG_ERROR("Failed to read file %s", bin);
        fclose(fp);
        return -1;
    }

    // Check if the RAM size is too small
    size_t extra_count = fread(&extra, 1, 1, fp);
    if (ferror(fp)) {
        LOG_ERROR("Failed to read file %s", bin);
        fclose(fp);
        return -1;
    }
    if (extra_count == 1) {
        LOG_ERROR("File size is larger than the RAM_SIZE: %llu", RAM_SIZE);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    LOG_INFO("Loaded %zu bytes", count);
    return 0;
}

/**
 * Print memory content by 32 bit. Used for debug so ignore the small error in the function.
 * Note: no boundary check, could go beyond boundary. Use with caution.
 */
void memory_print32(const memory_t *memory, uint64_t start, size_t size) {
    uint32_t data;
    for (uint64_t addr = start; addr < start + size; addr += 4) {
        if (memory_read(memory, addr, 4, &data) == 0) {
            printf("Memory content at addr %lx is %08x\n", addr, data);
        } else {
            LOG_ERROR("Failed to read memory");
        }
    }
}

// Check whether the memory access meets size, alignment, and range requirements.
static inline bool memory_access_check(const memory_t *memory, uint64_t addr, size_t size) {
    // check size
    if (size != 1 && size != 2 && size != 4 && size != 8) {
        LOG_ERROR("Unsupported memory access size: %zu", size);
        return false;
    }
    // check alignment
    if ((addr & (size-1)) != 0) {
        LOG_ERROR("Access %ld-bit data at unaligned address: %lx", size * 8, addr);
        return false;
    }
    // check address range
    if (addr < memory->base || addr > memory->end - size) {
        LOG_ERROR("Address out of memory range: %lx", addr);
        return false;
    }
    return true;
}

// Common helper function to read from memory.
// Supporting different size:
// - 1:  8 bits
// - 2: 16 bits
// - 4: 32 bits
// - 8: 64 bits
int memory_read(const memory_t *memory, uint64_t addr, size_t size, void *data) {
    uint64_t offset;
    if (!memory_access_check(memory, addr, size)) {
        return -1;
    }
    // RISC-V memory is little-endian.
    // memcpy preserves byte order, so this currently assumes a little-endian host.
    offset = addr - memory->base;
    memcpy(data, &memory->data[offset], size);
    return 0;
}

// Common helper function to store data to memory.
// Supporting different size:
// - 1:  8 bits
// - 2: 16 bits
// - 4: 32 bits
// - 8: 64 bits
int memory_write(memory_t *memory, uint64_t addr, size_t size, const void *data) {
    uint64_t offset;
    if (!memory_access_check(memory, addr, size)) {
        return -1;
    }
    // RISC-V memory is little-endian.
    // memcpy preserves byte order, so this currently assumes a little-endian host.
    offset = addr - memory->base;
    memcpy(&memory->data[offset], data, size);
    return 0;
}