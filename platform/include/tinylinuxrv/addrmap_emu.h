#ifndef ADDR_MAP_EMU_H
#define ADDR_MAP_EMU_H

#include "addrmap.h"

#define DTB_SIZE       (1024 * 1024)
#define DTB_START_ADDR (DRAM_END - DTB_SIZE)

#define KERNEL_START_ADDR 0x80200000ULL

#define OPENSBI_START_ADDR DRAM_BASE

#endif
