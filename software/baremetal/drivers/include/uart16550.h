#ifndef UART16550_H
#define UART16550_H

#include <stddef.h>
#include <stdint.h>

int  uart_putchar(uint64_t base, int c);
int  uart_getchar(uint64_t base);
void uart_putstr(uint64_t base, const char *s);
void uart_put_num(uint64_t base, uint64_t num);
size_t uart_readline(uint64_t base, char *buf, size_t size);

#endif
