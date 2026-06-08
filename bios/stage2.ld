OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x8000;

    .text :
    {
        *(.text*)
        *(.rodata*)
        *(.data*)
    }

    /DISCARD/ :
    {
        *(.bss*)
        *(COMMON)
        *(.comment*)
        *(.note*)
        *(.eh_frame*)
    }
}
