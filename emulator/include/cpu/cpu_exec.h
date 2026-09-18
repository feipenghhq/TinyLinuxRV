#ifndef CPU_EXEC_H
#define CPU_EXEC_H

#include <stdbool.h>

#include "bus/bus.h"
#include "cpu/cpu.h"
#include "device/device.h"

typedef enum CPU_EXEC_STATUS { FINISH, MEM_ERROR, CPU_ERROR, DEVICE_ERROR, POWEROFF, TIMEOUT } CPU_EXEC_STATUS_t;

CPU_EXEC_STATUS_t cpu_exec(cpu_t *cpu, bus_t *bus, dev_list_t *devices, bool trace, long max_instruction);

#endif
