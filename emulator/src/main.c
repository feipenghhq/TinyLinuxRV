#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "log.h"
#include "memory.h"

// -------------------------------------------------------------------
// Different type enum
// -------------------------------------------------------------------
typedef enum FILE_TYPE { AUTO, BIN, ELF } FILE_TYPE_t;

typedef enum RUN_MODE { NORMAL, RISCV_TESTS } RUN_MODE_t;

typedef struct {
    long        max_instruction;
    FILE_TYPE_t format;
    RUN_MODE_t  mode;
    char       *file;
    bool        poison_ram;
} argument_t;

// -------------------------------------------------------------------
// Command line parser
// -------------------------------------------------------------------

static const char USAGE[] =
    "rvemu [OPTION] FILE \n\n"
    "Options:\n"
    "--help\n"
    "        Print this help message.\n\n"
    "--max-instruction <max_instruction_count>\n"
    "        Specify the max instruction count. If not include, the test will run till the end.\n\n"
    "--format <auto|elf|bin>\n"
    "        Specify the format of the file. auto: automatically detect the file type. elf: elf file. bin: binary "
    "file.\n\n"
    "--riscv-tests\n"
    "        Run riscv-tests.\n\n"
    "--poison-ram\n"
    "        Fill ram content to 0xA5 before loading the program. Used mainly for testing.\n\n";

static struct option longopts[] = {
    {"help", no_argument, 0, 0},         {"max-instruction", required_argument, 0, 0},
    {"format", required_argument, 0, 0}, {"riscv-tests", no_argument, 0, 0},
    {"poison-ram", no_argument, 0, 0},   {0, 0, 0, 0},
};

int parse_arguments(int argc, char **argv, argument_t *argument) {
    int c;
    if (argc < 2) {
        printf("Incorrect argument. Please see usage:\n\n");
        printf("%s", USAGE);
        exit(EXIT_FAILURE);
    }

    while (1) {
        int option_index = 0;
        c                = getopt_long(argc, argv, "", longopts, &option_index);

        if (c == -1) {
            break;
        } else if (c == 0) {
            switch (option_index) {
            case 0: { // help
                printf("%s", USAGE);
                exit(EXIT_SUCCESS);
            }
            case 1: { // max_instruction
                argument->max_instruction =
                    atoi(optarg); // Note: assuming it is a digit for now. (But user could enter anything)
                break;
            }
            case 2: { // format
                if (strcmp(optarg, "auto") == 0)
                    argument->format = AUTO;
                else if (strcmp(optarg, "elf") == 0)
                    argument->format = ELF;
                else if (strcmp(optarg, "bin") == 0)
                    argument->format = BIN;
                else {
                    printf("Incorrect argument type for format. Format must be auto, elf, or bin\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 3: { // riscv-tests
                argument->mode = RISCV_TESTS;
                break;
            }
            case 4: { // poison-ram
                argument->poison_ram = true;
                break;
            }
            }
        } else if (c == '?') {
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc) {
        printf("Missing program file. Please specify program file\n");
        exit(EXIT_FAILURE);
    }

    if (optind == argc - 1) {
        argument->file = argv[optind];
    }

    if (optind < argc - 1) {
        printf("Provided more than one program files.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// -------------------------------------------------------------------
// Checker for different test suites
// -------------------------------------------------------------------

static bool check_riscv_tests_result(cpu_t *cpu) {
    if (cpu->regs[10] == 0) {
        LOG_INFO("RISCV TESTS SUITE: TEST PASS");
    } else {
        LOG_ERROR("RISCV TESTS SUITE: TEST FAILED");
        LOG_ERROR("Failed test case: %ld", cpu->regs[11]);
    }
    return cpu->regs[10];
}

// -------------------------------------------------------------------
// Main function
// -------------------------------------------------------------------
int main(int argc, char **argv) {

    cpu_t      cpu;
    memory_t   memory;
    uint32_t   inst;
    argument_t argument   = {0, AUTO, NORMAL, NULL, false};
    long       inst_count = 0;
    int        result     = 0;

    // process the argument
    parse_arguments(argc, argv, &argument);
    LOG_INFO("Running: %s", argument.file);

    // initialize cpu and memory
    cpu_init(&cpu);
    if (memory_init(&memory, argument.poison_ram) != 0) {
        return EXIT_FAILURE;
    }
    // read the program
    switch (argument.format) {
    case AUTO: {
        result = memory_load_auto(&memory, argument.file, &cpu.pc);
        break;
    }
    case BIN: {
        result = memory_load_binary(&memory, argument.file);
        break;
    }
    case ELF: {
        result = memory_load_elf(&memory, argument.file, &cpu.pc);
        break;
    }
    }
    if (result != 0) {
        memory_free(&memory);
        return EXIT_FAILURE;
    }

    // execute instruction
    while (!cpu.halted) {
        if (memory_cpu_read(&memory, cpu.pc, 4, &inst) != 0) {
            LOG_ERROR("Memory read failed. Unable to fetch instruction");
            memory_free(&memory);
            return EXIT_FAILURE;
        }
        if (cpu_execute(&cpu, inst, &memory) != 0) {
            memory_free(&memory);
            LOG_ERROR("CPU execution failed");
            return EXIT_FAILURE;
        }

        inst_count++;
        if (argument.max_instruction > 0 && argument.max_instruction <= inst_count) {
            LOG_ERROR("Reach maximum instruction count but the program has not finished yet");
            memory_free(&memory);
            if (argument.mode == RISCV_TESTS) {
                LOG_ERROR("RISCV TESTS SUITE: TEST TIMEOUT");
            }
            return EXIT_FAILURE;
        }
    }

    LOG_INFO("CPU execution halted normally");
    memory_free(&memory);

    if (argument.mode == RISCV_TESTS) {
        if (check_riscv_tests_result(&cpu) == 0) {
            return EXIT_SUCCESS;
        } else {
            return EXIT_FAILURE;
        }
    }
    // Normal bare-metal programs report their result through a0.
    else {
        if (cpu.regs[10] == 0) {
            return EXIT_SUCCESS;
        } else {
            // Keep the host exit status portable instead of returning a0 directly.
            LOG_ERROR("Guest program failed. a0 = %ld", cpu.regs[10]);
            return EXIT_FAILURE;
        }
    }
}
