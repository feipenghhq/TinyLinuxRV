
ARCH  = rv64ima
ABI   = lp64

CROSS_COMPILE := riscv64-linux-gnu-

LINKER_SCRIPT := link.ld

CPPFLAGS += -I../../baremetal/drivers/include
CPPFLAGS += -I../../../platform/include/tinylinuxrv

CFLAGS  += -march=$(ARCH) -mabi=$(ABI) -mcmodel=medany
CFLAGS  += -ffreestanding -fno-pie -fno-pic -fno-stack-protector
CFLAGS  += -T $(LINKER_SCRIPT)
CFLAGS  += -nostartfiles -nostdlib

BUILD_DIR := build

SOURCES := opensbi-test.c crt0.S ../../baremetal/drivers/uart16550/uart16550.c

.PHONY: all clean

all: $(BUILD_DIR)/opensbi-test.elf

$(BUILD_DIR)/opensbi-test.elf: $(SOURCES)
	mkdir -p $(@D)
	$(CROSS_COMPILE)gcc $(CPPFLAGS) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)

