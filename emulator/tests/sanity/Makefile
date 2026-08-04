# Make file for the tool-chain rv64i check

ARCH  = rv64i
ABI   = lp64

START ?= 0x80000000

CROSS_COMPILE := riscv64-linux-gnu-
ASFLAGS := -march=$(ARCH) -mabi=$(ABI)
LDFLAGS := -e _start -Ttext $(START)

BUILD_DIR := build

SOURCES := rv64i.S
OBJECTS := $(patsubst %.S,$(BUILD_DIR)/%.o,   $(SOURCES))
ELFS    := $(patsubst %.S,$(BUILD_DIR)/%.elf, $(SOURCES))
BINS    := $(patsubst %.S,$(BUILD_DIR)/%.bin, $(SOURCES))


.PHONY: all clean

all: $(OBJECTS) $(ELFS) $(BINS)

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(CROSS_COMPILE)objcopy $< -O binary $@

$(BUILD_DIR)/%.elf: $(BUILD_DIR)/%.o
	$(CROSS_COMPILE)ld $(LDFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.S
	mkdir -p $(@D)
	$(CROSS_COMPILE)as $(ASFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)
