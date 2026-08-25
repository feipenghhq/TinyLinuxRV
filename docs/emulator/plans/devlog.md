# Emulator Development Log

This log records milestone-level progress and important design decisions for
the TinyLinuxRV emulator. Detailed implementation changes remain available in
the Git history.

---

## Phase 0 — Project Foundation

> ✅ **Completed** · 2026-07-29

### Milestone 0 — Development Environment

> ✅ **Completed** · 2026-07-29

* **2026-07-21** — Defined the emulator development strategy for building
  a Linux-capable RISC-V CPU and SoC.
* **2026-07-29** — Completed the development environment and RV64I toolchain
  smoke test.

---

## Phase 1 — RV64 Unprivileged ISA Emulator

> ✅ **Completed** · 2026-08-17

### Milestone 1 — Minimal RV64I Execution Engine

> ✅ **Completed** · 2026-08-13

* **2026-08-01** — Built the initial emulator skeleton with CPU state, RAM,
  instruction execution, and debug logging.
* **2026-08-08** — Implemented the RV64I execution engine and added a
  script-generated instruction-decoder table to keep instruction matching
  maintainable.
* **2026-08-11** — Integrated `riscv-tests` and added the TinyLinuxRV RV64I
  regression environment.
* **2026-08-12** — Automated regression testing and added emulator controls for
  scripted test execution.
* **2026-08-13** — Completed the Minimal RV64I Execution Engine milestone.

  * All 53 enabled RV64UI tests pass.
  * `FENCE.I` is implemented as a no-op because the emulator has no instruction
    cache.
  * `ma_data` remains skipped until misaligned load/store trap handling is
    implemented.

### Milestone 2 — ELF Loading and Bare-Metal C Programs

> ✅ **Completed** · 2026-08-16

* **2026-08-15** — Added RISC-V ELF64 loading with `PT_LOAD` segment support,
  BSS initialization, image validation, and automatic ELF/raw-binary format
  selection.

  * Switched the RV64UI regression to ELF images; all 53 enabled tests continue
    to pass.

* **2026-08-16** — Added the bare-metal C platform and runtime environment,
  including the memory map, linker script, startup code, stack setup, and C
  entry path.

  * Added Fibonacci as the first C example.

* **2026-08-16** — Added a broader bare-metal C test covering data sections,
  stack usage, function calls, control flow, pointers, and memory access sizes.

  * Raw-binary testing with poisoned RAM exposed and fixed a `.bss`
    initialization bug.

* **2026-08-16** — Completed the ELF Loading and Bare-Metal C Programs
  milestone.

  * ELF and raw-binary bare-metal programs run successfully.
  * The sanity test and all 53 enabled RV64UI tests continue to pass.
  * ACT4, continuous integration, configurable RAM, instruction tracing, and
    register or memory dumps remain deferred.

### Milestone 3 — RV64IMA Extension Support

> ✅ **Completed** · 2026-08-17

* **2026-08-16** — Implemented the RV64M multiplication and division
  instructions and enabled the RV64UM `riscv-tests` suite.

* **2026-08-17** — Implemented RV64A with deterministic single-hart LR/SC
  reservation behavior and word/doubleword AMOs.

  * Added focused sanity tests for AMO register aliasing and LR/SC behavior.
  * Updated the bare-metal build environment to use RV64IMA.

* **2026-08-17** — Completed the RV64IMA Extension Support milestone.

  * All 53 RV64UI, 13 RV64UM, and 19 RV64UA tests pass.
  * The complete emulator regression passes.
  * ACT4, continuous integration, and dedicated compiled RV64M/RV64A examples
    remain deferred.
