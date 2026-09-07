#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "addrmap.h"

#define RAM_BASE DRAM_BASE
#define RAM_SIZE DRAM_SIZE

typedef struct {
    uint8_t *data;
    uint64_t base;
    uint64_t size;
} memory_t;

// Initialization and free
int  memory_init(memory_t *memory, bool poison_ram, size_t ram_size);
void memory_free(memory_t *memory);

// CPU memory access interface
int ram_read(const memory_t *memory, uint64_t addr, size_t size, void *data);
int ram_write(memory_t *memory, uint64_t addr, size_t size, const void *data);

// Host memory access interface
void *memory_set(memory_t *memory, uint64_t start_addr, int c, size_t n);

// Loader
int memory_load_binary(memory_t *memory, const char *file);
// ELF loaders return the address where CPU execution should begin.
int memory_load_elf(memory_t *memory, const char *file, uint64_t *entry_point);
int memory_load_auto(memory_t *memory, const char *file, uint64_t *entry_point);

#endif
