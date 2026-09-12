#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus/bus.h"
#include "cpu/cpu.h"
#include "device/device.h"
#include "device/syscon.h"
#include "memory/memory.h"
#include "utils/iringbuf.h"
#include "utils/log.h"

// -------------------------------------------------------------------
// Different type enum
// -------------------------------------------------------------------
typedef enum FILE_TYPE { AUTO, BIN, ELF } FILE_TYPE_t;

typedef enum RUN_MODE { NORMAL, RISCV_TESTS } RUN_MODE_t;

typedef enum EXEC_STATUS { FINISH, MEM_ERROR, CPU_ERROR, DEVICE_ERROR, POWEROFF, TIMEOUT } EXEC_STATUS_t;

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

static void parse_arguments(int argc, char **argv, argument_t *argument) {
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
                argument->max_instruction = atoi(optarg);
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
                int dram_size_mib   = atoi(optarg);
                argument->dram_size = (size_t)dram_size_mib * (1024 * 1024);
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
static void poweroff(memory_t *memory, dev_list_t *devices) {
    memory_free(memory);
    device_free(devices);
}

// -------------------------------------------------------------------
// Common boot function
// -------------------------------------------------------------------

static int boot(memory_t *memory, dev_list_t *devices, cpu_t *cpu, argument_t *argument) {
    int result = 0;

    // initialize cpu
    cpu_init(cpu);

    // initialize memory
    if (memory_init(memory, argument->poison_ram, argument->dram_size) != 0) {
        // No need to free memory as when init failed. the memory will not be allocated
        return -1;
    }

    // initialize device
    if (device_init(devices) != 0) {
        poweroff(memory, devices);
        return -1;
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

    // clean up the memory
    if (result != 0) {
        poweroff(memory, devices);
        return -1;
    }
    return 0;
}

// -------------------------------------------------------------------
// Common reset function
// -------------------------------------------------------------------
static void reset(dev_list_t *devices, cpu_t *cpu) {
    // re-initialize cpu
    cpu_init(cpu);
    // reset device
    device_reset(devices);
    // Noting to be done for memory
}

// -------------------------------------------------------------------
// Main function
// -------------------------------------------------------------------
int main(int argc, char **argv) {
    argument_t    argument = {.max_instruction = 0,
                              .format          = AUTO,
                              .mode            = NORMAL,
                              .file            = NULL,
                              .poison_ram      = false,
                              .dram_size       = RAM_SIZE,
                              .trace           = false};
    cpu_t         cpu;
    dev_list_t    devices;
    memory_t      memory;
    bus_t         bus;
    uint32_t      inst;
    long          inst_count  = 0;
    EXEC_STATUS_t exec_status = FINISH;

    bus.devices = &devices;
    bus.memory  = &memory;

    // process the argument
    parse_arguments(argc, argv, &argument);
    LOG_INFO("Running: %s", argument.file);

    // boot and initialize all the component
    if (boot(&memory, &devices, &cpu, &argument) != 0) {
        // exit the execution directly if boot failed. The memory has been freed in boot function.
        return EXIT_FAILURE;
    }

    // main instruction execution loop
    while (!cpu.halted) {
        // read instruction from memory
        if (bus_read(&bus, cpu.pc, 4, &inst) != 0) {
            LOG_ERROR("Memory read failed. Unable to fetch instruction");
            exec_status = MEM_ERROR;
            break;
        }

        if (argument.trace) {
            iringbuf_write(cpu.pc, inst);
        }

        // execute the instruction
        cpu_execute(&cpu, inst, &bus);

        // check poweroff/reboot
        if (syscon_poweroff_requested(devices.syscon.device)) {
            LOG_INFO("Poweroff requested");
            exec_status = POWEROFF;
            break;
        }

        if (syscon_reboot_requested(devices.syscon.device)) {
            LOG_INFO("Reboot requested");
            reset(&devices, &cpu);
        }

        // check instruction limit
        inst_count++;
        if (argument.max_instruction > 0 && argument.max_instruction <= inst_count) {
            LOG_ERROR("Reach maximum instruction count but the program has not finished yet");
            exec_status = TIMEOUT;
            break;
        }

        // Device update
        if (device_update(&devices) != 0) {
            LOG_ERROR("Device poll input failed. Exiting.");
            exec_status = DEVICE_ERROR;
            break;
        }
        device_irq_level(&devices);
    }

    // free up memory
    poweroff(&memory, &devices);

    // Check execution status
    switch (exec_status) {
    case POWEROFF: // fall-through
    case FINISH: {
        LOG_INFO("CPU execution halted normally");
        break;
    }
    case MEM_ERROR:    // fall-through
    case CPU_ERROR:    // fall-through
    case DEVICE_ERROR: // fall-through
    case TIMEOUT: {
        if (argument.trace) {
            iringbuf_print();
            cpu_print_regs(&cpu);
        }
        return EXIT_FAILURE;
    }
    }

    // Check result
    if (argument.mode == RISCV_TESTS) {
        if (check_riscv_tests_result(&cpu) == 0) {
            return EXIT_SUCCESS;
        } else {
            if (argument.trace) {
                iringbuf_print();
            }
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
