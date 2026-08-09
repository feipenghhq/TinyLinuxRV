#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define RAM_BASE 0x80000000ULL

// 1 MB RAM
#define RAM_SIZE (1 * 1024 * 1024ULL)

typedef struct {
    uint8_t *data;
    uint64_t size;
    uint64_t base;
    uint64_t end;
} memory_t;


int memory_init(memory_t *memory);
void memory_free(memory_t *memory);
int memory_load_binary(memory_t *memory, const char *bin);
void memory_print32(const memory_t *memory, uint64_t start, size_t size);
int memory_read(const memory_t *memory, uint64_t addr, size_t size, void *data);
int memory_write(memory_t *memory, uint64_t addr, size_t size, const void *data);

#endif