#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addrmap.h"
#include "addrmap_emu.h"
#include "bus/bus.h"
#include "cpu/cpu.h"
#include "cpu/cpu_exec.h"
#include "device/device.h"
#include "memory/memory.h"
#include "utils/func_trace.h"
#include "utils/iringbuf.h"
#include "utils/log.h"
#include "utils/symtable.h"

// -------------------------------------------------------------------
// Different type enum
// -------------------------------------------------------------------
typedef enum { AUTO, BIN, ELF } file_type_t;

typedef enum { RUN_BAREMETAL, RUN_RISCV_TESTS, RUN_LINUX } run_mode_t;

typedef enum { BOOT_MODE_DIRECT, BOOT_MODE_BOOTROM } boot_mode_t;

typedef struct {
    // run mode
    run_mode_t run_mode;

    // boot options
    boot_mode_t boot_mode;
    char       *bootrom;
    char       *bios;
    char       *kernel;
    char       *dtb;

    // machine options
    size_t dram_size;

    // execution options
    long max_instruction;
    bool poison_ram;
    bool itrace;
    bool ftrace;

    // program options
    file_type_t format;

    // others
    char *file;
} args_t;

// -------------------------------------------------------------------
// Command line parser
// -------------------------------------------------------------------

typedef struct {
    const char *name;
    int         key;
    int         has_arg;
    const char *arg_name;
    const char *help;
} cli_option_t;

enum {
    OPT_HELP = 256,
    OPT_BOOTROM,
    OPT_BIOS,
    OPT_KERNEL,
    OPT_DTB,
    OPT_DRAM_SIZE,
    OPT_MAX_INST,
    OPT_POISON,
    OPT_ITRACE,
    OPT_FTRACE,
    OPT_FORMAT,
    OPT_RISCV_TESTS
};

static const cli_option_t options[] = {
    // Help
    {"help", OPT_HELP, no_argument, NULL, "Show this help message."},
    // boot options
    {"bootrom", OPT_BOOTROM, required_argument, "file", "Load boot ROM image."},
    {"bios", OPT_BIOS, required_argument, "file", "Load firmware image."},
    {"kernel", OPT_KERNEL, required_argument, "file", "Load kernel image."},
    {"dtb", OPT_DTB, required_argument, "file", "Load device tree blob."},
    // machine options
    {"dram-size", OPT_DRAM_SIZE, required_argument, "MiB", "Set DRAM size (In MiB). Default: 128."},
    // execution options
    {"max-instruction", OPT_MAX_INST, required_argument, "count",
     "Stop after executing the specified number of instructions."},
    {"poison-ram", OPT_POISON, no_argument, NULL, "Fill RAM with 0xA5 before loading images."},
    {"itrace", OPT_ITRACE, no_argument, NULL, "Dump instruction trace when CPU execution fails."},
    {"ftrace", OPT_FTRACE, no_argument, NULL, "Dump function trace."},
    // Program options
    {"format", OPT_FORMAT, required_argument, "auto|elf|bin", "Input format for positional FILE."},
    // Test options
    {"riscv-tests", OPT_RISCV_TESTS, no_argument, 0, "Run RISC-V tests."},

};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static const size_t option_count = ARRAY_SIZE(options);

static void print_usage(void) {
    printf("  rvemu [OPTIONS] [FILE]\n\n");
    for (size_t i = 0; i < option_count; i++) {
        const cli_option_t *option = &options[i];
        printf("  --%s", option->name);
        if (option->has_arg == required_argument)
            printf(" <%s>\n", option->arg_name);
        else
            printf("\n");
        printf("        %s\n\n", option->help);
    }
}

static struct option *build_longopts(void) {
    struct option *longopts = calloc(option_count + 1, sizeof(*longopts));

    if (!longopts) {
        LOG_ERROR("Cannot allocate memory for CLI options");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < option_count; i++) {
        longopts[i].name    = options[i].name;
        longopts[i].has_arg = options[i].has_arg;
        longopts[i].flag    = NULL;
        longopts[i].val     = options[i].key;
    }

    // Note: end condition taken care by calloc

    return longopts;
}

