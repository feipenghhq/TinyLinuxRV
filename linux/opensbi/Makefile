# Makefile to build OpenSBI image

#------------------------------------------------
# Directory
#------------------------------------------------

REPO_ROOT    := $(shell git rev-parse --show-toplevel)
OPENSBI_DIR  := $(REPO_ROOT)/linux/opensbi
PLATFORM_DIR := $(OPENSBI_DIR)/platform

LOCAL_OPENSBI_DIR  := $(REPO_ROOT)/local/opensbi

OUTPUT_DIR := $(OPENSBI_DIR)/build

#------------------------------------------------
# Build OpenSBI image
#------------------------------------------------

OPENSBI_VERSION = v1.9

.PHONY: build-sbi-emu clean

all: build-sbi-emu

# Need to use riscv64-linux-gcc as CC because openSBI requires PIE (Position Independent Executable)
export CROSS_COMPILE=riscv64-linux-gnu-

EMU = tinylinuxrv_emu
build-sbi-emu: $(LOCAL_OPENSBI_DIR)
	mkdir -p $(OUTPUT_DIR)/$(EMU)
	$(MAKE) -C $(LOCAL_OPENSBI_DIR) PLATFORM_DIR=$(PLATFORM_DIR)/$(EMU) O=$(OUTPUT_DIR)/$(EMU)


$(LOCAL_OPENSBI_DIR):
	@if [ ! -d "$@" ]; then \
	git clone --branch $(OPENSBI_VERSION) --depth 1 \
	https://github.com/riscv-software-src/opensbi.git \
	$(LOCAL_OPENSBI_DIR); \
	fi

clean:
	rm -rf $(OUTPUT_DIR)
