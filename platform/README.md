# TinyLinuxRV Platform

This directory defines the hardware and software contract shared by the
TinyLinuxRV emulator, RTL and FPGA implementations, firmware, and
operating-system software.

It also stores definitions shared by the emulator and bare-metal software.

The platform specification covers:

- The physical address map.
- Reset and boot conventions.
- Memory and device properties.
- Interrupt routing.
- Interfaces that must remain consistent across implementations.

## Specifications

- [TinyLinuxRV v0.1 memory map](memory-map/tinylinuxrv-v0.1.md)

## Shared Code

- `include/tinylinuxrv/addrmap.h`: Defines the TinyLinuxRV address map.
