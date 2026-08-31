#ifndef IRINGBUF_H
#define IRINGBUF_H

#include <stdint.h>

void iringbuf_write(uint64_t addr, uint32_t inst);
void iringbuf_print(void);

#endif
