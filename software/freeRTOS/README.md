# TinyLinuxRV FreeRTOS Port

This directory contains the TinyLinuxRV port of FreeRTOS Kernel V11.3.1.
It runs in machine mode and uses the CLINT timer interrupt for scheduler ticks.

## Current Checkpoint

> ✅ **Completed** · 2026-09-27

The demo creates two equal-priority tasks. Each task prints through UART, and
the CLINT timer drives preemptive context switches between them. This verifies
timer interrupts, task scheduling, and register and stack restoration.

## Prerequisites

Initialize the FreeRTOS submodule and install the bare-metal compiler and C
library:

```shell
git submodule update --init --recursive
sudo apt install gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf
```

The build expects the Picolibc specs file at
`/usr/lib/picolibc/riscv64-unknown-elf/picolibc.specs`.

## Build and Run

From this directory:

```shell
make build-demo
make run-demo
```

`build-demo` creates `demo/build/demo.elf`. `run-demo` also builds the emulator
and runs the image. Successful execution repeatedly prints activity from both
tasks; the demo does not terminate on its own.

## Port Layout

- `demo/`: application and `FreeRTOSConfig.h`.
- `mk/freeRTOS.mk`: shared RV64IMA/LP64 build rules and kernel sources.
- `runtime/`: startup, linker script, UART console, and platform support.
