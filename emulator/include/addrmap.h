/**
 * This file defines the Physical Address Map for the TinyLinuxRV emulator
 */

#ifndef ADDR_MAP_H
#define ADDR_MAP_H

// BootROM
#define BootROM_BASE 0x00001000ULL
#define BootROM_SIZE 0x0000f000ULL
#define BootROM_END  (BootROM_BASE + BootROM_SIZE)

// Syscon
#define Syscon_BASE 0x00100000ULL
#define Syscon_SIZE 0x00001000ULL
#define Syscon_END  (Syscon_BASE + Syscon_SIZE)

// ACLINT
#define ACLINT_BASE 0x02000000ULL
#define ACLINT_SIZE 0x00010000ULL
#define ACLINT_END  (ACLINT_BASE + ACLINT_SIZE)

// PLIC
#define PLIC_BASE 0x0c000000ULL
#define PLIC_SIZE 0x04000000ULL
#define PLIC_END  (PLIC_BASE + PLIC_SIZE)

// UART0
#define UART0_BASE 0x10000000ULL
#define UART0_SIZE 0x00001000ULL
#define UART0_END  (UART0_BASE + UART0_SIZE)

// VirtIO MMIO
#define VirtIO_BASE 0x10001000ULL
#define VirtIO_SIZE 0x00008000ULL
#define VirtIO_END  (VirtIO_BASE + VirtIO_SIZE)

// DRAM
#define DRAM_BASE 0x80000000ULL
#define DRAM_SIZE 0x08000000ULL
#define DRAM_END  (DRAM_BASE + DRAM_SIZE)

#endif
