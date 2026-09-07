#include "uart16550.h"

#include <stddef.h>
#include <stdint.h>

#include "mmio.h"
#include "uart16550_reg.h"

/**
 * Write a character c to UART device
 */
int uart_putchar(uint64_t base, int c) {
    // Wait till the TX FIFO is empty. Although this is not efficient as the FIFO can hold more data
    // But there is no other way to check if the FIFO is full or not so just be safe and not so efficient
    while (REG_FIELD_GET(mmio_read8(base + UART_LSR_OFFSET), UART_LSR_TEMT_MASK, UART_LSR_TEMT_OFFSET) == 0)
        ;
    mmio_write8(base + UART_THR_OFFSET, (uint8_t)c);
    return (int)((unsigned char)c);
}

/**
 * Get a character from UART device. Blocking if no data available
 */
int uart_getchar(uint64_t base) {
    // Wait till we get all something
    while (REG_FIELD_GET(mmio_read8(base + UART_LSR_OFFSET), UART_LSR_DR_MASK, UART_LSR_DR_OFFSET) == 0)
        ;
    return mmio_read8(base + UART_RBR_OFFSET);
}

/**
 * Write a string through UART
 */

void uart_putstr(uint64_t base, const char *s) {
    for (; *s != '\0'; s++) {
        uart_putchar(base, *s);
    }
}

/**
 * Read a line from the UART input, and put the result in the string s.
 * If the length of the line (excluding the \n) is larger then size - 1, then the exceeding character will be discared
 */
size_t uart_readline(uint64_t base, char *buf, size_t size) {
    char   c;
    size_t n = 0;
    if (size == 0) {
        return 0;
    }
    while (1) {
        c = (char)uart_getchar(base);
        if (c == '\n') {
            *buf = '\0';
            return n;
        }
        if (n < size - 1) {
            *buf++ = c;
        }
        n++;
    }
}

void uart_put_num(uint64_t base, uint64_t num) {
    char buf[21]; // Can handle up to 20-digit numbers
    int  i = 20;

    buf[i] = '\0';

    if (num == 0) {
        uart_putchar(base, '0');
        return;
    }

    while (num > 0) {
        buf[--i] = '0' + (char)(num % 10);
        num /= 10;
    }

    uart_putstr(base, &buf[i]);
}
