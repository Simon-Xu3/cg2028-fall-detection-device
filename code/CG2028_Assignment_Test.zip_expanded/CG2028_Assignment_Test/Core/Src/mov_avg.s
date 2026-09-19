/*
 * mov_avg.s
 *
 * CG2028 Assignment starter file.
 */
.syntax unified
.cpu cortex-m4
.thumb
.global ewma_filter
.type ewma_filter, %function

.text
.align 2

@ CG2028 Assignment
@ (c) ECE NUS
@ Write Student 1's Name here: Xu Ruijie (A0334914A)
@ Write Student 2's Name here: Lyu Jinhan (A0330854E)
@
@ Function prototype:
@   int ewma_filter(int new_data, int old_output, int alpha_percent);
@
@ ARM calling convention:
@   R0 = new_data       (signed integer sensor sample)
@   R1 = old_output     (previous filtered output)
@   R2 = alpha_percent  (integer from 0 to 100)
@   Return R0 = (alpha_percent * new_data
@                + (100 - alpha_percent) * old_output) / 100
@
@ Notes:
@ - Use signed integer arithmetic.
@ - Integer division must truncate towards zero, matching C integer division.
@ - Preserve all callee-saved registers that you use (R4-R11).
@ - Do not call a C helper function and do not use floating-point instructions.
@
@ Register table:
@   R0 new_data
@   R1 old_output
@   R2 alpha_percent
@   R3 = constant and divisor 100
@   R4 = alpha_percent * new_data
@   R5 = 100 - alpha_percent
@   R6 = (100 - alpha_percent) * old_output
@   R7 = numerator before division
@
@ Write your program from here.
ewma_filter:
    PUSH {r4-r7, lr}

    MOVS r3, #100
    MUL r4, r0, r2
    SUB r5, r3, r2
    MUL r6, r5, r1
    ADD r7, r4, r6
    SDIV r0, r7, r3


    POP  {r4-r7, pc}

.size ewma_filter, .-ewma_filter
