// written by saphhic.
// ui_terminal_window.cpp — Implementación del terminal de texto en ventana

#include "ui_terminal_window.h"
#include "ui_font.h"
#include "ui_theme.h"
#include "ui_primitives.h" // Para draw_rect
#include "../../KryptonKernel/drivers/frameb/framebuffer.hpp"
#include "../../KryptonKernel/hal/userf/kmalloc.h" // Para kmalloc
#include "fb_externcmds.h" // Para la ejecución de comandos externos (está en la misma carpeta)

// Definiciones de colores para la terminal (estilo moderno oscuro)
#define UI_C_TERMINAL_BG     0xFF0A0A0Fu
#define UI_C_TERMINAL_TEXT   0xFFE0E0E0u
#define UI_C_TERMINAL_CURSOR 0xFF707070u

// --- Estado Interno del Terminal ---
struct UiTerminal {
    int window_id;
    char buffer[TERM_MAX_LINES][TERM_MAX_COLS];
    int cursor_x;
    int cursor_y;
    char input_buffer[TERM_INPUT_BUF_SIZE];
    int input_len;
    bool active; // ¿Está activa esta instancia de terminal?
};

static UiTerminal s_terminals[UI_WIN_MAX]; // Una terminal por cada posible ventana

// --- Declaraciones adelantadas para funciones auxiliares internas ---
static void term_scroll_up(int term_idx);
static void term_put_char(int term_idx, char c);
static void term_print_str(int term_idx, const char* str);
static void term_execute_command(int term_idx, const char* command);
static void term_output_callback(const char* str); // Callback para extrcmds

// --- Variable global para almacenar el índice de la terminal actual para el callback ---
static int s_current_callback_term_idx = -1;

// --- Implementación ---

void ui_terminal_init_system() {
    for (int i = 0; i < UI_WIN_MAX; ++i) {
        s_terminals[i].active = false;
    }
}

int ui_terminal_create(const char* name, int x, int y, int w, int h) {
    int win_id = ui_window_create(name, x, y, w, h);
    if (win_id == -1) {
        return -1;
    }
    
    // Usamos el win_id directamente como índice para asociar la terminal a la ventana
    UiTerminal* term = &s_terminals[win_id];
    term->window_id = win_id;
    term->active = true;
    term->cursor_x = 0;
    term->cursor_y = 0;
    term->input_len = 0;

    // Limpiar el buffer de la terminal
    for (int i = 0; i < TERM_MAX_LINES; ++i) {
        for (int j = 0; j < TERM_MAX_COLS; ++j) {
            term->buffer[i][j] = ' ';
        }
    }
    term->input_buffer[0] = '\0';

    term_print_str(win_id, "KryptonOS Terminal v0.1\n");
    term_print_str(win_id, "Escribe 'help' para ver los comandos.\n");
    term_print_str(win_id, "> ");

    return win_id;
}

void ui_terminal_draw(int window_id) {
    if (window_id < 0 || window_id >= UI_WIN_MAX || !s_terminals[window_id].active) {
        return;
    }

    UiTerminal* term = &s_terminals[window_id];
    UiRect body = ui_window_body_rect(window_id);

    // Limpiar el área del cuerpo de la terminal
    draw_rect(body.x, body.y, body.w, body.h, UI_C_TERMINAL_BG);

    // Calcular líneas y columnas visibles basadas en las dimensiones del cuerpo y el tamaño de la fuente
    int visible_cols = body.w / UI_FONT16_W;
    int visible_lines = body.h / UI_FONT16_H;

    // Dibujar el contenido del buffer de la terminal
    for (int i = 0; i < visible_lines && i < TERM_MAX_LINES; ++i) {
        for (int j = 0; j < visible_cols && j < TERM_MAX_COLS; ++j) {
            char c = term->buffer[i][j];
            // Solo dibujar si no es un espacio para optimizar, o si el fondo no es transparente
            ui_font16_draw_char(body.x + j * UI_FONT16_W,
                                body.y + i * UI_FONT16_H,
                                c, UI_C_TERMINAL_TEXT, UI_C_TERMINAL_BG, false);
        }
    }

    // Dibujar el cursor si la ventana tiene el foco
    if (ui_window_focused() == window_id) {
        draw_rect(body.x + term->cursor_x * UI_FONT16_W,
                  body.y + term->cursor_y * UI_FONT16_H,
                  UI_FONT16_W, UI_FONT16_H, UI_C_TERMINAL_CURSOR);
    }
}

