# TinyLinuxRV Bare-Metal Notes

This is a quick reference for understanding the TinyLinuxRV bare-metal build
and startup flow. See the [bare-metal environment README](../../../software/baremetal/README.md)
for build commands and current programs.

## Core Idea

A bare-metal program runs directly on the CPU without an operating system or a
hosted C runtime. TinyLinuxRV provides two core runtime files:

- [`linker.ld`](../../../software/baremetal/runtime/linker.ld) defines the
  program's memory layout.
- [`crt0.S`](../../../software/baremetal/runtime/crt0.S) prepares the execution
  environment before calling `main`.

## Build and Startup Flow

```text
C source + crt0.S
        ↓ compile
     object files
        ↓ ld + linker.ld
    ELF executable
        ↓ emulator loads PT_LOAD segments
      PC = e_entry
        ↓
       _start
        ↓
initialize gp and sp
        ↓
     clear .bss
        ↓
      call main
        ↓
return value stays in a0
        ↓
       EBREAK
```

## `linker.ld`

The linker script tells `ld` how to arrange input sections from the object
files in the final ELF executable.

| Output section | Contents |
| --- | --- |
| `.text.init` | Startup code containing `_start` |
| `.text` | Program instructions |
| `.rodata` | Read-only data |
| `.data` | Initialized global and static data |
| `.sdata` | Small initialized data accessed relative to `gp` |
| `.bss` / `.sbss` | Zero-initialized global and static data |
| `COMMON` | Common symbols collected into `.bss` |

The script also defines symbols used by the runtime:

- `__global_pointer$` initializes `gp` for small-data access.
- `__bss_start` and `__bss_end` delimit the memory cleared by `crt0.S`.
- `__stack_bottom` and `__stack_top` reserve and locate the stack.
- `__image_end` and the linker assertion prevent the program from overlapping
  the stack.

The `.bss` section is marked `NOLOAD`, so its zero bytes are not stored in the
ELF file. Startup code must clear this memory before entering C code.

## `crt0.S`

`crt0.S` contains `_start`, the first code executed by a bare-metal program. It:

1. Initializes `gp` from `__global_pointer$`.
2. Initializes `sp` from `__stack_top`.
3. Clears the memory between `__bss_start` and `__bss_end`.
4. Calls `main`.
5. Executes `EBREAK` while preserving the return value in `a0`.

The runtime does not copy `.data`: the emulator's ELF loader places initialized
data directly at its runtime address. For raw-binary tests, poisoned RAM checks
that `crt0.S`, rather than the loader, clears `.bss` correctly.
