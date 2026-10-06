#------------------------------------------------
# Directory
#------------------------------------------------

REPO_ROOT    := $(shell git rev-parse --show-toplevel)
OPENSBI_DIR  := $(REPO_ROOT)/third-party/opensbi
PLATFORM_DIR := $(REPO_ROOT)/software/opensbi/platform
EMULATOR_DIR := $(REPO_ROOT)/emulator

MY_OPENSBI_DIR = $(REPO_ROOT)/software/opensbi

OUTPUT_DIR = $(MY_OPENSBI_DIR)/build

EMU = tinylinuxrv_emu

#------------------------------------------------
#
#------------------------------------------------

.PHONY: build-emu-sbi clean

#------------------------------------------------
# Run opensbi-test program after openSBI is loaded
#------------------------------------------------

run-opensbi-test-emu:
	$(MAKE) -C $(MY_OPENSBI_DIR)/test
	$(MAKE) -C $(EMULATOR_DIR) run-opensbi

#------------------------------------------------
# Build opensbi image
#------------------------------------------------

# Need to use riscv64-linux-gcc as CC because openSBI requires PIE (Position Independent Executable)
export CROSS_COMPILE=riscv64-linux-gnu-

build-emu-sbi:
	mkdir -p $(OUTPUT_DIR)/$(EMU)
	$(MAKE) -C $(OPENSBI_DIR) PLATFORM_DIR=$(PLATFORM_DIR)/$(EMU) O=$(OUTPUT_DIR)/$(EMU)

clean:
	rm -rf $(OUTPUT_DIR)
