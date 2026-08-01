# Make file for the tool-chain rv64i check

CROSS_COMPILE = riscv64-linux-gnu-
ARCH = rv64i
ABI = lp64

BUILD_DIR = build

.PHONY: all disasm clean

all: $(BUILD_DIR)/rv64i.bin

disasm: $(BUILD_DIR)/rv64i.elf
	$(CROSS_COMPILE)objdump -d -M no-aliases $<

$(BUILD_DIR)/rv64i.bin: $(BUILD_DIR)/rv64i.elf
	$(CROSS_COMPILE)objcopy $< -O binary $(BUILD_DIR)/rv64i.bin

$(BUILD_DIR)/rv64i.elf: $(BUILD_DIR)/rv64i.o
	$(CROSS_COMPILE)ld -e _start -Ttext 0x80000000 -o $(BUILD_DIR)/rv64i.elf $<

$(BUILD_DIR)/rv64i.o: rv64i.S | $(BUILD_DIR)
	$(CROSS_COMPILE)as -march=$(ARCH) -mabi=$(ABI) -o $(BUILD_DIR)/rv64i.o $<

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)