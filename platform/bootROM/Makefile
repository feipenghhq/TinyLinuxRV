
ARCH  = rv64ima
ABI   = lp64

CROSS_COMPILE := riscv64-unknown-elf-

LINKER_SCRIPT := src/link.ld

CPPFLAGS += -I../include/tinylinuxrv
CFLAGS  += -march=$(ARCH) -mabi=$(ABI)
CFLAGS  += -e _boot -T $(LINKER_SCRIPT)
CFLAGS  += -nostartfiles -nostdlib

BUILD_DIR := build

SOURCES := bootROM_emu.S

ELFS    := $(patsubst %.S,$(BUILD_DIR)/%.elf, $(SOURCES))
BINS    := $(patsubst %.S,$(BUILD_DIR)/%.bin, $(SOURCES))

VPATH = src

.PHONY: all clean

all: $(ELFS) $(BINS)

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(CROSS_COMPILE)objcopy $< -O binary $@

$(BUILD_DIR)/%.elf: src/%.S
	mkdir -p $(@D)
	$(CROSS_COMPILE)gcc $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

