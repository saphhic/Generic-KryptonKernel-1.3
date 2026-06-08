.intel_syntax noprefix
.code32

.section .text, "ax"
.global read_input

read_input:
wait_for_key:
    in al, 0x64
    test al, 1
    jz wait_for_key

    test al, 0x20
    jz read_keyboard_byte
    in al, 0x60
    jmp wait_for_key

read_keyboard_byte:
    in al, 0x60
    ret
