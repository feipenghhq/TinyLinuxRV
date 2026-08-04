#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "memory.h"

// reset to 0x80000000
#define RST_VEC 0x80000000ULL

// CPU status
typedef struct {
    uint64_t regs[32];
    uint64_t pc;
    bool     halted;
} cpu_t;


void cpu_init(cpu_t *cpu);
int cpu_execute(cpu_t *cpu, uint32_t inst, memory_t *memory);

#endif
