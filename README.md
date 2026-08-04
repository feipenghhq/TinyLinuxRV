# TinyLinuxRV

> A tiny RV64 RISC-V CPU and SoC built from scratch to run Linux.

TinyLinuxRV is a long-term project to design and implement a complete RV64
RISC-V system, starting with a C emulator and eventually progressing to RTL
simulation and FPGA hardware.

The goal is to build a working Linux-capable CPU and SoC with a clear,
understandable architecture and a reusable software stack across the emulator,
RTL implementation, and FPGA platform.

---

## Project Strategy

TinyLinuxRV is developed in three major stages:

1. **Build the emulator**
   - Implement the RV64 ISA, privileged architecture, virtual memory, and devices.
   - Boot OpenSBI, Linux, and BusyBox.
   - Stabilize the emulator as the architectural reference model for RTL.

2. **Build the RTL CPU and SoC**
   - Implement the same architectural behavior in SystemVerilog.
   - Reuse the emulator's memory map, firmware, device tree, tests, and software images.
   - Validate the RTL against the emulator through differential testing.

3. **Deploy on FPGA**
   - Integrate physical memory, UART, clocking, reset, and board-specific logic.
   - Boot the same OpenSBI, Linux, and BusyBox software stack on hardware.

---

## Current Status

The project is currently in the emulator stage.

The immediate focus is:

- RV64I execution.
- ELF loading and bare-metal C programs.
- RV64M and RV64A extensions.
- Machine, supervisor, and user privilege modes.
- Sv39 virtual memory.
- UART, timer, and interrupt-controller support.
- OpenSBI, Linux, and BusyBox bring-up.
- Deterministic architectural tracing for later RTL verification.

See [PLAN_emulator.md](PLAN_emulator.md) for the detailed emulator roadmap.

---

## Planned Architecture

### Emulator

- Written in C99.
- RV64IMA.
- `Zicsr` and `Zifencei`.
- Machine, supervisor, and user privilege modes.
- Sv39 virtual memory.
- ELF64 loading.
- UART, timer, software interrupts, and external interrupts.
- OpenSBI, Linux, and BusyBox support.
- Deterministic architectural commit tracing.

### CPU

- RV64 in-order core.
- Five-stage pipeline.
- Machine, supervisor, and user modes.
- Sv39 virtual memory.
- Exceptions and interrupts.
- CSR support.

### SoC

- AHB-Lite interconnect.
- Boot ROM.
- RAM interface.
- UART.
- ACLINT/CLINT-style timer and software-interrupt device.
- PLIC-compatible external interrupt controller.
- Reset and shutdown device.
- External SDRAM or DDR memory support for FPGA deployment.

---

## Design Principles

- Keep the architecture small enough to understand and debug.
- Prefer correctness and observability over performance.
- Reuse the same platform definition and software stack across emulator and RTL.
- Add verification alongside each architectural feature.
- Keep the emulator deterministic so it can serve as the RTL reference model.
- Avoid unnecessary complexity until it is required by Linux or hardware integration.

---

## Planned Repository Structure

```text
TinyLinuxRV/
├── docs/
├── emulator/
│   ├── include/
│   ├── src/
│   ├── tests/
│   └── README.md
├── rtl/
│   ├── core/
│   ├── bus/
│   ├── peripherals/
│   └── soc/
├── platform/
│   ├── include/
│   ├── device-tree/
│   ├── memory-map/
│   └── config/
├── software/
│   ├── tests/
│   ├── baremetal/
│   ├── opensbi/
│   ├── linux/
│   ├── busybox/
│   └── images/
├── sim/
├── fpga/
├── scripts/
├── Makefile
├── README.md
├── PLAN_emulator.md
└── LICENSE
```

Directories will be added as their corresponding components are implemented.

---

## Project Goals

- Build a complete RV64 CPU capable of running Linux.
- Understand and control the full hardware and software stack.
- Create a reusable emulator for architectural validation.
- Implement the same system in synthesizable RTL.
- Boot OpenSBI, Linux, and BusyBox in simulation and on FPGA.
- Maintain clear documentation and reproducible builds.

---

## License

MIT License