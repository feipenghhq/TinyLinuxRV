# TinyLinuxRV Development Roadmap

This document shows the overall TinyLinuxRV development flow.

Detailed emulator tasks are in the [emulator milestone plan](plan.md). Additional
software checkpoints are listed in [software.md](software.md).

## Overall Flow

```text
RV64 ISA
   ↓
Bare-metal
   ↓
Machine / Devices
   ↓
Privilege
   ↓
Virtual Memory
   ↓
Small OS
   ↓
OpenSBI
   ↓
Linux
   ↓
Reference Emulator
   ↓
RTL CPU / SoC
   ↓
FPGA
```

## 1. ISA and Bare-metal

```text
RV64I
  ↓
ELF loader
  ↓
bare-metal C
  ↓
RV64M
  ↓
RV64A
```

Main topics:

- RISC-V ISA
- ELF
- linker script
- `crt0`
- stack / ABI
- `.data` / `.bss`

Verification:

- `riscv-tests`
- architecture tests
- focused tests
- bare-metal C programs

---

## 2. Machine and Devices

Turn the CPU emulator into a machine.

```text
CPU
 │
 ├── RAM
 ├── MMIO
 ├── UART
 ├── Timer
 ├── Interrupt Controller
 └── Reset/Shutdown
```

Define one stable memory map and reuse it later for RTL, FPGA, OpenSBI and Linux.

After UART works:

```text
CoreMark
Dhrystone
Embench (optional)
```

---

## 3. M-mode and Interrupt

Add:

```text
CSR
 ↓
Exception / Trap
 ↓
MRET
 ↓
Timer Interrupt
 ↓
Software Interrupt
 ↓
External Interrupt
```

After timer interrupt works, run **FreeRTOS**.

```text
Timer
  ↓
FreeRTOS Scheduler
  ↓
Task A <-> Task B
```

This checks interrupt + context switch together.

---

## 4. S-mode / U-mode

Add:

```text
M-mode
   ↓ delegation
S-mode
   ↓ SRET
U-mode
   ↑ ECALL
```

Main topics:

- supervisor CSR
- exception/interrupt delegation
- `SRET`
- user mode
- privilege checks

---

## 5. Sv39

Add virtual memory.

```text
Virtual Address
      ↓
Sv39 Page Table Walk
      ↓
Permission Check
      ↓
Physical Address
```

Support:

- 4 KiB / 2 MiB / 1 GiB pages
- page faults
- user/supervisor permission
- `SUM` / `MXR`
- A/D bit behavior
- `SFENCE.VMA`

A TLB is not required initially.

---

## 6. xv6

Use xv6 before Linux as a simpler OS integration test.

```text
xv6 S-mode kernel
        ↓
enable Sv39
        ↓
U-mode program
        ↓
ECALL
        ↓
S-mode
        ↓
return to U-mode
```

Required:

1. Reach S-mode kernel.
2. Enable Sv39 and continue.
3. Run user code and handle a syscall.

Full filesystem/xv6 shell is optional.

---

## 7. OpenSBI

Target software stack:

```text
Linux / S-mode Program
          │
         SBI
          │
       OpenSBI
        M-mode
          │
      TinyLinuxRV
```

First run a small S-mode test program:

```text
OpenSBI
  ↓
print hello
  ↓
set timer
  ↓
receive timer
  ↓
shutdown
```

Then move to Linux.

---

## 8. Linux

### Early Boot

```text
OpenSBI
   ↓
Linux
   ↓
early console
   ↓
device tree
   ↓
memory
   ↓
Sv39 enabled
```

### Init

```text
timer
  ↓
scheduler
  ↓
kernel threads
  ↓
initramfs
  ↓
/init
```

### BusyBox

```text
Linux
  ↓
BusyBox
  ↓
shell
```

At this point TinyLinuxRV can run a real Linux userspace.

After that, try:

```text
shell scripts
  ↓
gzip / tar / sha256
  ↓
Lua
  ↓
SQLite
```

---

## 9. Reference Emulator

After Linux + BusyBox works, make the emulator a reference model for RTL.

```text
Program
   │
   ├── Emulator -> architectural trace
   │
   └── RTL      -> architectural trace
                       │
                    compare
```

Need:

- deterministic timer/input
- commit trace
- register/CSR/memory events
- trap/interrupt events
- trace comparator

---

## 10. RTL CPU and SoC

Implement the same behavior in RTL.

```text
RV64 core
   ↓
pipeline
   ↓
CSR / Trap
   ↓
Interrupt
   ↓
S/U mode
   ↓
Sv39
   ↓
SoC
```

Reuse the software checkpoints:

```text
riscv-tests
CoreMark
FreeRTOS
xv6
OpenSBI
Linux
```

The SoC should be reusable so future CPU cores can use the same platform.

Possible future cores:

- 5-stage in-order
- superscalar
- out-of-order
- multi-core

---

## 11. FPGA

```text
RTL CPU / SoC
     ↓
On-chip RAM
     ↓
UART
     ↓
Timer / Interrupt
     ↓
SDRAM
     ↓
OpenSBI
     ↓
Linux
     ↓
BusyBox
```

Later add:

- VGA / framebuffer
- keyboard / PS2
- storage
- audio

Fun final demos:

```text
Bad Apple
DOOM
```

---

## Full Roadmap

```text
RV64I
  ↓
ELF + bare-metal C
  ↓
RV64M/A
  ↓
Machine + UART + Timer
  ↓
CoreMark / Dhrystone
  ↓
M-mode + Interrupt
  ↓
FreeRTOS
  ↓
S/U mode
  ↓
Sv39
  ↓
xv6
  ↓
OpenSBI
  ↓
Linux
  ↓
BusyBox
  ↓
Real applications
  ↓
Reference Emulator
  ↓
RTL CPU / SoC
  ↓
FPGA
  ↓
Linux on FPGA
  ↓
Bad Apple / DOOM
```
