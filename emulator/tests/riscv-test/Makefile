# Build the riscv-tests suites used by the TinyLinuxRV emulator.

.DEFAULT_GOAL := all

# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

REPO_ROOT      := $(shell git rev-parse --show-toplevel)
RISCV_TESTS    := $(REPO_ROOT)/third-party/riscv-tests
ISA_DIR        := $(RISCV_TESTS)/isa
ENV_DIR        := env
BUILD_DIR      := build
LINKER_SCRIPT  := $(ENV_DIR)/link.ld

# The test sources include riscv_test.h from our emulator-specific environment
# and test_macros.h from the upstream scalar test macros.
CPPFLAGS := -I$(ENV_DIR)
CPPFLAGS += -I$(ISA_DIR)/macros/scalar

ENV_HEADERS := $(wildcard $(ENV_DIR)/*.h)
TEST_MACROS := $(ISA_DIR)/macros/scalar/test_macros.h

# -----------------------------------------------------------------------------
# RISC-V cross-toolchain
# -----------------------------------------------------------------------------

CROSS_COMPILE ?= riscv64-linux-gnu-
CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

LDFLAGS := -T $(LINKER_SCRIPT)

# -----------------------------------------------------------------------------
# Enabled test suites and their compiler settings
# -----------------------------------------------------------------------------

# Only enabled suites are included in `all`. Add a suite when the emulator
# implements the corresponding ISA extension.
SUITES ?= rv64ui rv64um

rv64ui_MARCH := rv64i
rv64ui_MABI  := lp64

rv64um_MARCH := rv64im
rv64um_MABI  := lp64

# Future examples:
#
# SUITES += rv64ua
# rv64ua_MARCH := rv64ia
# rv64ua_MABI  := lp64

# Each upstream Makefrag defines a list such as rv64ui_sc_tests.
include $(addsuffix /Makefrag,$(addprefix $(ISA_DIR)/,$(SUITES)))

# -----------------------------------------------------------------------------
# Per-suite build rules
# -----------------------------------------------------------------------------

# Instantiate the same source -> object -> ELF -> binary pipeline for every
# enabled suite. Outputs include the suite name so tests with the same filename
# cannot overwrite each other, for example build/rv64ui/add.bin.
#
# This template is expanded by call/eval before Make executes its recipes.
# Double dollar signs preserve Make variables and automatic variables such as
# $@ and $< until the generated rule is evaluated.
define SUITE_template

$(1)_NAMES    := $$($(1)_sc_tests)
$(1)_OBJECTS  := $$(addprefix $(BUILD_DIR)/$(1)/,$$(addsuffix .o,$$($(1)_NAMES)))
$(1)_ELFS     := $$(addprefix $(BUILD_DIR)/$(1)/,$$(addsuffix .elf,$$($(1)_NAMES)))
$(1)_BINS     := $$(addprefix $(BUILD_DIR)/$(1)/,$$(addsuffix .bin,$$($(1)_NAMES)))
$(1)_MANIFEST := $(BUILD_DIR)/$(1)/tests.txt

$(1): $$($(1)_OBJECTS) $$($(1)_ELFS) $$($(1)_BINS) $$($(1)_MANIFEST)

$(BUILD_DIR)/$(1)/%.o: $(ISA_DIR)/$(1)/%.S $(ENV_HEADERS) $(TEST_MACROS)
	mkdir -p $$(@D)
	$$(CC) -c \
		-march=$$($(1)_MARCH) \
		-mabi=$$($(1)_MABI) \
		$$(CPPFLAGS) \
		$$< -o $$@

$(BUILD_DIR)/$(1)/%.elf: $(BUILD_DIR)/$(1)/%.o $(LINKER_SCRIPT)
	$$(LD) $$(LDFLAGS) $$< -o $$@

$(BUILD_DIR)/$(1)/%.bin: $(BUILD_DIR)/$(1)/%.elf
	$$(OBJCOPY) -O binary $$< $$@


$$($(1)_MANIFEST): Makefile $(ISA_DIR)/$(1)/Makefrag | $$($(1)_BINS)
	@printf '%s\n' $$($(1)_BINS) > $$@

endef

$(foreach suite,$(SUITES),$(eval $(call SUITE_template,$(suite))))

# -----------------------------------------------------------------------------
# Top-level targets
# -----------------------------------------------------------------------------

.PHONY: all clean $(SUITES)

all: $(SUITES)

clean:
	rm -rf $(BUILD_DIR)
