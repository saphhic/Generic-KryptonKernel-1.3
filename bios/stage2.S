#ifndef KERNEL_START_LBA
#define KERNEL_START_LBA 2
#endif

#ifndef KERNEL_SECTORS
#define KERNEL_SECTORS 1
#endif

#ifndef KERNEL_FILE_SIZE
#define KERNEL_FILE_SIZE 512
#endif

#ifndef KERNEL_BUFFER_SEG
#define KERNEL_BUFFER_SEG 0x1000
#endif

#ifndef KERNEL_LOAD_ADDR
#define KERNEL_LOAD_ADDR 0x00010000
#endif

#ifndef KERNEL_ENTRY_ADDR
#define KERNEL_ENTRY_ADDR 0x00100000
#endif

#ifndef FB_INFO_ADDR
#define FB_INFO_ADDR 0x00009000
#endif

#define FB_INFO_MAGIC 0x4246524B

.intel_syntax noprefix
.code16

.section .text, "ax"
.global _start

_start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000

    mov [boot_drive], dl

    mov al, 'S'
    call debug_char

    mov ax, KERNEL_BUFFER_SEG
    mov es, ax
    xor bx, bx
    mov word ptr [sectors_remaining], KERNEL_SECTORS
    mov dword ptr [current_lba], KERNEL_START_LBA
    mov dword ptr [current_lba + 4], 0

    xor ah, ah
    mov dl, [boot_drive]
    int 0x13

read_kernel:
    mov bp, 3

read_kernel_retry:
    mov word ptr [dap_count], 1
    mov word ptr [dap_offset], bx
    mov ax, es
    mov word ptr [dap_segment], ax
    mov eax, [current_lba]
    mov dword ptr [dap_lba], eax
    mov eax, [current_lba + 4]
    mov dword ptr [dap_lba + 4], eax

    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jnc kernel_sector_loaded

    xor ah, ah
    mov dl, [boot_drive]
    int 0x13

    call read_sector_chs
    jnc kernel_sector_loaded

    xor ah, ah
    mov dl, [boot_drive]
    int 0x13

    dec bp
    jnz read_kernel_retry
    jmp disk_error

kernel_sector_loaded:

    mov ax, es
    add ax, 0x20
    mov es, ax

    add dword ptr [current_lba], 1
    adc dword ptr [current_lba + 4], 0

    dec word ptr [sectors_remaining]
    jnz read_kernel

    mov al, 'L'
    call debug_char

    call setup_vbe
    call enable_a20

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    .byte 0x66
    .byte 0xEA
    .long protected_mode_entry
    .word 0x08

disk_error:
    mov al, 'E'
    call debug_char

    mov si, disk_error_message
    call print_string

hang:
    cli
    hlt
    jmp hang

print_string:
    lodsb
    test al, al
    jz print_done

    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print_string

print_done:
    ret

debug_char:
    push ax
    push bx

    out 0xE9, al
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10

    pop bx
    pop ax
    ret

enable_a20:
    in al, 0x92
    or al, 0x02
    and al, 0xFE
    out 0x92, al

    call kbc_wait_input_clear
    mov al, 0xD1
    out 0x64, al

    call kbc_wait_input_clear
    mov al, 0xDF
    out 0x60, al

    call kbc_wait_input_clear
    ret

kbc_wait_input_clear:
    mov cx, 0xFFFF

kbc_wait_input_clear_loop:
    in al, 0x64
    test al, 0x02
    jz kbc_wait_done
    loop kbc_wait_input_clear_loop

kbc_wait_done:
    ret

setup_vbe:
    push ax
    push bx
    push cx
    push dx
    push di
    push es

    mov dword ptr [FB_INFO_ADDR], 0

    mov bx, 0x4144
    call try_vbe_mode
    jnc setup_vbe_done

    mov bx, 0x4143
    call try_vbe_mode
    jnc setup_vbe_done

    mov bx, 0x4142
    call try_vbe_mode
    jnc setup_vbe_done

    mov bx, 0x4118
    call try_vbe_mode

setup_vbe_done:
    pop es
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

try_vbe_mode:
    push ax
    push bx
    push cx
    push dx
    push di
    push es

    mov word ptr [vbe_requested_mode], bx

    xor ax, ax
    mov es, ax
    lea di, [vbe_mode_info]
    mov ax, 0x4F01
    mov cx, bx
    and cx, 0x3FFF
    int 0x10
    cmp ax, 0x004F
    jne try_vbe_mode_failed

    mov ax, word ptr [vbe_mode_info]
    test ax, 0x0001
    jz try_vbe_mode_failed
    test ax, 0x0080
    jz try_vbe_mode_failed
    cmp byte ptr [vbe_mode_info + 25], 32
    jne try_vbe_mode_failed

    mov ax, 0x4F02
    mov bx, word ptr [vbe_requested_mode]
    int 0x10
    cmp ax, 0x004F
    jne try_vbe_mode_failed

    mov dword ptr [FB_INFO_ADDR], FB_INFO_MAGIC

    mov eax, dword ptr [vbe_mode_info + 40]
    mov dword ptr [FB_INFO_ADDR + 4], eax

    movzx eax, word ptr [vbe_mode_info + 18]
    mov dword ptr [FB_INFO_ADDR + 8], eax

    movzx eax, word ptr [vbe_mode_info + 20]
    mov dword ptr [FB_INFO_ADDR + 12], eax

    movzx eax, word ptr [vbe_mode_info + 16]
    mov dword ptr [FB_INFO_ADDR + 16], eax

    movzx eax, byte ptr [vbe_mode_info + 25]
    mov dword ptr [FB_INFO_ADDR + 20], eax

    movzx eax, word ptr [vbe_requested_mode]
    mov dword ptr [FB_INFO_ADDR + 24], eax

    clc
    jmp try_vbe_mode_done

try_vbe_mode_failed:
    mov dword ptr [FB_INFO_ADDR], 0
    stc

try_vbe_mode_done:
    pop es
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

read_sector_chs:
    push ax
    push bx
    push cx
    push dx

    mov ax, word ptr [current_lba]
    xor dx, dx
    mov cx, 18
    div cx
    inc dl
    mov byte ptr [chs_sector], dl

    xor dx, dx
    mov cx, 2
    div cx

    mov ch, al
    mov cl, byte ptr [chs_sector]
    and ah, 0x03
    shl ah, 6
    or cl, ah
    mov dh, dl
    mov dl, [boot_drive]
    mov bx, word ptr [dap_offset]
    mov ah, 0x02
    mov al, 0x01
    int 0x13

    pop dx
    pop cx
    pop bx
    pop ax
    ret

.p2align 3
gdt:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt - 1
    .long gdt

boot_drive:
    .byte 0

chs_sector:
    .byte 0

sectors_remaining:
    .word 0

current_lba:
    .quad 0

.p2align 4
dap:
    .byte 0x10
    .byte 0
dap_count:
    .word 0
dap_offset:
    .word 0
dap_segment:
    .word 0
dap_lba:
    .quad 0

vbe_requested_mode:
    .word 0

.p2align 4
vbe_mode_info:
    .space 256

disk_error_message:
    .asciz "kernel load error"

.code32
protected_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x00090000

    mov al, 'P'
    out 0xE9, al

    cld
    mov esi, KERNEL_LOAD_ADDR
    mov edi, KERNEL_ENTRY_ADDR
    mov ecx, KERNEL_FILE_SIZE
    rep movsb

    mov al, 'J'
    out 0xE9, al

    mov eax, KERNEL_ENTRY_ADDR
    jmp eax
