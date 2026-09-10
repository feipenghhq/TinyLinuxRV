
#include <stdbool.h>
#include <stdint.h>

#include "addrmap.h"
#include "mmio.h"
#include "uart16550.h"

#define putstr(str)         uart_putstr(UART0_BASE, str)
#define putnum(num)         uart_put_num(UART0_BASE, num)
#define putchar(ch)         uart_putchar(UART0_BASE, ch)
#define getchar()           uart_getchar(UART0_BASE)
#define readline(buf, size) uart_readline(UART0_BASE, buf, size)

// test 1: check if interrupt process
static int test1(void) {
    int      error = 0;
    uint32_t pending;
    uint32_t id;
    char     buf[100];

    // Enabled the interrupt on both of the context
    mmio_write32(PLIC_BASE + 0x002000, (1U << 10)); // enable the interrupt on context 0
    mmio_write32(PLIC_BASE + 0x002080, (1U << 10)); // enable the interrupt on context 1
    // Set priority to 1
    mmio_write32(PLIC_BASE + 0x4 * 10, 1);

    // User input
    uart_putstr(UART0_BASE, "Please enter something less then 32 character\n");

    // read a character to make sure the user has entered something before checking PLIC status
    buf[0] = (char)getchar();

    // Check PLIC register to make sure we have the pending bit set
    pending = mmio_read32(PLIC_BASE + 0x1000);
    if (pending != (1U << 10)) {
        putstr("test1: pending not set.\n");
        error = 1;
    }
    // Claim the interrupt for context 0
    id = mmio_read32(PLIC_BASE + 0x200004);
    // We should get ID as 10
    if (id != 10) {
        putstr("test1: claim return incorrect ID: ");
        putnum(id);
        putchar('\n');
        error = 1;
    }
    // We should see pending becomes zero
    pending = mmio_read32(PLIC_BASE + 0x1000);
    if (pending != 0) {
        putstr("test1: pending not clear.\n");
        error = 1;
    }
    // Now resolve the interrupt by reading from UART
    readline(buf + 1, 32);
    // Now complete the interrupt for context 0
    mmio_write32(PLIC_BASE + 0x200004, 10);

    return error;
}

// test 2: check enable
static int test2(void) {
    int      error = 0;
    uint32_t pending;
    uint32_t id;
    char     buf[100];

    // Enabled the interrupt on context 1
    mmio_write32(PLIC_BASE + 0x002000, 0);          // disable the interrupt on context 0
    mmio_write32(PLIC_BASE + 0x002080, (1U << 10)); // enable the interrupt on context 1
    // Set priority to 1
    mmio_write32(PLIC_BASE + 0x4 * 10, 1);

    // User input
    uart_putstr(UART0_BASE, "Please enter something less then 32 character\n");

    // read a character to make sure the user has entered something before checking PLIC status
    buf[0] = (char)getchar();

    // Claim the interrupt for context 0
    id = mmio_read32(PLIC_BASE + 0x200004);
    // We should get ID as 0 as it is not enabled
    if (id != 0) {
        putstr("test2: claim context 0 return incorrect ID: ");
        putnum(id);
        putchar('\n');
        error = 1;
    }
    // We should see pending still valid
    pending = mmio_read32(PLIC_BASE + 0x1000);
    if (pending != (1U << 10)) {
        putstr("test2 pending is not set.\n");
        error = 1;
    }
    // Claim the interrupt for context 1
    id = mmio_read32(PLIC_BASE + 0x200004 + 0x1000);
    // We should get ID as 10.
    if (id != 10) {
        putstr("test2: claim context 1 return incorrect ID: ");
        putnum(id);
        putchar('\n');
        error = 1;
    }
    // We should see pending becomes zero
    pending = mmio_read32(PLIC_BASE + 0x1000);
    if (pending != 0) {
        putstr("test2:  pending is not cleared.\n");
        error = 1;
    }
    // Now resolve the interrupt by reading from UART
    readline(buf + 1, 32);
    // Now complete the interrupt for context 1
    mmio_write32(PLIC_BASE + 0x200004 + 0x1000, 10);

    return error;
}

