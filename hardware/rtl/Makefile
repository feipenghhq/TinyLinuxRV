# Makefile for rtl

# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

REPO_ROOT := $(shell git rev-parse --show-toplevel)
RTL_DIR   := $(REPO_ROOT)/hardware/rtl

# -----------------------------------------------------------------------------
# Source Files
# -----------------------------------------------------------------------------

SRCS += $(RTL_DIR)/common/skid_buffer.sv
SRCS += $(RTL_DIR)/cpu/pipeline.sv
SRCS += $(RTL_DIR)/cpu/IF.sv

INCS += $(RTL_DIR)/include

# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------

IVERILOG_INC   := $(addprefix -I ,$(INCS))
IVERILOG_FLAGS := -g2012 -Wall

OUTPUT := build

# -----------------------------------------------------------------------------
# Build
# -----------------------------------------------------------------------------

.PHONY: iverilog

iverilog: $(SRCS)
	mkdir -p $(OUTPUT)
	iverilog $(IVERILOG_FLAGS) $(SRCS) $(IVERILOG_INC) -o $(OUTPUT)/cpu.out
