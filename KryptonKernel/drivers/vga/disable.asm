;written by saphhic.

global disable_vga
disable_vga:
cli

mov dx, 0x3C4
mov al, 0x01
out dx, al
inc dx
in al, dx
or al, 0x20
out dx, al

sti
ret