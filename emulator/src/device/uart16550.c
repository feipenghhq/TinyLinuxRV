/**
 * This code implement a uart compatible UART for the TinyLinuxRV emulator
 *
 * Limitation:
 * - character timeout not implemented for FIFO mode.
 */

#include "uart16550.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

#include "utils/log.h"

#define BIT_MASK(width) ((1 << (width)) - 1)

#define FIELD_MASK(bit, width) (BIT_MASK(width) << (bit))

#define FIELD_READ(reg, bit, width) (((reg) >> (bit)) & BIT_MASK(width))

#define FIELD_WRITE(reg, data, bit, width) \
    (reg = (uint8_t)(((reg) & ~FIELD_MASK(bit, width)) | (((data) & BIT_MASK(width)) << (bit))))

#define FIFO_RX_INT_TRIGGER(uart) FIELD_READ(uart->reg.fcr, 6, 2)
#define FIFO_ENABLE(uart)         FIELD_READ(uart->reg.fcr, 0, 1)

static const int fifo_interrupt_level[] = {1, 4, 8, 14};

static inline void clear_reg(uart16550_t *uart) {
    uart->reg.rbr = 0;
    uart->reg.thr = 0;
    uart->reg.ier = 0;
    uart->reg.iir = 0xC1; // need to enable FIFO for now as our CPU can't handle interrupt at this point.
    uart->reg.fcr = 0xC1; // need to enable FIFO for now as our CPU can't handle interrupt at this point.
    uart->reg.lcr = 0x3;
    uart->reg.mcr = 0;
    uart->reg.lsr = 0x60;
    uart->reg.msr = 0;
    uart->reg.scr = 0;
    uart->reg.dll = 0;
    uart->reg.dlm = 0;
}

static inline void rx_fifo_clear(uart16550_t *uart) {
    uart->rx_fifo.size     = 0;
    uart->rx_fifo.wr_ptr   = 0;
    uart->rx_fifo.rd_ptr   = 0;
    uart->rx_fifo.overrun  = false;
    uart->rx_fifo.underrun = false;
}

static void rx_fifo_write(uart16550_t *uart, uint8_t data) {
    int fifo_enabled = FIFO_ENABLE(uart);

    // When FIFO is not enabled, overrun happens when we receive second data while first data has not been read.
    if (!fifo_enabled && uart->rx_fifo.size == 1) {
        uart->rx_fifo.overrun = true;
        return;
    }
    // When FIFO is enabled, overrun happens, discard new data
    if (fifo_enabled && uart->rx_fifo.size == 16) {
        uart->rx_fifo.overrun = true;
        return;
    }

    uart->rx_fifo.data[uart->rx_fifo.wr_ptr] = data;
    uart->rx_fifo.size++;
    uart->rx_fifo.wr_ptr = (uart->rx_fifo.wr_ptr + 1) & 0xF;
}

static uint8_t rx_fifo_read(uart16550_t *uart) {
    // if FIFO is empty, the SW should not read the UART.
    // then do nothing and return 0 as the read result
    if (uart->rx_fifo.size == 0) {
        uart->rx_fifo.underrun = true;
        return 0;
    }
    if (uart->rx_fifo.size > 0) {
        uart->rx_fifo.size--;
    }
    uint8_t data         = uart->rx_fifo.data[uart->rx_fifo.rd_ptr];
    uart->rx_fifo.rd_ptr = (uart->rx_fifo.rd_ptr + 1) & 0xF;
    return data;
}

void uart16550_reset(uart16550_t *uart) {
    clear_reg(uart);
    rx_fifo_clear(uart);
    FIELD_WRITE(uart->reg.iir, 1, 0, 1);
}

void uart16550_init(uart16550_t *uart, uint64_t base) {
    uart->base = base;
    uart16550_reset(uart);
}

