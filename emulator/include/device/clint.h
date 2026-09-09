#ifndef CLINT_H
#define CLINT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t msip;
    uint64_t mtimecmp;
    uint64_t mtime;
} clint_reg_t;

typedef struct {
    uint64_t    base;
    clint_reg_t reg;
    bool        MSIP;
    bool        MTIP;
} clint_t;

void clint_init(clint_t *clint, uint64_t base);
void clint_reset(clint_t *clint);
int  clint_write(clint_t *clint, uint64_t addr, size_t size, const void *data);
int  clint_read(clint_t *clint, uint64_t addr, size_t size, void *data);
void clint_mtime_tick(clint_t *clint);
bool clint_irq_level(clint_t *clint);
bool clint_msip(clint_t *clint);
bool clint_mtip(clint_t *clint);

#endif
