#include <stdio.h>

#include "ns16550.h"
#include "riscv-virt.h"

static int prvConsolePutChar(char c, FILE *stream) {

    // stream is not used right now
    (void)stream;

    // send a character
    vSendChar(c);

    return (unsigned char)c;
}

static FILE xConsoleStream = FDEV_SETUP_STREAM(prvConsolePutChar, NULL, NULL, _FDEV_SETUP_WRITE);

FILE *const stdout = &xConsoleStream;
FILE *const stderr = &xConsoleStream;
