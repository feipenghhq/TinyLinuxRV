# Makefile to build Linux Kernel image

#------------------------------------------------
# Directory
#------------------------------------------------

REPO_ROOT := $(shell git rev-parse --show-toplevel)
LINUX_DIR := $(REPO_ROOT)/linux/linux_kernel

LOCAL_LINUX_DIR := $(REPO_ROOT)/local/linux_kernel

OUTPUT_DIR := $(LINUX_DIR)/build

LINUX_SRC :=$(LOCAL_LINUX_DIR)

#------------------------------------------------
# Environment
#------------------------------------------------

export ARCH=riscv
export CROSS_COMPILE=riscv64-linux-gnu-

LINUX_VERSION := v6.18.55

# default config
PLATFORM  ?= emulator
CONFIG    ?= early_boot

LINUX_OUT :=$(OUTPUT_DIR)/$(PLATFORM)/$(CONFIG)

# Number of process or to build the image
NPROC = 8

#------------------------------------------------
# Build linux image
#------------------------------------------------

.DEFAULT_GOAL := build-image

.PHONY: setup defconfig menuconfig load-config build-image clean

$(LINUX_SRC):
	git clone --depth 1 --branch $(LINUX_VERSION) \
	https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git $(LOCAL_LINUX_DIR)

$(LINUX_OUT):
	mkdir -p $(LINUX_OUT)

setup: $(LINUX_SRC) $(LINUX_OUT)

defconfig: setup
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) defconfig

menuconfig:
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) menuconfig

load-config: setup
	cp configs/$(PLATFORM)/$(CONFIG)_defconfig $(LINUX_OUT)/.config
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) olddefconfig

# Initialize once; explicit load-config resets an existing configuration.
$(LINUX_OUT)/.config: | setup
	cp configs/$(PLATFORM)/$(CONFIG)_defconfig $(LINUX_OUT)/.config
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) olddefconfig

build-image: setup $(LINUX_OUT)/.config
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) -j$(NPROC) Image

clean:
	rm -rf build
