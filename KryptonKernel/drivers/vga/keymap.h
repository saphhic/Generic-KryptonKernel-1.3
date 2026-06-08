

// written by saphhic.

#pragma once
#include <stdint.h>
#include <stdbool.h>

enum KeyLayout {
    LAYOUT_EN = 0,
    LAYOUT_ES = 1,
};

enum SpecialKey {
    KEY_NONE = 0,
    
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN,
    KEY_INS, KEY_DEL,

    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
};

void kb_init();

void kb_set_layout(KeyLayout layout);
KeyLayout kb_get_layout();

SpecialKey kb_last_special();

void kb_set_framebuffer_echo(bool enabled);
void kb_queue_scancode(uint8_t scancode);
char* read_line();

char read_key();
bool kb_poll_key(char* out_key, SpecialKey* out_special);
