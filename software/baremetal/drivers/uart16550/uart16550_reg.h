#ifndef UART16550_REG_H
#define UART16550_REG_H

// Transmitter Holding Register (THR)
#define UART_THR_OFFSET 0

// Receiver Buffer Register (RBR)
#define UART_RBR_OFFSET 0

// Line Status Register (LSR)
#define UART_LSR_OFFSET 5
// LSR - Data Ready (DR)
#define UART_LSR_DR_OFFSET 0
#define UART_LSR_DR_MASK   (1 << UART_LSR_DR_OFFSET)
// LSR - Transmitter empty (TEMT)
#define UART_LSR_TEMT_OFFSET 6
#define UART_LSR_TEMT_MASK   (1 << UART_LSR_TEMT_OFFSET)

#endif
