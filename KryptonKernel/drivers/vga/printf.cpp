//usage printxy("Hello, World!", 10(x pixel), 5(y pixel));
// writen by saphhic.
#include "printf.h"

int cursor_x = 0;
int cursor_y = 0;

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void upd_cursor() {
    uint16_t pos = cursor_y * 80 + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void scroll() {
    volatile uint16_t* vga = (uint16_t*)0xB8000;

    for (int y = 1; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            vga[(y - 1) * 80 + x] = vga[y * 80 + x];
        }
    }

    for (int x = 0; x < 80; x++) {
        vga[24 * 80 + x] = (0x0F << 8) | ' ';
    }

        cursor_y = 24;
        upd_cursor();
}

void printxy(const char* str, int x, int y) {

    volatile char* vga = (char*)0xB8000;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            y++;
            x = 0;
            continue;
        }

        int index = (y * 80 + x) * 2;
        vga[index] = str[i];
        vga[index + 1] = 0x0F;

        x++;
    }
}

void putchar(char c) {
    if (c == '\n') {
        cursor_y++;
        cursor_x = 0;
    } 
    else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            volatile char* vga = (char*)0xB8000;
            int index = (cursor_y * 80 + cursor_x) * 2;
            vga[index] = ' ';
            vga[index + 1] = 0x0F;
        }
    }
    else {
        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
        }

        volatile char* vga = (char*)0xB8000;
        int index = (cursor_y * 80 + cursor_x) * 2;
        vga[index] = c;
        vga[index + 1] = 0x0F;
        cursor_x++;
    }

    if (cursor_y >= 25) {
        scroll();
    }
    else {
        upd_cursor();
    }
}

void printf(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i]);
    }
}

void clear_screen() {
    volatile char* vga = (char*)0xB8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = 0x0F;
    }
    cursor_x = 0;
    cursor_y = 0;
    upd_cursor();
}
