// written by saphhic.
// ui_terminal_window.h — Terminal de texto para el entorno gráfico de KryptonOS

#ifndef UI_TERMINAL_WINDOW_H
#define UI_TERMINAL_WINDOW_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_window.h" // Para UiRect

#define TERM_MAX_LINES 25
#define TERM_MAX_COLS  80
#define TERM_INPUT_BUF_SIZE 256

void ui_terminal_init_system();
int ui_terminal_create(const char* name, int x, int y, int w, int h);
void ui_terminal_draw(int window_id);
void ui_terminal_handle_key(int window_id, char key);

#endif // UI_TERMINAL_WINDOW_H