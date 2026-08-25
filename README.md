# TinyLinuxRV

![ISA](https://img.shields.io/badge/ISA-RV64IMA-blue)
[![Emulator](https://img.shields.io/badge/Emulator-C99-00599C)](emulator/README.md)
![RTL](https://img.shields.io/badge/RTL-SystemVerilog%20(planned)-lightgrey)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)

> A tiny RV64 RISC-V CPU and SoC built from scratch to run Linux.

## Introduction

TinyLinuxRV is a long-term project to design and implement a Linux-capable
RV64IMA system, starting with a C emulator and eventually progressing
to RTL simulation and FPGA hardware.

The goal is to build a working Linux-capable CPU and SoC with a clear,
understandable architecture and a reusable software stack across the emulator,
RTL implementation, and FPGA platform.

---

## Current Status

The project is currently in the emulator stage.

Completed:

- ✅ RV64I execution.
- ✅ ELF loading and bare-metal C programs.
- ✅ RV64M and RV64A extensions.

Next:

- TinyLinuxRV machine model, MMIO dispatch, and configurable DRAM.
- UART, ACLINT, and PLIC device support.
- Machine-mode CSRs, exceptions, traps, and interrupts.
- Supervisor and user privilege modes.
- Sv39 virtual memory.
- OpenSBI, Linux, and BusyBox bring-up.
- Deterministic architectural tracing for later RTL verification.

See the [emulator milestone plan](docs/emulator/plans/plan.md) for the detailed
emulator roadmap.

---

## Getting Started

Initialize the test dependency and run the emulator regression:

```shell
git submodule update --init --recursive
make -C emulator regression
```

See the [emulator documentation](emulator/README.md) for build requirements,
command-line usage, and individual test targets.

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
   - Reuse the emulator test suite to validate the RTL implementation.

3. **Deploy on FPGA**
   - Integrate physical memory, UART, clocking, reset, and board-specific logic.
   - Boot the same OpenSBI, Linux, and BusyBox software stack on hardware.

---

## Target Architecture

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

## Documentation

- [Emulator](emulator/README.md) — Build, run, and test the C emulator.
- [Emulator Milestone Plan](docs/emulator/plans/plan.md) — Planned milestones and current progress.
- [Platform Specification](platform/README.md) — Shared hardware and software platform definitions.
- [Bare-Metal Environment](software/baremetal/README.md) — Runtime, memory layout, and example programs.
- [Development Log](docs/devlog.md) — Project development history and decisions.

## License

Licensed under the [MIT License](LICENSE).
