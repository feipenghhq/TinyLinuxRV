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


void init_cpu(cpu_t*);
int execute(uint32_t inst, cpu_t* cpu, memory_t* memory);

#endif
