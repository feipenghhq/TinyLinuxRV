/**
 * This file test the syscon device.
 *
 */
#include "uart16550.h"

#include <stdbool.h>
#include <stdint.h>

#include "addrmap.h"

int strcmp(const char *s1, const char *s2) {
    while (*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int main(void) {
    int         c = 0;
    char        buf1[16];
    char        buf2[16];
    bool        result[3] = {false, false, false};
    const char *s1        = "123456789012345";
    const char *s2        = "abcdefg";

    // test output
    uart_putstr(UART0_BASE, "Hello, World!\n");

    // test single character input
    uart_putstr(UART0_BASE, "Test 1: Please enter the letter z\n");
    c = uart_getchar(UART0_BASE);
    if (c == 'z') {
        result[0] = true;
    }
    c = uart_getchar(UART0_BASE); // discard the new  line character

    // test multiple character input
    uart_putstr(UART0_BASE, "Test 2: Please enter \"123456789012345\"\n");
    uart_getstr(UART0_BASE, buf1);
    if (strcmp(buf1, s1) == 0) {
        result[1] = true;
    }

    // test rx FIFO ptr wrappering
    uart_putstr(UART0_BASE, "Test 3: Please enter \"abcdefg\"\n");
    uart_getstr(UART0_BASE, buf2);
    if (strcmp(buf2, s2) == 0) {
        result[2] = true;
    }

    if (result[0] && result[1] && result[2]) {
        uart_putstr(UART0_BASE, "All test passed!\n");
        return 0;
    } else {
        if (!result[0]) {
            uart_putstr(UART0_BASE, "Test 1 Failed!\n");
        }

        if (!result[1]) {
            uart_putstr(UART0_BASE, "Test 2 Failed!\n");
            uart_putstr(UART0_BASE, buf1);
            uart_putchar(UART0_BASE, '\n');
        }

        if (!result[2]) {
            uart_putstr(UART0_BASE, "Test 3 Failed!\n");
            uart_putstr(UART0_BASE, buf2);
            uart_putchar(UART0_BASE, '\n');
        }
        return -1;
    }
}
