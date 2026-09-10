
// assuming 3 interrupt source - 1, 2, 3
// assuming 2 context - 0, 1

#include <assert.h>
#include <stdint.h>

#include "plic.h"

#define MAX_INT 4
#define MAX_CNT 2

typedef struct {
    int priority[MAX_INT];
    int enable[MAX_CNT];
    int threshold[MAX_CNT];
} cfg_t;

static plic_t *plic;

static void set_priority(int id, int priority) {
    plic_write(plic, (uint64_t)(0x4 * id), 4, &priority);
}

// set for all the 3 sources
static void set_enable(int enable, int context) {
    plic_write(plic, (uint64_t)(0x2000 + 0x80 * context), 4, &enable);
}

static void set_threshold(int threshold, int context) {
    plic_write(plic, (uint64_t)(0x200000 + 0x1000 * context), 4, &threshold);
}

static int claim(int context) {
    int id;
    plic_read(plic, (uint64_t)(0x200004 + 0x1000 * context), 4, &id);
    return id;
}

static void complete(int id, int context) {
    plic_write(plic, (uint64_t)(0x200004 + 0x1000 * context), 4, &id);
}

static int read_pending(void) {
    int value;
    plic_read(plic, 0x1000, 4, &value);
    return value;
}

static void plic_config(cfg_t cfg) {
    for (int i = 0; i < MAX_INT; i++) {
        set_priority(i, cfg.priority[i]);
    }
    for (int i = 0; i < MAX_CNT; i++) {
        set_enable(cfg.enable[i], i);
        set_threshold(cfg.threshold[i], i);
    }
}

/**
 * Test basic interrupt flow
 * Covers the following:
 *  - basic register accessing flow
 *  - pending bit should be set after getting IRQ
 *  - MEIP/SEIP should not be set after reset without any configuration
 *  - MEIP/SEIP should not be set when enable is not set
 *  - claim and complete process
 */
static void test_basic_flow(void) {
    cfg_t cfg     = {.priority = {1, 1}, .enable = {0}, .threshold = {0}};
    bool  irq[11] = {false};

    irq[1] = true;

    plic_init(plic, 0);
    // everything should be 0 after reset
    assert(read_pending() == 0);  // pending should be 0
    assert(plic_meip(plic) == 0); // meip should be 0
    assert(plic_seip(plic) == 0); // seip should be 0

    // set interrupt
    plic_irq_update(plic, irq);

    // check pending bit, MEIP, SEIP.
    assert(read_pending() == 2);  // pending should be set
    assert(plic_meip(plic) == 0); // meip should be 0
    assert(plic_seip(plic) == 0); // seip should be 0

    // now set config
    plic_config(cfg);

    // check meip/seip. should not be set as enable is 0
    assert(plic_meip(plic) == 0); // meip should be 0
    assert(plic_seip(plic) == 0); // seip should be 0

    cfg.enable[0] = 0xE;
    cfg.enable[1] = 0xE;
    plic_config(cfg);

    // check meip/seip. should be set now
    assert(plic_meip(plic) == 1); // meip should be 1
    assert(plic_seip(plic) == 1); // seip should be 1

    // claim the interrupt in context 0
    assert(claim(0) == 1);
    // interrupt should be reset
    assert(read_pending() == 0);  // pending should be 0
    assert(plic_meip(plic) == 0); // meip should be 0
    assert(plic_seip(plic) == 0); // seip should be 0

    // complete the interrupt in context 0
    complete(1, 0);

    // set same interrupt again
    plic_irq_update(plic, irq);
    // check pending bit, MEIP, SEIP, should be set again
    assert(read_pending() == 2);  // pending should be set
    assert(plic_meip(plic) == 1); // meip should be 0
    assert(plic_seip(plic) == 1); // seip should be 0
}

/**
 * Test priority and threshold
 * Covers the following:
 *  - set priority to 0 disable interrupt
 *  - if priority < threshold, it should not generate ip.
 */
