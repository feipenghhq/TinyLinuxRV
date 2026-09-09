#include "plic.h"

#include <stdint.h>
#include <string.h>

#include "utils/log.h"

//---------------------------------------------------------
// macro and data type
//---------------------------------------------------------

// Offset for register
#define PRIORITY_OFFSET  0
#define PENDING_OFFSET   0x1000
#define ENABLE_OFFSET    0x2000
#define THRESHOLD_OFFSET 0x200000

// Address increase for next context
#define ENABLE_CONTEXT_INC    0x80
#define THRESHOLD_CONTEXT_INC 0x1000

typedef enum { R_PRIORITY, R_PENDING, R_ENABLE, R_THRESHOLD, R_CLAIM, R_INVALID } kind_t;

typedef struct {
    kind_t kind;
    int    context;
    int    index;
} reg_info_t;

//---------------------------------------------------------
// Local function
//---------------------------------------------------------

// function to help decode the address
static reg_info_t addr_decode(plic_t *plic, uint64_t addr, size_t size) {
    int offset = (int)(addr - plic->base);

    reg_info_t info = {.kind = R_INVALID, .context = 0, .index = 0};
    // size should be 4 for PLIC
    if (size != 4) {
        LOG_ERROR("PLIC only support 4 byte access. Received %zu byte", size);
        return info;
    }

    // The address must be within the PLIC range already.

    // interrupt priority
    if (offset >= 0 && offset < PRIORITY_OFFSET + PLIC_MAX_INTERRUPT * 4) {
        info.kind  = R_PRIORITY;
        info.index = (int)(offset - PRIORITY_OFFSET) / 4;
    }
    // interrupt pending
    else if (offset >= PENDING_OFFSET && offset < PENDING_OFFSET + PLIC_INTERRUPT_WORDS * 4) {
        info.kind  = R_PENDING;
        info.index = (int)(offset - PENDING_OFFSET) / 4;
    }
    // interrupt enable
    else if (offset >= ENABLE_OFFSET && offset < THRESHOLD_OFFSET) {
        offset       = offset - ENABLE_OFFSET;
        info.context = offset / ENABLE_CONTEXT_INC;
        if (info.context < PLIC_MAX_CONTEXT) {
            info.index = (offset - info.context * ENABLE_CONTEXT_INC) / 4;
            switch (info.index) {
            case 0: {
                info.kind = R_ENABLE;
                break;
            }
            default: {
                LOG_ERROR("PLIC: Access Reserved address.");
                info.kind = R_INVALID;
            }
            }
        } else {
            LOG_ERROR("PLIC only support %d context. But access context %d", PLIC_MAX_CONTEXT, info.context);
            info.kind = R_INVALID;
        }
    }
    // interrupt threshold and claim
    else if (offset >= THRESHOLD_OFFSET) {
        offset       = offset - THRESHOLD_OFFSET;
        info.context = offset / THRESHOLD_CONTEXT_INC;
        if (info.context < PLIC_MAX_CONTEXT) {
            info.index = (offset - info.context * THRESHOLD_CONTEXT_INC) / 4;
            switch (info.index) {
            case 0: { // interrupt threshold
                info.kind  = R_THRESHOLD;
                info.index = info.context;
                break;
            }
            case 1: { // interrupt claim
                info.kind  = R_CLAIM;
                info.index = info.context;
                break;
            }
            default: {
                LOG_ERROR("PLIC: Access Reserved address.");
                info.kind = R_INVALID;
            }
            }
        } else {
            LOG_ERROR("PLIC only support %d context. But access context %d", PLIC_MAX_CONTEXT, info.context);
            info.kind = R_INVALID;
        }
    }
    return info;
}

static void plic_gateway_irq(plic_t *plic, int id, bool irq) {
    int pending_idx = id / 32;
    int bit_idx     = id % 32;

    if (irq && !plic->gw_busy[id]) {
        plic->gw_busy[id] = true;
        // set the corresponding pending bit
        plic->regs.pending[pending_idx] |= (1U << bit_idx);
    }
}

static void plic_claim(plic_t *plic, int id) {
    int pending_idx = id / 32;
    int bit_idx     = id % 32;

    plic->regs.pending[pending_idx] &= ~(1U << bit_idx);
}

static void plic_complete(plic_t *plic, int id) {
    plic->gw_busy[id] = false;
}

