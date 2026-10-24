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
SRCS += $(RTL_DIR)/cpu/regfile.sv
SRCS += $(RTL_DIR)/cpu/decoder.sv
SRCS += $(RTL_DIR)/cpu/IF.sv
SRCS += $(RTL_DIR)/cpu/ID.sv
SRCS += $(RTL_DIR)/cpu/cpu.sv

INCS += $(RTL_DIR)/include/cpu

# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------

IVERILOG_INC   := $(addprefix -I ,$(INCS))
IVERILOG_FLAGS := -g2012 -Wall

VERILATOR_INC   := $(addprefix -I,$(INCS))

OUTPUT := build

# -----------------------------------------------------------------------------
# Build
# -----------------------------------------------------------------------------

.PHONY: iverilog verilator-lint

# lint only
verilator-lint: $(SRCS)
	verilator --lint-only -Wall $(SRCS) $(VERILATOR_INC)

iverilog: $(SRCS)
	mkdir -p $(OUTPUT)
	iverilog $(IVERILOG_FLAGS) $(SRCS) $(IVERILOG_INC) -o $(OUTPUT)/cpu.out
