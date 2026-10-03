#------------------------------------------------
# Directory
#------------------------------------------------

REPO_ROOT    := $(shell git rev-parse --show-toplevel)
OPENSBI_DIR  := $(REPO_ROOT)/third-party/opensbi
PLATFORM_DIR := $(REPO_ROOT)/software/opensbi/platform

OUTPUT_DIR = $(PLATFORM_DIR)/build

EMU = tinylinuxrv_emu

#------------------------------------------------
#
#------------------------------------------------

.PHONY: build-emu-sbi clean

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
