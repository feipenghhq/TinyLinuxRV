#include <stdint.h>

typedef struct {
    uint64_t gpr[32];
    uint64_t mepc;
    uint64_t mstatus;
    uint64_t mcause;
    uint64_t mtval;
} trap_frame_t;