static void test_priority(void) {
    cfg_t cfg     = {0};
    bool  irq[11] = {false};

    cfg.enable[0] = 0xE;
    cfg.enable[1] = 0xE;

    irq[1] = true;
    irq[2] = true;

    plic_init(plic, 0);
    plic_config(cfg);

    // set interrupt
    plic_irq_update(plic, irq);

    // pending should be set but meip/seip should be 0 because priority is 0
    assert(read_pending() == 6);
    assert(plic_meip(plic) == 0);
    assert(plic_seip(plic) == 0);

    // update the priority and threshold
    cfg.priority[1]  = 10;
    cfg.priority[2]  = 5;
    cfg.threshold[0] = 5;
    cfg.threshold[1] = 10;
    plic_config(cfg);

    // MEIP should set but SEIP should not be set
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);
}

/**
 * Test claim and complete
 * Covers the following:
 * - Claim should return correct interrupt id
 * - Complete should complete an interrupt
 */
static void test_claim(void) {
    cfg_t cfg     = {.priority = {0, 10, 10, 5}, .enable = {0xE, 0}, .threshold = {1, 10}};
    bool  irq[11] = {false};

    irq[1] = true;
    irq[2] = true;
    irq[3] = true;

    plic_init(plic, 0);
    plic_config(cfg);
    plic_irq_update(plic, irq);

    // sanity check
    assert(read_pending() == 14);
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);

    // claim context 0, should return id 1
    assert(claim(0) == 1);
    assert(read_pending() == 12);
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);
    complete(1, 0);

    // claim context 0, should return id 2
    assert(claim(0) == 2);
    assert(read_pending() == 8);
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);
    complete(2, 0);

    // claim context 1, should return id 0 as the enable is set to 0
    assert(claim(1) == 0);

    // Now enable context 1
    cfg.enable[1] = 0xE;
    plic_config(cfg);
    // claim context 1, should return id 3
    assert(claim(1) == 3);
    assert(read_pending() == 0);
    assert(plic_meip(plic) == 0);
    assert(plic_seip(plic) == 0);
    complete(3, 1);

    // claim context 0, should return id 0
    assert(claim(0) == 0);
    // claim context 1, should return id 0
    assert(claim(1) == 0);

    // assert irq again
    plic_irq_update(plic, irq);
    assert(read_pending() == 14);
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);
}

/**
 * Test claim and complete
 * Covers the following:
 *  - source 0 should not trigger pending/interrupt
 *  - ignore invalid complete ID
 *  - ignore not enabled complete
 *  - level interrupt should not re-trigger pending before complete
 *  - reset values
 */
static void test_misc(void) {
    cfg_t cfg     = {.priority = {0, 10, 10, 10}, .enable = {0xE, 0}, .threshold = {1, 1}};
    bool  irq[11] = {false};

    irq[0] = true;

    plic_init(plic, 0);
    plic_config(cfg);
    plic_irq_update(plic, irq);

    // source 0 should not trigger pending/interrupt
    assert(read_pending() == 0);
    assert(plic_meip(plic) == 0);
    assert(plic_seip(plic) == 0);

    irq[1] = true;
    irq[2] = true;
    irq[3] = true;
    plic_irq_update(plic, irq);
    assert(read_pending() == 0xE);
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);

    // ignore invalid complete ID
    complete(15, 0);
    assert(read_pending() == 0xE);
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);

    // ignore not enabled complete
    complete(1, 1);
    assert(read_pending() == 0xE);
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);

    //level interrupt should not re-trigger pending before complete
    assert(claim(0) == 1);
    assert(read_pending() == 0xC);
    plic_irq_update(plic, irq);
    assert(read_pending() == 0xC);

    // reset values
    plic_reset(plic);
    plic_config(cfg);
    plic_irq_update(plic, irq);
    assert(read_pending() == 0xE);
    assert(plic_meip(plic) == 1);
    assert(plic_seip(plic) == 0);
}

int main(void) {
    plic_t _plic;
    plic = &_plic;

    test_basic_flow();
    test_priority();
    test_claim();
    test_misc();

    return 0;
}
