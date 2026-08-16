#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define RAM_BASE 0x80000000ULL

// 1 MB RAM
#define RAM_SIZE (1 * 1024 * 1024ULL)

typedef struct {
    uint8_t *data;
    uint64_t size;
    uint64_t base;
    uint64_t end;
} memory_t;

// Initialization and free
int  memory_init(memory_t *memory);
void memory_free(memory_t *memory);

// CPU access interface
int memory_cpu_read(const memory_t *memory, uint64_t addr, size_t size, void *data);
int memory_cpu_write(memory_t *memory, uint64_t addr, size_t size, const void *data);

// Host access interface
void *memory_set(memory_t *memory, uint64_t start_addr, int c, size_t n);

// Loader
int memory_load_binary(memory_t *memory, const char *file);
// ELF loaders return the address where CPU execution should begin.
int memory_load_elf(memory_t *memory, const char *file, uint64_t *entry_point);
int memory_load_auto(memory_t *memory, const char *file, uint64_t *entry_point);

#endif
