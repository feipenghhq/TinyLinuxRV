
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
int init_memory(memory_t* memory) {
    memory->size = RAM_SIZE;
    memory->base = RAM_BASE;
    memory->end  = RAM_BASE + RAM_SIZE;
    memory->data = (uint8_t*) calloc(RAM_SIZE, sizeof(uint8_t));
    if (memory->data == NULL) {
        LOG_ERROR("Can't allocate memory data");
        return -1;
    }
    LOG_INFO("Initialize Memory done");
    return 0;
}

/**
 * Deconstruct memory
 */
void free_memory(memory_t* memory) {
    free(memory->data);
    LOG_INFO("Free Memory done");
}

/**
 * Load binary (.bin) file to the memory
 */
int load_bin(memory_t* memory, const char *bin) {
    FILE *fp = fopen(bin, "rb");

    if (fp == NULL) {
        LOG_ERROR("Can't open %s: %s", bin, strerror(errno));
        return -1;
    }
    size_t count = fread(memory->data, 1, RAM_SIZE, fp);
    if (ferror(fp)) {
        LOG_ERROR("Failed to read file %s", bin);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    LOG_INFO("Loaded %zu bytes", count);
    return 0;
}

/**
 * Print memory content by 32 bit.
 */
void mem_print32(memory_t* memory, uint64_t start, size_t size) {
    uint32_t data;
    for (uint64_t addr = start; addr < start + size; addr += 4) {
        mem_read32(memory, addr, &data);
        printf("Memory content at addr %lx is %08x\n", addr, data);
    }
}

/**
 * Read memory by 32 bit
 */
int mem_read32(memory_t* memory, uint64_t addr, uint32_t* data) {

    // addr must be aligned to 32 bit
    if ((addr & 0x3) != 0) {
        LOG_ERROR("Reading 32 bit data from memory but address it not aligned: %lx", addr);
        return -1;
    }
    if (addr >= memory->base && addr < memory->end) {
        *data = ((uint32_t *) memory->data)[(addr - memory->base) >> 2];
        return 0;
    } else {
        LOG_ERROR("Address out of memory range: %lx", addr);
        return -1;
    }
}
