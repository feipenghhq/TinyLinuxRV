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
    const char *bin;

    // process the argument
    if (argc < 2) {
        LOG_ERROR("Please provide the binary file to be loaded");
        return EXIT_FAILURE;
    }
    bin = argv[1];

    LOG_INFO("Running: %s", bin);

    // initialize cpu and memory
    cpu_init(&cpu);
    if (memory_init(&memory) != 0) {
        return EXIT_FAILURE;
    }

    // load the binary file
    LOG_INFO("Loading binary file: %s", bin);
    if (memory_load_binary(&memory, bin) != 0) {
        memory_free(&memory);
        return EXIT_FAILURE;
    }

    // execute instruction
    while (!cpu.halted) {
        if (memory_read32(&memory, cpu.pc, &inst) != 0) {
            LOG_ERROR("Memory read failed. Unable to fetch instruction");
            memory_free(&memory);
            return EXIT_FAILURE;
        }
        if (cpu_execute(&cpu, inst, &memory) != 0) {
            memory_free(&memory);
            LOG_ERROR("CPU execution failed");
            return EXIT_FAILURE;
        }
    }

    LOG_INFO("CPU execution halted normally");
    memory_free(&memory);
    return EXIT_SUCCESS;
}
