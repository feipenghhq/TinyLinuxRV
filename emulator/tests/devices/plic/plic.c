
#include <stdbool.h>
#include <stdint.h>

#include "addrmap.h"
#include "mmio.h"
#include "mtrap.h"
#include "uart16550.h"

#define putstr(str)         uart_putstr(UART0_BASE, str)
#define putnum(num)         uart_put_num(UART0_BASE, num)
#define putchar(ch)         uart_putchar(UART0_BASE, ch)
#define getchar()           uart_getchar(UART0_BASE)
#define readline(buf, size) uart_readline(UART0_BASE, buf, size)
#define INTERRUPT_MASK      (UINT64_C(1) << 63)

#define write_csr(csr, value)                                            \
    do {                                                                 \
        __asm__ volatile("csrw " #csr ", %0" : : "r"(value) : "memory"); \
    } while (0)

#define read_csr(csr, value)                                           \
    do {                                                               \
        __asm__ volatile("csrr %0, " #csr : "=r"(value) : : "memory"); \
    } while (0)

#define extract_mip(mip, bit) (mip & ((UINT64_C(1) << (bit))))

int  error     = 0;
bool mip_meip  = false;
bool processed = false;
char buf[100];

int main(void) {
    // enable uart RX buffer interrupt
    mmio_write8(UART0_BASE + 1, 1);
    // enable PLIC context 0 which is meip
    mmio_write32(PLIC_BASE + 0x002000, (1U << 10)); // enable the interrupt on context 0
    // Set UART0 priority to 1
    mmio_write32(PLIC_BASE + 0x4 * 10, 1);

    // wait for the interrupt to complete
    while (!processed)
        ;

    // check what user entered
    putstr("User entered:\n");
    putstr(buf);
    putstr("\n");

    if (!mip_meip)
        return 1;
    else
        return error;
}

void trap_handler(trap_frame_t *tf) {
    bool interrupt = false;

    uint64_t cause;
    uint64_t mip_value;
    uint32_t pending;
    uint32_t id;

    // make sure we get an interrupt
    interrupt = tf->mcause & INTERRUPT_MASK;
    if (!interrupt) {
        error = 1;
        return;
    }

    cause = tf->mcause & (INTERRUPT_MASK - 1);
    switch (cause) {
    case 11: // MEIP
        read_csr(mip, mip_value);
        mip_meip = extract_mip(mip_value, 11) != 0;
        // Check PLIC register to make sure we have the pending bit set
        pending = mmio_read32(PLIC_BASE + 0x1000);
        if (pending != (1U << 10)) {
            error = 3;
            return;
        }
        // Claim the interrupt for context 0
        id = mmio_read32(PLIC_BASE + 0x200004);
        if (id != 10) {
            error = 4;
            return;
        }
        // pending should be zero now
        pending = mmio_read32(PLIC_BASE + 0x1000);
        if (pending != 0) {
            error = 5;
            return;
        }
        // Resolve the interrupt by reading from UART
        readline(buf, 32);
        // Complete the interrupt for context 0
        mmio_write32(PLIC_BASE + 0x200004, 10);
        processed = true;
        break;

    default:
        error = 2;
        break;
    }
}
