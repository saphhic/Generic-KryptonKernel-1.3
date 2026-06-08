[bits 16]
[org 0x7C00]

section .data

   mode_info_buffer: 
     times 256 db 0

FRAMEBUFFER_ADDR dd 0
FRAMEBUFFER_WIDTH dd 0
FRAMEBUFFER_HEIGHT dd 0
FRAMEBUFFER_PITCH dd 0

section .text
global framebuffer_init_stub

framebuffer_init_stub:
   mov ax, 0x4F02
   mov bx, 0x4118
   int 0x10
   cmp ax, 0x004F
   jne .error

   mov ax, 0x4F01
   mov cx, 0x4118
   mov di, mode_info_buffer
   int 0x10
   cmp ax, 0x004F
   jne .error

   mov eax, [mode_info_buffer + 40]
   mov [FRAMEBUFFER_ADDR], eax
   mov ax, [mode_info_buffer + 16]
   mov [FRAMEBUFFER_PITCH], ax
   mov ax, [mode_info_buffer + 18]
   mov [FRAMEBUFFER_WIDTH], ax
   mov ax, [mode_info_buffer + 32]
   mov [FRAMEBUFFER_HEIGHT], ax

   ret

.error:

   ret