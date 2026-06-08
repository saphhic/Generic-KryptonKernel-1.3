// written by saphhic.
// ui_font.h — Lawai UI Font System
// Proporciona una fuente 16×16 bold moderna para la UI del escritorio.
// La fuente está definida como bitmaps de 2 bytes por fila × 16 filas.
// Se puede usar junto con la font8x8 existente eligiendo el tamaño adecuado.

#ifndef LAWAI_UI_FONT_H
#define LAWAI_UI_FONT_H

#include <stdint.h>

// ============================================================
// TAMAÑOS DISPONIBLES
// ============================================================
// FONT_8x8  — font8x8_basic existente (uso en terminales, escala 1)
// FONT_16x16 — esta fuente, para títulos y UI
// FONT_8x8_2X — font8x8 con scale=2 (mismo resultado que 16x16 pero menos sharp)

#define UI_FONT16_W    16
#define UI_FONT16_H    16
#define UI_FONT16_FIRST 0x20   // espacio (32)
#define UI_FONT16_LAST  0x7E   // ~ (126)
#define UI_FONT16_COUNT (UI_FONT16_LAST - UI_FONT16_FIRST + 1)

// Cada carácter: 16 filas × 2 bytes (uint16_t, MSB izquierda)
extern const uint16_t ui_font16[UI_FONT16_COUNT][UI_FONT16_H];

// ============================================================
// API DE DIBUJADO
// ============================================================

// Dibuja un carácter 16×16 en (px, py)
// transparent_bg: si true, el fondo no se dibuja (transparente)
void ui_font16_draw_char(int px, int py, char c, uint32_t fg, uint32_t bg,
                         bool transparent_bg);

// Dibuja una cadena de texto en (x, y)
void ui_font16_print(int x, int y, const char* str, uint32_t fg, uint32_t bg,
                     bool transparent_bg);

// Dibuja texto centrado horizontalmente dentro del área [x, x+area_w]
void ui_font16_print_centered(int x, int y, int area_w, const char* str,
                              uint32_t fg, uint32_t bg, bool transparent_bg);

// Ancho en píxeles de una cadena
int ui_font16_text_width(const char* str);

// Dibuja texto con elipsis si supera max_pixels de ancho
void ui_font16_print_ellipsis(int x, int y, const char* str, uint32_t fg,
                              uint32_t bg, bool transparent_bg, int max_pixels);

#endif // LAWAI_UI_FONT_H