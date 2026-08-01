
#include <stdio.h>
#include "cpu.h"
#include "memory.h"
#include "log.h"

int main(int argc, char **argv) {

    cpu_t    cpu;
    memory_t memory;
    uint32_t inst;
    char     *bin;

    init_cpu(&cpu);
    init_memory(&memory);

    // assuming argv[1] is the binary to be loaded for now
    bin = argv[1];
    LOG_INFO("Loading binary file %s", bin);
    load_bin(&memory, bin);

    while(!cpu.halted) {
        inst = mem_read32(&memory, cpu.pc);
        execute(inst, &cpu, &memory);
    }

    return 0;

}