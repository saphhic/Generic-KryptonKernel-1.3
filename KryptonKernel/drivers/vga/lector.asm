;written by saphhic.

[BITS 32]
global read_input

read_input:
.loop:
    in al, 0x64
    test al, 1
    jz .loop
    in al, 0x60
    ret