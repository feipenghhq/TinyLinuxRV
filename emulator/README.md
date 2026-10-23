# TinyLinuxRV Emulator

The TinyLinuxRV emulator is a C99 implementation of the RISC-V machine that
will later be implemented in RTL.

## Current Capabilities

See the [emulator milestone plan](../docs/emulator/plans/plan.md) for the
planned development milestones.

### Platform

- RV64IMA with `Zicsr`.
- Machine, supervisor, and user privilege modes.
- Machine and supervisor CSRs, trap delegation, and trap returns.
- Sv39 virtual memory with instruction and data TLBs.
- Configurable DRAM.
- MMIO dispatch.
- Syscon.
- UART16550 with host terminal I/O.
- CLINT timer and software-interrupt sources.
- PLIC external interrupt controller.
- Read-only boot ROM and OpenSBI handoff to an S-mode payload.
- Linux 6.18.55 with an embedded initramfs and interactive BusyBox 1.36.1 shell.

### Emulator

- Raw binary and RISC-V ELF64 loading.
- Configurable instruction limit.

### Limitations

- `FENCE` and `FENCE.I` are currently treated as no-ops.
  Memory accesses are executed in order, and the emulator has no instruction
  cache.
- `SFENCE.VMA` enforces privilege and TVM checks and flushes the instruction
  and data TLBs.

## Verification

- `sanity`: Tests basic instruction identification and covers corner cases not
  covered by `riscv-tests`.
- `riscv-tests`: RV64UI, RV64UM, RV64UA, RV64MI, and RV64SI regression.
- `baremetal-test`: Covers basic software use cases.
- `devices`: Verifies implemented devices.
- `device-unit-tests`: Runs focused host-side device tests.
- `regression`: Runs all emulator tests.

## Prerequisites

Building the emulator requires a host C compiler and Make. Building the
RISC-V tests also requires Python 3, Git, and RISC-V GNU cross-toolchains.
The current `riscv-tests` build uses `riscv64-unknown-elf-` and Picolibc headers;
installation details are in the toolchain notes below.

Initialize the `riscv-tests` submodule from the repository root after cloning:

```shell
git submodule update --init --recursive
```

The required tools and installation notes are documented in
[riscv-toolchain.md](../docs/emulator/notes/riscv-toolchain.md).

## Building

Change to the emulator directory. The remaining commands in this document
assume this as the working directory:

```shell
cd emulator
```

Build the emulator with:

```shell
make build
```

The resulting executable is `rvemu`.

The default build uses informational logging. A different compile-time log
level can be selected through the Make variable:

```shell
make clean
make build LOG_LEVEL=LOG_LEVEL_DEBUG
```

## Running a Program

The command-line form is:

```text
./rvemu [OPTIONS] [FILE]
```

For example:

```shell
./rvemu --max-instruction 10000 program.bin
./rvemu --format elf program.elf
./rvemu --format auto program.elf
```

The currently available options are:

| Option                    | Description                                                                                     |
| ------------------------- | ----------------------------------------------------------------------------------------------- |
| `--help`                  | Print the command-line help.                                                                    |
| `--max-instruction COUNT` | Stop with an error if the program exceeds the instruction limit.                                |
| `--format auto\|elf\|bin` | Select automatic detection, RISC-V ELF64 loading, or raw-binary loading. The default is `auto`. |
| `--riscv-tests`           | Interpret program termination using the emulator-specific `riscv-tests` PASS/FAIL protocol.     |
| `--poison-ram`            | Fill RAM with `0xA5` before loading the program. Used for testing.                              |
| `--dram-size SIZE`        | Set the DRAM size in MiB. The default is 128 MiB; the supported range is 1–512 MiB.             |
| `--itrace`                | Dump recent instruction trace on execution failure. |
| `--ftrace`                | Write function trace during execution. |
| `--bootrom FILE`          | Load a boot ROM ELF and start in boot ROM mode. |
| `--bios FILE`             | Load a firmware ELF into DRAM. |
| `--kernel FILE`           | Load a raw next-stage image into DRAM at `0x80200000`. |
| `--dtb FILE`              | Load a DTB at the fixed boot-path address. |

