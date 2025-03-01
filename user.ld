/*
 * -------------------------------------------------------
 * 커널이 사용할 메모리 공간에 대한 명세
 * -------------------------------------------------------
 */

ENTRY(start)

SECTIONS {
    . = 0x1000000;

    /* machine code */
    .text :{
        KEEP(*(.text.start));
        *(.text .text.*);
    }

    /* read-only data */
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    /* data section */
    .data : ALIGN(4) {
        *(.data .data.*);
    }

    /* bss section */
    .bss : ALIGN(4) {
        *(.bss .bss.* .sbss .sbss.*);

        . = ALIGN(16);
        . += 64 * 1024;      /* 64KB */
        __stack_top = .;

        ASSERT(. < 0x1800000, "too large executable");
    }
}
