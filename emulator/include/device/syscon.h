#ifndef SYSCON_H
#define SYSCON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYSCON_CMD_POWER_OFF 1
#define SYSCON_CMD_REBOOT    2

#define RESET_CAUSE_POWER_ON 0x0
#define RESET_CAUSE_REBOOT   0x1
#define RESET_CAUSE_WATCHDOG 0x2

typedef struct {
    uint32_t sys_ctrl;
    uint32_t reset_cause;
} syscon_reg_t;

typedef struct {
    uint64_t     base;
    syscon_reg_t reg;
} syscon_t;

int syscon_init(syscon_t *syscon, uint64_t base);
int syscon_reset(syscon_t *syscon);
int syscon_write(syscon_t *syscon, uint64_t addr, size_t size, const void *data);
int syscon_read(syscon_t *syscon, uint64_t addr, size_t size, void *data);

#endif // SYSCON_H
