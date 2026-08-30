#ifndef uart16550_H
#define uart16550_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    volatile uint8_t rbr; // Receiver Buffer Register (RO)
    volatile uint8_t thr; // Transmitter Holding Register (WO)
    volatile uint8_t ier; // Interrupt Enable Register
    volatile uint8_t iir; // Interrupt Ident Register (RO)
    volatile uint8_t fcr; // FIFO control Register (WO)
    volatile uint8_t lcr; // Line Control Register
    volatile uint8_t mcr; // Modem Control Register
    volatile uint8_t lsr; // Line Status Register
    volatile uint8_t msr; // Modem Status Register
    volatile uint8_t scr; // Scratch Register
    volatile uint8_t dll; // Divisor Latch (LSB)
    volatile uint8_t dlm; // Divisor Latch (MSB)
} uart16550_reg_t;

typedef struct {
    uint8_t data[16];
    uint8_t rd_ptr;
    uint8_t wr_ptr;
    uint8_t size;
} uart16550_fifo_t;

typedef struct {
    uint64_t         base;
    uart16550_reg_t  reg;
    uart16550_fifo_t rx_fifo;
} uart16550_t;

int uart16550_init(uart16550_t *uart16550, uint64_t base);
int uart16550_reset(uart16550_t *uart16550);
int uart16550_write(uart16550_t *uart16550, uint64_t addr, size_t size, const void *data);
int uart16550_read(uart16550_t *uart16550, uint64_t addr, size_t size, void *data);
int uart16550_poll_input(uart16550_t *uart);

#endif // uart16550_H
