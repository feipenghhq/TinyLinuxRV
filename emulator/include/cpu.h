#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

#include "device.h"
#include "memory.h"

// reset to 0x80000000
#define RST_VEC 0x80000000ULL

// reservation field for LR/SC
typedef struct {
    bool     valid;
    uint64_t addr_start;
    uint64_t addr_end;
} reservation_t;

// CPU status
typedef struct {
    uint64_t regs[32];
    uint64_t pc;
    bool     halted;

    reservation_t res;
} cpu_t;

void cpu_init(cpu_t *cpu);
int  cpu_execute(cpu_t *cpu, uint32_t inst, memory_t *memory, dev_list_t *devices);

#endif
