# Syscon

This document describes the syscon device.

## Introduction

The syscon device handles shutdown and reboot commands from guest software.

The syscon currently supports the following commands:

- shutdown
- reboot

The behavior is slightly different between the emulator and the hardware.

### Emulator

- **shutdown**: A shutdown command terminates CPU execution immediately.

- **reboot**: A reboot command resets the CPU and non-sticky device state. DRAM contents and sticky register bits are preserved.

### Hardware

TBD

## Register Space

The syscon register layout is intended to be identical in the emulator and RTL.

| Offset | Register      | Access | Definition                                         |
| -----: | ------------- | :----: | -------------------------------------------------- |
| `0x00` | `SYS_CTRL`    |   WO   | Writing this register triggers a syscon operation. |
| `0x04` | `RESET_CAUSE` |   RO   | Records the source of the reset or reboot.         |

### SYS_CTRL

- 1: power-off (shutdown)
- 2: reboot (reset)

### RESET_CAUSE

- 0: system power on
- 1: reboot
- 2: watchdog (planned)