// test 3: check priority = 0
static int test3(void) {
    int      error = 0;
    uint32_t pending;
    uint32_t id;
    char     buf[100];

    // Enabled the interrupt on context 0
    mmio_write32(PLIC_BASE + 0x002000, (1U << 10)); // enable the interrupt on context 0
    mmio_write32(PLIC_BASE + 0x002080, 0);          // disable the interrupt on context 1
    // Set priority to 0
    mmio_write32(PLIC_BASE + 0x4 * 10, 0);

    // User input
    uart_putstr(UART0_BASE, "Please enter something less then 32 character\n");

    // read a character to make sure the user has entered something before checking PLIC status
    buf[0] = (char)getchar();

    // Claim the interrupt for context 0
    id = mmio_read32(PLIC_BASE + 0x200004);
    // We should get ID as 0 as it is not enabled
    if (id != 0) {
        putstr("test3: claim context 0 return incorrect ID: ");
        putnum(id);
        putchar('\n');
        error = 1;
    }
    // We should see pending still valid
    pending = mmio_read32(PLIC_BASE + 0x1000);
    if (pending != (1U << 10)) {
        putstr("test3 pending is not set.\n");
        error = 1;
    }
    // Set priority to 1
    mmio_write32(PLIC_BASE + 0x4 * 10, 1);
    // Claim the interrupt for context 0
    id = mmio_read32(PLIC_BASE + 0x200004);
    // We should see pending becomes zero
    pending = mmio_read32(PLIC_BASE + 0x1000);
    if (pending != 0) {
        putstr("test3:  pending is not cleared.\n");
        error = 1;
    }
    // Now resolve the interrupt by reading from UART
    readline(buf + 1, 32);
    // Now complete the interrupt for context 0
    mmio_write32(PLIC_BASE + 0x200004, 10);

    return error;
}

// test 4: check threshold does not affect claim
static int test4(void) {
    int      error = 0;
    uint32_t pending;
    uint32_t id;
    char     buf[100];

    // Enabled the interrupt on context 0
    mmio_write32(PLIC_BASE + 0x002000, (1U << 10)); // enable the interrupt on context 0
    mmio_write32(PLIC_BASE + 0x002080, 0);          // disable the interrupt on context 1
    // Set priority to 10
    mmio_write32(PLIC_BASE + 0x4 * 10, 10);
    // Set threshold equal to the interrupt priority. This suppresses the
    // interrupt notification, but it must not prevent a claim.
    mmio_write32(PLIC_BASE + 0x200000, 10);

    // User input
    uart_putstr(UART0_BASE, "Please enter something less then 32 character\n");

    // read a character to make sure the user has entered something before checking PLIC status
    buf[0] = (char)getchar();

    // Claim the interrupt for context 0
    id = mmio_read32(PLIC_BASE + 0x200004);
    // Claim is not affected by threshold, so we should still get ID 10
    if (id != 10) {
        putstr("test4: (1) claim context 0 return incorrect ID: ");
        putnum(id);
        putchar('\n');
        error = 1;
    }
    // A successful claim should clear pending
    pending = mmio_read32(PLIC_BASE + 0x1000);
    if (pending != 0) {
        putstr("test4: (2) pending is not cleared.\n");
        error = 1;
    }
    // Now resolve the interrupt by reading from UART
    readline(buf + 1, 32);
    // Now complete the interrupt for context 0
    mmio_write32(PLIC_BASE + 0x200004, 10);

    return error;
}

int main(void) {
    int error;
    // enable uart RX buffer interrupt
    mmio_write8(UART0_BASE + 1, 1);

    error = test1();
    error += test2();
    error += test3();
    error += test4();

    // Now the MIP/EIP interrupt should be removed
    return error;
}
