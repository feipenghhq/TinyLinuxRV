# TinyLinuxRV Emulator

The TinyLinuxRV emulator is a C99 implementation of the RISC-V machine that
will later be implemented in RTL.

## Current Capabilities

- RV64IMA instruction execution.
- `FENCE` and `FENCE.I` are currently treated as no-ops. Memory accesses are
  executed in order, and the emulator has no instruction cache.
- A fixed 1 MiB RAM region starting at `0x80000000`.
- Raw binary and RISC-V ELF64 loading, including `PT_LOAD` segments and BSS
  initialization.
- Explicit `elf` and `bin` input formats plus automatic file-type detection.
- A configurable instruction limit for detecting programs that do not finish.
- Bare-metal program results reported through `a0`.
- Automated `riscv-tests` RV64UI, RV64UM, and RV64UA regression.
- A bare-metal C regression using poisoned RAM.

## Next

- Implement the TinyLinuxRV machine model, including MMIO dispatch,
  configurable DRAM, boot ROM, and multi-image loading.
- Add UART, ACLINT, and PLIC device models.
- Add machine-mode CSRs, exceptions, traps, and interrupts, followed by
  supervisor and user privilege modes.
- Misaligned load/store accesses are rejected. The `ma_data` test is therefore
  skipped by the regression runner until trap handling is available.
- Add Sv39 virtual memory after privilege-mode support.

See the [emulator milestone plan](../docs/emulator/plans/plan.md) for the
planned development milestones.

## Prerequisites

Building the emulator requires a host C compiler and Make. Building the
RISC-V tests also requires Python 3, Git, and a RISC-V GNU cross-toolchain.

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
./rvemu [OPTIONS] FILE
```

For example:

```shell
./rvemu --max-instruction 10000 program.bin
./rvemu --format elf program.elf
./rvemu --format auto program.elf
```

The currently available options are:

| Option                    | Description                                                                                    |
| ------------------------- | ---------------------------------------------------------------------------------------------- |
| `--help`                  | Print the command-line help.                                                                   |
| `--max-instruction COUNT` | Stop with an error if the program exceeds the instruction limit.                               |
| `--format auto\|elf\|bin` | Select automatic detection, RISC-V ELF64 loading, or raw-binary loading. The default is `auto`. |
| `--riscv-tests`           | Interpret program termination using the emulator-specific `riscv-tests` PASS/FAIL protocol.    |
| `--poison-ram`            | Fill RAM with `0xA5` before loading the program. Used for testing.                              |

The `--riscv-tests` option is intended for the automated test environment, not
for general programs.

## Running Tests

Run the small emulator sanity test with:

```shell
make sanity-test
```

Build and run the enabled RV64IMA `riscv-tests` suites with:

```shell
make riscv-tests
```

Run the example bare-metal programs with:

```shell
make run-fib
make run-baremetal-test
```

`run-baremetal-test` loads a raw binary into RAM filled with `0xA5`. This
checks that the startup code clears `.bss` before calling `main`.

Run all current tests with:

```shell
make regression
```

The regression stops when a test fails.

The regression Makefile builds each test as both an ELF file and a raw binary,
then generates a manifest consumed by the Python runner. The runner executes
the ELF image directly and returns a nonzero host exit status if any test fails
or times out.

At the current milestone, all 53 RV64UI, 13 RV64UM, and 19 RV64UA tests pass.
`ma_data` is reported as skipped because the current execution environment
does not handle misaligned load/store traps.

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
