# OpenSBI

OpenSBI is an open source implementation of the RISC-V Supervisor Binary Interface (SBI).

This directory contains the TinyLinuxRV platform adaptation and build entry point.
Upstream sources are in `third-party/opensbi/` at the repository root.

## Getting Started

The Makefile uses the `riscv64-linux-gnu-` toolchain. The upstream OpenSBI sources must be present.

### Build the emulator firmware

```shell
make build-emu-sbi
```

The output images are under
`build/tinylinuxrv_emu/platform/tinylinuxrv_emu/firmware/`:

- `fw_jump.elf`
- `fw_jump.bin`

### Testing OpenSBI

A simple "kernel" program is created to test OpenSBI handoff and running in Supervisor mode.

To run the OpenSBI and the program on the emulator:

```shell
make run-opensbi-test-emu
```

## Platform Adaptation

Platform-specific files are located in `platform/<platform name>/`.
The main adaptation files are:

- `objects.mk`: Platform build settings and firmware configuration.
- `platform.c`: Platform initialization and callbacks.

Device tree sources and build instructions are maintained separately in
[platform/devicetree/](../../platform/devicetree/README.md).

### Supported Platforms

| Platform | Directory |
| -------- | --------- |
| Emulator | tinylinuxrv_emu |

### Firmware Types

OpenSBI provides three reference firmware types:

- FW_PAYLOAD: Includes the next boot stage as a payload.
- FW_JUMP: Uses a statically configured next-stage address.
- FW_DYNAMIC: Receives next-stage information from the previous boot stage.

TinyLinuxRV currently builds FW_JUMP, with the next-stage address configured
in `objects.mk`. The planned Linux boot flow uses separate OpenSBI and Linux
images.

## References

1. [OpenSBI GitHub Repository](https://github.com/riscv-software-src/opensbi)
2. [RISC-V OpenSBI Deep Dive](https://riscv.org/wp-content/uploads/2024/12/13.30-RISCV_OpenSBI_Deep_Dive_v5.pdf)
