// written by saphhic.
// ui_window.h — Sistema de Ventanas Modernas para Lawai UI
//
// Uso básico:
//   int id = ui_window_create("Mi App", 100, 100, 600, 400);
//   ui_window_draw(id);
//
// Dentro del body de la ventana usas las primitivas normales:
//   WindowRect body = ui_window_body_rect(id);
//   draw_rect(body.x + 10, body.y + 10, 200, 30, 0xFF334455u);
//
// Manejo de entrada:
//   ui_window_handle_mouse(mouse_x, mouse_y, mouse_left_down);
//   ui_window_handle_click(mouse_x, mouse_y); // en mouse-up

#ifndef LAWAI_UI_WINDOW_H
#define LAWAI_UI_WINDOW_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_theme.h"

// ============================================================
// CONSTANTES
// ============================================================

#define UI_WIN_MAX          12        // máximo de ventanas simultáneas
#define UI_WIN_NAME_MAX     48        // longitud máxima del nombre

// ============================================================
// TIPOS
// ============================================================

struct UiRect {
    int x, y, w, h;
};

enum UiWinState {
    UI_WIN_CLOSED    = 0,
    UI_WIN_NORMAL    = 1,
    UI_WIN_MINIMIZED = 2,
    UI_WIN_MAXIMIZED = 3,
};

// Resultado de un hit-test de ventana
enum UiWinHit {
    UI_HIT_NONE      = 0,
    UI_HIT_BODY,          // área de contenido
    UI_HIT_TITLE,         // barra de título (drag)
    UI_HIT_BTN_CLOSE,     // botón ×
    UI_HIT_BTN_MIN,       // botón −
    UI_HIT_BTN_MAX,       // botón □ / ❐
    UI_HIT_RESIZE_BR,     // esquina inferior derecha (resize)
};

// Descriptor de una ventana
struct UiWindow {
    bool     active;
    char     name[UI_WIN_NAME_MAX];
    UiRect   rect;         // posición y tamaño actuales
    UiRect   restore_rect; // rect guardado antes de maximizar
    UiWinState state;
    bool     focused;      // tiene el foco del teclado/mouse
    int      z;            // orden Z (mayor = arriba)
};

// ============================================================
// SISTEMA (inicialización)
// ============================================================

// Inicializar el sistema de ventanas.
// screen_w, screen_h: resolución del framebuffer
// top_bar_h, bottom_bar_h: reserva para barras del sistema
void ui_win_system_init(int screen_w, int screen_h,
                        int top_bar_h, int bottom_bar_h);

// Notificar cambio de resolución
void ui_win_system_set_screen(int screen_w, int screen_h);

// ============================================================
// CICLO DE VIDA DE VENTANAS
// ============================================================

// Crear una nueva ventana. Devuelve su id (0..UI_WIN_MAX-1) o -1 si falla.
// x, y: posición inicial. w, h: tamaño.
// Pasa x=-1, y=-1 para centrarla automáticamente en pantalla.
int  ui_window_create(const char* name, int x, int y, int w, int h);

// Destruir definitivamente una ventana (libera el slot)
void ui_window_destroy(int id);

// Mostrar / ocultar sin destruir
void ui_window_show(int id);
void ui_window_hide(int id);   // equivale a minimizar sin animación

// Minimizar / restaurar / maximizar
void ui_window_minimize(int id);
void ui_window_restore(int id);
void ui_window_maximize(int id);
void ui_window_toggle_maximize(int id);

// Traer al frente
void ui_window_raise(int id);

// Foco de teclado
void ui_window_set_focus(int id);
int  ui_window_focused();  // id de la ventana con foco, -1 si ninguna

// ============================================================
// INFORMACIÓN
// ============================================================

bool        ui_window_is_open(int id);
bool        ui_window_is_minimized(int id);
bool        ui_window_is_maximized(int id);
UiRect      ui_window_rect(int id);          // rect completo (incluye titlebar)
UiRect      ui_window_body_rect(int id);     // rect del área de contenido
UiWinState  ui_window_state(int id);
const char* ui_window_name(int id);

// Número de ventanas abiertas y visibles
int ui_window_open_count();

// ============================================================
// RENDERIZADO
// ============================================================

// Dibuja una sola ventana (marco + titlebar + sombra).
// El contenido del body lo dibuja el caller usando ui_window_body_rect().
void ui_window_draw(int id);

// Dibuja todas las ventanas visibles en orden Z correcto.
void ui_window_draw_all();

// ============================================================
// MANEJO DE MOUSE
// ============================================================

// Llamar cada frame con la posición del cursor y estado del botón izquierdo.
// Maneja drag automáticamente.
// Devuelve true si el mouse está sobre alguna ventana (consume el evento).
bool ui_window_handle_mouse(int mx, int my, bool left_held);

// Llamar en mouse-up para procesar clicks en botones.
// Devuelve el UiWinHit que se procesó (puede ser UI_HIT_NONE).
UiWinHit ui_window_handle_click(int mx, int my);

// Hit-test: qué parte de qué ventana está bajo (mx, my).
// Devuelve id de ventana o -1. *out_hit recibe el tipo.
int ui_window_hit_test(int mx, int my, UiWinHit* out_hit);

// ============================================================
// UTILIDADES
// ============================================================

// Reposicionar una ventana (respeta límites de pantalla)
void ui_window_move(int id, int x, int y);

// Redimensionar (respeta mínimos)
void ui_window_resize(int id, int w, int h);

// Acceso al array raw (para casos avanzados)
UiWindow* ui_window_get(int id);

#endif // LAWAI_UI_WINDOW_H