#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

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
    LOG_INFO("Free Memory done");
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
        if (memory_read32(memory, addr, &data) == 0) {
            printf("Memory content at addr %lx is %08x\n", addr, data);
        } else {
            LOG_ERROR("Failed to read memory");
        }
    }
}

/**
 * Read one by 32-bit word from memory. Address must be aligned to boundary
 */
int memory_read32(const memory_t *memory, uint64_t addr, uint32_t *data) {
    uint64_t offset = addr - memory->base;

    // addr must be aligned to 32 bit
    if ((addr & 0x3) != 0) {
        LOG_ERROR("Reading 32 bit data from memory but address it not aligned: %lx", addr);
        return -1;
    }
    if (addr >= memory->base && addr < memory->end) {
        *data = memory->data[offset];
        *data = *data | ((uint32_t) memory->data[offset+1] << 8);
        *data = *data | ((uint32_t) memory->data[offset+2] << 16);
        *data = *data | ((uint32_t) memory->data[offset+3] << 24);
        return 0;
    } else {
        LOG_ERROR("Address out of memory range: %lx", addr);
        return -1;
    }
}
