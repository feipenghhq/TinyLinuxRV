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
DEFCONFIG ?= early_boot

LINUX_OUT :=$(OUTPUT_DIR)/$(PLATFORM)/$(DEFCONFIG)

# Number of process or to build the image
NPROC ?= 8
BUILD_JOBS = $(if $(filter -j%,$(MAKEFLAGS)),,-j$(NPROC))

#------------------------------------------------
# Build linux image
#------------------------------------------------

.DEFAULT_GOAL := build-image

.PHONY: setup defconfig menuconfig load-config save-config build-image clean mrproper rootfs \
        build-emulator-early_boot build-emulator-busybox

$(LINUX_SRC):
	git clone --depth 1 --branch $(LINUX_VERSION) \
	https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git $(LOCAL_LINUX_DIR)

$(LINUX_OUT):
	mkdir -p $(LINUX_OUT)

setup: $(LINUX_SRC) $(LINUX_OUT)

defconfig: setup
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) defconfig

menuconfig: $(LINUX_OUT)/.config
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) menuconfig

load-config: setup
	cp configs/$(PLATFORM)/$(DEFCONFIG)_defconfig $(LINUX_OUT)/.config
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) olddefconfig

save-config: $(LINUX_OUT)/.config
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) savedefconfig
	mkdir -p configs/$(PLATFORM)
	cp $(LINUX_OUT)/defconfig configs/$(PLATFORM)/$(DEFCONFIG)_defconfig


# clean up source tree
mrproper: $(LINUX_SRC)
	$(MAKE) -C $(LINUX_SRC) mrproper

# Initialize once; explicit load-config resets an existing configuration.
$(LINUX_OUT)/.config: | setup
	cp configs/$(PLATFORM)/$(DEFCONFIG)_defconfig $(LINUX_OUT)/.config
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) olddefconfig

build-image: setup $(LINUX_OUT)/.config
	$(MAKE) -C $(LINUX_SRC) O=$(LINUX_OUT) $(BUILD_JOBS) Image

ifeq ($(DEFCONFIG),busybox)
build-image: rootfs
endif

rootfs:
	$(MAKE) -C $(REPO_ROOT)/linux/busybox rootfs

clean:
	rm -rf build

#------------------------------------------------
# Build predefined linux image
#------------------------------------------------

build-emulator-early_boot:
	$(MAKE) PLATFORM=emulator DEFCONFIG=early_boot build-image

build-emulator-busybox:
	$(MAKE) PLATFORM=emulator DEFCONFIG=busybox build-image
