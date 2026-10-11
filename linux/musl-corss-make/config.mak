TARGET = riscv64-linux-musl
OUTPUT = $(abspath ../toolchain-rv64ima)

GCC_CONFIG += --with-arch=rv64ima --with-abi=lp64
GCC_CONFIG += --disable-multilib --enable-languages=c
GCC_CONFIG += --disable-shared
