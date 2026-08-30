#ifndef UART16550_H
#define UART16550_H

#include <stddef.h>
#include <stdint.h>

int  uart_putchar(uint64_t base, const int c);
int  uart_getchar(uint64_t base);
void uart_putstr(uint64_t base, const char *s);
size_t uart_getstr(uint64_t base, char *s);

#endif
