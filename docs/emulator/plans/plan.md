# TinyLinuxRV — Emulator Milestone Plan

TinyLinuxRV is a project to build a simple RV64 RISC-V system capable of
booting Linux, progressing through three implementation stages:

1. C emulator
2. RTL simulation
3. FPGA implementation

This document tracks the milestones for the **emulator stage**.

> [!NOTE]
> When a planned task changes, the original text is preserved with
> ~~strikethrough~~ and the revised decision is documented directly below it.

---

## Emulator Roadmap

| Phase   | Milestone                                 | Status     |
| ------- | ----------------------------------------- | ---------- |
| Phase 0 | Development Environment                   | ✅ Complete |
| Phase 1 | Minimal RV64I Execution Engine            | ✅ Complete |
| Phase 1 | ELF Loading & Bare-Metal C                | ✅ Complete |
| Phase 1 | RV64M / RV64A Extensions                  | ✅ Complete |
| Phase 2 | TinyLinuxRV Machine Model                 | 🟡 Next     |
| Phase 2 | Basic Platform Devices                    | Planned    |
| Phase 3 | Machine Mode, CSRs, Traps, and Interrupts | Planned    |
| Phase 3 | Supervisor and User Modes                 | Planned    |
| Phase 4 | Sv39 Address Translation                  | Planned    |
| Phase 5 | OpenSBI and SBI Validation                | Planned    |
| Phase 6 | Linux Early Boot                          | Planned    |
| Phase 6 | Scheduler and Initramfs                   | Planned    |
| Phase 6 | BusyBox User Space                        | Planned    |
| Phase 7 | Differential Reference Infrastructure     | Planned    |

## Compatibility Policy

Before work begins on a milestone that depends on an external specification or
software project, record the exact target version and build configuration in
the repository. In particular, pin the privileged architecture, SBI, OpenSBI,
Linux, and BusyBox versions instead of implicitly following their latest
releases.

---
# Phase 0 — Project Foundation

## Milestone 0: Development Environment

> ✅ **Completed** · 2026-07-29

### Goal

Create a reproducible development environment that supports both the emulator and the later RTL implementation.

### Tasks

- [x] Install and document the RV64 cross-compilation toolchain.
- [x] Use C99 and Make for the emulator.
- [x] Select SystemVerilog and Verilator for the later RTL implementation.
- [x] Add a small RV64 assembly toolchain smoke test.

### Completion Criteria

- [x] The development environment can be reproduced using documented commands.
- [x] A minimal RV64 assembly program can be compiled into ELF and binary formats.

### Deliverable

**`v0.1` — Development environment ready**

---

# Phase 1 — RV64 Unprivileged ISA Emulator

## Milestone 1: Minimal RV64I Execution Engine

> ✅ **Completed** · 2026-08-13

### Goal

Implement a basic emulator capable of running RV64I programs.

### CPU Core Components

- [x] 32 general-purpose registers and a 64-bit program counter.
- [x] Byte-addressable guest memory.
- [x] Instruction fetch, decode, and execute loop.
- [x] Explicit diagnostics for invalid instructions and invalid memory accesses.
- [x] ~~Raw binary loading at a configurable guest address.~~
  **Revised:** Load raw binaries at the fixed TinyLinuxRV reset address
  (`0x80000000`).
- [x] ~~Register dump and instruction trace support.~~
  **Revised:** Provide instruction-level debug logging and defer the register
  dump until it is needed.


### ISA Scope

- [x] Implement the complete RV64I base integer instruction set.
- [x] Implement `Zifencei`; treat `FENCE.I` as a valid no-op while the
  emulator has no instruction cache.
- [x] Reject unsupported and reserved instruction encodings explicitly.
- **Deferred:** architectural handling of `ECALL` and `EBREAK`, CSRs, privilege modes, and traps is
  deferred to later phases.

### Test Environment

- [x] Define a minimal memory layout, load address, and entry point for ISA tests.
- [x] Add the linker script and target macros required by `riscv-tests`.
- [x] Build tests as ELF files, convert them to flat binaries, and run them through the raw-binary loader.
- [x] Automate test execution, timeout handling, and pass/fail reporting.
- [x] ~~Produce failure traces automatically.~~
  **Revised:** Report failed test binaries for manual reruns with debug logging.

### Verification

- [x] Integrate the applicable RV64I tests from `riscv-tests`.
- [x] Run the complete RV64I regression after every ISA implementation change.
- **Deferred:** skip `ma_data` until misaligned load/store trap handling is implemented.

### Completion Criteria

- [x] The complete RV64I base integer instruction set is implemented.
- [x] All applicable RV64I tests from `riscv-tests` pass.
- [x] Invalid instructions and invalid memory accesses produce clear diagnostics.
- [x] Failed tests provide useful diagnostics and execution traces.
- [x] ~~Raw binary programs execute from a configurable address.~~
  **Revised:** Raw binary programs execute from the fixed TinyLinuxRV reset
    address.