void ui_terminal_handle_key(int window_id, char key) {
    if (window_id < 0 || window_id >= UI_WIN_MAX || !s_terminals[window_id].active) {
        return;
    }

    UiTerminal* term = &s_terminals[window_id];

    if (key == '\b') { // Retroceso
        if (term->input_len > 0) {
            term->input_len--;
            term->input_buffer[term->input_len] = '\0';
            if (term->cursor_x > 2) { // No borrar el prompt "> "
                term->cursor_x--;
                term->buffer[term->cursor_y][term->cursor_x] = ' ';
            }
        }
    } else if (key == '\n' || key == '\r') { // Enter
        term_put_char(window_id, '\n'); // Mover a la siguiente línea
        term_execute_command(window_id, term->input_buffer);
        term->input_len = 0;
        term->input_buffer[0] = '\0';
        term_print_str(window_id, "> "); // Nuevo prompt
    } else if (term->input_len < TERM_INPUT_BUF_SIZE - 1) {
        term->input_buffer[term->input_len++] = key;
        term->input_buffer[term->input_len] = '\0';
        term_put_char(window_id, key);
    }
}

// --- Implementación de funciones auxiliares internas ---

static void term_scroll_up(int term_idx) {
    for (int i = 1; i < TERM_MAX_LINES; ++i) {
        for (int j = 0; j < TERM_MAX_COLS; ++j) {
            s_terminals[term_idx].buffer[i - 1][j] = s_terminals[term_idx].buffer[i][j];
        }
    }
    // Limpiar la última línea
    for (int j = 0; j < TERM_MAX_COLS; ++j) {
        s_terminals[term_idx].buffer[TERM_MAX_LINES - 1][j] = ' ';
    }
    s_terminals[term_idx].cursor_y = TERM_MAX_LINES - 1;
}

static void term_put_char(int term_idx, char c) {
    UiTerminal* term = &s_terminals[term_idx];

    if (c == '\f') { // Señal de limpieza (Form Feed)
        for (int i = 0; i < TERM_MAX_LINES; i++) {
            for (int j = 0; j < TERM_MAX_COLS; j++) {
                term->buffer[i][j] = ' ';
            }
        }
        term->cursor_x = 0;
        term->cursor_y = 0;
        return;
    }

    if (c == '\n' || c == '\r') {
        term->cursor_x = 0;
        term->cursor_y++;
    } else if (c == '\b') {
        if (term->cursor_x > 0) {
            term->cursor_x--;
            term->buffer[term->cursor_y][term->cursor_x] = ' ';
        } else if (term->cursor_y > 0) {
            term->cursor_y--;
            term->cursor_x = TERM_MAX_COLS - 1;
            term->buffer[term->cursor_y][term->cursor_x] = ' ';
        }
    } else {
        term->buffer[term->cursor_y][term->cursor_x] = c;
        term->cursor_x++;
    }

    if (term->cursor_x >= TERM_MAX_COLS) {
        term->cursor_x = 0;
        term->cursor_y++;
    }

    if (term->cursor_y >= TERM_MAX_LINES) {
        term_scroll_up(term_idx);
    }
}

static void term_print_str(int term_idx, const char* str) {
    if (!str) return;
    while (*str) {
        term_put_char(term_idx, *str++);
    }
}

// Este es el callback "traductor" para extrcmds
static void term_output_callback(const char* str) {
    if (s_current_callback_term_idx != -1) {
        term_print_str(s_current_callback_term_idx, str);
    }
}

static void term_execute_command(int term_idx, const char* command) {
    term_print_str(term_idx, "Ejecutando: ");
    term_print_str(term_idx, command);
    term_print_str(term_idx, "\n");

    // Establecer el índice de la terminal global para el callback
    s_current_callback_term_idx = term_idx;

    // Llamar al ejecutor de comandos externo
    extern_cmd_execute(command, term_output_callback);

    s_current_callback_term_idx = -1; // Resetear después de la ejecución
}