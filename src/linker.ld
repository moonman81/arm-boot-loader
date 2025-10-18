/* Linker script for Raspberry Pi bootloader */

ENTRY(_start)

SECTIONS
{
    . = 0x80000; /* Load address for bootloader */

    _start = .;
    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    __bss_start = .;
    .bss : {
        *(.bss*)
    }
    __bss_end = .;

    . = ALIGN(4);
    . += 0x1000; /* Stack space */

    _end = .;
    stack_top = .;
}