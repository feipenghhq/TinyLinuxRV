# TinyLinuxRV Bare-Metal Environment

This directory contains the minimal runtime needed to run freestanding RISC-V
C programs on TinyLinuxRV.

The platform memory layout is documented in the
[TinyLinuxRV memory map](../../platform/memory-map/tinylinuxrv-v0.1.md).

## Startup Flow

The emulator loads the ELF file and starts it at `e_entry`. The initial
linker script places `_start` at `0x80000000`.

```text
Emulator loads ELF PT_LOAD segments
              ↓
        PC = ELF e_entry
              ↓
            _start
              ↓
       initialize gp and sp
              ↓
          clear .bss
              ↓
          call main
              ↓
  main return value stays in a0
              ↓
            EBREAK
```

For direct boot, `a0` contains hart ID `0` and `a1` contains `0` because a
Device Tree is not used yet. Startup code must not depend on the initial value
of other registers.

Raw binaries start at `0x80000000` because they do not contain an entry
address. ELF files start at the address stored in `e_entry`.

## Memory Layout

The current emulator has 1 MiB of DRAM:

```text
0x80000000  _start
            .text
            .rodata
            .data
            .bss
            free space
            stack (grows downward)
0x80100000  stack top
```

The stack is 16-byte aligned. The linker checks that the program does not
overlap the stack.

## Building

From this directory, build an example with:

```shell
make fib
make baremetal-test
```

The output files are placed under `build/examples/`.

The build is freestanding and uses RV64I/LP64 with the `medany` code model. It
also disables PIE, PIC, and the stack protector.

## Files

### `linker.ld`

The linker script:

- Sets `_start` as the ELF entry.
- Places the program in DRAM starting at `0x80000000`.
- Places `.text.init`, `.text`, `.rodata`, `.data`, `.sdata`, `.bss`, `.sbss`,
  and `COMMON`.
- Defines `__bss_start` and `__bss_end` for startup code.
- Defines the global-pointer symbol used to initialize `gp`.
- Defines the stack bottom and top.
- Checks that the program fits in DRAM.

`crt0.S` does not need to copy `.data` because the emulator loads it directly
at its runtime address.

### `crt0.S`

`crt0.S` contains `_start`. It:

1. Initializes `gp`.
2. Initializes the stack pointer.
3. Clears `.bss`.
4. Calls `main`.
5. Executes `EBREAK` without changing the return value in `a0`.

The initial C entry point is:

```text
int main(void)
```

## Program Result

The temporary completion convention is:

| Value of `a0` at `EBREAK` | Result               |
| ------------------------- | -------------------- |
| `0`                       | Guest program passed |
| Nonzero                   | Guest program failed |

Emulator errors such as invalid instructions, invalid memory accesses, and
timeouts are separate from the guest return value.

`EBREAK` is temporary. A shutdown/reset device will replace it later.

## Programs

### `fib`

A small freestanding C example. It calculates Fibonacci number 10 and returns
`0` when the result is correct.

### `baremetal-test`

A C test for global data, `.bss`, small data, stack use, function calls,
branches, loops, pointers, and different memory access sizes.

The emulator runs its raw binary with RAM filled with `0xA5`. This makes sure
that `crt0.S`, rather than the ELF loader, clears `.bss`.