int uart16550_write(uart16550_t *uart, uint64_t addr, size_t size, const void *data) {
    uint8_t  value;
    uint8_t  DLAB; // Divisor Latch Access Bit
    uint64_t offset;

    if (size != 1) {
        LOG_ERROR("uart only support byte access.");
        return -1;
    }

    offset = addr - uart->base;
    DLAB   = FIELD_READ(uart->reg.lcr, 7, 1);
    memcpy(&value, data, size);

    switch (offset) {
    case 0: { // THR or DLL
        if (DLAB == 0) {
            uart->reg.thr = value;
            // send the character out immediate as we are an emulator
            putchar(value);
        } else {
            uart->reg.dll = value;
        }
        break;
    }
    case 1: { // IER
        if (DLAB == 0) {
            uart->reg.ier = value;
        } else {
            uart->reg.dlm = value;
        }
        break;
    }
    case 2: { // FCR
        // clear FIFO when FIFO enable bit is changed
        if ((FIELD_READ(value, 0, 1) ^ FIFO_ENABLE(uart)) != 0) {
            rx_fifo_clear(uart);
        }
        // clear RX FIFO when RX FIFO clear is set by SW.
        if (FIELD_READ(value, 1, 1)) {
            rx_fifo_clear(uart);
        }
        uart->reg.fcr = value;
        break;
    }
    case 3: { // LCR
        uart->reg.lcr = value;
        break;
    }
    case 4: { // THR
        uart->reg.mcr = value;
        break;
    }
    case 5: { // LSR
        // skip, read only  register
        break;
    }
    case 6: { // MSR
        // skip, read only  register
        break;
    }
    case 7: { // SCR
        uart->reg.scr = value;
        break;
    }
    default: {
        LOG_ERROR("Unsupported address in uart16550");
        return -1;
    }
    }

    return 0;
}

int uart16550_read(uart16550_t *uart, uint64_t addr, size_t size, void *data) {
    uint8_t  DLAB; // Divisor Latch Access Bit
    uint64_t offset;
    uint32_t value;

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
            value = rx_fifo_read(uart);
        } else {
            value = uart->reg.dll;
        }
        break;
    }
    case 1: { // IER
        if (DLAB == 0) {
            value = uart->reg.ier;
        } else {
            value = uart->reg.dlm;
        }
        break;
    }
    case 2: { // IIR
        value = uart->reg.iir;
        break;
    }
    case 3: { // LCR
        value = uart->reg.lcr;
        break;
    }
    case 4: { // THR
        value = uart->reg.mcr;
        break;
    }
    case 5: { // LSR
        // update LSR on fly when reading
        FIELD_WRITE(uart->reg.lsr, uart->rx_fifo.size > 0 ? 1 : 0, 0, 1); // Data Ready (DR) indicator
        FIELD_WRITE(uart->reg.lsr, uart->rx_fifo.overrun, 1, 1);          // Data Ready (DR) indicator
        FIELD_WRITE(uart->reg.lsr, 1, 5, 1);                              // Transmit FIFO is empty
        FIELD_WRITE(uart->reg.lsr, 1, 6, 1);                              // Transmitter empty indicator

        // read the register
        value = uart->reg.lsr;

        // the following bit are cleared when LSR is read
        FIELD_WRITE(uart->reg.lsr, 0, 1, 1); // clear OE
        FIELD_WRITE(uart->reg.lsr, 0, 2, 1); // clear PE
        FIELD_WRITE(uart->reg.lsr, 0, 3, 1); // clear FE
        FIELD_WRITE(uart->reg.lsr, 0, 4, 1); // clear BI
        FIELD_WRITE(uart->reg.lsr, 0, 7, 1); // clear error

        // clear FIFO overrun when reading LSR
        uart->rx_fifo.overrun = false;
        break;
    }
    case 6: { // MSR
        value = uart->reg.msr;
        break;
    }
    case 7: { // SCR
        value = uart->reg.scr;
        break;
    }
    default: {
        LOG_ERROR("Unsupported address in uart16550");
        return -1;
    }
    }
    memcpy(data, &value, size);
    return 0;
}

