#ifndef PLIC_H
#define PLIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Support ID 1 - 10
#define PLIC_MAX_INTERRUPT   11
#define PLIC_INTERRUPT_WORDS ((PLIC_MAX_INTERRUPT + 31) / 32)

// Support 2 context for hart0 M-mode and S-mode
#define PLIC_MAX_CONTEXT 2

typedef struct {
    uint32_t priority[PLIC_MAX_INTERRUPT];
    uint32_t pending[PLIC_INTERRUPT_WORDS];
    uint32_t enable[PLIC_MAX_CONTEXT][PLIC_INTERRUPT_WORDS];
    uint32_t threshold[PLIC_MAX_CONTEXT];
    uint32_t claim[PLIC_MAX_CONTEXT];
} plic_reg_t;

typedef struct {
    uint64_t   base;
    plic_reg_t regs;
    bool       gw_busy[PLIC_MAX_INTERRUPT];
    bool       MEIP;
    bool       SEIP;
} plic_t;

void plic_reset(plic_t *plic);
void plic_init(plic_t *plic, uint64_t base);
int  plic_write(plic_t *plic, uint64_t addr, size_t size, const void *data);
int  plic_read(plic_t *plic, uint64_t addr, size_t size, void *data);
void plic_irq_update(plic_t *plic, bool *irq);
bool plic_meip(plic_t *plic);
bool plic_seip(plic_t *plic);

#endif
