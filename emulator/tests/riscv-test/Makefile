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
CPPFLAGS += -I$(ENV_DIR)
CPPFLAGS += -I$(ISA_DIR)/macros/scalar
CPPFLAGS += -DRVTEST_HAS_CSR

# Get C library header from picolibc
PICOLIBC_SPECS := -specs=/usr/lib/picolibc/riscv64-unknown-elf/picolibc.specs

CFLAGS += -nostdlib
CFLAGS += -static
CFLAGS += -mcmodel=medany
CFLAGS += -T $(LINKER_SCRIPT)
CFLAGS += $(PICOLIBC_SPECS)
CFLAGS += -DENTROPY=1

TEST_MACROS := $(ISA_DIR)/macros/scalar/test_macros.h

# -----------------------------------------------------------------------------
# RISC-V cross-toolchain
# -----------------------------------------------------------------------------

CROSS_COMPILE ?= riscv64-unknown-elf-
CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

# -----------------------------------------------------------------------------
# Enabled test suites and their compiler settings
# -----------------------------------------------------------------------------

# Only enabled suites are included in `all`. Add a suite when the emulator
# implements the corresponding ISA extension.
SUITES-all ?= rv64ui rv64um rv64ua rv64mi rv64si

SUITES-v ?= rv64ui

MABI  := lp64

rv64ui_p_MARCH := rv64if
rv64um_p_MARCH := rv64imf
rv64ua_p_MARCH := rv64iaf
rv64mi_p_MARCH := rv64if
rv64si_p_MARCH := rv64if

rv64ui_v_MARCH := rv64imaf
rv64um_v_MARCH := rv64imaf
rv64ua_v_MARCH := rv64imaf
rv64mi_v_MARCH := rv64imaf
rv64si_v_MARCH := rv64imaf

# Each upstream Makefrag defines a list such as rv64ui_sc_tests.
include $(addsuffix /Makefrag,$(addprefix $(ISA_DIR)/,$(SUITES-all)))

# -----------------------------------------------------------------------------
# Additional Source file
# -----------------------------------------------------------------------------

p_EXTRA_SRCS :=
v_EXTRA_SRCS := $(ENV_DIR)/v/entry.S \
                $(ENV_DIR)/v/vm.c \
                $(ENV_DIR)/v/string.c

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

ENV_HEADERS := $(wildcard $(ENV_DIR)/$(2)/*.h)

$(1)_$(2)_NAMES    := $$($(1)_sc_tests)
$(1)_$(2)_OBJECTS  := $$(addprefix $(BUILD_DIR)/$(2)/$(1)/,$$(addsuffix .o,$$($(1)_$(2)_NAMES)))
$(1)_$(2)_ELFS     := $$(addprefix $(BUILD_DIR)/$(2)/$(1)/,$$(addsuffix .elf,$$($(1)_$(2)_NAMES)))
$(1)_$(2)_BINS     := $$(addprefix $(BUILD_DIR)/$(2)/$(1)/,$$(addsuffix .bin,$$($(1)_$(2)_NAMES)))
$(1)_$(2)_MANIFEST := $(BUILD_DIR)/$(2)/$(1)/tests.txt

$(1)-$(2): $$($(1)_$(2)_ELFS) $$($(1)_$(2)_BINS) $$($(1)_$(2)_MANIFEST)

$(BUILD_DIR)/$(2)/$(1)/%.elf: $(ISA_DIR)/$(1)/%.S $(ENV_HEADERS) $(TEST_MACROS) $($(2)_EXTRA_SRCS)
	mkdir -p $$(@D)
	$$(CC) \
		-march=$$($(1)_$(2)_MARCH) \
		-mabi=$$(MABI) \
		$$(CPPFLAGS) \
		$$(CFLAGS) \
		-I$(ENV_DIR)/$(2) \
		$$< $($(2)_EXTRA_SRCS) \
		-o $$@

$(BUILD_DIR)/$(2)/$(1)/%.bin: $(BUILD_DIR)/$(2)/$(1)/%.elf
	$$(OBJCOPY) -O binary $$< $$@

$$($(1)_$(2)_MANIFEST): Makefile $(ISA_DIR)/$(1)/Makefrag | $$($(1)_$(2)_BINS)
	@printf '%s\n' $$($(1)_$(2)_BINS) > $$@

endef

$(foreach suite,$(SUITES-all),$(eval $(call SUITE_template,$(suite),p)))
$(foreach suite,$(SUITES-v),$(eval $(call SUITE_template,$(suite),v)))

# -----------------------------------------------------------------------------
# Top-level targets
# -----------------------------------------------------------------------------

.PHONY: all clean $(SUITES-all)

P_TARGETS := $(addsuffix -p,$(SUITES-all))
V_TARGETS := $(addsuffix -v,$(SUITES-v))

all: $(P_TARGETS) $(V_TARGETS)

clean:
	rm -rf $(BUILD_DIR)
