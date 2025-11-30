/* linker.ld - Linker script for the kernel */
ENTRY(kmain) /* Define the entry point symbol */
OUTPUT_FORMAT(elf32-i386) /* Specify output format */

KERNEL_LOAD_ADDR = 0x8000; /* Match the address where the bootloader loads the kernel */

SECTIONS {
    . = KERNEL_LOAD_ADDR; /* Set the current location counter */

    .text : {
        *(.text) /* Place all code sections */
    }

    .rodata : {
        *(.rodata*) /* Place all read-only data sections */
    }

    .data : {
        *(.data) /* Place all initialized data sections */
    }

    .bss : {
        *(.bss) /* Place all uninitialized data sections */
    }

    /DISCARD/ : {
        *(.comment) /* Discard comments, stack info, etc. */
        *(.eh_frame)
    }
}