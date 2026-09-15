
BAREMETAL_ROOT = ../../../../software/baremetal
EMULATOR_ROOT = ../../..
RUNTIME_ROOT = $(BAREMETAL_ROOT)/runtime

BUILD_DIR := build

SRCS += plic.c
ASM_SRCS += trap.S

RUNTIME_ASMS += $(RUNTIME_ROOT)/mtrap.S

CPPFLAGS += -I$(RUNTIME_ROOT)

PROGRAM := plic

include $(BAREMETAL_ROOT)/mk/baremetal.mk

CPPFLAGS += -I$(EMULATOR_ROOT)/include -I$(EMULATOR_ROOT)/include/device
