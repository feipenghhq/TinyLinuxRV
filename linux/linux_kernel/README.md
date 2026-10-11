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
```

The first build downloads the source into `local/linux_kernel` and loads
`configs/$(PLATFORM)/$(CONFIG)_defconfig`. Subsequent builds preserve `.config`.
Outputs are under `build/$(PLATFORM)/$(CONFIG)/`; the defaults are
`PLATFORM=emulator` and `CONFIG=early_boot`.

To build the boot images and launch the emulator from the repository root:

```shell
make -C emulator run-linux-emulator-early-boot
```


## Detailed instructions on how to build the Linux Kernel Image

These flows are part of the Makefile already. This section documents the detailed instructions

### Steps

1. Clone the Linux repo

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

TBD

