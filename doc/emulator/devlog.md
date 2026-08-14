# Emulator Development Log

This log records milestone-level progress and important design decisions for
the TinyLinuxRV emulator. Detailed implementation changes remain available in
the Git history.

## Phase 0 — Project Foundation (Completed — 07/29/2026)

### Milestone 0 — Development Environment and Repository Structure (Completed — 07/29/2026)

- 07/21/2026 — Defined the project strategy and the emulator-first roadmap for
  building a Linux-capable RISC-V CPU and SoC.
- 07/29/2026 — **Completed Milestone 0: development environment and RV64I
  smoke test.**

## Phase 1 — RV64 User-Mode Emulator (In Progress)

### Milestone 1 — Minimal RV64I Execution Engine (Completed — 08/13/2026)

- 08/01/2026 — Built the initial emulator skeleton.
  - Added CPU state, RAM, the fetch-decode-execute loop, and logging.
  - Executed the first `ADD` instruction with a small sanity program.
- 08/03/2026 — Hardened the emulator foundation and reorganized the Makefile
  around incremental compilation and dependency tracking.
- 08/08/2026 — Implemented the RV64I instruction execution engine and added a
  script-generated instruction decoder table.
- 08/11/2026 — Integrated `riscv-tests` for RV64I.
  - Added the upstream repository as a submodule.
  - Added a minimal TinyLinuxRV test environment and suite-based build system.
- 08/12/2026 — Automated test execution.
  - Added command-line modes and an instruction limit to the emulator.
  - Added per-suite manifests and a Python regression runner with host exit
    status propagation.
- 08/13/2026 — **Completed Milestone 1: Minimal RV64I Execution Engine.**
  - Added `FENCE.I` as a no-op because the emulator has no instruction cache.
  - All 53 executed RV64UI tests pass.
  - `ma_data` is skipped until misaligned load/store trap handling is
    implemented.
