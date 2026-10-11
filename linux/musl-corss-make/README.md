# musl-cross-make

## Introduction

The `libgcc` provided by the `riscv64-linux-gnu-` toolchain contains RVC and floating-point instructions that are not supported by TinyLinuxRV. Therefore, this toolchain cannot be used to build BusyBox for our Linux image.

To address this limitation, we use [`musl-cross-make`](https://github.com/richfelker/musl-cross-make), a tool for building GCC-based cross-compilation toolchains with musl libc. It allows us to build a custom RISC-V toolchain targeting the instruction set supported by TinyLinuxRV, enabling us to compile a statically linked BusyBox for our Linux image.

## Build with Makefile

The makefile will git clone the musl-cross-make for the first time, then build and install the tool chain.
Installation depends on a successful build and downloads the Linux source if needed.
Linux headers are generated out of tree in `linux/linux_kernel/build/headers`,
then installed into the toolchain sysroot. `make -j4 all` preserves this ordering.

The tool chain is located at `local/toolchain-rv64ima`

```shell
make all
```

## Detailed instructions on how to build the musl-cross-make

1. Get the repo

```shell
cd <repo>

git clone https://github.com/richfelker/musl-cross-make.git local/musl-cross-make

cd local/musl-cross-make
```

2. Create `config.mak`

```shell
cat > config.mak <<'EOF'
TARGET = riscv64-linux-musl
OUTPUT = $(abspath ../toolchain-rv64ima)

GCC_CONFIG += --with-arch=rv64ima --with-abi=lp64
GCC_CONFIG += --disable-multilib --enable-languages=c
GCC_CONFIG += --disable-shared
EOF
```

3. build and install.

```shell
make -j4
make install
```

Then go back to repo root to verify the tool is using the correct ABI

```shell
../toolchain-rv64ima/bin/riscv64-linux-musl-gcc -Q --help=target | grep -E 'march=|mabi='

# should return:
  -mabi=                                lp64
  -march=                               rv64ima
  Supported ABIs (for use with the -mabi= option):
```
