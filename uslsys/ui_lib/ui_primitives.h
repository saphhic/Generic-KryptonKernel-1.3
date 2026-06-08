// written by saphhic.
// ui_primitives.h — Primitivas de Renderizado Extendidas para Lawai UI
// Complementa el framebuffer base con shapes modernas: esquinas redondeadas,
// sombras, degradados y glifos de iconos vectoriales simples.

#ifndef LAWAI_UI_PRIMITIVES_H
#define LAWAI_UI_PRIMITIVES_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// ROUNDED RECTANGLE — esquinas redondeadas rellenas
// ============================================================

// Dibuja un rectángulo relleno con esquinas redondeadas.
// radius: radio de la esquina en píxeles (0 = rect normal)
void ui_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);

// Dibuja solo el borde de un rounded rect
void ui_draw_rounded_rect_outline(int x, int y, int w, int h, int radius,
                                  uint32_t color, int border_w);

// Rounded rect con degradado vertical (top_color → bottom_color)
void ui_draw_rounded_rect_vgrad(int x, int y, int w, int h, int radius,
                                uint32_t top_color, uint32_t bottom_color);

// ============================================================
// SOMBRA (fake drop shadow)
// ============================================================

// Dibuja una sombra rectangular semitransparente desplazada.
// offset_x, offset_y: desplazamiento; spread: expansión; alpha: 0..255
void ui_draw_shadow(int x, int y, int w, int h, int radius,
                    int offset_x, int offset_y, int spread, uint8_t alpha);

// ============================================================
// TEXTO ALINEADO
// ============================================================

// Dibuja texto centrado horizontalmente dentro de [x, x+w]
void ui_draw_text_centered(int x, int y, int area_w, const char* text,
                           uint32_t color, int scale);

// Dibuja texto alineado a la derecha dentro de [x, x+w]
void ui_draw_text_right(int x, int y, int area_w, const char* text,
                        uint32_t color, int scale);

// Dibuja texto con elipsis si supera max_chars
void ui_draw_text_ellipsis(int x, int y, const char* text, uint32_t color,
                           int scale, int max_chars);

// ============================================================
// ICONOS VECTORIALES SIMPLES (draw con líneas/rects primitivos)
// Tamaño canónico: 16x16 píxeles, escalable con 'scale'
// ============================================================

// × para el botón cerrar
void ui_icon_close(int cx, int cy, int size, uint32_t color);

// − para el botón minimizar
void ui_icon_minimize(int cx, int cy, int size, uint32_t color);

// □ para el botón maximizar
void ui_icon_maximize(int cx, int cy, int size, uint32_t color);

// ❐ para el botón restaurar (dos rectángulos superpuestos)
void ui_icon_restore(int cx, int cy, int size, uint32_t color);

// ▶ flecha derecha
void ui_icon_arrow_right(int cx, int cy, int size, uint32_t color);

// ☰ hamburger menu
void ui_icon_menu(int cx, int cy, int size, uint32_t color);

// ⬡ icono genérico de app (hexágono simple)
void ui_icon_app_default(int cx, int cy, int size, uint32_t fg, uint32_t bg);

// ⌘ Terminal (>_)
void ui_icon_terminal(int cx, int cy, int size, uint32_t color);

// 📁 Files (carpeta)
void ui_icon_folder(int cx, int cy, int size, uint32_t color);

// ⚙ Settings (engranaje simplificado — círculo con puntitos)
void ui_icon_settings(int cx, int cy, int size, uint32_t color);

// ℹ System info
void ui_icon_system(int cx, int cy, int size, uint32_t color);

// ============================================================
// CURSOR DEL MOUSE
// ============================================================

// Dibuja el cursor (flecha clásica de 11x18 píxeles)
void ui_draw_cursor(int x, int y);

// ============================================================
// SEPARADOR VISUAL
// ============================================================

// Línea horizontal de separación con fade en los extremos
void ui_draw_separator_h(int x, int y, int w, uint32_t color);

// Línea vertical
void ui_draw_separator_v(int x, int y, int h, uint32_t color);

// ============================================================
// BADGE / INDICADOR
// ============================================================

// Punto indicador circular (e.g. ventana activa en taskbar)
void ui_draw_indicator_dot(int cx, int cy, int radius, uint32_t color);

// Barra indicadora horizontal centrada bajo un área
void ui_draw_indicator_bar(int cx, int y, int bar_w, int bar_h, uint32_t color);

// ============================================================
// HELPER INTERNO: kstrlen seguro (sin dependencia de stdlib)
// ============================================================
static inline int ui_strlen(const char* s) {
    if (!s) return 0;
    int n = 0;
    while (s[n]) n++;
    return n;
}

#endif // LAWAI_UI_PRIMITIVES_H