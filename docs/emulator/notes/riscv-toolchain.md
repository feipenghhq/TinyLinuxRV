# Tool Chain

## Required Toolchain

```shell
gcc --version
make --version
git --version

riscv64-linux-gnu-gcc --version
riscv64-linux-gnu-as --version
riscv64-linux-gnu-ld --version
riscv64-linux-gnu-objdump --version
riscv64-linux-gnu-readelf --version
riscv64-linux-gnu-objcopy --version
```

- as: Assembles source code into an object file. (汇编)
- ld: Links object files into an executable. (链接)
- gcc: Can also serve as a compiler driver that invokes the assembler and linker. (也可以作为统一驱动调用汇编器和链接器)
- readelf: Displays the structure and metadata of ELF files. (查看 ELF 结构)
- objdump: Disassembles object files and executables. (反汇编)
- objcopy: Converts an ELF file into a raw binary image. (转换成 raw binary)

### RISC-V GNU Toolchain Installation (riscv64-linux-gcc)

```bash
sudo apt install g++-riscv64-linux-gnu binutils-riscv64-linux-gnu gcc-riscv64-linux-gnu
```

## Manual Compilation Steps (from scratch)

```bash
cd emulator/tests/toolchain/smoke
mkdir -p build

# 1. Assemble the file (smoke.S -> smoke.o)
riscv64-linux-gnu-as -march=rv64i -mabi=lp64 -o build/smoke.o smoke.S

# check the output, should be relocatable object file
file build/smoke.o

# [Optional] Check object file
riscv64-linux-gnu-readelf -h build/smoke.o
riscv64-linux-gnu-readelf -S build/smoke.o
riscv64-linux-gnu-readelf -s build/smoke.o
riscv64-linux-gnu-objdump -d build/smoke.o

# 2. link the file to ELF executable (smoke.o -> smoke.elf)
# use 0x80000000 as starting address for the smoke check
riscv64-linux-gnu-ld -e _start -Ttext 0x80000000  -o build/smoke.elf build/smoke.o

# [Optional] Check elf file
riscv64-linux-gnu-readelf -h build/smoke.elf
riscv64-linux-gnu-readelf -S build/smoke.elf
riscv64-linux-gnu-readelf -l build/smoke.elf
riscv64-linux-gnu-readelf -s build/smoke.elf
riscv64-linux-gnu-objdump -d build/smoke.elf

# [Optional] Check the instruction
riscv64-linux-gnu-objdump -d -M no-aliases build/smoke.elf

# 3. create raw binary (smoke.elf -> smoke.bin)
riscv64-linux-gnu-objcopy -O binary build/smoke.elf build/smoke.bin

# [Optional] Check the binary
xxd build/smoke.bin
hexdump -C build/smoke.bin

```