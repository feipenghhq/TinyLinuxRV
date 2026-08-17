#include <stdint.h>

// Global variables
static int test_data_array[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static int test_bss_array[32];

int small_data0 = 0x0eefcafe;
int small_data1;

/**
 * Check global data with initial value
 */
static int test_data(void) {

    // check the initial value
    for (int i = 0; i < 16; i++) {
        if (test_data_array[i] != i) {
            return -1;
        }
    }

    // check access
    for (int i = 0; i < 16; i++) {
        test_data_array[i] = i * 12;
    }
    for (int i = 0; i < 16; i++) {
        if (test_data_array[i] != i * 12) {
            return -1;
        }
    }

    return 0;
}

/**
 * Make sure crt0.S can clear BSS
 */
static int test_bss(void) {

    // check the initial value
    for (int i = 0; i < 32; i++) {
        if (test_bss_array[i] != 0) {
            return -1;
        }
    }

    // check access
    for (int i = 0; i < 32; i++) {
        test_bss_array[i] = i * 8;
    }
    for (int i = 0; i < 32; i++) {
        if (test_bss_array[i] != i * 8) {
            return -1;
        }
    }

    return 0;
}

/**
 * test .sdata, .sbss and gp
 */
static int test_small_data(void) {
    if (small_data0 != 0x0eefcafe) {
        return -1;
    }

    if (small_data1 != 0) {
        return -1;
    }

    small_data0 = 0x12345678;
    small_data1 = 0x00A0FFFF;

    if (small_data0 != 0x12345678) {
        return -1;
    }

    if (small_data1 != 0x00A0FFFF) {
        return -1;
    }

    return 0;
}

/**
 * Test stack is initialized and the local variable can be used
 */
static int test_stack(void) {
    int volatile array[256];
    int sum = 0;

    for (int i = 0; i < 256; i++) {
        array[i] = i;
    }

    for (int i = 0; i < 256; i++) {
        sum += array[i];
    }

    if (sum != 32640) {
        return -1;
    }

    return 0;
}

/**
 * Test function call
 */

static int add(int a, int b) {
    return a + b;
}

static int add3(int a, int b) {
    int r;
    r = add(a, b);
    return r + 3;
}

static int add12num(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11) {
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11;
}

static int test_function(void) {
    int result;
    result = add(1, 2) + add3(3, 4);
    result += add12num(1, 2, 2, 3, 4, 5, 6, 77, 8, 9, 100, 11);

    if (result != 241) {
        return -1;
    }
    return 0;
}

/**
 * Test control
 */
static int test_control(void) {
    int sum = 0;
    int i   = 0;
    while (i < 101) {
        sum += i;
        i++;
    }

    if (sum != 5050) {
        return -1;
    }
    return 0;
}

/**
 * Test pointer
 */

static int test_pointer(void) {
    uint8_t  arr8[8], *ptr8;
    uint16_t arr16[8], *ptr16;
    uint32_t arr32[8], *ptr32;
    uint64_t arr64[8], *ptr64;

    ptr8  = arr8;
    ptr16 = arr16;
    ptr32 = arr32;
    ptr64 = arr64;

    for (int i = 0; i < 8; i++) {
        *ptr8++  = (uint8_t) i * 8;
        *ptr16++ = (uint16_t) i * 16;
        *ptr32++ = (uint32_t) i * 32;
        *ptr64++ = (uint64_t) i * 64;
    }

    for (int i = 0; i < 8; i++) {
        if (arr8[i] != i * 8)
            return -1;
        if (arr16[i] != i * 16)
            return -1;
        if (arr32[i] != (uint32_t) i * 32)
            return -1;
        if (arr64[i] != (uint64_t) i * 64)
            return -1;
    }

    return 0;
}

int main(void) {

    if (test_data() != 0) {
        return 1;
    }

    else if (test_bss() != 0) {
        return 2;
    }

    else if (test_small_data() != 0) {
        return 3;
    }

    else if (test_stack() != 0) {
        return 4;
    }

    else if (test_function() != 0) {
        return 5;
    }

    else if (test_control() != 0) {
        return 6;
    }

    else if (test_pointer() != 0) {
        return 7;
    }

    return 0;
}
