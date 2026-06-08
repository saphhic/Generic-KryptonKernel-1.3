OUTPUT_FORMAT(elf32-i386)
ENTRY(kernel_entry)

SECTIONS
{
    . = 0x00100000;

    .text ALIGN(16) :
    {
        *(.text.entry)
        *(.text*)
    }

    .rodata ALIGN(16) :
    {
        *(.rodata*)
    }

    .data ALIGN(16) :
    {
        *(.data*)
    }

    __bss_start = .;

    .bss ALIGN(16) :
    {
        *(.bss*)
        *(COMMON)
    }

    __bss_end = .;

    /DISCARD/ :
    {
        *(.comment*)
        *(.note*)
        *(.eh_frame*)
    }
}
