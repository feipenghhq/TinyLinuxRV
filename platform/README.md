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

- [TinyLinuxRV v0.1 memory map](doc/memory-map/tinylinuxrv-v0.1.md)
- [Syscon device](doc/devices/syscon.md)

## Directories

- `doc/`: Platform specifications, including memory maps and device behavior.
- `include/`: Platform definitions shared by code.
- `devicetree/`: Device tree sources and build instructions.

OpenSBI adaptation and build instructions are in
[software/opensbi/](../software/opensbi/README.md).

## Shared Code

- `include/tinylinuxrv/addrmap.h`: Defines the TinyLinuxRV address map.
