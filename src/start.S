/* Raspberry Pi Bootloader Entry Point */

.global _start
_start:
    /* Disable interrupts */
    MSR DAIFSET, #0xF

    /* Set up stack pointer */
    LDR X1, =stack_top
    MOV SP, X1

    /* Clear BSS */
    LDR X0, =__bss_start
    LDR X1, =__bss_end
    MOV X2, #0
clear_bss:
    CMP X0, X1
    B.EQ clear_bss_done
    STRB W2, [X0], #1  /* Store 1 byte at a time to avoid overflow */
    B clear_bss

clear_bss_done:
    /* Jump to main */
    BL main

    /* Should not return */
hang:
    WFI
    B hang

/* Exception vectors (minimal) */
.global exception_vector
exception_vector:
    B hang  /* Synchronous */
    B hang  /* IRQ */
    B hang  /* FIQ */
    B hang  /* SError */