### Deliverable

**v0.2 — Tested RV64I emulator**

---

## Milestone 2: ELF Loading and Bare-Metal C Programs

> ✅ **Completed** · 2026-08-16

### Goal

Turn the verified RV64I interpreter into a practical bare-metal execution environment capable of loading standard RISC-V ELF executables and running freestanding C programs.

### Bare-Metal Platform

- [x] Define the RAM layout, program entry point, stack convention, and program exit mechanism.
- **Deferred:** Keep the initial platform simple; implement the complete Linux
  and MMIO machine model in Phase 2.

### ELF Loader

- [x] Load valid RISC-V ELF64 executables using `PT_LOAD` segments and start execution at `e_entry`.
- [x] Support initialized and zero-filled segments.
- [x] Validate ELF metadata and reject malformed or out-of-range images with clear diagnostics.

### Bare-Metal Build Environment

- [x] Add a linker script and minimal startup code for freestanding RV64I programs.
- [x] Initialize the stack, clear `.bss`, call `main`, and report its return value.
- [x] Document the required compiler and linker flags and provide example assembly and C builds.

### Emulator Interface

- [x] ~~Add command-line support for ELF and raw binaries, memory configuration, tracing, dumps, and instruction limits.~~
  **Revised:** Support ELF, raw binaries, automatic format detection, and
    instruction limits. Memory configuration, tracing, and dumps are deferred.
- [x] ~~Return distinct host exit codes for guest results and emulator errors.~~
  **Revised:** Use normal host success and failure statuses. Logs show whether
    the failure came from the guest or the emulator.

### Bare-Metal C Verification

- [x] Add freestanding C tests covering global data, `.bss`, stack usage, function calls, control flow, arrays, pointers, and different memory access widths.

### Architectural Verification

- [x] ~~Integrate the applicable RV64I architectural tests using ACT4.~~
  **Revised:** ACT4 is deferred until the emulator supports more of the ISA.
- [x] ~~Add the required TinyLinuxRV target configuration and automated test runner.~~
  **Revised:** Use `riscv-tests` and the bare-metal C test for this milestone.
- [x] ~~Run both ACT4 and `riscv-tests` regressions in CI.~~
  **Revised:** CI is deferred to a later milestone.

### Completion Criteria

- [x] The emulator loads valid RISC-V ELF64 executables using `PT_LOAD` segments.
- [x] A freestanding RV64I C program runs successfully.
- [x] Global data, `.bss`, stack usage, function calls, arrays, pointers, branches, and loops work correctly.
- [x] ~~The applicable RV64I architectural tests pass.~~
  **Revised:** All 53 enabled RV64UI tests and the bare-metal C test pass.
- [x] The complete Milestone 1 `riscv-tests` regression continues to pass.
- [x] Invalid ELF metadata, entry points, and load addresses produce useful
  errors. More malformed-ELF tests are deferred.

### Deliverable

**v0.3 — Verified RV64I bare-metal emulator**

---

## Milestone 3: RV64IMA Extension Support

> ✅ **Completed** · 2026-08-17

### Goal

Extend the verified RV64I emulator with the multiplication, division, and atomic instructions needed by compiled system software and the later Linux software stack.

### ISA Scope

- [x] Implement the complete RV64M extension.
- [x] Implement the complete RV64A extension with deterministic single-hart reservation behavior.
- [x] Define the initial reservation policy: the latest LR reserves exactly
  the bytes it accesses, and every SC clears the reservation whether it
  succeeds or fails.

### Build Environment

- [x] Update the bare-metal toolchain configuration for RV64IMA.
- ~~Add compiled C and assembly examples exercising multiplication, division, and atomic operations.~~
  **Revised:** Use the upstream RV64M/RV64A suites and focused sanity tests for
  this milestone. Dedicated compiled examples are deferred until a system
  software workload requires them.

### Verification

- [x] Run the applicable RV64M and RV64A tests from `riscv-tests`.
- ~~Run the applicable RV64M and RV64A architectural tests.~~
  **Revised:** Defer ACT4 integration until the privileged architecture is
  available; use `riscv-tests` for this milestone.
- ~~Add focused tests for multiplication high halves, division corner cases, word-result sign extension, LR/SC success and failure, reservation invalidation, and word/doubleword AMOs.~~
  **Revised:** Add instruction-coverage sanity tests plus focused AMO register
  aliasing and LR/SC tests. Rely on the upstream RV64M/RV64A suites for the
  remaining arithmetic and atomic corner cases at this milestone.
- [x] Run the complete RV64I regression after adding M and A support.

### Completion Criteria

