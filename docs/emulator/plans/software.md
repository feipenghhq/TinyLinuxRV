# TinyLinuxRV Software Checkpoints

This document lists real software and workloads to run during TinyLinuxRV
development.

These checkpoints complement the emulator milestone plan. They verify that
CPU and system features work together, but do not block emulator milestones
unless they are explicitly added to the plan. Linux remains the main goal.

## 1. Bare-metal Benchmark

After Milestone 5 — Basic Platform Devices.

Run after UART is available so the guest can print the result through UART.

### CoreMark

Primary bare-metal benchmark.

- Run CoreMark successfully.
- Print result through UART.
- Keep the build/run flow so it can be reused by RTL and FPGA later.

### Dhrystone

Run as another simple C workload.

The main purpose is functional verification. The performance number is not very important.

### Embench (Optional)

Run some Embench workloads after CoreMark/Dhrystone.

This can provide more real C workloads and may catch bugs not covered by a single benchmark.

---

## 2. FreeRTOS

After Milestone 6 — Machine Mode, CSRs, Traps, and Interrupts.

FreeRTOS is a required checkpoint after M-mode interrupt and timer support.

Main goal:

- Start FreeRTOS scheduler.
- Run at least 2 tasks.
- Use timer interrupt for task switching.
- Verify context switch and register/stack restore.
- Print task activity through UART.

Example:

```text
Task A -> print A
Task B -> print B
Timer interrupt -> context switch
```

Stop after the basic scheduler/context switch works. No need to study the full FreeRTOS API.

---

## 3. xv6

After Milestone 8 — Sv39 Address Translation.

xv6 is a required checkpoint before Linux.

Use a pinned xv6-riscv version and make only small changes needed for TinyLinuxRV.

### Checkpoint A — S-mode

Boot xv6 into supervisor mode and print through UART.

Mainly verify:

- M -> S transition
- supervisor CSR
- trap delegation

### Checkpoint B — Sv39

Enable the xv6 kernel page table and continue running.

Mainly verify:

- `satp`
- Sv39 page walk
- PTE permission
- page fault
- `SFENCE.VMA`

### Checkpoint C — User mode

Run a small user program:

```text
S-mode kernel
    ↓
U-mode
    ↓
ECALL
    ↓
S-mode trap
    ↓
return to U-mode
```

After this works, the required xv6 checkpoint is complete.

### xv6 Shell (Optional)

Filesystem, VirtIO block device and the full xv6 shell are optional.

Only add them if the same support is also useful for Linux. Do not add hardware only for xv6.

---

## 4. OpenSBI Test Program

During or after Milestone 9 — OpenSBI and SBI Validation.

Before Linux, run a small S-mode program through OpenSBI.

Test:

- OpenSBI -> S-mode transition
- UART/SBI output
- SBI timer
- timer interrupt
- SBI shutdown/reset

This gives a simple test between xv6 and Linux.

---

## 5. Linux + BusyBox

Milestones 10–12.

Main project goal.

Progress:

```text
OpenSBI
  ↓
Linux early boot
  ↓
MMU enabled
  ↓
scheduler
  ↓
initramfs
  ↓
/init
  ↓
BusyBox shell
```

After BusyBox works, run a few basic commands:

```sh
echo TinyLinuxRV
uname -a
ls /
cat /proc/cpuinfo
```

---

## 6. More Linux Workloads

After Milestone 12 — BusyBox User Space.

After BusyBox is stable, try some real programs.

### Simple tools

- shell scripts
- `dd`
- `sha256sum`
- `gzip`
- `tar`

### Lua

A small interpreter and a good next step after BusyBox.

```lua
print("Hello TinyLinuxRV")
for i = 1, 10 do
    print(i * i)
end
```

### SQLite

Run SQLite after filesystem support is stable.

Example:

```sql
CREATE TABLE cpu(id INTEGER, name TEXT);
INSERT INTO cpu VALUES(1, 'TinyLinuxRV');
SELECT * FROM cpu;
```

---

## 7. Fun Workloads

After Milestone 12. These are optional future demos.

These are not required milestones.

### Bad Apple

Possible future demo:

```text
frame data -> framebuffer -> VGA
```

Can be used to test display and sustained memory access.

### DOOM

Possible final full-system demo:

```text
TinyLinuxRV
    ↓
Linux
    ↓
framebuffer + input
    ↓
DOOM
```

Other possible workloads:

- Tiny BASIC
- Forth
- zlib
- software SHA/AES
- simple framebuffer demos

---

## Checkpoint Order

```text
riscv-tests / bare-metal tests
        ↓
UART
        ↓
CoreMark + Dhrystone
        ↓
Embench (optional)
        ↓
M-mode + timer interrupt
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
small S-mode program
        ↓
Linux
        ↓
BusyBox
        ↓
Lua / gzip / tar / SQLite
        ↓
Bad Apple / DOOM
```

Try to reuse the same software on emulator, RTL and FPGA when possible.
