# OpenSBI

OpenSBI is an open source implementation of the RISC-V Supervisor Binary Interface (SBI).

This directory contains the TinyLinuxRV related platform and the flow to build sbi image.

## Getting Started

The Makefile uses the `riscv64-linux-gnu-` toolchain.

### Build the firmware image

```shell
make build-sbi-emu  # build the sbi for emulator
```

The output images are under
`build/<platform>/platform/<platform>/firmware/`:

- `fw_jump.elf`
- `fw_jump.bin`

### Testing OpenSBI

A small S-mode program checks runtime behavior and shuts down through
SBI SRST and syscon.

The program is in `<repo>/software/opensbi-test`

The following target builds the emulator, boot ROM, device tree, OpenSBI,
and test program before running it:

```shell
cd <repo>/emulator
make run-opensbi
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
in `objects.mk`. The Linux boot flow uses separate OpenSBI and Linux
images.

## References

1. [OpenSBI GitHub Repository](https://github.com/riscv-software-src/opensbi)
2. [RISC-V OpenSBI Deep Dive](https://riscv.org/wp-content/uploads/2024/12/13.30-RISCV_OpenSBI_Deep_Dive_v5.pdf)
