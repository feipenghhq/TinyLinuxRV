/**
 * This code implement a uart compatible UART for the TinyLinuxRV emulator
 * For simplicity
 */

#include "uart16550.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>

#include "log.h"

#define BYTE(data) ((uint8_t *)data)

#define BIT_MASK(width) ((1 << (width)) - 1)

#define FIELD_MASK(bit, width) (BIT_MASK(width) << (bit))

#define FIELD_READ(reg, bit, width) (((reg) >> (bit)) & BIT_MASK(width))

#define FIELD_WRITE(reg, data, bit, width) \
    (reg = (uint8_t)(((reg) & ~FIELD_MASK(bit, width)) | (((data) & BIT_MASK(width)) << (bit))))

#define FIFO_RX_INT_TRIGGER(reg) FIELD_READ(reg, 6, 2)
#define FIFO_CLR_RX_FIFO(reg)    FIELD_READ(reg, 1, 1)

static int fifo_interrupt_level[] = {1, 4, 8, 14};

static inline void clear_reg(uart16550_t *uart) {
    uart->reg.rbr = 0;
    uart->reg.thr = 0;
    uart->reg.ier = 0;
    uart->reg.iir = 0xC1;
    uart->reg.fcr = 0xC0;
    uart->reg.lcr = 0x3;
    uart->reg.mcr = 0;
    uart->reg.lsr = 0x60;
    uart->reg.msr = 0;
    uart->reg.scr = 0;
    uart->reg.dll = 0;
    uart->reg.dlm = 0;
}

static inline void rx_fifo_clear(uart16550_t *uart) {
    uart->rx_fifo.size   = 0;
    uart->rx_fifo.wr_ptr = 0;
    uart->rx_fifo.rd_ptr = 0;
    // !Clear DR ready as there is no data in the RX FIFO
    FIELD_WRITE(uart->reg.lsr, 0, 0, 1);
}

static void set_interrupt(uart16550_t *uart, uint8_t enable, uint8_t code) {
    // Only set the interrupt when the corresponding enable bit is set
    if (FIELD_READ(uart->reg.ier, enable, 1)) {
        // Set Receiver data available interrupt
        FIELD_WRITE(uart->reg.iir, 0, 0, 1);
        FIELD_WRITE(uart->reg.iir, code, 1, 3);
    }
}

static void clear_interrupt(uart16550_t *uart, uint8_t enable) {
    // Only set the interrupt when the corresponding enable bit is set
    if (FIELD_READ(uart->reg.ier, enable, 1)) {
        FIELD_WRITE(uart->reg.iir, 1, 0, 1);
    }
}

static void rx_fifo_write(uart16550_t *uart, uint8_t data) {
    // check interrupt trigger
    if (uart->rx_fifo.size == fifo_interrupt_level[FIFO_RX_INT_TRIGGER(uart->reg.fcr)] - 1) {
        set_interrupt(uart, 0, 2); // 2 indicate Receiver data available
    }
    // overrun happens, discard new data
    if (uart->rx_fifo.size == 16) {
        set_interrupt(uart, 2, 3);           // 3 indicate Receiver Line Status (Overrun)
        FIELD_WRITE(uart->reg.lsr, 1, 1, 1); // Set OE
        return;
    }
    FIELD_WRITE(uart->reg.lsr, 1, 0, 1); // Set DR indicator
    uart->rx_fifo.data[uart->rx_fifo.wr_ptr] = data;
    uart->rx_fifo.size++;
    uart->rx_fifo.wr_ptr = (uart->rx_fifo.wr_ptr + 1) & 0xF;
}

// Note: assuming no overrun could happens from guest
static uint8_t rx_fifo_read(uart16550_t *uart) {
    // check interrupt trigger
    if (uart->rx_fifo.size == fifo_interrupt_level[FIFO_RX_INT_TRIGGER(uart->reg.fcr)]) {
        clear_interrupt(uart, 0);
    }
    if (uart->rx_fifo.size > 0) {
        uart->rx_fifo.size--;
    }
    // No data remaining, clear DR indicator
    if (uart->rx_fifo.size == 0) {
        FIELD_WRITE(uart->reg.lsr, 0, 0, 1);
    }
    uint8_t data         = uart->rx_fifo.data[uart->rx_fifo.rd_ptr];
    uart->rx_fifo.rd_ptr = (uart->rx_fifo.rd_ptr + 1) & 0xF;
    return data;
}

int uart16550_reset(uart16550_t *uart) {
    clear_reg(uart);
    rx_fifo_clear(uart);
    FIELD_WRITE(uart->reg.iir, 1, 0, 1);
    return 0;
}

