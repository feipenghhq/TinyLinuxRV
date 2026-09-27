#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "riscv-virt.h"

#define configMTIME_BASE_ADDRESS       (CLINT_ADDR + CLINT_MTIME)
#define configMTIMECMP_BASE_ADDRESS    (CLINT_ADDR + CLINT_MTIMECMP)
#define configISR_STACK_SIZE_WORDS     (300)
#define configUSE_PREEMPTION           1
#define configUSE_IDLE_HOOK            0
#define configUSE_TICK_HOOK            0
#define configCPU_CLOCK_HZ             ((unsigned long)25000000)
#define configTICK_RATE_HZ             ((TickType_t)1000)
#define configTICK_TYPE_WIDTH_IN_BITS  TICK_TYPE_WIDTH_64_BITS
#define configMINIMAL_STACK_SIZE       ((unsigned short)240)
#define configTOTAL_HEAP_SIZE          ((size_t)(220 * 1024))
#define configUSE_MALLOC_FAILED_HOOK   0
#define configCHECK_FOR_STACK_OVERFLOW 0
#define configMAX_PRIORITIES           (9UL)

#endif