// check if a specific context has interrupt
static bool interrupt_per_context(plic_t *plic, int context) {
    uint32_t enabled_pending[PLIC_INTERRUPT_WORDS];
    bool     irq_pending = false;

    for (int i = 0; i < PLIC_INTERRUPT_WORDS; i++) {
        enabled_pending[i] = plic->regs.pending[i] & plic->regs.enable[context][i];
    }

    for (int i = 0; i < PLIC_MAX_INTERRUPT; i++) {
        int      reg_idx = i / 32;
        int      bit_idx = i % 32;
        uint32_t mask    = 1U << bit_idx;
        irq_pending |= ((enabled_pending[reg_idx] & mask) != 0) && (plic->regs.priority[i] > 0) &&
                       (plic->regs.priority[i] > plic->regs.threshold[context]);
    }

    return irq_pending;
}

static int get_claim_id(plic_t *plic, int context) {
    uint32_t enabled_pending[PLIC_INTERRUPT_WORDS];
    int      id = 0;

    for (int i = 0; i < PLIC_INTERRUPT_WORDS; i++) {
        enabled_pending[i] = plic->regs.pending[i] & plic->regs.enable[context][i];
    }

    for (int i = 0; i < PLIC_MAX_INTERRUPT; i++) {
        int      reg_idx = i / 32;
        int      bit_idx = i % 32;
        uint32_t mask    = 1U << bit_idx;
        if ((enabled_pending[reg_idx] & mask) != 0) {
            if (plic->regs.priority[i] > 0 && plic->regs.priority[i] > plic->regs.threshold[context] &&
                plic->regs.priority[i] > plic->regs.priority[id]) {
                id = i;
            }
        }
    }

    return id;
}

//---------------------------------------------------------
// Main function
//---------------------------------------------------------

void plic_reset(plic_t *plic) {
    plic->MEIP = false;
    plic->SEIP = false;
    memset(&plic->regs, 0, sizeof(plic->regs));
}

void plic_init(plic_t *plic, uint64_t base) {
    plic->base = base;
    plic_reset(plic);
}

int plic_write(plic_t *plic, uint64_t addr, size_t size, const void *data) {
    reg_info_t info;

    info = addr_decode(plic, addr, size);

    switch (info.kind) {
    case R_PRIORITY: {
        memcpy(&plic->regs.priority[info.index], data, 4);
        break;
    }
    case R_PENDING: {
        // interrupt pending is RO
        break;
    }
    case R_ENABLE: {
        memcpy(&plic->regs.enable[info.context][info.index], data, 4);
        break;
    }
    case R_THRESHOLD: {
        memcpy(&plic->regs.threshold[info.index], data, 4);
        break;
    }
    case R_CLAIM: {
        int value;
        memcpy(&value, data, 4);
        // write claim/complete register will complete the interrupt
        plic_complete(plic, value);
        break;
    }
    case R_INVALID: {
        LOG_ERROR("PLIC: Invalid address access: 0x%lx", addr);
        return -1;
    }
    }

    // Restore Source 0
    plic->regs.priority[0] = 0;
    for (int i = 0; i < PLIC_MAX_CONTEXT; i++) {
        plic->regs.enable[i][0] &= ~1U;
    }

    return 0;
}

int plic_read(plic_t *plic, uint64_t addr, size_t size, void *data) {
    reg_info_t info;
    uint32_t   value;

    info = addr_decode(plic, addr, size);

    switch (info.kind) {
    case R_PRIORITY: {
        value = plic->regs.priority[info.index];
        break;
    }
    case R_PENDING: {
        value = plic->regs.pending[info.index];
        break;
    }
    case R_ENABLE: {
        value = plic->regs.enable[info.context][info.index];
        break;
    }
    case R_THRESHOLD: {
        value = plic->regs.threshold[info.index];
        break;
    }
    case R_CLAIM: {
        // read claim register will return the claimed interrupt id
        value = (uint32_t)get_claim_id(plic, info.context);
        // It should also clear the corresponding pending bit
        plic_claim(plic, (int)value);
        break;
    }
    case R_INVALID: {
        LOG_ERROR("PLIC: Invalid address access: 0x%lx", addr);
        return -1;
    }
    }
    memcpy(data, &value, 4);
    return 0;
}

void plic_irq_update(plic_t *plic, bool *irq) {
    for (int i = 0; i < PLIC_MAX_INTERRUPT; i++) {
        plic_gateway_irq(plic, i, irq[i]);
    }
}

bool plic_meip(plic_t *plic) {
    return interrupt_per_context(plic, 0);
}

bool plic_seip(plic_t *plic) {
    return interrupt_per_context(plic, 1);
}
