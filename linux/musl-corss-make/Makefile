# Makefile to build musl-cross-make

#------------------------------------------------
# Directory
#------------------------------------------------

REPO_ROOT := $(shell git rev-parse --show-toplevel)
MUSL_SRC  := $(REPO_ROOT)/local/musl-cross-make
LINUX_SRC := $(REPO_ROOT)/local/linux_kernel
HEADERS_OUT := $(REPO_ROOT)/linux/linux_kernel/build/headers
NPROC ?= 4
BUILD_JOBS = $(if $(filter -j%,$(MAKEFLAGS)),,-j$(NPROC))

#------------------------------------------------
# Build the tool
#------------------------------------------------

.DEFAULT_GOAL := all
.PHONY: all build install clean

all: install

$(MUSL_SRC):
	mkdir -p $(dir $(MUSL_SRC))
	git clone https://github.com/richfelker/musl-cross-make.git $(MUSL_SRC)

$(LINUX_SRC):
	$(MAKE) -C $(REPO_ROOT)/linux/linux_kernel $(LINUX_SRC)

build: $(MUSL_SRC)
	cp config.mak $(MUSL_SRC)
	$(MAKE) -C $(MUSL_SRC) $(BUILD_JOBS)

install: build $(LINUX_SRC)
	$(MAKE) -C $(MUSL_SRC) install
	mkdir -p $(HEADERS_OUT)
	$(MAKE) -C $(LINUX_SRC) O=$(HEADERS_OUT) \
		ARCH=riscv \
		INSTALL_HDR_PATH=$(REPO_ROOT)/local/toolchain-rv64ima/riscv64-linux-musl \
		headers_install

clean:
	$(MAKE) -C $(MUSL_SRC) clean
