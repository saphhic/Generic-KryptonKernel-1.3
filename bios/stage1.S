#ifndef STAGE2_SECTORS
#define STAGE2_SECTORS 1
#endif

#ifndef STAGE2_START_LBA
#define STAGE2_START_LBA 1
#endif

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
    mov sp, 0x7C00

    mov [boot_drive], dl

    mov al, '1'
    call debug_char

    mov ax, 0x0800
    mov es, ax
    xor bx, bx
    mov word ptr [sectors_remaining], STAGE2_SECTORS
    mov dword ptr [current_lba], STAGE2_START_LBA
    mov dword ptr [current_lba + 4], 0

    xor ah, ah
    mov dl, [boot_drive]
    int 0x13

read_stage2:
    mov bp, 3

read_stage2_retry:
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
    jnc stage2_sector_loaded

    xor ah, ah
    mov dl, [boot_drive]
    int 0x13

    call read_sector_chs
    jnc stage2_sector_loaded

    xor ah, ah
    mov dl, [boot_drive]
    int 0x13

    dec bp
    jnz read_stage2_retry
    jmp disk_error

stage2_sector_loaded:

    mov ax, es
    add ax, 0x20
    mov es, ax

    add dword ptr [current_lba], 1
    adc dword ptr [current_lba + 4], 0

    dec word ptr [sectors_remaining]
    jnz read_stage2

    mov al, '2'
    call debug_char

    .byte 0xEA
    .word 0x0000
    .word 0x0800

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

disk_error_message:
    .asciz "disk read error"

.org 510
.word 0xAA55
