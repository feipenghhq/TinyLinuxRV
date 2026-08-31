
.PHONY: all baremetal-test fib coremark
.PHONY: run-coremark
.PHONY: clean

all: baremetal-test fib coremark

BUILD_DIR := build
EMULATOR_DIR := ../../emulator
EMULATOR := $(EMULATOR_DIR)/rvemu

# ---------------------------------------------------------
# Example program
# ---------------------------------------------------------

baremetal-test:
	$(MAKE) -C examples/baremetal-test

fib:
	$(MAKE) -C examples/fib

# ---------------------------------------------------------
# Benchmark
# ---------------------------------------------------------

coremark:
	$(MAKE) -C benchmarks/coremark

# ---------------------------------------------------------
# run programs
# ---------------------------------------------------------

run-coremark: coremark
	$(MAKE) -C $(EMULATOR_DIR) build
	$(EMULATOR) --format elf $(BUILD_DIR)/benchmarks/coremark/coremark.elf

clean:
	rm -rf $(BUILD_DIR)
