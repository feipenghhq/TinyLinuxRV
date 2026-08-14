# TinyLinuxRV Emulator

The TinyLinuxRV emulator is a C99 implementation of the RISC-V machine that
will later be implemented in RTL.

## Current Capabilities

- RV64I base integer instruction execution.
- `FENCE` and `FENCE.I` are currently treated as no-ops. Memory accesses are
  executed in order, and the emulator has no instruction cache.
- A fixed 1 MiB RAM region starting at `0x80000000`.
- Raw binary loading at the RAM base, with execution starting at the same
  address.
- A configurable instruction limit for detecting programs that do not finish.
- Automated `riscv-tests` RV64UI regression.

## Next

- ELF loading and automatic file-type detection are not implemented.
- Privilege modes, CSRs, exceptions, interrupts, and trap handling are not
  implemented.
- Misaligned load/store accesses are rejected. The `ma_data` test is therefore
  skipped by the regression runner until trap handling is available.
- The M and A extensions, virtual memory, and devices are not implemented.

See the [emulator roadmap](../doc/PLAN_emulator.md) for the planned development
milestones.

## Prerequisites

Building the emulator requires a host C compiler and Make. Building the
RISC-V tests also requires Python 3, Git, and a RISC-V GNU cross-toolchain.

Initialize the `riscv-tests` submodule from the repository root after cloning:

```shell
git submodule update --init --recursive
```

The required tools and installation notes are documented in
[toolchain.md](../doc/emulator/toolchain.md).

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

## Running a Raw Binary

The command-line form is:

```text
./rvemu [OPTIONS] FILE
```

For example:

```shell
./rvemu --max-instruction 10000 program.bin
```

The currently available options are:

| Option | Description |
| --- | --- |
| `--help` | Print the command-line help. |
| `--max-instruction COUNT` | Stop with an error if the program exceeds the instruction limit. |
| `--format auto\|elf\|bin` | Select an input format. Only raw binary loading is currently implemented; `auto` and `elf` are placeholders. |
| `--riscv-tests` | Interpret program termination using the emulator-specific `riscv-tests` PASS/FAIL protocol. |

The `--riscv-tests` option is intended for the automated test environment, not
for general programs.

## Running Tests

Run the small emulator sanity test with:

```shell
make sanity-test
```

Build and run the complete enabled RV64UI regression with:

```shell
make riscv-tests
```

The regression Makefile builds each test as an ELF file, converts it to a raw
binary, and generates a manifest consumed by the Python runner. The runner
returns a nonzero host exit status if any executed test fails or times out.

At the current milestone, 53 RV64UI tests are executed and pass. `ma_data` is
reported as skipped because the current execution environment does not handle
misaligned load/store traps.

For a detailed explanation of the test build system, see
[riscv-tests-makefile.md](../doc/emulator/riscv-tests-makefile.md).
