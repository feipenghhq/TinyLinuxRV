# Make file for the tool-chain smoke check

CROSS_COMPILE = riscv64-linux-gnu-
ARCH = rv64i
ABI = lp64

BUILD_DIR = build

.PHONY: all disasm clean

all: $(BUILD_DIR)/smoke.bin

disasm: $(BUILD_DIR)/smoke.elf
	$(CROSS_COMPILE)objdump -d -M no-aliases $<

$(BUILD_DIR)/smoke.bin: $(BUILD_DIR)/smoke.elf
	$(CROSS_COMPILE)objcopy $< -O binary $(BUILD_DIR)/smoke.bin

$(BUILD_DIR)/smoke.elf: $(BUILD_DIR)/smoke.o
	$(CROSS_COMPILE)ld -e _start -Ttext 0x80000000 -o $(BUILD_DIR)/smoke.elf $<

$(BUILD_DIR)/smoke.o: smoke.S | $(BUILD_DIR)
	$(CROSS_COMPILE)as -march=$(ARCH) -mabi=$(ABI) -o $(BUILD_DIR)/smoke.o $<

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)