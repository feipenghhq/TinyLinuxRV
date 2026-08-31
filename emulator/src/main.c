#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "device.h"
#include "iringbuf.h"
#include "log.h"
#include "memory.h"
#include "uart16550.h"

extern bool poweroff_requested;
extern bool reboot_requested;

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
    size_t      dram_size;
    bool        trace;
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
    "        Fill ram content to 0xA5 before loading the program. Used mainly for testing.\n\n"
    "--dram-size\n"
    "        Assign DRAM size (in MiB). Default is 128MiB. Support 1MiB to 512MiB.\n\n"
    "--trace\n"
    "        Dump debug trace when cpu execution failed.\n\n";

static struct option longopts[] = {
    {"help", no_argument, 0, 0},         {"max-instruction", required_argument, 0, 0},
    {"format", required_argument, 0, 0}, {"riscv-tests", no_argument, 0, 0},
    {"poison-ram", no_argument, 0, 0},   {"dram-size", required_argument, 0, 0},
    {"trace", no_argument, 0, 0},        {0, 0, 0, 0},
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
            case 5: { // dram-size
                size_t dram_size_mib = (size_t)atoi(optarg);
                argument->dram_size  = dram_size_mib * (1024 * 1024);
                if (dram_size_mib <= 0 || dram_size_mib > 512) {
                    printf("Unsupported dram size\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 6: { // trace
                argument->trace = true;
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
// Common poweroff function
// -------------------------------------------------------------------
int poweroff(memory_t *memory, dev_list_t *devices) {
    memory_free(memory);
    device_free(devices);
    return 0;
}

// -------------------------------------------------------------------
// Common boot function
// -------------------------------------------------------------------
int boot(memory_t *memory, dev_list_t *devices, cpu_t *cpu, argument_t *argument) {
    int result = 0;

    // initialize cpu
    cpu_init(cpu);

    // initialize memory
    if (memory_init(memory, argument->poison_ram, argument->dram_size) != 0) {
        return EXIT_FAILURE;
    }

    // initialize device
    if (device_init(devices) != 0) {
        return EXIT_FAILURE;
    }

    // read the program
    switch (argument->format) {
    case AUTO: {
        result = memory_load_auto(memory, argument->file, &cpu->pc);
        break;
    }
    case BIN: {
        result = memory_load_binary(memory, argument->file);
        break;
    }
    case ELF: {
        result = memory_load_elf(memory, argument->file, &cpu->pc);
        break;
    }
    }
    if (result != 0) {
        poweroff(memory, devices);
        return EXIT_FAILURE;
    }
    return 0;
}

// -------------------------------------------------------------------
// Common reset function
// -------------------------------------------------------------------
int reset(dev_list_t *devices, cpu_t *cpu) {

    // re-initialize cpu
    cpu_init(cpu);

    // reset device
    if (device_reset(devices) != 0) {
        return -1;
    }

    // Noting to be done for memory
    return 0;
}

// -------------------------------------------------------------------
// Main function
// -------------------------------------------------------------------
int main(int argc, char **argv) {

    cpu_t      cpu;
    dev_list_t devices;
    memory_t   memory;

    uint32_t   inst;
    argument_t argument   = {0, AUTO, NORMAL, NULL, false, RAM_SIZE, false};
    long       inst_count = 0;

    // process the argument
    parse_arguments(argc, argv, &argument);
    LOG_INFO("Running: %s", argument.file);

    // boot and initialize all the component
    if (boot(&memory, &devices, &cpu, &argument) != 0) {
        return EXIT_FAILURE;
    }

    // main instruction execution loop
    while (!cpu.halted) {
        // read instruction from memory
        if (memory_cpu_read(&memory, &devices, cpu.pc, 4, &inst) != 0) {
            LOG_ERROR("Memory read failed. Unable to fetch instruction");
            poweroff(&memory, &devices);
            return EXIT_FAILURE;
        }

        if (argument.trace) {
            iringbuf_write(cpu.pc, inst);
        }
        // execute the instruction
        if (cpu_execute(&cpu, inst, &memory, &devices) != 0) {
            poweroff(&memory, &devices);
            LOG_ERROR("CPU execution failed");
            if (argument.trace) {
                iringbuf_print();
                cpu_print_regs(&cpu);
            }
            return EXIT_FAILURE;
        }

        // check poweroff/reboot
        if (poweroff_requested) {
            LOG_INFO("Poweroff requested");
            break; // Exit the execution loop
        }

        if (reboot_requested) {
            LOG_INFO("Reboot requested");
            if (reset(&devices, &cpu) != 0) {
                LOG_ERROR("Failed to reset the devices");
                poweroff(&memory, &devices);
                return EXIT_FAILURE;
            }
        }

        // check instruction limit
        inst_count++;
        if (argument.max_instruction > 0 && argument.max_instruction <= inst_count) {
            LOG_ERROR("Reach maximum instruction count but the program has not finished yet");
            poweroff(&memory, &devices);
            if (argument.mode == RISCV_TESTS) {
                LOG_ERROR("RISCV TESTS SUITE: TEST TIMEOUT");
            }
            return EXIT_FAILURE;
        }

        // Device polling, continue even if failed to pull from FIFO
        uart16550_poll_input(devices.uart0.device);
    }

    // execution completed
    LOG_INFO("CPU execution halted normally");
    poweroff(&memory, &devices);
    // Check result
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
