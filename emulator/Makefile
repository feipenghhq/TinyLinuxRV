# Makefile for emulator

###############################################################################
#                                Build Emulator                               #
###############################################################################

# -----------------------------------------------------------------------------
# GCC toolchain
# -----------------------------------------------------------------------------

LOG_LEVEL ?= LOG_LEVEL_INFO

CC       := gcc

CPPFLAGS += -Iinclude -Iinclude/device -Iinclude/cpu -Iinclude/memory -Iinclude/utils
CPPFLAGS += -I../platform/include/tinylinuxrv

CPPFLAGS += -DLOG_LEVEL=$(LOG_LEVEL)

CFLAGS 	 += -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -O2

DEPFLAGS += -MMD -MP

LDFLAGS  +=

LDLIBS   +=

#-------------------------------------------------
# Build emulator
#-------------------------------------------------

SOURCES += src/cpu/cpu.c
SOURCES += src/cpu/cpu_exec.c
SOURCES += src/cpu/csr.c
SOURCES += src/cpu/mmu.c
SOURCES += src/cpu/tlb.c
SOURCES += src/memory/memory.c
SOURCES += src/memory/loader.c
SOURCES += src/bus/bus.c
SOURCES += src/utils/iringbuf.c
SOURCES += src/utils/func_trace.c
SOURCES += src/utils/symtable.c
SOURCES += src/device/device.c
SOURCES += src/device/syscon.c
SOURCES += src/device/uart16550.c
SOURCES += src/device/clint.c
SOURCES += src/device/plic.c
SOURCES += src/device/bootROM.c
SOURCES += src/main.c

#-------------------------------------------------
# Build emulator
#-------------------------------------------------

EMULATOR := rvemu

BUILD_DIR := build

OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS	:= $(OBJECTS:.o=.d)

.PHONY: build clean

build: $(EMULATOR)

$(EMULATOR): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

$(BUILD_DIR)/cpu/cpu.o: include/cpu/decode.h
include/cpu/decode.h: scripts/generate_inst_patterns.py
	python3 $<

clean:
	rm -rf $(EMULATOR) $(BUILD_DIR)

###############################################################################
#                                Run tests/programs                           #
###############################################################################

# Makefile to run various tests and programs

#-------------------------------------------------
# Tests
#-------------------------------------------------

.PHONY: run-sanity-test run-riscv-tests run-device-tests

# sanity test
SANITY_DIR := tests/sanity
run-sanity-test: $(EMULATOR)
	$(MAKE) -C tests/sanity all
	python3 $(SANITY_DIR)/run.py

# riscv-tests test
RISCV_TESTS_DIR := tests/riscv-test
run-riscv-tests: $(EMULATOR)
	$(MAKE) -C $(RISCV_TESTS_DIR) all
	python3 $(RISCV_TESTS_DIR)/run.py

# device unit tests
DEVICE_UNIT_TESTS_DIR := tests/unit
run-device-unit-tests: $(EMULATOR)
	$(MAKE) -C $(DEVICE_UNIT_TESTS_DIR)

# device tests
DEVICE_TESTS_DIR := tests/devices
run-device-tests: $(EMULATOR)
	python3 $(DEVICE_TESTS_DIR)/run.py

# Full regression test list for emulator
REGRESSION_DIR := tests/regression
run-regression:
	python3 $(REGRESSION_DIR)/run.py

#-------------------------------------------------
# General command to run ELF file
#-------------------------------------------------

ELF ?=

run-elf: rvemu
	./rvemu --format elf $(ELF)

#-------------------------------------------------
# Example program
#-------------------------------------------------

.PHONY: run-fib run-baremetal-test

BAREMETAL_DIR = ../software/baremetal

# fibonacci program from baremetal example program
run-fib: rvemu
	$(MAKE) -C $(BAREMETAL_DIR) fib
	./rvemu  $(BAREMETAL_DIR)/build/examples/fib/fib.elf

# baremetal-test program from baremetal example program
run-baremetal-test: rvemu
	$(MAKE) -C $(BAREMETAL_DIR) baremetal-test
	./rvemu --poison-ram $(BAREMETAL_DIR)/build/examples/baremetal-test/baremetal-test.elf

# coremark
run-coremark: rvemu
	$(MAKE) -C $(BAREMETAL_DIR) coremark
	./rvemu --format elf $(BAREMETAL_DIR)/build/benchmarks/coremark/coremark.elf


#-------------------------------------------------
# OpenSBI
#-------------------------------------------------

.PHONY: boot-images run-opensbi

boot-images:
	$(MAKE) -C ../platform/bootROM all
	$(MAKE) -C ../platform/devicetree all
	$(MAKE) -C ../linux/opensbi all

run-opensbi: rvemu boot-images
	$(MAKE) -C ../software/opensbi-test all
	./rvemu \
	--max-instruction 1400000 \
	--bootrom=../platform/bootROM/build/bootROM_emu.elf \
	--dtb=../platform/devicetree/tinylinuxrv_emu.dtb \
	--bios=../linux/opensbi/build/tinylinuxrv_emu/platform/tinylinuxrv_emu/firmware/fw_jump.elf \
	--kernel=../software/opensbi-test/build/opensbi-test.bin


#-------------------------------------------------
# Linux
#-------------------------------------------------

.PHONY: run-linux-emulator run-linux-emulator-early-boot run-linux-emulator-busybox

KERNEL_CONFIG ?= busybox

# Pass keystrokes directly to the guest shell without host echo.
# Restore host terminal settings on exit.
run-linux-emulator: rvemu boot-images
	$(MAKE) -C ../linux/linux_kernel build-image PLATFORM=emulator DEFCONFIG=$(KERNEL_CONFIG)
	@ saved_tty=$$(stty -g); \
	trap 'stty "$$saved_tty"' EXIT; \
	stty -icanon -echo min 1 time 0; \
	./rvemu \
		--bootrom=../platform/bootROM/build/bootROM_emu.elf \
		--dtb=../platform/devicetree/tinylinuxrv_emu.dtb \
		--bios=../linux/opensbi/build/tinylinuxrv_emu/platform/tinylinuxrv_emu/firmware/fw_jump.elf \
		--kernel=../linux/linux_kernel/build/emulator/$(KERNEL_CONFIG)/arch/riscv/boot/Image

run-linux-emulator-early-boot:
	$(MAKE) run-linux-emulator KERNEL_CONFIG=early_boot

run-linux-emulator-busybox:
	$(MAKE) run-linux-emulator KERNEL_CONFIG=busybox
