
# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

BAREMETAL_ROOT := ../..
COREMARK_SRC_DIR := $(BAREMETAL_ROOT)/../../third-party/coremark
BUILD_DIR := $(BAREMETAL_ROOT)/build/benchmarks/coremark

# -----------------------------------------------------------------------------
# Source File
# -----------------------------------------------------------------------------

PROGRAM := coremark

COREMARK_SRCS += $(COREMARK_SRC_DIR)/core_list_join.c
COREMARK_SRCS += $(COREMARK_SRC_DIR)/core_main.c
COREMARK_SRCS += $(COREMARK_SRC_DIR)/core_matrix.c
COREMARK_SRCS += $(COREMARK_SRC_DIR)/core_state.c
COREMARK_SRCS += $(COREMARK_SRC_DIR)/core_util.c

COREMARK_SRCS += tinylinuxrv/core_portme.c
COREMARK_SRCS += tinylinuxrv/ee_printf.c

CPPFLAGS += -I$(COREMARK_SRC_DIR) -Itinylinuxrv

# local mapping for OBJS to resolve the path issue.
OBJS += $(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(COREMARK_SRCS)))

vpath %.c $(COREMARK_SRC_DIR) tinylinuxrv

# -----------------------------------------------------------------------------
# Compiler flag and coremark configuration
# -----------------------------------------------------------------------------

FLAGS_STR = $(CFLAGS)

# preprocessing macro required for barebones
# use 1 iteration for now
CPPFLAGS += -DITERATIONS=1
# to indicate standalone environment
CPPFLAGS += -DSTANDALONE
# focus on correctness instead of performance
CPPFLAGS += -DVALIDATION_RUN=1

CPPFLAGS += -DFLAGS_STR='"$(FLAGS_STR)"'

# -----------------------------------------------------------------------------
# Include common makefile
# -----------------------------------------------------------------------------

include $(BAREMETAL_ROOT)/mk/baremetal.mk

# -----------------------------------------------------------------------------
# Additional task
# -----------------------------------------------------------------------------

objdump: $(BUILD_DIR)/$(PROGRAM).elf
	rm -rf $(BUILD_DIR)/$(PROGRAM).txt
	$(OBJDUMP) -d -j .text -j .rodata -j .data -j .sdata $< \
	> $(BUILD_DIR)/$(PROGRAM).txt
