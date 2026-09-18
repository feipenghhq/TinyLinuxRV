#include "cpu/cpu_exec.h"

#include <stdint.h>

#include "bus/bus.h"
#include "cpu/cpu.h"
#include "device/device.h"
#include "device/syscon.h"
#include "utils/iringbuf.h"
#include "utils/log.h"

CPU_EXEC_STATUS_t cpu_exec(cpu_t *cpu, bus_t *bus, dev_list_t *devices, bool trace, long max_instruction) {
    CPU_EXEC_STATUS_t exec_status = FINISH;

    bool     inst_valid = true;
    long     inst_count = 0;
    uint32_t inst       = 0;

    // main instruction execution loop
    while (!cpu->halted) {

        // read instruction from memory
        inst_valid = true;
        if (bus_read(bus, cpu->pc, 4, &inst) != 0) {
            inst_valid = false;
            LOG_ERROR("Memory read failed. Unable to fetch instruction");
        }

        if (trace) {
            iringbuf_write(cpu->pc, inst);
        }

        // execute the instruction
        cpu_step(cpu, bus, inst, inst_valid);

        // check poweroff/reboot
        if (syscon_poweroff_requested(devices->syscon.device)) {
            LOG_INFO("Poweroff requested");
            exec_status = POWEROFF;
            break;
        }

        if (syscon_reboot_requested(devices->syscon.device)) {
            LOG_INFO("Reboot requested");
            cpu_init(cpu);
            device_reset(devices);
        }

        // check instruction limit
        inst_count++;
        if (max_instruction > 0 && max_instruction <= inst_count) {
            LOG_ERROR("Reach maximum instruction count but the program has not finished yet");
            exec_status = TIMEOUT;
            break;
        }

        // Device update
        if (device_update(devices) != 0) {
            LOG_ERROR("Device poll input failed. Exiting.");
            exec_status = DEVICE_ERROR;
            break;
        }
        device_irq_level(devices, &cpu->msip, &cpu->mtip, &cpu->meip);
    }

    return exec_status;
}
