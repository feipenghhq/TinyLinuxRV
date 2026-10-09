
# Linux

This repo port and build linux image for TinyLinuxRV.

We use `Linux 6.18 LTS (v6.18.55)`

## Building Linux Image

### Clone the Linux repo

> Note: the linux repo has already been added as a submodule in this repo

```shell
cd <repo>
git clone --depth 1 --branch v6.18.55 https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git third-party/linux
```

### Prepare environment

```shell
cd software/linux

LINUX_SRC=`realpath ../../third-party/linux`
LINUX_OUT=`realpath linux-build`

mkdir -p $LINUX_OUT

export ARCH=riscv
export CROSS_COMPILE=riscv64-linux-gnu-
```

### Generate .config

```shell
# Build the .config using default configuration
# .config is required for building kernel
make -C "$LINUX_SRC" O="$LINUX_OUT" defconfig
```

### Config Linux to match TinyLinuxRV

Making minimal configuration change from default to make the linux image support tinylinuxrv.

Open the menuconfig menu:

```shell
make -C "$LINUX_SRC" O="$LINUX_OUT" menuconfig
```
#### Minimal configuration for TinyLinuxRV

Type `/` to search for the keyword. It will tell you where to find the option

1. CPU Configuration

| Item           | Target | Reason                      |
| -------------- | ------ | --------------------------- |
| `64BIT`        | `y`    | 64 XLEN                     |
| `MMU`          | `y`    | Use virtual memory          |
| `RISCV_M_MODE` | `n`    | Linux running in S-mode     |
| `SMP`          | `n`    | Single hart                 |
| `RISCV_ISA_C`  | `n`    | C extension not implemented |
| `FPU`          | `n`    | F extension not implemented |
| `RISCV_ISA_V`  | `n`    | V extension not implemented |

2. SBI/Device Tree/Timer/Interrupt Controller

| Item          | Target | Usage                             |
| ------------- | ------ | --------------------------------- |
| `MMU`         | `y`    | Sv39 virtual memory               |
| `RISCV_SBI`   | `y`    | Use OpenSBI                       |
| `OF`          | `y`    | Read DTB                          |
| `RISCV_TIMER` | `y`    | RISC-V timer driver               |
| `RISCV_INTC`  | `y`    | CPU local interrupt controller    |
| `SIFIVE_PLIC` | `y`    | SIFIVE compatible PLIC controller |

3. UART and earlycon

| Item                  | Target | Usage                                              |
| --------------------- | ------ | -------------------------------------------------- |
| `TTY`                 | `y`    | Terminal support                                   |
| `SERIAL_8250`         | `y`    | 8250/16550 Serial driver                           |
| `SERIAL_8250_CONSOLE` | `y`    | Use Serial as kernel console                       |
| `SERIAL_OF_PLATFORM`  | `y`    | Discover Serial from DTB                           |
| `SERIAL_EARLYCON`     | `y`    | Output logs before the driver is fully initialized |

4. Kernel Boot argument

Add the following to DTS

```dts
chosen {
    stdout-path = "/soc/uart@10000000";
    bootargs = "earlycon console=ttyS0,115200 loglevel=8";
};
```

| Parameter              | Usage                                                                                            |
| ---------------------- | ------------------------------------------------------------------------------------------------ |
| `earlycon`             | Enable early serial console output using the device specified by stdout-path in the Device Tree. |
| `console=ttyS0,115200` | Use the first serial port (ttyS0) as the primary kernel console at a baud rate of 115200.        |
| `loglevel=8`           | Enable all kernel log messages, including debug-level messages, on the console.                  |

5. Log Support

| Parameter  | Target | Usage                                                                                                 |
| ---------- | ------ | ----------------------------------------------------------------------------------------------------- |
| `PRINTK`   | `y`    | Enable kernel logging and diagnostic messages.                                                        |
| `BUG`      | `y`    | Enable BUG/WARN checks and diagnostic reporting.                                                      |
| `KALLSYMS` | `y`    | Include kernel symbol information to display function names in stack traces, making debugging easier. |

6. Others

| Parameter | Target | Usage                                          |
| --------- | ------ | ---------------------------------------------- |
| `FTRACE`  | `n`    | Disable kernel tracing to save linux boot time |

7. Initframs

Make sure `INITRAMFS_SOURCE` is empty. Ignore initframs at first try.

#### Save a local copy

```shell
make -C "$LINUX_SRC" O="$LINUX_OUT" ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- savedefconfig
mkdir -p configs
cp "$LINUX_OUT/defconfig" configs/linux_defconfig
```

### Build Linux Image

```shell
make -C "$LINUX_SRC" O="$LINUX_OUT" \
    ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- \
    -j"$(nproc)" Image
```

Output image is located in: `linux-build/arch/riscv/boot/Image`
