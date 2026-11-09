# Makefile for rtl

# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

REPO_ROOT := $(shell git rev-parse --show-toplevel)
RTL_DIR   := $(REPO_ROOT)/hardware/rtl


# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------

OUTPUT := build

# -----------------------------------------------------------------------------
# Build
# -----------------------------------------------------------------------------

.PHONY: iverilog verilator-lint

# lint only
verilator-lint: $(SRCS)
	verilator --lint-only -Wall -F rtl.f $(VERILATOR_INC)

iverilog: $(SRCS)
	mkdir -p $(OUTPUT)
	iverilog -g2012 -Wall -f rtl.f -o $(OUTPUT)/cpu.out
