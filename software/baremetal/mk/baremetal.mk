# Common Makefile for building bare-metal programs for RISC-V architecture.

# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

LINKER_SCRIPT := $(BAREMETAL_ROOT)/runtime/linker.ld

RUNTIME_DIR := $(BAREMETAL_ROOT)/runtime

# -----------------------------------------------------------------------------
# RISC-V cross-toolchain
# -----------------------------------------------------------------------------

CROSS_COMPILE ?= riscv64-linux-gnu-
CC      := $(CROSS_COMPILE)gcc
AS      := $(CROSS_COMPILE)as
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

ARCH  = rv64i
ABI   = lp64

# medany allows code and data to live at the DRAM base, 0x80000000.
ARCH_FLAGS := -march=$(ARCH) -mabi=$(ABI) -mcmodel=medany

# Do not depend on a hosted C runtime or Linux PIE and stack-protector defaults.
COMMON_FLAGS := -ffreestanding -fno-pie -fno-pic -fno-stack-protector

CPPFLAGS +=

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

all: $(ELFS) $(BINS) $(RUNTIME_OBJS)

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) $< -O binary $@

$(ELFS): $(OBJS) $(RUNTIME_OBJS) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) $(filter %.o,$^) -o $@

$(BAREMETAL_ROOT)/build/runtime/%.o: $(RUNTIME_DIR)/%.S
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
