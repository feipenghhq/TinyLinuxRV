
.PHONY: all baremetal-test fib coremark
.PHONY: run-coremark
.PHONY: clean

all: baremetal-test fib coremark

BUILD_DIR := build

baremetal-test:
	$(MAKE) -C examples/baremetal-test

fib:
	$(MAKE) -C examples/fib

coremark:
	$(MAKE) -C benchmarks/coremark


clean:
	rm -rf $(BUILD_DIR)
