#ifndef BOOT_ROM_H
#define BOOT_ROM_H

#include <stddef.h>
#include <stdint.h>

#include "memory/memory.h"

#define BOOT_ROM_SIZE 4096

int  bootROM_init(memory_t *rom, uint64_t base);
void bootROM_free(memory_t *rom);
int  bootROM_write(memory_t *bootROM, uint64_t addr, size_t size, const void *data);
int  bootROM_read(const memory_t *bootROM, uint64_t addr, size_t size, void *data);

#endif
