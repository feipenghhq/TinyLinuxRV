
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "log.h"


/**
 * Initialize memory
 */
void init_memory(memory_t* memory) {
    memory->size = RAM_SIZE;
    memory->base = RAM_BASE;
    memory->end  = RAM_BASE + RAM_SIZE;
    memory->data = (uint8_t*) malloc(RAM_SIZE * sizeof(uint8_t));
    LOG_INFO("Initialize Memory done");
}

/**
 * Load binary (.bin) file to the memory
 */
void load_bin(memory_t* memory, char *bin) {
    FILE *file = fopen(bin, "r");
    if (file == NULL) {
        LOG_ERROR("Can't open file: %s", bin);
    }
    fread(memory->data, RAM_SIZE, 1, file);
    fclose(file);
}

/**
 * Print memory content by 32 bit
 */
void mem_print32(memory_t* memory, uint32_t start, size_t size) {
    for (uint32_t addr = start; addr < start + size; addr += 4) {
        printf("Memory content at addr %x is %08x\n", addr, ((uint32_t*) memory->data)[(addr - memory->base) >> 2]);
    }
}

/**
 * Read memory by 32 bit
 */
uint32_t mem_read32(memory_t* memory, uint32_t addr) {
    // addr must be aligned to 32 bit
    if ((addr & 0x3) != 0) {
        LOG_ERROR("Reading 32 bit data from memory but address it not aligned: %x", addr);
    }
    if (addr >= memory->base && addr < memory->end) {

        return ((uint32_t *) memory->data)[(addr - memory->base) >> 2];
    } else {
        LOG_ERROR("Address out of memory range: %x", addr);
        return 0;
    }
}
