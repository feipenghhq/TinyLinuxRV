# Device Tree

The device tree describes platform hardware for OpenSBI and Linux.
This directory contains Device Tree Source (`.dts`) files and compiled
Device Tree Blob (`.dtb`) files for TinyLinuxRV platforms.

## Getting Started

The `device-tree-compiler` package is required. Run these commands from this
directory.

Build the all the DTB:

```shell
make all
```

Build a specific DTB:

```shell
make tinylinuxrv_emu.dtb
```

DTB files are generated alongside their DTS sources.

Dump a reference device tree from QEMU virt (requires `qemu-system-riscv64`):

```shell
make qemu-ref-dtb
```

This produces `qemu-virt.dtb` and `qemu-virt.dts` for reference.

## Files and Supported Platforms

| DTS/DTB base name | Platform |
| ----------------- | -------- |
| tinylinuxrv_emu | Emulator |

## References

1. [Linux and the Devicetree](https://www.kernel.org/doc/html/latest/devicetree/usage-model.html)
2. [Device Tree Specification](https://www.devicetree.org/specifications)
