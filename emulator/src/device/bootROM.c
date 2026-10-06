#include "bootROM.h"

#include <stdint.h>
#include <stdlib.h>

#include "memory/memory.h"
#include "utils/log.h"

int bootROM_init(memory_t *rom, uint64_t base) {
    rom->data = malloc(BOOT_ROM_SIZE);
    if (!rom->data) {
        LOG_ERROR("Can't allocate bootROM");
        return 1;
    }
    rom->size = BOOT_ROM_SIZE;
    rom->base = base;
    return 0;
}

void bootROM_free(memory_t *rom) {
    // rom itself could be NULL if bootROM device failed malloc.
    // Make sure rom is valid before free data
    if (rom) {
        free(rom->data);
        rom->data = NULL;
    }
}

int bootROM_write(memory_t *bootROM, uint64_t addr, size_t size, const void *data) {
    (void)bootROM;
    (void)addr;
    (void)size;
    (void)data;
    LOG_ERROR("Can't write to boot rom");
    return 1;
}

int bootROM_read(const memory_t *bootROM, uint64_t addr, size_t size, void *data) {
    // BootROM is a memory so just use memory access
    return ram_read(bootROM, addr, size, data);
}
