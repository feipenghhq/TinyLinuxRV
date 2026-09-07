#include "clint.h"

#include <stdbool.h>
#include <stdint.h>

#include "addrmap.h"
#include "uart16550.h"

int main(void) {

    clint_set_msip(CLINT_BASE);
    clint_clear_msip(CLINT_BASE);

    uint64_t mtime = clint_read_mtime(CLINT_BASE);
    uart_putstr(UART0_BASE, "Current mtime: ");
    uart_put_num(UART0_BASE, mtime);
    uart_putstr(UART0_BASE, "\n");


    clint_set_mtimecmp(CLINT_BASE, 1000);
    uint64_t mtimecmp = clint_read_mtimecmp(CLINT_BASE);
    uart_putstr(UART0_BASE, "Current mtimecmp: ");
    uart_put_num(UART0_BASE, mtimecmp);
    uart_putstr(UART0_BASE, "\n");

    clint_set_mtime(CLINT_BASE, 1000);
    mtime = clint_read_mtime(CLINT_BASE);
    uart_putstr(UART0_BASE, "Current mtime: ");
    uart_put_num(UART0_BASE, mtime);
    uart_putstr(UART0_BASE, "\n");

    return 0;
}
