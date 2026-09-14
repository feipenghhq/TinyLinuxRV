#include "clint.h"

#include <stdbool.h>
#include <stdint.h>

#include "addrmap.h"
#include "mtrap.h"
#include "uart16550.h"

#define putstr(str)    uart_putstr(UART0_BASE, str)
#define putnum(num)    uart_put_num(UART0_BASE, num)
#define INTERRUPT_MASK (UINT64_C(1) << 63)

#define read_csr(csr, value)                                           \
    do {                                                               \
        __asm__ volatile("csrr %0, " #csr : "=r"(value) : : "memory"); \
    } while (0)

#define extract_mip(mip, bit) (mip & ((UINT64_C(1) << (bit))))

int error = 0;
int msip  = false;
int mtip  = false;

int mip_msip = false;
int mip_mtip = false;

void trap_handler(trap_frame_t *tf) {
    bool     interrupt = false;
    uint64_t cause;
    uint64_t mtimecmp;
    uint64_t mip_value;

    interrupt = tf->mcause & INTERRUPT_MASK;
    if (!interrupt) {
        error++;
        return;
    }

    cause = tf->mcause & (INTERRUPT_MASK - 1);
    switch (cause) {
    case 3: // MSIP
        read_csr(mip, mip_value);
        mip_msip  = extract_mip(mip_value, 3) != 0;
        msip      = true;

        clint_clear_msip(CLINT_BASE);
        break;

    case 7: // MTIP
        read_csr(mip, mip_value);
        mip_mtip  = extract_mip(mip_value, 7) != 0;
        mtip = true;

        mtimecmp  = clint_read_mtimecmp(CLINT_BASE);
        putstr("Current mtimecmp: ");
        putnum(mtimecmp);
        putstr("\n");
        // set mtimecmp to a very high value to disable the interrupt and clear timer
        clint_set_mtimecmp(CLINT_BASE, UINT64_MAX);
        clint_set_mtime(CLINT_BASE, 0);
        break;

    default:
        error++;
        break;
    }
}

int main(void) {

    // Trigger MSIP
    clint_set_msip(CLINT_BASE);

    // Check current timer
    uint64_t mtime = clint_read_mtime(CLINT_BASE);
    putstr("Current mtime: ");
    putnum(mtime);
    putstr("\n");

    // Trigger MTIP
    clint_set_mtimecmp(CLINT_BASE, 1000);

    // Check  current timer again after clearing it
    mtime = clint_read_mtime(CLINT_BASE);
    putstr("Current mtime: ");
    putnum(mtime);
    putstr("\n");

    if (msip && mtip && mip_mtip && mip_msip)
        return error;
    else
        return 1;
}
