# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

BAREMETAL_DIR = ../software/baremetal


# -----------------------------------------------------------------------------
# GCC toolchain
# -----------------------------------------------------------------------------

LOG_LEVEL ?= LOG_LEVEL_INFO

CC       := gcc
CPPFLAGS := -Iinclude
CPPFLAGS += -DLOG_LEVEL=$(LOG_LEVEL)
CFLAGS 	 := -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -O2
DEPFLAGS := -MMD -MP
LDFLAGS  :=
LDLIBS   :=

#-------------------------------------------------
# Build emulator
#-------------------------------------------------

EMULATOR := rvemu

BUILD_DIR := build

SOURCES := src/cpu/cpu.c
SOURCES += src/memory/memory.c
SOURCES += src/memory/loader.c
SOURCES += src/main.c

OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS	:= $(OBJECTS:.o=.d)


.PHONY: build clean regression sanity-test riscv-tests run-elf run-fib run-baremetal-test

build: $(EMULATOR)

$(EMULATOR): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(EMULATOR) $(BUILD_DIR)

#-------------------------------------------------
# Run ELF file and program
#-------------------------------------------------

ELF ?=

run-elf: rvemu
	./rvemu --format elf $(ELF)

run-fib: rvemu
	$(MAKE) -C $(BAREMETAL_DIR) fib
	./rvemu --format elf $(BAREMETAL_DIR)/build/examples/fib/fib.elf

run-baremetal-test: rvemu
	$(MAKE) -C $(BAREMETAL_DIR) baremetal-test
	./rvemu --format bin --poison-ram $(BAREMETAL_DIR)/build/examples/baremetal-test/baremetal-test.bin

#-------------------------------------------------
# Run sanity test
#-------------------------------------------------

SANITY_DIR := tests/sanity/build

sanity-test: $(EMULATOR)
	$(MAKE) -C tests/sanity all
	./$(EMULATOR) $(SANITY_DIR)/rv64i.bin

#-------------------------------------------------
# Run riscv-tests test
#-------------------------------------------------

RISCV_TESTS_DIR := tests/riscv-test

riscv-tests: $(EMULATOR)
	$(MAKE) -C $(RISCV_TESTS_DIR) all
	python3 $(RISCV_TESTS_DIR)/run.py

#-------------------------------------------------
# Full regression test list for emulator
#-------------------------------------------------

regression: sanity-test riscv-tests run-fib run-baremetal-test
