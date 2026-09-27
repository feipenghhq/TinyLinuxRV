# ---------------------------------------------------------
# Directory
# ---------------------------------------------------------

EMULATOR_DIR := ../../emulator
EMULATOR := $(EMULATOR_DIR)/rvemu

BUILD_DIR := build

.PHONY: run-demo

# ---------------------------------------------------------
# build the programs
# ---------------------------------------------------------

build-demo:
	$(MAKE) -C demo

# -------------------------- -------------------------------
# run programs
# ---------------------------------------------------------

run-demo: build-demo
	$(MAKE) -C $(EMULATOR_DIR) build
	$(EMULATOR) --max-instruction 1000000  demo/$(BUILD_DIR)/demo.elf
