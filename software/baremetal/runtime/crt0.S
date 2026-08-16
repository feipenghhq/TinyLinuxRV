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
    beq t0, t1, 2f
2:

# Call main function
    call main

# ebreak to exit
    ebreak
