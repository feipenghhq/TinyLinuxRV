# TinyLinuxRV Milestone Plan

TinyLinuxRV is a project to build a simple RV64 RISC-V system that can boot Linux, first in a C emulator, then in RTL simulation, and finally on an FPGA.

---

# Project Strategy

TinyLinuxRV follows three major stages:

1. **Build a complete RV64 emulator**
   - Implement the ISA, privileged architecture, virtual memory, and devices.
   - Boot OpenSBI, Linux, and BusyBox.

2. **Build the RTL processor and SoC**
   - Implement the same architectural behavior in hardware.
   - Reuse the same memory map, device tree, firmware, kernel configuration, and test programs.

3. **Deploy on FPGA**
   - Integrate physical memory and UART.
   - Boot the same Linux software stack on hardware.

The emulator and RTL implementations live in the same repository so they can share:

---

# Repository Layout (Planned)

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
└── LICENSE
```

---

# Phase 0 — Project Foundation

## Milestone 0: Development Environment and Repository Structure

### Goals

Create a reproducible development environment that supports both the emulator and the later RTL implementation.

### Tasks

- Install and document the RV64 cross-compilation toolchain.
- Use C11 and Make for the emulator.
- Use SystemVerilog and Verilator for the later RTL implementation.
- Add a small RV64 assembly toolchain smoke test.

### Completion Criteria

- The development environment can be reproduced using documented commands.
- A minimal RV64 assembly program can be compiled into ELF and binary formats.

### Deliverable

**v0.1 — Development environment ready**

---

# Phase 1 — RV64 User-Mode Emulator

## Milestone 1: Minimal RV64I Execution Engine

### Goals

Implement a basic emulator capable of running RV64I programs.

### Core Components

- 32 general-purpose registers and a 64-bit program counter.
- Byte-addressable guest memory.
- Instruction fetch, decode, and execute loop.
- Raw binary loading at a configurable guest address.
- Register dump and instruction trace support.
- Explicit diagnostics for invalid instructions and invalid memory accesses.

### ISA Scope

- Implement the complete RV64I base integer instruction set.
- Reject unsupported and reserved instruction encodings explicitly.
- Defer architectural handling of `ECALL` and `EBREAK`, CSRs, privilege modes, and traps to later phases.

### Test Environment

- Define a minimal memory layout, load address, and entry point for ISA tests.
- Add the linker script and target macros required by `riscv-tests`.
- Build tests as ELF files, convert them to flat binaries, and run them through the raw-binary loader.
- Automate test execution, timeout handling, pass/fail reporting, and failure traces.

### Verification

- Integrate the applicable RV64I tests from `riscv-tests`.
- Run tests incrementally as each instruction family is implemented.
- Run the complete RV64I regression after every ISA implementation change.

### Completion Criteria

- The complete RV64I base integer instruction set is implemented.
- All applicable RV64I tests from `riscv-tests` pass.
- Raw binary programs execute from a configurable address.
- Invalid instructions and invalid memory accesses produce clear diagnostics.
- Failed tests provide useful diagnostics and execution traces.

### Deliverable

**v0.2 — Tested RV64I emulator**

---

## Milestone 2: ELF Loading and Bare-Metal C Programs

### Goals

Turn the verified RV64I interpreter into a practical bare-metal execution environment capable of loading standard RISC-V ELF executables and running freestanding C programs.

### Bare-Metal Platform

- Define the RAM layout, program entry point, stack convention, and program exit mechanism.
- Keep the initial platform simple; define the complete Linux and MMIO memory map in Phase 2.

### ELF Loader

- Load valid RISC-V ELF64 executables using `PT_LOAD` segments and start execution at `e_entry`.
- Support initialized and zero-filled segments.
- Validate ELF metadata and reject malformed or out-of-range images with clear diagnostics.

### Bare-Metal Build Environment

- Add a linker script and minimal startup code for freestanding RV64I programs.
- Initialize the stack, clear `.bss`, call `main`, and report its return value.
- Document the required compiler and linker flags and provide example assembly and C builds.

### Emulator Interface

- Add command-line support for ELF and raw binaries, memory configuration, tracing, dumps, and instruction limits.
- Return distinct host exit codes for guest results and emulator errors.

### Bare-Metal C Verification

- Add freestanding C tests covering global data, `.bss`, stack usage, function calls, control flow, arrays, pointers, and different memory access widths.

### Architectural Verification

- Integrate the applicable RV64I architectural tests using ACT4.
- Add the required TinyLinuxRV target configuration and automated test runner.
- Run both ACT4 and `riscv-tests` regressions in CI.

### Completion Criteria

- The emulator loads valid RISC-V ELF64 executables using `PT_LOAD` segments.
- A freestanding RV64I C program runs successfully.
- Global data, `.bss`, stack usage, function calls, arrays, pointers, branches, and loops work correctly.
- The applicable RV64I architectural tests pass.
- The complete Milestone 1 `riscv-tests` regression continues to pass.
- Malformed ELF files and invalid memory mappings produce useful diagnostics.

### Deliverable

**v0.3 — Verified RV64I bare-metal emulator**

---

## Milestone 3: RV64IMA Extension Support

### Goals

Extend the verified RV64I emulator with the multiplication, division, and atomic instructions needed by compiled system software and the later Linux software stack.

### ISA Scope

- Implement the complete RV64M extension.
- Implement the complete RV64A extension with deterministic single-hart reservation behavior.

### Build Environment

- Update the bare-metal toolchain configuration for RV64IMA.
- Add compiled C and assembly examples exercising multiplication, division, and atomic operations.

### Verification

- Run the applicable RV64M and RV64A tests from `riscv-tests`.
- Run the applicable RV64M and RV64A architectural tests.
- Add focused tests for multiplication high halves, division corner cases, word-result sign extension, LR/SC success and failure, reservation invalidation, and word/doubleword AMOs.
- Run the complete RV64I regression after adding M and A support.

### Completion Criteria

- All applicable RV64I, RV64M, and RV64A tests pass.
- Compiled bare-metal programs can use multiplication, division, and atomic operations.
- Single-hart reservation behavior is documented and verified.
- The RV64IMA regression runs automatically in CI.

### Deliverable

**v0.4 — Verified RV64IMA emulator**

---

# Phase 2 — Emulator Platform and Basic Devices

## Milestone 4: TinyLinuxRV Machine Model

### Goals

Define the machine-level platform required by firmware, operating-system bring-up, and the later RTL implementation.

### Platform Definition

- Define the reset address and physical memory map for ROM, RAM, and MMIO devices.
- Keep the platform definition shared by the emulator, future RTL, firmware, and device tree.
- Reserve address ranges for later interrupt controllers and additional peripherals.

### Machine Model

- Add MMIO address dispatch.
- Add boot ROM loading.
- Add reset and shutdown behavior.
- Reject unmapped and invalid device accesses with clear diagnostics.
- Keep platform constants in a shared definition suitable for reuse by software and RTL.

### Verification

- Add tests for ROM, RAM, and MMIO address decoding.
- Verify reset and shutdown behavior.
- Verify invalid and overlapping memory mappings are rejected.
- Run the complete RV64IMA regression after introducing the machine model.

### Completion Criteria

- The TinyLinuxRV reset address and memory map are documented.
- Boot ROM, RAM, and MMIO regions behave correctly.
- Invalid physical accesses produce clear diagnostics.
- Platform definitions can be reused by later firmware and RTL work.
- Existing ISA regressions continue to pass.

### Deliverable

**v0.5 — TinyLinuxRV machine model**

---

## Milestone 5: UART and Timer Devices

### Goals

Add the basic devices required for observable bare-metal software and early firmware development without depending on privileged CPU interrupt handling.

### UART

- Implement a polling-capable memory-mapped UART.
- Support guest transmit and receive registers.
- Connect UART input and output to the host terminal.
- Expose status information required by polling software.
- Expose a device-level interrupt-pending signal for later privileged-architecture integration.

### Timer

- Implement a deterministic ACLINT/CLINT-style machine timer.
- Provide a readable machine-time counter and writable compare register.
- Define deterministic timer progression suitable for testing.
- Expose a timer-pending condition when the counter reaches the compare value.
- Defer architectural timer-interrupt delivery to Phase 3.

### Verification

- Add MMIO register tests for UART and timer behavior.
- Run bare-metal programs that use polling UART input and output.
- Verify that software can read the timer counter and program the compare register.
- Verify UART and timer pending conditions at the device level.
- Run the complete RV64IMA regression after adding the devices.

### Completion Criteria

- Bare-metal software can communicate through the UART using polling.
- UART input and output work through the host terminal.
- Software can read the timer and program its compare register.
- Timer and UART pending conditions are generated correctly.
- No privileged CSR or trap handling is required to test this milestone.
- Existing ISA and bare-metal regressions continue to pass.

### Deliverable

**v0.6 — Basic UART and timer platform**

---

# Phase 3 — Privileged Architecture

## Milestone 6: Machine Mode, CSRs, and Interrupts

### Goals

Implement the machine-mode privileged architecture required for traps, interrupts, firmware, and later supervisor-mode execution.

### ISA and CSR Support

- Implement the `Zicsr` extension.
- Implement `Zifencei`; treat `FENCE.I` as a valid no-op while the emulator has no instruction cache.
- Implement the machine-mode CSRs required by traps and interrupts, including:
  - `mstatus`
  - `misa`
  - `medeleg`
  - `mideleg`
  - `mie`
  - `mtvec`
  - `mcounteren`
  - `mscratch`
  - `mepc`
  - `mcause`
  - `mtval`
  - `mip`
- Enforce CSR privilege levels, read-only behavior, and WARL constraints where required.

### Exceptions and Trap Handling

- Implement synchronous exceptions for:
  - Illegal instructions.
  - Instruction-address misalignment.
  - Load- and store-address misalignment.
  - Instruction, load, and store access faults.
  - Environment calls.
  - Breakpoints.
- Implement direct and vectored `mtvec` behavior where applicable.
- Save trap state in `mepc`, `mcause`, and `mtval`.
- Update interrupt-enable state in `mstatus` during trap entry.
- Implement `MRET` and restore the previous privilege and interrupt state.

### Machine Interrupts

- Connect the Phase 2 timer-pending condition to the machine timer interrupt.
- Add a machine software-interrupt source.
- Add external interrupt support through a PLIC-compatible interrupt controller.
- Route UART interrupt requests through the external interrupt controller.
- Implement `mip`, `mie`, and `mstatus.MIE` behavior.
- Evaluate interrupts at instruction boundaries and apply architectural priority rules.

### Verification

- Add focused tests for CSR access permissions and side effects.
- Test each synchronous exception and verify trap state.
- Test direct and vectored trap entry.
- Test `MRET` state restoration.
- Add end-to-end timer, software, and external interrupt tests.
- Run the applicable privileged-architecture and CSR tests.
- Continue running the complete RV64IMA regression.

### Completion Criteria

- Machine-mode CSR instructions and required CSRs behave correctly.
- Exceptions enter machine-mode trap handlers with correct cause and state.
- `MRET` restores execution correctly.
- Timer, software, and UART external interrupts are delivered end to end.
- PLIC claim and completion behavior works for the supported devices.
- Existing user-mode and ISA regressions continue to pass.

### Deliverable

**v0.7 — Machine-mode privileged emulator**

---

## Milestone 7: Supervisor and User Modes

### Goals

Add supervisor and user privilege modes, delegation, and privilege transitions required by OpenSBI and Linux.

### Supervisor Mode

- Implement supervisor-mode CSR views and state, including:
  - `sstatus`
  - `sie`
  - `stvec`
  - `scounteren`
  - `sscratch`
  - `sepc`
  - `scause`
  - `stval`
  - `sip`
  - `satp`
- Implement machine-to-supervisor exception and interrupt delegation.
- Implement supervisor trap entry.
- Implement `SRET`.
- Implement `WFI` with deterministic emulator behavior.
- Implement `satp.MODE=Bare`; Sv39 translation is added in Phase 4.

### User Mode

- Implement user-mode execution.
- Enforce privilege checks for instructions, CSRs, and memory operations.
- Support transitions from supervisor mode to user mode.
- Support traps from user mode back to supervisor or machine mode.
- Preserve the required trap and return state across privilege transitions.

### Verification

- Add machine-to-supervisor transition tests.
- Test exception and interrupt delegation.
- Test supervisor trap entry and `SRET`.
- Test supervisor-to-user transitions and user-mode traps.
- Test CSR accessibility in each privilege mode.
- Test `WFI` wake-up through pending interrupts.
- Run applicable privileged-architecture tests.

### Completion Criteria

- Machine, supervisor, and user modes execute correctly.
- Exception and interrupt delegation works as configured.
- Supervisor trap handling and `SRET` work correctly.
- User-mode programs can execute and trap back to supervisor mode.
- `satp.MODE=Bare` behaves correctly.
- Privilege-transition regressions run automatically.

### Deliverable

**v0.8 — M/S/U privileged emulator**

---

# Phase 4 — Sv39 Virtual Memory

## Milestone 8: Sv39 Address Translation

### Goals

Implement the Sv39 virtual-memory architecture required by supervisor software and Linux.

### Address Translation

- Implement Sv39 virtual-address validation and sign-extension rules.
- Implement the three-level page-table walk.
- Support 4 KiB, 2 MiB, and 1 GiB leaf mappings.
- Apply address translation to instruction fetches, loads, and stores.
- Preserve physical-address access for machine-mode operations that bypass translation.

### Page-Table Entries and Permissions

- Decode and validate Sv39 page-table entries.
- Enforce valid, readable, writable, executable, user, global, accessed, and dirty bits.
- Enforce privilege and access permissions using `SUM` and `MXR`.
- Detect invalid PTE combinations and misaligned superpages.
- Initially raise page faults when required accessed or dirty bits are clear.
- Keep the A/D-bit policy explicit so it can later be matched by the RTL implementation.

### Faults and Synchronization

- Generate instruction, load, and store page faults with correct `stval` or `mtval`.
- Implement `SFENCE.VMA`.
- Treat `SFENCE.VMA` as a valid synchronization operation even before a TLB is added.
- Initially perform page-table walks directly without a TLB.
- Add a TLB later only if performance requires it.

### Verification

- Test identity and non-identity virtual mappings.
- Test 4 KiB, 2 MiB, and 1 GiB mappings.
- Test user and supervisor permissions.
- Test execute-only, read-only, and writable mappings.
- Test invalid PTEs and misaligned superpages.
- Test instruction, load, and store page faults.
- Test `SUM`, `MXR`, and A/D-bit behavior.
- Test `SFENCE.VMA` behavior.
- Run applicable Sv39 architectural tests.

### Completion Criteria

- Sv39 address translation works for instruction fetches, loads, and stores.
- All supported page sizes behave correctly.
- Permission checks and page faults match the privileged specification.
- Supervisor and user programs can execute through virtual mappings.
- Applicable Sv39 architectural tests pass.
- Existing ISA and privilege regressions continue to pass.

### Deliverable

**v0.9 — Sv39-capable emulator**

---

# Phase 5 — OpenSBI Bring-up

## Milestone 9: OpenSBI and SBI Validation

### Goals

Boot OpenSBI on TinyLinuxRV and validate the machine-to-supervisor firmware interface before attempting Linux.

### Platform Integration

- Prefer the OpenSBI generic platform with a device tree.
- Add a TinyLinuxRV-specific OpenSBI platform only if the generic platform cannot support the required devices.
- Provide a device tree describing:
  - CPU and ISA capabilities.
  - Physical memory.
  - UART.
  - Timer and software-interrupt device.
  - External interrupt controller.
  - Reset or shutdown device.
- Keep the device tree consistent with the emulator memory map.

### Firmware Loading

- Define the OpenSBI firmware load address and entry point.
- Load the firmware and supervisor payload into guest memory.
- Pass the hart ID and device-tree address according to the selected boot convention.
- Start OpenSBI in machine mode.

### SBI Support

- Validate the SBI implementation provided by OpenSBI against the TinyLinuxRV platform.
- Verify the SBI calls required for early Linux boot, including:
  - Console or debug output as applicable.
  - Timer programming.
  - System reset and shutdown.
  - Base extension queries.
- Defer multi-hart IPI behavior while TinyLinuxRV remains single-hart.

### Supervisor Payload

- Add a small supervisor-mode payload that OpenSBI can launch.
- Verify that the payload:
  - Enters supervisor mode successfully.
  - Prints output through SBI or the platform UART.
  - Programs a timer event.
  - Receives the resulting supervisor timer interrupt.
  - Returns or shuts down cleanly.

### Verification

- Add an automated OpenSBI boot test.
- Check for a deterministic OpenSBI boot-complete marker.
- Test transfer from OpenSBI to the supervisor payload.
- Test required SBI calls independently where practical.
- Preserve traces for firmware boot failures.

### Completion Criteria

- OpenSBI boots and reports the expected TinyLinuxRV platform.
- OpenSBI enters a supervisor-mode payload successfully.
- Required SBI calls execute correctly.
- Supervisor timer delivery works through OpenSBI.
- The device tree accurately describes the emulated platform.
- OpenSBI boot is covered by an automated regression.

### Deliverable

**v1.0 — OpenSBI-capable emulator**

---

# Phase 6 — Linux and BusyBox

## Milestone 10: Linux Early Boot

### Goals

Boot a Linux kernel far enough to initialize the architecture, parse the device tree, enable virtual memory, and produce early console output.

### Kernel Boot Interface

- Build a Linux kernel for the supported TinyLinuxRV ISA profile.
- Load the kernel `Image` at an address satisfying the RISC-V boot protocol and required alignment.
- Enter the kernel in supervisor mode with:
  - `a0` containing the hart ID.
  - `a1` containing the physical address of the device tree.
  - Virtual memory initially disabled.
- Keep the kernel, OpenSBI, device tree, and initramfs load addresses documented and non-overlapping.

### Early Platform Bring-up

- Provide early console output.
- Verify CPU and ISA detection.
- Verify physical-memory discovery from the device tree.
- Verify timer and interrupt-controller discovery.
- Support early page-table creation and MMU enablement.
- Use Linux boot failures to identify missing architectural behavior and add a regression test for each fixed issue.

### Verification

- Add an automated Linux early-boot test.
- Detect a stable kernel log marker after MMU initialization.
- Preserve serial output and execution traces on failure.
- Continue running OpenSBI and lower-level regressions.

### Completion Criteria

- Linux starts through OpenSBI.
- Early console output is visible.
- Linux discovers the CPU, memory, and required platform devices.
- Linux enables Sv39 and continues executing.
- The early-boot checkpoint is reproducible and automated.

### Deliverable

**v1.1 — Linux early boot**

---

## Milestone 11: Scheduler and Initramfs

### Goals

Bring Linux far enough to initialize timers, scheduling, kernel threads, and an initial RAM filesystem.

### Kernel Progress

- Support the SBI timer interface required by Linux.
- Verify periodic timer events and scheduler ticks.
- Bring up the scheduler and kernel threads.
- Provide the platform behavior required for memory allocation and process creation.
- Add a minimal initramfs containing an executable `/init`.
- Continue fixing missing privileged, MMU, or device behavior with focused regression tests.

### Console and Interrupts

- Support reliable console input and output.
- Polling UART may be used initially if it allows Linux bring-up to continue.
- Complete interrupt-driven UART input when required for a stable interactive system.
- Validate external interrupt claim and completion through the PLIC.

### Verification

- Detect stable log markers for scheduler initialization.
- Verify that `/init` is executed.
- Test timer delivery over extended deterministic runs.
- Preserve logs and traces for kernel panics and hangs.

### Completion Criteria

- Linux initializes the scheduler and timer subsystem.
- Kernel threads run.
- The initramfs is mounted.
- Linux executes `/init`.
- Console I/O is reliable enough for user-space bring-up.
- The checkpoint runs as an automated regression.

### Deliverable

**v1.2 — Linux reaches init**

---

## Milestone 12: BusyBox User Space

### Goals

Run a minimal BusyBox-based user space and reach an interactive shell.

### User-Space Image

- Build a static BusyBox configuration suitable for RV64.
- Create an initramfs with:
  - BusyBox applets.
  - Device nodes or an appropriate early userspace device setup.
  - An `/init` script or binary.
  - A serial console.
- Keep the initial user space intentionally small and reproducible.

### User-Mode Execution

- Verify supervisor-to-user transitions.
- Verify system calls and traps from user mode.
- Support the process, memory, timer, and console behavior required by BusyBox.
- Fix architectural or platform issues discovered by user-space workloads and add focused regressions.

### System Operation

- Reach a shell prompt.
- Run basic commands such as:
  - `echo`
  - `ls`
  - `cat`
  - `uname`
  - `mount`
- Support clean shutdown or reboot through the SBI system-reset interface.
- Complete interrupt-driven UART input if it was deferred during earlier Linux bring-up.

### Verification

- Add a noninteractive boot test that waits for a shell-ready marker.
- Run a small scripted command sequence and verify its output.
- Test clean shutdown.
- Preserve the full serial log on failure.

### Completion Criteria

- Linux reaches a BusyBox user-space environment.
- User-mode processes and system calls operate correctly.
- A serial shell is available.
- Basic BusyBox commands execute successfully.
- Shutdown or reboot works.
- The full boot-to-shell flow is automated.

### Deliverable

**v1.3 — Linux and BusyBox shell**

---

# Phase 7 — Emulator Reference Model

## Milestone 13: Differential Reference Infrastructure

### Goals

Stabilize the emulator as a deterministic architectural reference model for the future RTL implementation.

### Architectural Trace

- Define a stable machine-readable commit-trace format containing:
  - Retired program counter.
  - Retired instruction.
  - Current privilege mode.
  - Integer-register writes.
  - Architectural CSR writes.
  - Committed memory writes.
  - Trap and interrupt events.
  - Optional load results when useful for debugging.
- Ensure trace output reflects committed architectural state rather than internal implementation details.
- Version the trace format so later changes are explicit.

### Deterministic Execution

- Make timer progression deterministic.
- Make external input reproducible through scripted UART input.
- Avoid host timing dependencies in automated tests.
- Ensure identical inputs produce identical architectural traces.
- Add explicit instruction or cycle limits for detecting hangs.

### Differential Testing

- Add a trace comparator suitable for emulator-to-RTL comparison.
- Report the first architectural divergence with surrounding context.
- Allow selective comparison of registers, CSRs, memory writes, traps, and privilege state.
- Provide short reproducible workloads for differential debugging.

### Regression Consolidation

- Organize the existing tests into reproducible groups:
  - ISA tests.
  - Privileged-architecture tests.
  - Sv39 tests.
  - Device and interrupt tests.
  - OpenSBI boot tests.
  - Linux early-boot tests.
  - BusyBox boot-to-shell tests.
- Provide documented commands for running individual groups and the complete regression.
- Consolidate the existing automation into continuous integration.
- Keep long-running full-system tests separate from fast per-change checks where appropriate.

### Reference-Model Validation

- Verify deterministic traces across repeated runs.
- Verify that failures preserve the required trace, serial log, and test metadata.
- Use the emulator trace format in a small mock differential test before RTL integration begins.
- Optionally add snapshots or checkpoints only if they materially improve full-system debugging speed.

### Completion Criteria

- The emulator produces stable, deterministic architectural commit traces.
- A trace comparator identifies the first architectural mismatch.
- ISA, privilege, virtual-memory, device, firmware, and Linux regressions are organized and reproducible.
- The Linux boot-to-shell test can run noninteractively.
- The emulator is ready to serve as the reference model for RTL differential testing.

### Deliverable

**v1.4 — RTL-ready emulator reference model**
