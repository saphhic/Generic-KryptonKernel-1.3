.intel_syntax noprefix
.code32

.section .text.entry, "ax"
.global kernel_entry
.extern kernel_main
.extern __bss_start
.extern __bss_end

.equ VGA_TEXT_BASE, 0xB8000

kernel_entry:
    cli

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov al, 'K'
    out 0xE9, al
    mov word ptr [VGA_TEXT_BASE], 0x0F4B

    lea esp, [boot_stack_top]

    xor eax, eax
    lea edi, [__bss_start]
    lea ecx, [__bss_end]
    sub ecx, edi
    cld
    rep stosb

    lea edi, [idt]
    lea eax, [fault_handler]
    mov ecx, 256

setup_idt:
    mov word ptr [edi], ax
    mov word ptr [edi + 2], 0x0008
    mov byte ptr [edi + 4], 0x00
    mov byte ptr [edi + 5], 0x8E
    mov edx, eax
    shr edx, 16
    mov word ptr [edi + 6], dx
    add edi, 8
    loop setup_idt

    lidt [idt_descriptor]

    mov al, 'M'
    out 0xE9, al
    mov word ptr [VGA_TEXT_BASE + 2], 0x0F4D

    /* Remap PIC: IRQ0-7 -> INT 0x20-0x27, IRQ8-15 -> INT 0x28-0x2F */
    mov al, 0x11
    out 0x20, al
    out 0xA0, al

    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al

    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al

    mov al, 0x01
    out 0x21, al
    out 0xA1, al

    /* Mask TODAS las IRQs - el kernel usa polling, no interrupciones */
    mov al, 0xFF
    out 0x21, al
    mov al, 0xFF
    out 0xA1, al

    /* NO hacer sti - el login y el desktop usan polling puro */

    call kernel_main

halt:
    cli
    hlt
    jmp halt

fault_handler:
    mov al, 'X'
    out 0xE9, al
    mov word ptr [VGA_TEXT_BASE + 4], 0x4F58
    jmp halt

.section .rodata, "a"
idt_descriptor:
    .word idt_end - idt - 1
    .long idt

.section .bss, "aw", @nobits
.align 16
idt:
    .space 256 * 8
idt_end:

.align 16
boot_stack:
    .space 16384
boot_stack_top:
