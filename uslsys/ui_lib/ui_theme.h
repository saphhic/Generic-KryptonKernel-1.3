// written by saphhic.
// ui_theme.h — Lawai UI Design System
// Paleta y constantes visuales para el escritorio moderno.
// Inspiración: GNOME Libadwaita, KDE Breeze Dark, Pop!_OS

#ifndef LAWAI_UI_THEME_H
#define LAWAI_UI_THEME_H

#include <stdint.h>

// ============================================================
// PALETA DE COLORES — formato 0xAARRGGBB
// ============================================================

// Fondo del escritorio
#define UI_C_DESKTOP_BG        0xFF0F0F14u   // casi negro azulado
#define UI_C_DESKTOP_BG2       0xFF161620u   // variante

// Barra superior (panel system)
#define UI_C_TOPBAR_BG         0xFF0A0A0Fu   // negro profundo
#define UI_C_TOPBAR_TEXT       0xFFE8E8ECu
#define UI_C_TOPBAR_ACCENT     0xFF6C71C4u   // violeta suave (Solarized-like)
#define UI_C_TOPBAR_SEPARATOR  0xFF2A2A35u

// Taskbar inferior (dock)
#define UI_C_TASKBAR_BG        0xE8121218u   // semitransparente oscuro
#define UI_C_TASKBAR_ITEM      0xFF1E1E2Au
#define UI_C_TASKBAR_ITEM_HOV  0xFF2A2A3Au
#define UI_C_TASKBAR_ITEM_ACT  0xFF373750u
#define UI_C_TASKBAR_INDICATOR 0xFF6C71C4u   // punto activo bajo el icono
#define UI_C_TASKBAR_BORDER    0xFF2E2E3Eu

// Ventanas — titlebar
#define UI_C_WIN_TITLE_BG      0xFF111116u   // negro elegante
#define UI_C_WIN_TITLE_BG_INV  0xFF181820u   // ventana inactiva
#define UI_C_WIN_TITLE_TEXT    0xFFEAEAEEu
#define UI_C_WIN_TITLE_TEXT_IN 0xFF6A6A7Au   // texto inactivo
#define UI_C_WIN_BORDER        0xFF2C2C3Cu
#define UI_C_WIN_BORDER_ACT    0xFF5A5F8Au   // borde ventana activa
#define UI_C_WIN_BODY          0xFF1A1A24u
#define UI_C_WIN_SHADOW        0x60000000u   // sombra (semitransparente)

// Botones de ventana (tipo macOS pero estilo oscuro)
#define UI_C_BTN_CLOSE         0xFFEC6A5Eu   // rojo coral
#define UI_C_BTN_CLOSE_HOV     0xFFFF8078u
#define UI_C_BTN_MIN           0xFFF4BF4Fu   // amarillo
#define UI_C_BTN_MIN_HOV       0xFFFFD068u
#define UI_C_BTN_MAX           0xFF61C554u   // verde
#define UI_C_BTN_MAX_HOV       0xFF7ADF6Cu

// Colores de acento del sistema
#define UI_C_ACCENT            0xFF6C71C4u   // violeta principal
#define UI_C_ACCENT2           0xFF268BD2u   // azul secundario
#define UI_C_ACCENT_HOVER      0xFF8287D4u
#define UI_C_ACCENT_PRESS      0xFF5055A0u

// Texto
#define UI_C_TEXT_PRIMARY      0xFFEAEAEEu
#define UI_C_TEXT_SECONDARY    0xFFAAAAAEu
#define UI_C_TEXT_DISABLED     0xFF606068u
#define UI_C_TEXT_INVERSE      0xFF0A0A0Fu

// Paneles y superficies
#define UI_C_SURFACE_0         0xFF0F0F14u
#define UI_C_SURFACE_1         0xFF161620u
#define UI_C_SURFACE_2         0xFF1E1E2Au
#define UI_C_SURFACE_3         0xFF252534u
#define UI_C_SURFACE_ELEVATED  0xFF2A2A38u

// Cursor del mouse
#define UI_C_CURSOR            0xFFFFFFFFu
#define UI_C_CURSOR_SHADOW     0x80000000u

// ============================================================
// DIMENSIONES DEL SISTEMA
// ============================================================

// Barras
#define UI_TOPBAR_H            28    // altura barra superior
#define UI_TASKBAR_H           60    // altura taskbar inferior
#define UI_TASKBAR_ICON_SZ     42    // tamaño del icono en el dock
#define UI_TASKBAR_ICON_GAP    10    // separación entre iconos
#define UI_TASKBAR_ICON_PAD    9     // padding interno del icono
#define UI_TASKBAR_INDICATOR_H 3     // altura del indicador bajo el icono activo
#define UI_TASKBAR_MAX_ICONS   8     // máximo de iconos en el dock

// Ventanas
#define UI_WIN_TITLE_H         32    // altura de la titlebar
#define UI_WIN_RADIUS          10    // radio de esquinas redondeadas
#define UI_WIN_BORDER_W        1     // grosor del borde
#define UI_WIN_SHADOW_OFFSET   4     // desplazamiento de sombra
#define UI_WIN_SHADOW_SPREAD   8     // expansión de sombra
#define UI_WIN_MIN_W           260
#define UI_WIN_MIN_H           160

// Botones de control de ventana
#define UI_BTN_RADIUS          7     // radio (círculo de 14px diámetro)
#define UI_BTN_DIAMETER        14
#define UI_BTN_GAP             8     // separación entre botones
#define UI_BTN_MARGIN_LEFT     12    // margen desde el borde izquierdo
#define UI_BTN_MARGIN_TOP      9     // margen desde la parte superior del título

// Font
#define UI_FONT_W              8     // ancho del glyph (font8x8)
#define UI_FONT_H              8     // alto del glyph
#define UI_FONT_SCALE_SM       1     // escala pequeña
#define UI_FONT_SCALE_MD       2     // escala media
#define UI_FONT_SCALE_LG       3     // escala grande

// ============================================================
// MACROS DE LAYOUT HELPER
// ============================================================

// Centro horizontal de un texto de N caracteres a escala S dentro de un ancho W
#define UI_TEXT_CENTER_X(text_len, scale, area_w) \
    (((area_w) - (text_len) * UI_FONT_W * (scale)) / 2)

// Centro vertical de texto a escala S dentro de una altura H
#define UI_TEXT_CENTER_Y(scale, area_h) \
    (((area_h) - UI_FONT_H * (scale)) / 2)

// ============================================================
// HELPER DE COLOR — interpolación simple 0..255
// ============================================================
static inline uint32_t ui_color_lerp(uint32_t a, uint32_t b, uint8_t t) {
    uint8_t ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    uint8_t br = (b >> 16) & 0xFF, bg_ = (b >> 8) & 0xFF, bb = b & 0xFF;
    uint8_t rr = (uint8_t)(ar + (int)(br - ar) * t / 255);
    uint8_t rg = (uint8_t)(ag + (int)(bg_ - ag) * t / 255);
    uint8_t rb = (uint8_t)(ab + (int)(bb - ab) * t / 255);
    return 0xFF000000u | ((uint32_t)rr << 16) | ((uint32_t)rg << 8) | rb;
}

// Oscurecer un color en un factor 0..255
static inline uint32_t ui_color_darken(uint32_t c, uint8_t amount) {
    return ui_color_lerp(c, 0xFF000000u, amount);
}

// Aclarar un color en un factor 0..255
static inline uint32_t ui_color_lighten(uint32_t c, uint8_t amount) {
    return ui_color_lerp(c, 0xFFFFFFFFu, amount);
}

#endif // LAWAI_UI_THEME_H