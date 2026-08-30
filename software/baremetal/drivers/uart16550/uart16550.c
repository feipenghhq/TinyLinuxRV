#include "uart16550.h"

#include <stdint.h>
#include <stddef.h>

#include "mmio.h"
#include "uart16550_reg.h"

#define REG_WRITE_BYTE(addr, data) *((volatile uint8_t *)addr) = (data)

#define REG_READ_BYTE(addr, data) *((volatile uint8_t *)addr)

#define REG_FIELD_READ(reg, bit, width) (((reg) >> (bit)) & BIT_MASK(width))

/**
 * Write a character c to UART device
 */
int uart_putchar(uint64_t base, const int c) {
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
 * Get a string from UART device. End with newline character. Blocking if no data available
 */
size_t uart_getstr(uint64_t base, char *s) {
    char c;
    size_t size = 0;
    // wait till we get all something
    while(1) {
        c = (char) uart_getchar(base);
        if (c == '\n') {
            *s = '\0';
            return size;
        }
        size++;
        *s++ = c;
    }
    return 0;
}
