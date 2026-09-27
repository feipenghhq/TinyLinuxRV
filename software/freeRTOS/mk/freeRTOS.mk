# Common Makefile for building FreeRTOS for TinyLinuxRV.

# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

REPO_ROOT   := $(shell git rev-parse --show-toplevel)
RUNTIME_DIR := $(REPO_ROOT)/software/freeRTOS/runtime

FREERTOS_DIR 		   := $(REPO_ROOT)/third-party/freertos-kernel
FREERTOS_RISCV_DIR 	   := $(REPO_ROOT)/third-party/freertos-kernel/portable/GCC/RISC-V
FREERTOS_RISCV_EXT_DIR := $(FREERTOS_RISCV_DIR)/chip_specific_extensions/RISCV_MTIME_CLINT_no_extensions

# -----------------------------------------------------------------------------
# RISC-V cross-toolchain
# -----------------------------------------------------------------------------

CROSS_COMPILE ?= riscv64-unknown-elf-

CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

ARCH  := rv64ima
ABI   := lp64

# medany allows code and data to live at the DRAM base, 0x80000000.
ARCH_FLAGS := 	-march=$(ARCH) \
				-mabi=$(ABI) \
				-mcmodel=medany

# Do not depend on a hosted C runtime or Linux PIE and stack-protector defaults.
COMMON_FLAGS := -ffreestanding \
				-fno-pie \
				-fno-pic \
				-fno-stack-protector

# Get C library header from picolibc
PICOLIBC_SPECS := -specs=/usr/lib/picolibc/riscv64-unknown-elf/picolibc.specs

# Linker Script
LINKER_SCRIPT := $(RUNTIME_DIR)/linker.ld

CPPFLAGS += -I$(FREERTOS_DIR)/include
CPPFLAGS += -I$(FREERTOS_RISCV_DIR)
CPPFLAGS += -I$(FREERTOS_RISCV_EXT_DIR)
CPPFLAGS += -I$(RUNTIME_DIR)

CFLAGS += $(ARCH_FLAGS)
CFLAGS += $(COMMON_FLAGS)
CFLAGS += $(PICOLIBC_SPECS)

ASFLAGS += $(ARCH_FLAGS)
ASFLAGS += $(COMMON_FLAGS)

LDFLAGS += $(ARCH_FLAGS)
LDFLAGS += $(PICOLIBC_SPECS)
LDFLAGS += -nostartfiles
LDFLAGS += -T $(LINKER_SCRIPT)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

#  Source file from freertos
SRCS += $(FREERTOS_DIR)/tasks.c
SRCS += $(FREERTOS_DIR)/list.c
SRCS += $(FREERTOS_DIR)/list.c
SRCS += $(FREERTOS_DIR)/portable/MemMang/heap_4.c
#SRCS += $(FREERTOS_DIR)/queue.c
#SRCS += $(FREERTOS_DIR)/timers.c
#SRCS += $(FREERTOS_DIR)/event_groups.c

SRCS += $(FREERTOS_RISCV_DIR)/port.c
SRCS += $(RUNTIME_DIR)/riscv-virt.c
SRCS += $(RUNTIME_DIR)/ns16550.c
SRCS += $(RUNTIME_DIR)/console.c

ASMS += $(FREERTOS_RISCV_DIR)/portASM.S
ASMS += $(RUNTIME_DIR)/start.S
//ASMS += $(RUNTIME_DIR)/vector.S


# -----------------------------------------------------------------------------
# Common build rule
# -----------------------------------------------------------------------------

BUILD_DIR := build

PROGRAM ?=

OBJS += $(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(SRCS)))
OBJS += $(patsubst %.S, $(BUILD_DIR)/%.o, $(notdir $(ASMS)))

ELFS += $(addsuffix .elf, $(BUILD_DIR)/$(PROGRAM))

VPATH = $(FREERTOS_DIR) $(FREERTOS_RISCV_DIR) $(FREERTOS_DIR)/portable/MemMang $(RUNTIME_DIR)

all: $(OBJS) $(ELFS)

$(ELFS): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.S
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(ASFLAGS) $< -o $@

clean:
	rm -r $(BUILD_DIR)