static void parse_arguments(int argc, char **argv, args_t *args) {

    struct option *longopts = build_longopts();

    if (argc < 2) {
        printf("Incorrect arguments. Please see usage:\n\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    // default value
    args->boot_mode = BOOT_MODE_DIRECT;

    while (1) {
        int c = getopt_long(argc, argv, "", longopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case OPT_HELP:
            print_usage();
            exit(EXIT_SUCCESS);

        case OPT_BOOTROM:
            args->bootrom   = optarg;
            args->boot_mode = BOOT_MODE_BOOTROM;
            args->run_mode  = RUN_LINUX;
            break;

        case OPT_BIOS:
            args->bios = optarg;
            break;

        case OPT_KERNEL:
            args->kernel = optarg;
            break;

        case OPT_DTB:
            args->dtb = optarg;
            break;

        case OPT_DRAM_SIZE: {
            char *end;
            long  value = strtol(optarg, &end, 10);

            if (errno == ERANGE || *end != '\0' || value <= 0 || value > 512) {
                printf("Invalid dram size\n");
                exit(EXIT_FAILURE);
            }

            args->dram_size = (size_t)value * (1024 * 1024);
            break;
        }

        case OPT_MAX_INST: {
            char *end;
            long  value = strtol(optarg, &end, 10);

            if (errno == ERANGE || *end != '\0' || value <= 0) {
                fprintf(stderr, "Invalid instruction count: %s\n", optarg);
                exit(EXIT_FAILURE);
            }

            args->max_instruction = value;
            break;
        }

        case OPT_POISON:
            args->poison_ram = true;
            break;

        case OPT_ITRACE:
            args->itrace = true;
            break;

        case OPT_FTRACE:
            args->ftrace = true;
            break;

        case OPT_FORMAT: { // format
            if (strcmp(optarg, "auto") == 0)
                args->format = AUTO;
            else if (strcmp(optarg, "elf") == 0)
                args->format = ELF;
            else if (strcmp(optarg, "bin") == 0)
                args->format = BIN;
            else {
                printf("Incorrect args type for format. Format must be auto, elf, or bin\n");
                exit(EXIT_FAILURE);
            }
            break;
        }

        case OPT_RISCV_TESTS: // riscv-tests
            args->run_mode = RUN_RISCV_TESTS;
            break;

        case '?':
            exit(EXIT_FAILURE);
        }
    }

    if (args->boot_mode == BOOT_MODE_DIRECT) {
        if (optind == argc) {
            printf("Missing program file. Please specify program file\n");
            exit(EXIT_FAILURE);
        }

        if (optind == argc - 1) {
            args->file = argv[optind];
        }

        if (optind < argc - 1) {
            printf("Provided more than one program files.\n");
            exit(EXIT_FAILURE);
        }
        LOG_INFO("Running: %s", args->file);
    } else {
        LOG_INFO("Running on Linux Mode.");
        LOG_INFO("BootROM: %s", args->bootrom);
        LOG_INFO("DTB: %s", args->dtb);
        LOG_INFO("BIOS: %s", args->bios);
        LOG_INFO("Kernel: %s", args->kernel);
    }

    free(longopts);
}

// -------------------------------------------------------------------
// Checker for different test suites
// -------------------------------------------------------------------

static bool check_riscv_tests_result(cpu_t *cpu) {
    if (cpu->regs[10] == 0) {
        LOG_INFO("RISCV TESTS SUITE: TEST PASS");
    } else {
        LOG_ERROR("RISCV TESTS SUITE: TEST FAILED");
        LOG_ERROR("Return code: %ld", cpu->regs[10]);
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

static int boot(memory_t *memory, dev_list_t *devices, cpu_t *cpu, args_t *args) {
    int result = 0;

    // initialize cpu
    cpu_init(cpu);

    // initialize memory
    if (memory_init(memory, args->poison_ram, args->dram_size) != 0) {
        // No need to free memory as when init failed. the memory will not be allocated
        return -1;
    }

    // initialize device
    if (device_init(devices) != 0) {
        poweroff(memory, devices);
        return -1;
    }

    // initialize the symbol table. Will only be initialized when trace is enabled
    if (args->ftrace)
        symbol_table_init(1);

    // read the program
    if (args->boot_mode == BOOT_MODE_DIRECT) {
        switch (args->format) {
        case AUTO: {
            result = memory_load_auto(memory, args->file, &cpu->pc, args->ftrace);
            break;
        }
        case BIN: {
            result = memory_load_binary(memory, args->file);
            break;
        }
        case ELF: {
            result = memory_load_elf(memory, args->file, &cpu->pc, args->ftrace);
            break;
        }
        }
    }

    // read the bootrom/dtb
    if (args->boot_mode == BOOT_MODE_BOOTROM) {
        uint64_t entry_point; // dummy entry point

        // BootROM
        if (args->bootrom) {
            result += memory_load_elf(devices->bootROM.device, args->bootrom, &entry_point, args->ftrace);
        } else {
            LOG_ERROR("BootROM does not exist. Please specify bootROM ELF");
            result++;   // In BOOTROM mode we need at least bootROM
        }
        // DTB
        // DTB is loaded into the dram at DRAM.END - 1MiB.
        // We need to create an memory device aliased with dram but starting at dtb address to work with
        // memory_load_binary
        memory_t dtb_segment = {NULL, DTB_START_ADDR, DTB_SIZE};
        dtb_segment.data     = &memory->data[DTB_START_ADDR - DRAM_BASE];
        if (args->dtb) {
            result += memory_load_binary(&dtb_segment, args->dtb);
        }

        // BIOS
        // BIOS is loaded into the dram at DRAM.BASE
        if (args->bios) {
            result += memory_load_elf(memory, args->bios, &entry_point, args->ftrace);
        }

        // Kernel
        // Kernel is loaded into the dram at DRAM.BASE + 0x200000
        // The kernel image is linked directly to DRAM.BASE + 0x200000
        if (args->kernel) {
            result += memory_load_elf(memory, args->kernel, &entry_point, args->ftrace);
        }

        // CPU start from boot rom
        cpu->pc = BootROM_BASE;
    }

    // clean up the memory
    if (result != 0) {
        poweroff(memory, devices);
        return -1;
    }

    // initialize function trace
    const char *func_trace_file = "func_trace.log";
    if (args->ftrace) {
        result = func_trace_init(func_trace_file);
        if (result != 0) {
            return -1;
        }
    }

    return 0;
}

// -------------------------------------------------------------------
// Main function
// -------------------------------------------------------------------
int main(int argc, char **argv) {
    args_t     args = {.max_instruction = 0,
                       .format          = AUTO,
                       .run_mode        = RUN_BAREMETAL,
                       .file            = NULL,
                       .poison_ram      = false,
                       .dram_size       = RAM_SIZE,
                       .itrace          = false,
                       .ftrace          = false};
    cpu_t      cpu;
    dev_list_t devices;
    memory_t   memory;
    bus_t      bus;

    CPU_EXEC_STATUS_t exec_status = FINISH;

    bus.devices = &devices;
    bus.memory  = &memory;

    // process the args
    parse_arguments(argc, argv, &args);

    // boot and initialize all the component
    if (boot(&memory, &devices, &cpu, &args) != 0) {
        // exit the execution directly if boot failed. The memory has been freed in boot function.
        return EXIT_FAILURE;
    }

    exec_status = cpu_exec(&cpu, &bus, &devices, args.itrace, args.ftrace, args.max_instruction);

    // free up memory
    poweroff(&memory, &devices);
    if (args.ftrace) {
        func_trace_end();
        symbol_table_free();
    }

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
        if (args.itrace) {
            iringbuf_print();
            cpu_print_regs(&cpu);
        }
        return EXIT_FAILURE;
    }
    }

    // Check result
    if (args.run_mode == RUN_LINUX) {
        return EXIT_SUCCESS;
    }
    else if (args.run_mode == RUN_RISCV_TESTS) {
        if (check_riscv_tests_result(&cpu) == 0) {
            return EXIT_SUCCESS;
        } else {
            if (args.itrace) {
                iringbuf_print();
                cpu_print_regs(&cpu);
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
