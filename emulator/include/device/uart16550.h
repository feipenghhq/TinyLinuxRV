#ifndef UART16550_H
#define UART16550_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t rbr; // Receiver Buffer Register (RO)
    uint8_t thr; // Transmitter Holding Register (WO)
    uint8_t ier; // Interrupt Enable Register
    uint8_t iir; // Interrupt Ident Register (RO)
    uint8_t fcr; // FIFO control Register (WO)
    uint8_t lcr; // Line Control Register
    uint8_t mcr; // Modem Control Register
    uint8_t lsr; // Line Status Register
    uint8_t msr; // Modem Status Register
    uint8_t scr; // Scratch Register
    uint8_t dll; // Divisor Latch (LSB)
    uint8_t dlm; // Divisor Latch (MSB)
} uart16550_reg_t;

typedef struct {
    uint8_t data[16];
    uint8_t rd_ptr;
    uint8_t wr_ptr;
    uint8_t size;
    bool    overrun;
    bool    underrun;
} uart16550_fifo_t;

typedef struct {
    uint64_t         base;
    uart16550_reg_t  reg;
    uart16550_fifo_t rx_fifo;
} uart16550_t;

void uart16550_init(uart16550_t *uart16550, uint64_t base);
void uart16550_reset(uart16550_t *uart16550);
int  uart16550_write(uart16550_t *uart16550, uint64_t addr, size_t size, const void *data);
int  uart16550_read(uart16550_t *uart16550, uint64_t addr, size_t size, void *data);
int  uart16550_poll_input(uart16550_t *uart);
bool uart16550_irq_level(uart16550_t *uart);

#endif // UART16550_H
