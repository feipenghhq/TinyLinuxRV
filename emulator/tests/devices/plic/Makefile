
BAREMETAL_ROOT = ../../../../software/baremetal
EMULATOR_ROOT = ../../..

BUILD_DIR := build

SRCS := plic.c

PROGRAM := plic

include $(BAREMETAL_ROOT)/mk/baremetal.mk

CPPFLAGS += -I$(EMULATOR_ROOT)/include -I$(EMULATOR_ROOT)/include/device