- [x] All applicable tests pass: 53 RV64UI, 13 RV64UM, and 19 RV64UA
  tests. `ma_data` remains skipped until misaligned-access traps are available.
- ~~Compiled bare-metal programs can use multiplication, division, and atomic operations.~~
  **Revised:** The bare-metal build uses `-march=rv64ima`; dedicated compiled
  examples are deferred as described above.
- [x] The initial single-hart reservation policy is documented in this
  milestone and verified by the RV64A regression and focused sanity tests.
- ~~The RV64IMA regression runs automatically in CI.~~
  **Revised:** The complete RV64IMA regression runs locally with
  `make -C emulator regression`. CI integration is deferred to Milestone 13.

### Deliverable

**v0.4 — Verified RV64IMA emulator**

---

# Phase 2 — Emulator Platform and Basic Devices

## Milestone 4: TinyLinuxRV Machine Model

### Goals

Define the machine-level platform required by firmware, operating-system bring-up, and the later RTL implementation.

### Platform Definition

- Finalize and implement the documented TinyLinuxRV v0.1 reset address and
  physical memory map for ROM, RAM, and MMIO devices.
- Keep the platform definition shared by the emulator, future RTL, firmware, and device tree.
- Reserve address ranges for later interrupt controllers and additional peripherals.
- Add a configurable DRAM size while retaining `0x80000000` as the DRAM base.
- Select a default DRAM size large enough for OpenSBI, Linux, a device tree,
  and an initramfs.

### Machine Model

- Add MMIO address dispatch.
- Add boot ROM loading.
- Add reset and shutdown behavior.
- Reject unmapped and invalid device accesses with clear diagnostics.
- Keep platform constants in a shared definition suitable for reuse by software and RTL.

### Boot and Image Loading

- Define non-overlapping load regions for OpenSBI, Linux, the device tree, and
  the initramfs.
- Extend the emulator interface so a system boot can load multiple images at
  documented guest addresses.
- Preserve the existing direct ELF and raw-binary modes for bare-metal tests.

### Verification

- Add tests for ROM, RAM, and MMIO address decoding.
- Verify reset and shutdown behavior.
- Verify invalid and overlapping memory mappings are rejected.
- Test configurable DRAM sizes and multi-image overlap detection.
- Run the complete RV64IMA regression after introducing the machine model.

### Completion Criteria

- The TinyLinuxRV reset address and memory map are documented.
- Boot ROM, RAM, and MMIO regions behave correctly.
- DRAM size is configurable and large system images can be loaded without
  overlapping one another.
- Invalid physical accesses produce clear diagnostics.
- Platform definitions can be reused by later firmware and RTL work.
- Existing ISA regressions continue to pass.

### Deliverable

**v0.5 — TinyLinuxRV machine model**

---

## Milestone 5: Basic Platform Devices

### Goals

Add the devices required for observable bare-metal software and early firmware
development without depending on privileged CPU interrupt handling.

### UART

- Implement a polling-capable memory-mapped UART.
- Support guest transmit and receive registers.
- Connect UART input and output to the host terminal.
- Expose status information required by polling software.
- Expose a device-level interrupt-pending signal for later privileged-architecture integration.

### ACLINT

- Implement deterministic ACLINT-style machine timer and machine
  software-interrupt register blocks.
- Provide a readable machine-time counter and writable compare register.
- Provide a writable machine software-interrupt register for the single hart.
- Define deterministic timer progression suitable for testing.
- Expose device-level `MTIP` and `MSIP` pending conditions.
- Defer architectural timer- and software-interrupt delivery to Phase 3.

### PLIC

- Implement a minimal PLIC-compatible MMIO model for a single hart.
- Provide machine and supervisor contexts needed by the selected firmware and
  Linux configuration.
- Route the UART interrupt source into the PLIC.
- Implement interrupt enable, priority, threshold, claim, and completion
  behavior for the supported sources.
- Expose device-level external-interrupt pending conditions to be connected to
  the CPU in Phase 3.

### Verification

- Add MMIO register tests for UART and timer behavior.
- Run bare-metal programs that use polling UART input and output.
- Verify that software can read the timer counter and program the compare register.
- Verify software-interrupt register behavior.
- Verify UART, timer, software, and external-interrupt pending conditions at
  the device level.
- Test PLIC enable, priority, threshold, claim, and completion behavior.
- Run the complete RV64IMA regression after adding the devices.

### Completion Criteria

- Bare-metal software can communicate through the UART using polling.
- UART input and output work through the host terminal.
- Software can read the timer and program its compare register.
- Software can assert and clear the machine software-interrupt source.
- The PLIC handles the supported external interrupt sources and contexts.
- Timer, software, and external-interrupt pending conditions are generated correctly.
- No privileged CSR or trap handling is required to test this milestone.
- Existing ISA and bare-metal regressions continue to pass.

