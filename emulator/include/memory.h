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


int init_memory(memory_t*);
void free_memory(memory_t*);
int load_bin(memory_t* , const char*);
void mem_print32(memory_t*, uint64_t, size_t);
int mem_read32(memory_t*, uint64_t, uint32_t*);

#endif