With `--ftrace`, function trace is written to `func_trace.log` in the current
working directory during execution, including successful runs.

The `--riscv-tests` option is intended for the automated test environment, not
for general programs.

For OpenSBI build and sanity-run instructions, see
[OpenSBI](../linux/opensbi/README.md). This boot path currently assumes
128 MiB DRAM; use the default DRAM size.

## Running Linux and BusyBox

From the emulator directory, run in an interactive terminal:

```shell
make run-linux-emulator-busybox
```

This builds the emulator, boot ROM, DTB, OpenSBI, BusyBox rootfs, and Linux
kernel, then boots into a serial shell. `make run-linux-emulator` uses the
same BusyBox preset by default. The Makefile configures terminal input and
restores the host terminal settings when the emulator exits.

The boot flow is:

```text
Boot ROM → OpenSBI → Linux → embedded initramfs → /init → BusyBox shell
```

Use the default 128 MiB DRAM configuration. OpenSBI starts at `0x80000000`,
the raw kernel `Image` at `0x80200000`, and the DTB at `0x87f00000`.
The initramfs is embedded in the kernel; no separate initrd is loaded.

Try these commands in the guest:

```shell
echo TinyLinuxRV
uname -a
ls /
cat /proc/cpuinfo
```

For the kernel-only early-boot preset:

```shell
make run-linux-emulator-early-boot
```

This preset is for kernel bring-up and does not provide the BusyBox rootfs.
Linux/BusyBox boot is checked manually, separate from `run-regression`.

The first build needs network access and cross-toolchains. Rootfs preparation
may invoke `sudo` to create `/dev/console`. See the [Linux build notes](../linux/linux_kernel/README.md)
and [BusyBox build notes](../linux/busybox/README.md) for configuration and
additional prerequisites.

## Running Tests

Run the small emulator sanity test with:

```shell
make run-sanity-test
```

Build and run the enabled RV64IMA `riscv-tests` suites with:

```shell
make run-riscv-tests
```

Run the example bare-metal programs with:

```shell
make run-fib
make run-baremetal-test
```

`run-baremetal-test` loads an ELF into RAM filled with `0xA5`. The ELF loader
already clears `.bss`, so this run does not independently verify startup
BSS clearing.

Run all current tests with:

```shell
make run-regression
```

The regression runs all test groups and returns a failure status if any group fails.

The regression Makefile builds each test as both an ELF file and a raw binary,
then generates a manifest consumed by the Python runner. The runner executes
the ELF image directly and returns a nonzero host exit status if any test fails
or times out.

At the current milestone, `riscv-tests` supports the `p` (physical-address)
and `v` (Sv39 virtual-memory) environments.

All 192 enabled tests pass:

```text
[PASS] rv64ui-p 53/53  skipped: ma_data
[PASS] rv64um-p 13/13
[PASS] rv64ua-p 19/19
[PASS] rv64mi-p 15/15  skipped: pmpaddr, breakpoint
[PASS] rv64si-p  7/7
[PASS] rv64ui-v 53/53  skipped: ma_data
[PASS] rv64um-v 13/13
[PASS] rv64ua-v 19/19
```

For a concise explanation of the emulator and test build system, see
[makefiles.md](../docs/emulator/notes/makefiles.md).

## Related Documentation

- [Milestone Plan](../docs/emulator/plans/plan.md) — Detailed emulator
  milestones and current progress.
- [Development Roadmap](../docs/emulator/plans/roadmap.md) — Emulator-centered
  development and learning path.
- [Software Checkpoints](../docs/emulator/plans/software.md) — Real workloads
  for integration testing.
- [Development Log](../docs/emulator/plans/devlog.md) — Completed milestones,
  test results, and design decisions.
- [RISC-V Toolchain Notes](../docs/emulator/notes/riscv-toolchain.md) — Required
  tools and manual build steps.
- [Makefile Notes](../docs/emulator/notes/makefiles.md) — Build-system reference.
