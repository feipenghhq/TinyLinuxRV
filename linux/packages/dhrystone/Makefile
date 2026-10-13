#------------------------------------------------
# Directory
#------------------------------------------------

REPO_ROOT := $(shell git rev-parse --show-toplevel)
LOCAL_DIR := $(REPO_ROOT)/local
DHRYSTONE_DIR := $(LOCAL_DIR)/ansibench/dhrystone

#------------------------------------------------
# Tools
#------------------------------------------------

CC := $(REPO_ROOT)/local/toolchain-rv64ima/bin/riscv64-linux-musl-gcc

#------------------------------------------------
# sources
#------------------------------------------------

SOURCES += $(DHRYSTONE_DIR)/src/dhry_1.c
SOURCES += $(DHRYSTONE_DIR)/src/dhry_2.c

CPPFLAGS += -I$(DHRYSTONE_DIR)/include
CPPFLAGS += -DDHRY_HZ=100 -DTIME

CFLAGS += -O2 -ansi -static

OUTPUT = build

#------------------------------------------------
# Build
#------------------------------------------------

.PHONY: all dhrystone ansibench clean

all: dhrystone

ansibench: $(LOCAL_DIR)/ansibench

$(LOCAL_DIR)/ansibench:
	git clone git@github.com:nfinit/ansibench.git $(LOCAL_DIR)/ansibench

dhrystone: $(OUTPUT)/dhrystone

$(SOURCES): | ansibench

$(OUTPUT)/dhrystone: $(SOURCES) | ansibench
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

clean:
	rm -rf $(OUTPUT)