/**
 * The device main execution loop will call this function to grep the input for uart
 */
int uart16550_poll_input(uart16550_t *uart) {
    struct pollfd fsd[] = {{STDIN_FILENO, POLLIN, POLLIN}};
    char          buf;
    ssize_t       nbytes;

    // For now, only try to poll when the rx FIFO has space.
    if (uart->rx_fifo.size < 16) {
        nbytes = poll(fsd, 1, 0);
        if (nbytes >= 0) {
            if (fsd[0].revents & POLLIN) {
                nbytes = read(STDIN_FILENO, &buf, 1);
                // error
                if (nbytes == -1) {
                    LOG_ERROR("Failed to read from UART: %s", strerror(errno));
                    return -1;
                }
                // EOF
                if (nbytes == 0) {
                    return 0;
                }
                // normal case
                rx_fifo_write(uart, (uint8_t)buf);
            }
        } else {
            LOG_ERROR("Failed to poll for UART: %s", strerror(errno));
            return -1;
        }
    }
    return 0;
}

/**
 * The device main execution loop will call this function to check if uart has interrupt
 */
bool uart16550_irq_level(uart16550_t *uart) {
    int receiver_line_status_en               = 0;
    int receiver_data_available_en            = 0;
    int timeout_indication_en                 = 0;
    int transmitter_holding_register_empty_en = 0;
    int modem_status_en                       = 0;
    int fifo_enabled                          = 0;

    bool receiver_line_status               = false;
    bool receiver_data_available            = false;
    bool timeout_indication                 = false;
    bool transmitter_holding_register_empty = false;
    bool modem_status                       = false;

    bool has_interrupt = false;

    // FIFO enabled
    fifo_enabled = FIFO_ENABLE(uart);

    receiver_line_status_en = FIELD_READ(uart->reg.ier, 2, 1);
    receiver_line_status    = receiver_line_status_en && uart->rx_fifo.overrun;

    receiver_data_available_en = FIELD_READ(uart->reg.ier, 0, 1);
    receiver_data_available    = receiver_data_available_en &&
                                 (fifo_enabled ? (uart->rx_fifo.size >= fifo_interrupt_level[FIFO_RX_INT_TRIGGER(uart)])
                                               : uart->rx_fifo.size > 0);

    // no timeout interrupt at this point
    timeout_indication = timeout_indication_en && false;

    // TX FIFO is always empty
    transmitter_holding_register_empty_en = FIELD_READ(uart->reg.ier, 1, 1);
    transmitter_holding_register_empty    = transmitter_holding_register_empty_en && true;

    // not supported
    modem_status_en = FIELD_READ(uart->reg.ier, 3, 1);
    modem_status    = modem_status_en && false;

    // update the IIR register based on interrupt status
    has_interrupt =
        receiver_line_status || receiver_data_available || timeout_indication || transmitter_holding_register_empty;
    if (has_interrupt) {
        FIELD_WRITE(uart->reg.iir, 0, 0, 1); // 0 - interrupt pending
    } else {
        FIELD_WRITE(uart->reg.iir, 1, 0, 1); // 1- no interrupt
    }

    // FIFOs enabled
    FIELD_WRITE(uart->reg.iir, fifo_enabled, 6, 1);
    FIELD_WRITE(uart->reg.iir, fifo_enabled, 7, 1);

    // interrupt priority
    if (receiver_line_status) {
        FIELD_WRITE(uart->reg.iir, 3, 1, 3);
    } else if (receiver_data_available) {
        FIELD_WRITE(uart->reg.iir, 2, 1, 3);
    } else if (timeout_indication) {
        FIELD_WRITE(uart->reg.iir, 6, 1, 3);
    } else if (transmitter_holding_register_empty) {
        FIELD_WRITE(uart->reg.iir, 1, 1, 3);
    } else if (modem_status) {
        FIELD_WRITE(uart->reg.iir, 0, 1, 3);
    }

    return has_interrupt;
}
