// written by saphhic.
// ui_desktop.h — Sistema de Escritorio Moderno para Lawai UI
//
// Comprende tres elementos independientes:
//   1. Wallpaper  — fondo sólido o imagen (bitmap raw 32bpp)
//   2. Topbar     — barra superior con menú, reloj y usuario
//   3. Taskbar    — dock inferior con iconos configurables
//
// Uso mínimo:
//   ui_desktop_init(screen_w, screen_h);
//   ui_taskbar_add_icon("Terminal", UI_APP_TERMINAL, draw_cb_terminal);
//   ui_taskbar_add_icon("Files",    UI_APP_FILES,    draw_cb_files);
//   // -- en el render loop --
//   ui_desktop_draw();
//   ui_topbar_draw(hour, minute, username);
//   ui_taskbar_draw(active_app_id);
//
// Para manejar clicks en el taskbar:
//   int app = ui_taskbar_hit(mouse_x, mouse_y);
//   // app >= 0 → lanzar esa aplicación

#ifndef LAWAI_UI_DESKTOP_H
#define LAWAI_UI_DESKTOP_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_theme.h"

// ============================================================
// WALLPAPER
// ============================================================

// Modo de fondo
enum UiWallpaperMode {
    UI_WALLPAPER_SOLID = 0,  // color sólido
    UI_WALLPAPER_GRADIENT,   // degradado vertical
    UI_WALLPAPER_IMAGE,      // imagen bitmap raw 32bpp ARGB
};

// Configurar fondo de color sólido
void ui_wallpaper_set_color(uint32_t color);

// Configurar fondo degradado vertical
void ui_wallpaper_set_gradient(uint32_t top, uint32_t bottom);

// Configurar fondo de imagen
// data: puntero a buffer de píxeles 32bpp (ARGB), row-major
// img_w, img_h: dimensiones de la imagen
// La imagen se escala (stretch) para cubrir la pantalla completa.
// El buffer debe mantenerse válido mientras se use este wallpaper.
void ui_wallpaper_set_image(const uint32_t* data, int img_w, int img_h);

// Dibujar el fondo (llamar antes que topbar, taskbar y ventanas)
void ui_desktop_draw();

// ============================================================
// TOPBAR (barra superior)
// ============================================================

// Altura importada del tema
#define UI_TOPBAR_HEIGHT  UI_TOPBAR_H

// Dibujar la topbar completa.
// hour, minute: hora actual (0..23, 0..59)
// username: cadena del usuario logado (puede ser nullptr → "user")
void ui_topbar_draw(int hour, int minute, const char* username);

// Zona de click de la topbar:
// Devuelve -1 si no hay hit, 0 si es la zona izquierda (menú), 1 centro, 2 derecha
int ui_topbar_hit(int mx, int my);

// ============================================================
// TASKBAR (dock inferior)
// ============================================================

// Callback que el caller define para dibujar el icono de su app.
// cx, cy: centro del área del icono. size: lado del área (típicamente 26-30px).
// active: si la app está en primer plano.
typedef void (*UiIconDrawCb)(int cx, int cy, int size, bool active);

// Añadir un icono al taskbar.
// app_id: identificador de la aplicación (lo que devuelve ui_taskbar_hit)
// label: etiqueta (tooltip / accesibilidad)
// draw_cb: función de dibujo del icono (nullptr → icono genérico)
// Devuelve el índice del icono o -1 si el taskbar está lleno.
int ui_taskbar_add_icon(int app_id, const char* label, UiIconDrawCb draw_cb);

// Eliminar un icono por índice
void ui_taskbar_remove_icon(int index);

// Limpiar todos los iconos
void ui_taskbar_clear();

// Número actual de iconos
int ui_taskbar_icon_count();

// Dibujar el taskbar completo.
// active_app_id: id de la app activa (se resalta con indicador)
// hover_mx, hover_my: posición del cursor (para hover effects)
void ui_taskbar_draw(int active_app_id, int hover_mx, int hover_my);

// Hit-test del taskbar.
// Devuelve el app_id del icono bajo (mx, my), o -1 si no hay hit.
int ui_taskbar_hit(int mx, int my);

// ============================================================
// INICIALIZACIÓN CONJUNTA
// ============================================================

// Inicializar todo el sistema de escritorio.
// Llama a ui_wallpaper_set_color con el color por defecto.
void ui_desktop_init(int screen_w, int screen_h);

// Notificar cambio de resolución
void ui_desktop_set_screen(int screen_w, int screen_h);

#endif // LAWAI_UI_DESKTOP_H