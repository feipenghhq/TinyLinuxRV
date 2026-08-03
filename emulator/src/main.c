
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "memory.h"
#include "log.h"

/**
 * Expect 1 argument - the bin file
 */
int main(int argc, char **argv) {

    cpu_t    cpu;
    memory_t memory;
    uint32_t inst;
    char     *bin;

    // process the argument
    if (argc < 2) {
        LOG_ERROR("Please provide the binary file to be loaded");
        return EXIT_FAILURE;
    } else {
        bin = argv[1];
    }

    LOG_INFO("Running: %s", bin);

    // initialize cpu and memory
    init_cpu(&cpu);
    if (init_memory(&memory) != 0) {
        return EXIT_FAILURE;
    }

    // load the binary file
    LOG_INFO("Loading binary file: %s", bin);
    if (load_bin(&memory, bin) != 0) {
        free_memory(&memory);
        return EXIT_FAILURE;
    }

    // execute instruction
    while(!cpu.halted) {
        if (mem_read32(&memory, cpu.pc, &inst) != 0) {
            LOG_ERROR("Test FAILED");
            free_memory(&memory);
            return EXIT_FAILURE;
        }
        if (execute(inst, &cpu, &memory) != 0) {
            free_memory(&memory);
            LOG_ERROR("Test FAILED");
            return EXIT_FAILURE;
        }
    }

    LOG_INFO("Test PASSED");
    free_memory(&memory);
    return 0;
}
