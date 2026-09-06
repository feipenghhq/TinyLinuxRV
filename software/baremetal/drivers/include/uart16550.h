#ifndef UART16550_H
#define UART16550_H

#include <stddef.h>
#include <stdint.h>

int  uart_putchar(uint64_t base, const int c);
int  uart_getchar(uint64_t base);
void uart_putstr(uint64_t base, const char *s);
size_t uart_readline(uint64_t base, char *buf, size_t size);

#endif
