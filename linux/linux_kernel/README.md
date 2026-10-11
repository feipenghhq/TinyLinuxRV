# Linux

This directory port and build linux kernel image for TinyLinuxRV.

## Version

Linux: `Linux 6.18 LTS (v6.18.55)`

## Build with Make

From this directory:

```shell
make                    # Build emulator/early_boot Image
make menuconfig         # Edit the current configuration
make build-image        # Rebuild, preserving configuration edits
make load-config        # Reset to the selected preset
make build-emulator-busybox # Build rootfs, then the BusyBox kernel
make mrproper           # Explicitly clean the source tree
```

The first build downloads the source into `local/linux_kernel` and loads
`configs/$(PLATFORM)/$(DEFCONFIG)_defconfig`. Subsequent builds preserve `.config`.
Outputs are under `build/$(PLATFORM)/$(DEFCONFIG)/`; the defaults are
`PLATFORM=emulator` and `DEFCONFIG=early_boot`.

Preset build targets preserve existing configuration edits. To reset and rebuild, run
`make DEFCONFIG=busybox load-config` followed by `make build-emulator-busybox`.
Do not pass `load-config build-image` together with `-j`: those are separate goals.
Source cleanup is explicit; header installation uses a separate `build/headers` directory.

To build the boot images and launch the emulator from the repository root:

```shell
make -C emulator run-linux-emulator-early-boot
```

## Detailed instructions on how to build the Linux Kernel Image

These flows are part of the Makefile already. This section documents the detailed instructions

### Instructions

1. Clone the Linux repo

```shell
cd <repo>
git clone --depth 1 --branch v6.18.55 https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git local/linux_kernel
```

2. Setup Environment

```shell
cd <repo>/linux/linux_kernel

LINUX_SRC=`realpath ../../local/linux_kernel`
LINUX_OUT=`realpath build`

mkdir -p $LINUX_OUT

export ARCH=riscv
export CROSS_COMPILE=riscv64-linux-gnu-
```

3. Config Linux to match with TinyLinuxRV environment

We make minimal configuration change from default config to make the linux kernel image to support TinyLinuxRV

The folder has pre-defined config file in `config`. Use the following command to load a predefined config

```shell
cp configs/<config you want to load> "$LINUX_OUT/.config"
make -C "$LINUX_SRC" O="$LINUX_OUT" olddefconfig
```

If you want to manually config it from default config:

```shell
make -C "$LINUX_SRC" O="$LINUX_OUT" defconfig
make -C "$LINUX_SRC" O="$LINUX_OUT" menuconfig
# ... Change the config
make -C "$LINUX_SRC" O="$LINUX_OUT" savedefconfig   # Save a local copy
cp "$LINUX_OUT/defconfig" configs/<config you want to name>
```

4. Build the linux kernel image

After getting correct configuration. We can build the linux image.

> !Note: the commands use all the available CPU cores to speed up the process.
> It could take some time to build the image

```shell
make -C "$LINUX_SRC" O="$LINUX_OUT" -j"$(nproc)" Image
```

## Detailed configuration made to work with TinyLinuxRV

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
