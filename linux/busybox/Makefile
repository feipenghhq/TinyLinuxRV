# Makefile to build BUSYBOX-cross-make

#------------------------------------------------
# Directory
#------------------------------------------------

BUSYBOX_VERSION = 1.36.1

REPO_ROOT := $(shell git rev-parse --show-toplevel)
LOCAL_DIR := $(REPO_ROOT)/local
BUSYBOX_SRC := $(LOCAL_DIR)/busybox-$(BUSYBOX_VERSION)
ROOTFS_DIR := $(REPO_ROOT)/local/rootfs
BUSYBOX_BIN := $(BUSYBOX_SRC)/busybox
INSTALLED_BUSYBOX := $(ROOTFS_DIR)/bin/busybox
TOOLCHAIN_CC := $(REPO_ROOT)/local/toolchain-rv64ima/bin/riscv64-linux-musl-gcc
NPROC ?= 4
BUILD_JOBS = $(if $(filter -j%,$(MAKEFLAGS)),,-j$(NPROC))

#------------------------------------------------
# Environment
#------------------------------------------------

export CROSS_COMPILE=$(REPO_ROOT)/local/toolchain-rv64ima/bin/riscv64-linux-musl-

#------------------------------------------------
# Build the tool
#------------------------------------------------

.DEFAULT_GOAL := all
.PHONY: all setup defconfig menuconfig load-config save-config build install rootfs FORCE

all: rootfs

$(TOOLCHAIN_CC):
	@if [ ! -x "$@" ]; then \
		$(MAKE) -C $(REPO_ROOT)/linux/musl-corss-make install; \
	fi

$(BUSYBOX_SRC):
	@if [ ! -d "$@" ]; then \
		mkdir -p $(LOCAL_DIR) && \
		{ [ -f $(LOCAL_DIR)/busybox-$(BUSYBOX_VERSION).tar.bz2 ] || \
		  wget -O $(LOCAL_DIR)/busybox-$(BUSYBOX_VERSION).tar.bz2 https://busybox.net/downloads/busybox-$(BUSYBOX_VERSION).tar.bz2; } && \
		tar -xf $(LOCAL_DIR)/busybox-$(BUSYBOX_VERSION).tar.bz2 -C $(LOCAL_DIR); \
	fi

setup: $(BUSYBOX_SRC)

defconfig: setup
	$(MAKE) -C $(BUSYBOX_SRC) defconfig

menuconfig: $(BUSYBOX_SRC)/.config
	$(MAKE) -C $(BUSYBOX_SRC) menuconfig

load-config: setup
	cp configs/.config $(BUSYBOX_SRC)/.config
	$(MAKE) -C $(BUSYBOX_SRC) oldconfig </dev/null

# Preserve menuconfig edits; load-config explicitly restores the preset.
$(BUSYBOX_SRC)/.config: | setup
	@if [ ! -f "$@" ]; then \
		cp configs/.config $@ && \
		$(MAKE) -C $(BUSYBOX_SRC) oldconfig </dev/null; \
	fi

save-config: $(BUSYBOX_SRC)/.config
	mkdir -p configs
	cp $(BUSYBOX_SRC)/.config configs/.

build: $(BUSYBOX_BIN)

# Always let BusyBox check its own source dependencies. An unchanged binary
# keeps its timestamp, so downstream installation can be skipped.
FORCE:

$(BUSYBOX_BIN): FORCE $(BUSYBOX_SRC)/.config $(TOOLCHAIN_CC)
	$(MAKE) -C $(BUSYBOX_SRC) $(BUILD_JOBS)

install: $(INSTALLED_BUSYBOX)

$(INSTALLED_BUSYBOX): $(BUSYBOX_BIN) $(BUSYBOX_SRC)/.config
	$(MAKE) -C $(BUSYBOX_SRC) CONFIG_PREFIX=$(ROOTFS_DIR) install

rootfs: install init logo.sh
	mkdir -p $(ROOTFS_DIR) $(ROOTFS_DIR)/dev $(ROOTFS_DIR)/proc $(ROOTFS_DIR)/sys $(ROOTFS_DIR)/tmp $(ROOTFS_DIR)/etc
	cp init $(ROOTFS_DIR)/init
	cp logo.sh $(ROOTFS_DIR)/etc/logo.sh
	chmod +x $(ROOTFS_DIR)/init $(ROOTFS_DIR)/etc/logo.sh
	@if [ ! -e $(ROOTFS_DIR)/dev/console ]; then \
		sudo mknod -m 600 $(ROOTFS_DIR)/dev/console c 5 1; \
	fi
