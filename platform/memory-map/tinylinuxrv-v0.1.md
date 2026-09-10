# TinyLinuxRV Memory Map

This document defines version 0.1 of the TinyLinuxRV physical address map.
The layout follows the broad structure of the QEMU RISC-V `virt` platform so
that existing RISC-V firmware and operating-system conventions can be reused
where practical.

## Address Conventions

- All addresses are physical byte addresses.
- The platform is little-endian.
- Address ranges use half-open notation: `[base, end)` includes `base` and
  excludes `end`.
- A reserved window does not imply that storage exists for every address in
  the window.
- Accesses to unmapped addresses must fail with an access fault once trap
  handling is implemented. Until then, the emulator must report an execution
  error.
- Devices marked as planned are reserved in the address map but are not
  currently visible to guest software.

## Physical Address Map

| Device                   |         Base |                  Size | End Exclusive | Access | Status      |
| ------------------------ | -----------: | --------------------: | ------------: | :----: | ----------- |
| Boot ROM                 | `0x00001000` |          `0x0000f000` |  `0x00010000` |  R-X   | Planned     |
| Reset/syscon             | `0x00100000` |          `0x00001000` |  `0x00101000` |  RW-   | Implemented |
| CLINT                    | `0x02000000` |          `0x00010000` |  `0x02010000` |  RW-   | Implemented |
| PLIC                     | `0x0c000000` | `0x04000000` reserved |  `0x10000000` |  RW-   | Implemented |
| UART0 (16550-compatible) | `0x10000000` | `0x00001000` reserved |  `0x10001000` |  RW-   | Implemented |
| VirtIO MMIO (8 slots)    | `0x10001000` |          `0x00008000` |  `0x10009000` |  RW-   | Planned     |
| DRAM                     | `0x80000000` |  `0x08000000` default |  `0x88000000` |  RWX   | Implemented |

- The access column describes the intended platform behavior. Memory protection
  and execute permissions are not yet enforced by the emulator.
- DRAM size is configurable. The default is 128 MiB.

## Region Notes

### Boot ROM

The Boot ROM window is reserved for the reset stub that will eventually run
at CPU reset and transfer control to M-mode firmware. Direct ELF and raw-binary
boot modes may bypass the Boot ROM.

### Reset/syscon

The reset system controller provides shutdown and reset requests. It will
eventually replace `EBREAK` as the normal bare-metal completion mechanism and
serve as the platform backend for SBI system reset operations.

See the [syscon device documentation](../devices/syscon.md) for its behavior
and register layout.

### CLINT

The CLINT provides machine-level timer and software-interrupt functions. Their
pending conditions will connect directly to a hart and do not pass through the
PLIC.

### PLIC

The PLIC window reserves all addresses through the start of UART0. The
implemented register range contains only the supported interrupt sources and
hart contexts. No memory is allocated for unused addresses in the reserved
window.

### UART0

UART0 reserves one 4 KiB page even though the implemented 16550-compatible
register block is smaller. It provides transmit and receive data and signals
external interrupts through the PLIC.

### VirtIO MMIO

The VirtIO transport window contains eight 4 KiB slots:

| Slot |         Base | End Exclusive | PLIC IRQ |
| ---: | -----------: | ------------: | -------: |
|    0 | `0x10001000` |  `0x10002000` |        1 |
|    1 | `0x10002000` |  `0x10003000` |        2 |
|    2 | `0x10003000` |  `0x10004000` |        3 |
|    3 | `0x10004000` |  `0x10005000` |        4 |
|    4 | `0x10005000` |  `0x10006000` |        5 |
|    5 | `0x10006000` |  `0x10007000` |        6 |
|    6 | `0x10007000` |  `0x10008000` |        7 |
|    7 | `0x10008000` |  `0x10009000` |        8 |

Unused slots remain unmapped. The first planned use is a VirtIO block device
for a Linux root filesystem.

### DRAM

The emulator provides 128 MiB of DRAM by default at
`[0x80000000, 0x88000000)`. Users can override the size with the
`--dram-size` option. The base address remains `0x80000000`.

## Interrupt Map

| Source                   | Route          |               Interrupt ID | Status       |
| ------------------------ | -------------- | -------------------------: | ------------ |
| VirtIO MMIO slots 0-7    | PLIC           |                        1-8 | Planned      |
| UART0                    | PLIC           |                         10 | Implemented  |
| CLINT software interrupt | Direct to hart | Machine software interrupt | Pending only |
| CLINT timer interrupt    | Direct to hart |    Machine timer interrupt | Pending only |

PLIC interrupt source 9 and source IDs above 10 are reserved for future
devices. Interrupt IDs describe PLIC sources and are independent of MMIO
addresses.