### Deliverable

**v0.6 — Basic platform devices**

---

# Phase 3 — Privileged Architecture

## Milestone 6: Machine Mode, CSRs, Traps, and Interrupts

### Goals

Implement the machine-mode privileged architecture required for traps, interrupts, firmware, and later supervisor-mode execution.

### ISA and CSR Support

- Select and document the target RISC-V privileged-architecture version.
- Implement the `Zicsr` extension.
- Preserve and validate the existing `FENCE.I` no-op behavior.
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
  - `mvendorid`
  - `marchid`
  - `mimpid`
  - `mhartid`
- Implement the counter CSRs required by the selected OpenSBI and Linux
  versions, including a deterministic `time` CSR backed by the Phase 2 timer.
- Define and document the supported `cycle`, `time`, and `instret` behavior and
  their access controls.
- Enforce CSR privilege levels, read-only behavior, and WARL constraints where required.

### Exceptions and Trap Handling

- Implement synchronous exceptions for:
  - Illegal instructions.
  - Instruction-address misalignment.
  - Load- and store-address misalignment.
  - Instruction, load, and store access faults.
  - Environment calls.
  - Breakpoints.
- Implement direct `mtvec` behavior.
- Defer vectored `mtvec` mode until required by software or verification.
- Save trap state in `mepc`, `mcause`, and `mtval`.
- Update interrupt-enable state in `mstatus` during trap entry.
- Implement `MRET` and restore the previous privilege and interrupt state.

### Machine Interrupts

- Connect the Phase 2 `MTIP`, `MSIP`, and PLIC pending conditions to the CPU.
- Implement `mip`, `mie`, and `mstatus.MIE` behavior.
- Evaluate interrupts at instruction boundaries and apply architectural priority rules.

### Verification

- Add focused tests for CSR access permissions and side effects.
- Test each synchronous exception and verify trap state.
- Test direct trap entry.
- Test `MRET` state restoration.
- Add end-to-end timer, software, and external interrupt tests.
- Run the applicable privileged-architecture and CSR tests.
- Continue running the complete RV64IMA regression.

### Completion Criteria

- Machine-mode CSR instructions and required CSRs behave correctly.
- Exceptions enter machine-mode trap handlers with correct cause and state.
- `MRET` restores execution correctly.
- Timer, software, and UART external interrupts are delivered end to end.
- PLIC interrupts are delivered correctly after device-level claim and
  completion behavior was verified in Phase 2.
- Existing unprivileged ISA and bare-metal regressions continue to pass.

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
- Initially implement `Svade`: raise a page fault when the required accessed
  or dirty bit is clear.
- Advertise the selected A/D-bit behavior consistently to OpenSBI and Linux.
- Defer `Svadu` hardware updating of A/D bits until it is required.
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
- Test `SUM`, `MXR`, and `Svade` behavior.
- Test `SFENCE.VMA` behavior.
- Run applicable Sv39 architectural tests.

### Completion Criteria

- Sv39 address translation works for instruction fetches, loads, and stores.
- All supported page sizes behave correctly.
- Permission checks and page faults match the privileged specification.
- `Svade` behavior is implemented, tested, and reported consistently to
  firmware and operating-system software.
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

- Select and record the exact SBI specification and OpenSBI release used for
  this milestone.
- Prefer the OpenSBI generic platform with a device tree.
- Add a TinyLinuxRV-specific OpenSBI platform only if the generic platform cannot support the required devices.
- Match device-tree compatible strings and properties to the drivers provided
  by the selected OpenSBI release.
- Provide a device tree describing:
  - CPU and ISA capabilities.
  - Physical memory.
  - UART.
  - Timer and software-interrupt device.
  - External interrupt controller.
  - Reset or shutdown device.
- Define `/chosen/stdout-path` for the platform UART.
- Keep the device tree consistent with the emulator memory map.

### Firmware Loading

- Define the OpenSBI firmware load address and entry point.
- Use the Phase 2 multi-image interface to load the firmware, device tree, and
  supervisor payload into guest memory.
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

- Select and record an exact Linux release, kernel configuration, compiler
  version, and TinyLinuxRV ISA string for reproducible builds.
- Build a Linux kernel for the supported TinyLinuxRV ISA profile.
- Load the RV64 kernel `Image` at a 2 MiB-aligned physical address as required
  by the RISC-V Linux boot protocol.
- Enter the kernel in supervisor mode with:
  - `a0` containing the hart ID.
  - `a1` containing the physical address of the device tree.
  - Virtual memory initially disabled.
- Keep the kernel, OpenSBI, device tree, and initramfs load addresses documented and non-overlapping.
- Configure enough DRAM for the selected kernel and initramfs images.

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

- Select and record an exact BusyBox release and build configuration.
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
