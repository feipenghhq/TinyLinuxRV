# Makefile for cocotb

# -----------------------------------------------------------------------------
# Repository paths
# -----------------------------------------------------------------------------

REPO_ROOT  := $(shell git rev-parse --show-toplevel)
RTL_DIR    := $(REPO_ROOT)/hardware/rtl
COCOTB_DIR := $(REPO_ROOT)/hardware/tests/cocotb

# -----------------------------------------------------------------------------
# Cocotb Config
# -----------------------------------------------------------------------------

# The simulator we want to run our tests with.
SIM = verilator

# The top-level language of the design (verilog or vhdl).
TOPLEVEL_LANG = verilog

# The name of the toplevel module/entity in your design.
COCOTB_TOPLEVEL = cpu_tb

# Enable waveform generation.
#WAVES = 1
ifeq ($(SIM), verilator)
ifeq ($(WAVES),1)
EXTRA_ARGS += --trace --trace-fst --trace-structs
endif
endif

# -----------------------------------------------------------------------------
# Source Files
# -----------------------------------------------------------------------------

VERILOG_SOURCES += $(RTL_DIR)/common/skid_buffer.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/pipeline.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/regfile.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/decoder.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/multiplier.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/divider.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/bu.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/alu.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/amo_alu.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/IF.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/ID.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/EX.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/MEM.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/WB.sv
VERILOG_SOURCES += $(RTL_DIR)/cpu/cpu.sv

VERILOG_SOURCES += $(COCOTB_DIR)/cpu_tb.sv

VERILOG_INCLUDE_DIRS += $(RTL_DIR)/include/cpu

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

# The Python modules which contain your testcases.
# The current directory is included in the Python path,
# so you can just specify the module name without the .py extension.
COCOTB_TEST_MODULES ?= cpu_regression


# Includes cocotb's Makefiles which define the build and simulation flows for each supported simulator.
# This must be included *after* all of the variables above are defined.
include $(shell cocotb-config --makefiles)/Makefile.sim

# -----------------------------------------------------------------------------
# Run test
# -----------------------------------------------------------------------------

.DEFAULT_GOAL := regression

run-regression:
	@status=0; \
	$(MAKE) sim COCOTB_TEST_MODULES=cpu_regression || status=$$?; \
	python3 report.py; \
	exit $$status

run-test:
	$(MAKE) sim COCOTB_TEST_MODULES=cpu_regression COCOTB_TEST_FILTER=$(TEST)

openwave:
	gtkwave dump.fst &
