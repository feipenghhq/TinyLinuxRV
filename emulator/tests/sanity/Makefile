# Make file for the tool-chain rv64i check

ARCH  = rv64i
ABI   = lp64

START ?= 0x80000000

CROSS_COMPILE = riscv64-linux-gnu-
ASFLAGS := -march=$(ARCH) -mabi=$(ABI)
LDFLAGS := -e _start -Ttext $(START)

OUTPUT := build

SOURCES := rv64i.S
OBJECTS := $(patsubst %.S,$(OUTPUT)/%.o,   $(SOURCES))
ELFS 	:= $(patsubst %.S,$(OUTPUT)/%.elf, $(SOURCES))
BINS	:= $(patsubst %.S,$(OUTPUT)/%.bin, $(SOURCES))


.PHONY: all clean

all: $(OBJECTS) $(ELFS) $(BINS)

$(OUTPUT)/%.bin: $(OUTPUT)/%.elf
	$(CROSS_COMPILE)objcopy $< -O binary $@

$(OUTPUT)/%.elf: $(OUTPUT)/%.o
	$(CROSS_COMPILE)ld $(LDFLAGS) $< -o $@

$(OUTPUT)/%.o: %.S
	mkdir -p $(@D)
	$(CROSS_COMPILE)as $(ASFLAGS) $< -o $@

clean:
	rm -rf $(OUTPUT)
