/**
 * This file test the syscon device.
 *
 */
#include "syscon.h"

#include <stdint.h>

#include "addrmap.h"

#define SYS_CTRL_OFFSET    0x00
#define RESET_CAUSE_OFFSET 0x04

#define reg_sys_ctrl    ((volatile uint32_t *)(Syscon_BASE + SYS_CTRL_OFFSET))
#define reg_reset_cause ((volatile uint32_t *)(Syscon_BASE + RESET_CAUSE_OFFSET))

int main(void) {

    // test reboot, only test once
    if (*reg_reset_cause == RESET_CAUSE_POWER_ON)
        *reg_sys_ctrl = SYSCON_CMD_REBOOT;

    // test poweroff
    else if (*reg_reset_cause == RESET_CAUSE_REBOOT)
        *reg_sys_ctrl = SYSCON_CMD_POWER_OFF;

    // should not enter infinite loop as we requested poweroff
    while (1)
        ;
    return 0;
}