int uart16550_init(uart16550_t *uart, uint64_t base) {
    uart16550_reset(uart);
    uart->base = base;
    return 0;
}

int uart16550_write(uart16550_t *uart, uint64_t addr, size_t size, const void *data) {
    uint8_t  DLAB; // Divisor Latch Access Bit
    uint64_t offset;

    if (size != 1) {
        LOG_ERROR("uart only support byte access.");
        return -1;
    }

    offset = addr - uart->base;
    DLAB   = FIELD_READ(uart->reg.lcr, 7, 1);

    switch (offset) {
    case 0: { // THR or DLL
        if (DLAB == 0) {
            uart->reg.thr = *BYTE(data);
            // send the character out immediate as we are an emulator
            putchar(*BYTE(data));
            // set the Transmit FIFO is empty bit in LSR as we don't have TX FIFO
            FIELD_WRITE(uart->reg.lsr, 1, 5, 1);
            FIELD_WRITE(uart->reg.lsr, 1, 6, 1);
        } else {
            uart->reg.dll = *BYTE(data);
        }
        break;
    }
    case 1: { // IER
        if (DLAB == 0) {
            uart->reg.ier = *BYTE(data);
        } else {
            uart->reg.dlm = *BYTE(data);
        }
        break;
    }
    case 2: { // FCR
        uart->reg.fcr = *BYTE(data);
        if (FIFO_CLR_RX_FIFO(uart->reg.fcr)) {
            rx_fifo_clear(uart);
        }
        break;
    }
    case 3: { // LCR
        uart->reg.lcr = *BYTE(data);
        break;
    }
    case 4: { // THR
        uart->reg.mcr = *BYTE(data);
        break;
    }
    case 5: { // LSR
        uart->reg.lsr = *BYTE(data);
        break;
    }
    case 6: { // MSR
        uart->reg.msr = *BYTE(data);
        break;
    }
    case 7: { // SCR
        uart->reg.scr = *BYTE(data);
        break;
    }
    }

    return 0;
}

int uart16550_read(uart16550_t *uart, uint64_t addr, size_t size, void *data) {
    uint8_t  DLAB; // Divisor Latch Access Bit
    uint64_t offset;

    if (size != 1) {
        LOG_ERROR("uart only support byte access.");
        return -1;
    }

    offset = addr - uart->base;
    DLAB   = FIELD_READ(uart->reg.lcr, 7, 1);

    switch (offset) {
    case 0: { // RBR or DLL
        if (DLAB == 0) {
            // reading rbr is basically getting the data from RX FIFO
            // guest should not read when FIFO is empty
            *BYTE(data) = rx_fifo_read(uart);
        } else {
            *BYTE(data) = uart->reg.dll;
        }
        break;
    }
    case 1: { // IER
        if (DLAB == 0) {
            *BYTE(data) = uart->reg.ier;
        } else {
            *BYTE(data) = uart->reg.dlm;
        }
        break;
    }
    case 2: { // IIR
        *BYTE(data) = uart->reg.iir;
        break;
    }
    case 3: { // LCR
        *BYTE(data) = uart->reg.lcr;
        break;
    }
    case 4: { // THR
        *BYTE(data) = uart->reg.mcr;
        break;
    }
    case 5: { // LSR
        *BYTE(data) = uart->reg.lsr;
        FIELD_WRITE(uart->reg.lsr, 0, 1, 1); // clear OE
        FIELD_WRITE(uart->reg.lsr, 0, 2, 1); // clear PE
        FIELD_WRITE(uart->reg.lsr, 0, 3, 1); // clear FE
        FIELD_WRITE(uart->reg.lsr, 0, 4, 1); // clear BI
        FIELD_WRITE(uart->reg.lsr, 0, 7, 1); // clear error

        break;
    }
    case 6: { // MSR
        *BYTE(data) = uart->reg.msr;
        break;
    }
    case 7: { // SCR
        *BYTE(data) = uart->reg.scr;
        break;
    }
    }

    return 0;
}

// pulling/update function
int uart16550_poll_input(uart16550_t *uart) {
    struct pollfd fsd[]  = {{STDIN_FILENO, POLLIN, POLLIN}};
    char          buf;

    // For now, only try to poll when the rx FIFO has space.
    if (uart->rx_fifo.size < 16) {
        if (poll(fsd, 1, 0) > 0) {
            if (fsd[0].revents & POLLIN) {
                if (read(STDIN_FILENO, &buf, 1) <= 0) {
                    LOG_ERROR("Failed to read from UART");
                    return -1;
                }
                rx_fifo_write(uart, (uint8_t)buf);
            }
        }

    }
    return 0;
}
