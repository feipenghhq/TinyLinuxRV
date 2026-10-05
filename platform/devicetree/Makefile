DRAM_SIZE = 128M

TARGET = tinylinuxrv_emu.dtb

all: $(TARGET)

tinylinuxrv_emu.dtb: tinylinuxrv_emu.dts
	dtc -I dts -O dtb -o tinylinuxrv_emu.dtb tinylinuxrv_emu.dts

# Use qemu to dump a sample DTB and Translate sample DTB to DTS
qemu-ref-dtb:
	qemu-system-riscv64 -M virt,dumpdtb=qemu-virt.dtb -smp 1 -nographic
	dtc -I dtb -O dts -o qemu-virt.dts qemu-virt.dtb

clean:
	rm -f $(TARGET)
