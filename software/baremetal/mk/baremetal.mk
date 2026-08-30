# Common Makefile for building bare-metal programs for RISC-V architecture.

# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

LINKER_SCRIPT := $(BAREMETAL_ROOT)/runtime/linker.ld

RUNTIME_DIR := $(BAREMETAL_ROOT)/runtime
DRIVERS_DIR := $(BAREMETAL_ROOT)/drivers

PLATFORM_DIR := $(BAREMETAL_ROOT)/../../platform

# -----------------------------------------------------------------------------
# RISC-V cross-toolchain
# -----------------------------------------------------------------------------

CROSS_COMPILE ?= riscv64-linux-gnu-
CC      := $(CROSS_COMPILE)gcc
AS      := $(CROSS_COMPILE)as
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

ARCH  = rv64ima
ABI   = lp64

# medany allows code and data to live at the DRAM base, 0x80000000.
ARCH_FLAGS := -march=$(ARCH) -mabi=$(ABI) -mcmodel=medany

# Do not depend on a hosted C runtime or Linux PIE and stack-protector defaults.
COMMON_FLAGS := -ffreestanding -fno-pie -fno-pic -fno-stack-protector

CPPFLAGS += -I$(PLATFORM_DIR)/include/tinylinuxrv
CPPFLAGS += -I$(DRIVERS_DIR)/include
CPPFLAGS += -I$(DRIVERS_DIR)/uart16550

CFLAGS += -std=c99
CFLAGS += $(ARCH_FLAGS) $(COMMON_FLAGS)
CFLAGS += -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion

ASFLAGS += $(ARCH_FLAGS) $(COMMON_FLAGS)

LDFLAGS  += -T $(LINKER_SCRIPT)

# -----------------------------------------------------------------------------
# Common build rule
# -----------------------------------------------------------------------------

.PHONY: all clean

OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))
ELFS := $(addsuffix .elf,$(BUILD_DIR)/$(PROGRAM))
BINS := $(addsuffix .bin,$(BUILD_DIR)/$(PROGRAM))

RUNTIME_SRCS := $(RUNTIME_DIR)/crt0.S
RUNTIME_OBJS := $(patsubst $(RUNTIME_DIR)/%.S, $(BAREMETAL_ROOT)/build/runtime/%.o, $(RUNTIME_SRCS))

DRIVERS_SRCS := $(DRIVERS_DIR)/uart16550/uart16550.c
DRIVERS_OBJS := $(patsubst $(DRIVERS_DIR)/%.c, $(BAREMETAL_ROOT)/build/drivers/%.o, $(DRIVERS_SRCS))

all: $(ELFS) $(BINS) $(RUNTIME_OBJS) $(DRIVERS_OBJS)

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) $< -O binary $@

$(ELFS): $(OBJS) $(RUNTIME_OBJS) $(DRIVERS_OBJS) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) $(filter %.o,$^) -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

# runtime
$(BAREMETAL_ROOT)/build/runtime/%.o: $(RUNTIME_DIR)/%.S
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(ASFLAGS) $< -o $@

# driver
$(BAREMETAL_ROOT)/build/drivers/%.o: $(DRIVERS_DIR)/%.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
