.section .text.init

.globl _start
_start:

# Initialize gp
/* Load the small-data base defined by the linker script. Disable relaxation
   here because setting gp must not depend on the old value of gp. */
.option push
.option norelax
    la gp, __global_pointer$
.option pop

# Initialize sp
    la sp, __stack_top

# Clear bss
    la t0, __bss_start
    la t1, __bss_end
    beq t0, t1, 2f
1:
    sb x0, 0(t0)
    addi t0, t0, 1
    bne t0, t1, 1b
2:

# call trap init function
    call mtrap_init

# Call main function
    call main

// write to syscon with poweroff command to indicate test end
    li t0, 0x00100000
    li t1, 0x1
    sw t1, 0(t0)

# placeholder for trap init function
.weak mtrap_init
.type mtrap_init, @function
mtrap_init:
    ret
