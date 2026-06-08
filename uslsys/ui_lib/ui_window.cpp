// written by saphhic.
// ui_window.cpp — Implementación del sistema de ventanas modernas de Lawai UI

#include "ui_window.h"
#include "ui_primitives.h"
#include "ui_font.h"
#include "ui_theme.h"
#include "../../KryptonKernel/drivers/frameb/framebuffer.hpp"

// ============================================================
// ESTADO GLOBAL DEL SISTEMA
// ============================================================

static UiWindow s_windows[UI_WIN_MAX];
static int      s_z_order[UI_WIN_MAX];  // índices ordenados de fondo a frente
static int      s_win_count = 0;        // slots usados
static int      s_focused   = -1;
static int      s_screen_w  = 1024;
static int      s_screen_h  = 768;
static int      s_top_bar   = UI_TOPBAR_H;
static int      s_bot_bar   = UI_TASKBAR_H;
static bool     s_initialized = false;

// Drag
static bool s_dragging     = false;
static int  s_drag_id      = -1;
static int  s_drag_off_x   = 0;
static int  s_drag_off_y   = 0;

// Resize
static bool s_resizing     = false;
static int  s_resize_id    = -1;
static int  s_resize_orig_x = 0;
static int  s_resize_orig_y = 0;
static int  s_resize_orig_w = 0;
static int  s_resize_orig_h = 0;

// Hover de botones (para efectos visuales)
static int  s_hover_id     = -1;
static UiWinHit s_hover_hit = UI_HIT_NONE;

// ============================================================
// HELPERS INTERNOS
// ============================================================

static inline int ui__min2(int a, int b) { return a < b ? a : b; }
static inline int ui__max2(int a, int b) { return a > b ? a : b; }
static inline int ui__clamp2(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static inline bool ui__valid(int id) {
    return id >= 0 && id < UI_WIN_MAX && s_windows[id].active;
}
static inline bool ui__visible(int id) {
    return ui__valid(id) && s_windows[id].state != UI_WIN_CLOSED
           && s_windows[id].state != UI_WIN_MINIMIZED;
}

// Área de trabajo (sin topbar ni taskbar)
static UiRect s_work_area() {
    UiRect r;
    r.x = 0;
    r.y = s_top_bar;
    r.w = s_screen_w;
    r.h = s_screen_h - s_top_bar - s_bot_bar;
    return r;
}

// Clamping de rect para que no salga de la pantalla
static void s_clamp_rect(UiRect* r) {
    if (!r) return;
    int min_w = UI_WIN_MIN_W;
    int min_h = UI_WIN_MIN_H;
    int max_w = s_screen_w - 8;
    int max_h = s_screen_h - s_top_bar - s_bot_bar - 4;

    if (r->w < min_w) r->w = min_w;
    if (r->h < min_h) r->h = min_h;
    if (r->w > max_w) r->w = max_w;
    if (r->h > max_h) r->h = max_h;

    int min_x = 2;
    int min_y = s_top_bar + 2;
    int max_x = s_screen_w - 20;
    int max_y = s_screen_h - s_bot_bar - 20;

    if (r->x < min_x) r->x = min_x;
    if (r->y < min_y) r->y = min_y;
    if (r->x > max_x) r->x = max_x;
    if (r->y > max_y) r->y = max_y;
}

// Posiciones de los tres botones circulares en la titlebar
static inline int s_btn_cx(int id, int btn_idx) {
    // btn_idx: 0=close, 1=min, 2=max  (izquierda a derecha)
    return s_windows[id].rect.x + UI_BTN_MARGIN_LEFT
           + btn_idx * (UI_BTN_DIAMETER + UI_BTN_GAP)
           + UI_BTN_RADIUS;
}
static inline int s_btn_cy(int id) {
    return s_windows[id].rect.y + UI_BTN_MARGIN_TOP + UI_BTN_RADIUS;
}

// Devuelve true si (px,py) está dentro del círculo del botón
static bool s_btn_hit(int cx, int cy, int px, int py) {
    int dx = px - cx, dy = py - cy;
    return (dx * dx + dy * dy) <= (UI_BTN_RADIUS + 2) * (UI_BTN_RADIUS + 2);
}

// Recalcular z_order (simple insertion sort por campo .z)
static void s_rebuild_z_order() {
    int count = 0;
    for (int i = 0; i < UI_WIN_MAX; i++) {
        if (s_windows[i].active) s_z_order[count++] = i;
    }
    // Ordenar de menor a mayor z (fondo → frente)
    for (int i = 1; i < count; i++) {
        int key = s_z_order[i];
        int j = i - 1;
        while (j >= 0 && s_windows[s_z_order[j]].z > s_windows[key].z) {
            s_z_order[j + 1] = s_z_order[j];
            j--;
        }
        s_z_order[j + 1] = key;
    }
    s_win_count = count;
}

// Asignar el mayor z+1 a una ventana (traer al frente)
static void s_push_to_front(int id) {
    int max_z = 0;
    for (int i = 0; i < UI_WIN_MAX; i++) {
        if (s_windows[i].active && i != id && s_windows[i].z > max_z)
            max_z = s_windows[i].z;
    }
    s_windows[id].z = max_z + 1;
    s_rebuild_z_order();
}

// ============================================================
// INICIALIZACIÓN
// ============================================================

void ui_win_system_init(int screen_w, int screen_h,
                        int top_bar_h, int bottom_bar_h) {
    s_screen_w = screen_w;
    s_screen_h = screen_h;
    s_top_bar  = top_bar_h;
    s_bot_bar  = bottom_bar_h;
    s_focused  = -1;
    s_dragging = false;
    s_resizing = false;
    s_win_count = 0;

    for (int i = 0; i < UI_WIN_MAX; i++) {
        s_windows[i].active = false;
        s_windows[i].state  = UI_WIN_CLOSED;
        s_windows[i].z      = 0;
        s_windows[i].name[0] = '\0';
        s_z_order[i] = -1;
    }
    s_initialized = true;
}

void ui_win_system_set_screen(int screen_w, int screen_h) {
    s_screen_w = screen_w;
    s_screen_h = screen_h;
    for (int i = 0; i < UI_WIN_MAX; i++) {
        if (!s_windows[i].active) continue;
        if (s_windows[i].state == UI_WIN_MAXIMIZED) {
            UiRect wa = s_work_area();
            s_windows[i].rect = wa;
        } else {
            s_clamp_rect(&s_windows[i].rect);
            s_clamp_rect(&s_windows[i].restore_rect);
        }
    }
}

// ============================================================
// CICLO DE VIDA
// ============================================================

int ui_window_create(const char* name, int x, int y, int w, int h) {
    if (!s_initialized) return -1;

    // Buscar slot libre
    int slot = -1;
    for (int i = 0; i < UI_WIN_MAX; i++) {
        if (!s_windows[i].active) { slot = i; break; }
    }
    if (slot == -1) return -1;

    UiWindow* win = &s_windows[slot];
    win->active = true;
    win->state  = UI_WIN_NORMAL;
    win->z      = 0;
    win->focused = false;

    // Copiar nombre
    int ni = 0;
    if (name) {
        while (name[ni] && ni < UI_WIN_NAME_MAX - 1) {
            win->name[ni] = name[ni]; ni++;
        }
    }
    win->name[ni] = '\0';

    // Tamaño y posición
    win->rect.w = w > UI_WIN_MIN_W ? w : UI_WIN_MIN_W;
    win->rect.h = h > UI_WIN_MIN_H ? h : UI_WIN_MIN_H;

    if (x == -1 || y == -1) {
        // Centrar en el área de trabajo
        win->rect.x = (s_screen_w - win->rect.w) / 2;
        win->rect.y = s_top_bar + (s_screen_h - s_top_bar - s_bot_bar - win->rect.h) / 2;
    } else {
        // Cascade leve: desplazar por índice
        int cascade = slot * 24;
        win->rect.x = x + cascade;
        win->rect.y = y + cascade;
    }
    s_clamp_rect(&win->rect);
    win->restore_rect = win->rect;

    s_push_to_front(slot);
    ui_window_set_focus(slot);
    return slot;
}

void ui_window_destroy(int id) {
    if (!ui__valid(id)) return;
    s_windows[id].active = false;
    s_windows[id].state  = UI_WIN_CLOSED;
    if (s_focused == id) s_focused = -1;
    if (s_drag_id == id) { s_dragging = false; s_drag_id = -1; }
    if (s_resize_id == id) { s_resizing = false; s_resize_id = -1; }
    s_rebuild_z_order();
}

void ui_window_show(int id) {
    if (!ui__valid(id)) return;
    s_windows[id].state = UI_WIN_NORMAL;
    s_push_to_front(id);
    ui_window_set_focus(id);
}

void ui_window_hide(int id) {
    if (!ui__valid(id)) return;
    s_windows[id].state = UI_WIN_MINIMIZED;
    if (s_focused == id) s_focused = -1;
}

void ui_window_minimize(int id) { ui_window_hide(id); }

void ui_window_restore(int id) {
    if (!ui__valid(id)) return;
    if (s_windows[id].state == UI_WIN_MAXIMIZED) {
        s_windows[id].rect = s_windows[id].restore_rect;
        s_clamp_rect(&s_windows[id].rect);
    }
    s_windows[id].state = UI_WIN_NORMAL;
    s_push_to_front(id);
    ui_window_set_focus(id);
}

void ui_window_maximize(int id) {
    if (!ui__valid(id)) return;
    if (s_windows[id].state != UI_WIN_MAXIMIZED) {
        s_windows[id].restore_rect = s_windows[id].rect;
    }
    UiRect wa = s_work_area();
    s_windows[id].rect  = wa;
    s_windows[id].state = UI_WIN_MAXIMIZED;
    s_push_to_front(id);
    ui_window_set_focus(id);
}

void ui_window_toggle_maximize(int id) {
    if (!ui__valid(id)) return;
    if (s_windows[id].state == UI_WIN_MAXIMIZED) ui_window_restore(id);
    else ui_window_maximize(id);
}

void ui_window_raise(int id) {
    if (!ui__valid(id)) return;
    if (s_windows[id].state == UI_WIN_MINIMIZED)
        s_windows[id].state = UI_WIN_NORMAL;
    s_push_to_front(id);
    ui_window_set_focus(id);
}

void ui_window_set_focus(int id) {
    if (s_focused != -1 && s_focused != id && ui__valid(s_focused))
        s_windows[s_focused].focused = false;
    s_focused = id;
    if (ui__valid(id)) s_windows[id].focused = true;
}

int ui_window_focused() { return s_focused; }

// ============================================================
// INFORMACIÓN
// ============================================================

bool ui_window_is_open(int id) {
    return ui__valid(id) && s_windows[id].state != UI_WIN_CLOSED
           && s_windows[id].state != UI_WIN_MINIMIZED;
}
bool ui_window_is_minimized(int id) {
    return ui__valid(id) && s_windows[id].state == UI_WIN_MINIMIZED;
}
bool ui_window_is_maximized(int id) {
    return ui__valid(id) && s_windows[id].state == UI_WIN_MAXIMIZED;
}
UiRect ui_window_rect(int id) {
    if (!ui__valid(id)) return {0,0,0,0};
    return s_windows[id].rect;
}
UiRect ui_window_body_rect(int id) {
    if (!ui__valid(id)) return {0,0,0,0};
    UiRect r = s_windows[id].rect;
    r.y += UI_WIN_TITLE_H;
    r.h -= UI_WIN_TITLE_H;
    if (r.h < 0) r.h = 0;
    return r;
}
UiWinState ui_window_state(int id) {
    if (!ui__valid(id)) return UI_WIN_CLOSED;
    return s_windows[id].state;
}
const char* ui_window_name(int id) {
    if (!ui__valid(id)) return "";
    return s_windows[id].name;
}
int ui_window_open_count() {
    int cnt = 0;
    for (int i = 0; i < UI_WIN_MAX; i++)
        if (s_windows[i].active && s_windows[i].state == UI_WIN_NORMAL) cnt++;
    return cnt;
}
UiWindow* ui_window_get(int id) {
    if (!ui__valid(id)) return nullptr;
    return &s_windows[id];
}

// ============================================================
// RENDERIZADO DE UNA VENTANA
// ============================================================

void ui_window_draw(int id) {
    if (!ui__visible(id)) return;
    if (!framebuffer_available()) return;

    UiWindow* win = &s_windows[id];
    int x = win->rect.x, y = win->rect.y;
    int w = win->rect.w, h = win->rect.h;
    bool focused = win->focused;
    bool maximized = (win->state == UI_WIN_MAXIMIZED);

    int radius = maximized ? 0 : UI_WIN_RADIUS;

    // ── Sombra ──────────────────────────────────────────────
    if (!maximized && focused) {
        ui_draw_shadow(x, y, w, h, radius, 2, 4, 6, 100);
    } else if (!maximized) {
        ui_draw_shadow(x, y, w, h, radius, 1, 2, 3, 50);
    }

    // ── Cuerpo de la ventana ────────────────────────────────
    uint32_t body_color = UI_C_WIN_BODY;
    ui_draw_rounded_rect(x, y, w, h, radius, body_color);

    // ── Borde exterior ──────────────────────────────────────
    uint32_t border_color = focused ? UI_C_WIN_BORDER_ACT : UI_C_WIN_BORDER;
    ui_draw_rounded_rect_outline(x, y, w, h, radius, border_color, 1);

    // ── Titlebar ────────────────────────────────────────────
    // Degradado sutil: negro profundo arriba → ligeramente más claro abajo
    uint32_t title_top = focused ? UI_C_WIN_TITLE_BG : UI_C_WIN_TITLE_BG_INV;
    uint32_t title_bot = focused ? 0xFF1A1A22u : 0xFF1E1E26u;

    // Sólo la parte superior con el radio de las esquinas de arriba
    // Dibujamos en dos partes: arcos arriba + rect abajo hasta UI_WIN_TITLE_H
    ui_draw_rounded_rect_vgrad(x, y, w, UI_WIN_TITLE_H, radius, title_top, title_bot);

    // Línea separadora titlebar / body
    draw_rect(x + 1, y + UI_WIN_TITLE_H - 1, w - 2, 1,
              focused ? UI_C_WIN_BORDER_ACT : UI_C_WIN_BORDER);

    // ── Botones circulares ────────────────────────────────
    // 0: close (rojo), 1: min (amarillo), 2: max (verde)
    static const uint32_t BTN_COLORS[3]     = { UI_C_BTN_CLOSE, UI_C_BTN_MIN, UI_C_BTN_MAX };
    static const uint32_t BTN_COLORS_HOV[3] = { UI_C_BTN_CLOSE_HOV, UI_C_BTN_MIN_HOV, UI_C_BTN_MAX_HOV };

    bool hover_this = (s_hover_id == id);
    int bcy = s_btn_cy(id);

    for (int bi = 0; bi < 3; bi++) {
        int bcx = s_btn_cx(id, bi);
        bool hov = hover_this && (
            (bi == 0 && s_hover_hit == UI_HIT_BTN_CLOSE) ||
            (bi == 1 && s_hover_hit == UI_HIT_BTN_MIN)   ||
            (bi == 2 && s_hover_hit == UI_HIT_BTN_MAX)
        );

        uint32_t c = hov ? BTN_COLORS_HOV[bi] : BTN_COLORS[bi];

        // Sombra interior del botón
        draw_filled_circle(bcx + 1, bcy + 1, UI_BTN_RADIUS - 1, ui_color_darken(c, 80));
        draw_filled_circle(bcx, bcy, UI_BTN_RADIUS, c);

        // Brillo sutil en la parte superior del botón
        uint32_t highlight = ui_color_lighten(c, 60);
        draw_filled_circle(bcx, bcy - 1, UI_BTN_RADIUS - 3, highlight);
        draw_filled_circle(bcx, bcy, UI_BTN_RADIUS - 3, c);

        // Icono si está en hover
        if (hov || hover_this) {
            uint32_t icon_c = ui_color_darken(c, 120);
            int s = UI_BTN_DIAMETER / 2 - 1;
            if (bi == 0) ui_icon_close(bcx, bcy, s, icon_c);
            else if (bi == 1) ui_icon_minimize(bcx, bcy, s, icon_c);
            else {
                if (win->state == UI_WIN_MAXIMIZED)
                    ui_icon_restore(bcx, bcy, s, icon_c);
                else
                    ui_icon_maximize(bcx, bcy, s, icon_c);
            }
        }
    }

    // ── Título de la ventana ─────────────────────────────────
    // Centrado en la titlebar, entre los botones y el borde derecho
    int title_x = x + UI_BTN_MARGIN_LEFT + 3 * (UI_BTN_DIAMETER + UI_BTN_GAP) + 8;
    int title_area_w = w - (title_x - x) - 12;
    int title_text_y = y + (UI_WIN_TITLE_H - UI_FONT16_H) / 2;
    if (title_text_y < y + 2) title_text_y = y + 2;

    uint32_t title_text_color = focused ? UI_C_WIN_TITLE_TEXT : UI_C_WIN_TITLE_TEXT_IN;
    ui_font16_print_ellipsis(title_x, title_text_y, win->name,
                             title_text_color, 0, true, title_area_w);

    // ── Indicador de resize (esquina inferior derecha) ──────
    if (!maximized) {
        int rx = x + w - 12, ry = y + h - 12;
        for (int i = 2; i < 8; i++) {
            put_pixel(rx + i, ry + 7, UI_C_WIN_BORDER_ACT);
            put_pixel(rx + 7, ry + i, UI_C_WIN_BORDER_ACT);
            put_pixel(rx + i + 2, ry + 7 + 2, UI_C_SURFACE_ELEVATED);
        }
    }
}

// Dibujar todas las ventanas en orden Z
void ui_window_draw_all() {
    s_rebuild_z_order();
    for (int i = 0; i < s_win_count; i++) {
        int id = s_z_order[i];
        if (id >= 0 && id < UI_WIN_MAX) ui_window_draw(id);
    }
}

// ============================================================
// HIT TEST
// ============================================================

int ui_window_hit_test(int mx, int my, UiWinHit* out_hit) {
    if (out_hit) *out_hit = UI_HIT_NONE;

    // Iterar de frente a fondo (z_order al revés)
    for (int i = s_win_count - 1; i >= 0; i--) {
        int id = s_z_order[i];
        if (!ui__visible(id)) continue;

        UiRect r = s_windows[id].rect;

        // ¿está dentro del rect?
        if (mx < r.x || my < r.y || mx >= r.x + r.w || my >= r.y + r.h) continue;

        // ¿botones?
        int bcy = s_btn_cy(id);
        if (s_btn_hit(s_btn_cx(id, 0), bcy, mx, my)) {
            if (out_hit) *out_hit = UI_HIT_BTN_CLOSE; return id;
        }
        if (s_btn_hit(s_btn_cx(id, 1), bcy, mx, my)) {
            if (out_hit) *out_hit = UI_HIT_BTN_MIN;   return id;
        }
        if (s_btn_hit(s_btn_cx(id, 2), bcy, mx, my)) {
            if (out_hit) *out_hit = UI_HIT_BTN_MAX;   return id;
        }

        // ¿resize grip (esquina BR)?
        if (!ui_window_is_maximized(id)) {
            if (mx >= r.x + r.w - 16 && my >= r.y + r.h - 16) {
                if (out_hit) *out_hit = UI_HIT_RESIZE_BR; return id;
            }
        }

        // ¿titlebar?
        if (my < r.y + UI_WIN_TITLE_H) {
            if (out_hit) *out_hit = UI_HIT_TITLE; return id;
        }

        // Body
        if (out_hit) *out_hit = UI_HIT_BODY;
        return id;
    }
    return -1;
}

// ============================================================
// MANEJO DE MOUSE
// ============================================================

bool ui_window_handle_mouse(int mx, int my, bool left_held) {
    // Actualizar hover para efectos visuales
    UiWinHit hover_hit = UI_HIT_NONE;
    int hover_id = ui_window_hit_test(mx, my, &hover_hit);
    s_hover_id  = hover_id;
    s_hover_hit = hover_hit;

    // Drag
    if (s_dragging) {
        if (left_held) {
            int nx = mx - s_drag_off_x;
            int ny = my - s_drag_off_y;
            ui_window_move(s_drag_id, nx, ny);
        } else {
            s_dragging = false;
            s_drag_id  = -1;
        }
        return true;
    }

    // Resize
    if (s_resizing) {
        if (left_held) {
            int new_w = s_resize_orig_w + (mx - s_resize_orig_x);
            int new_h = s_resize_orig_h + (my - s_resize_orig_y);
            ui_window_resize(s_resize_id, new_w, new_h);
        } else {
            s_resizing  = false;
            s_resize_id = -1;
        }
        return true;
    }

    // Iniciar drag/resize en mouse-down sobre titlebar o resize grip
    if (left_held && !s_dragging && !s_resizing) {
        UiWinHit hit = UI_HIT_NONE;
        int id = ui_window_hit_test(mx, my, &hit);
        if (id >= 0) {
            if (hit == UI_HIT_TITLE && !ui_window_is_maximized(id)) {
                // Empezar drag
                s_dragging    = true;
                s_drag_id     = id;
                s_drag_off_x  = mx - s_windows[id].rect.x;
                s_drag_off_y  = my - s_windows[id].rect.y;
                ui_window_raise(id);
                return true;
            }
            if (hit == UI_HIT_RESIZE_BR) {
                s_resizing      = true;
                s_resize_id     = id;
                s_resize_orig_x = mx;
                s_resize_orig_y = my;
                s_resize_orig_w = s_windows[id].rect.w;
                s_resize_orig_h = s_windows[id].rect.h;
                ui_window_raise(id);
                return true;
            }
            // Cualquier click en una ventana la trae al frente
            if (hit != UI_HIT_NONE) {
                ui_window_raise(id);
                return true;
            }
        }
    }

    return hover_id >= 0;
}

UiWinHit ui_window_handle_click(int mx, int my) {
    // Solo procesar botones en mouse-up (no en drag)
    if (s_dragging || s_resizing) return UI_HIT_NONE;

    UiWinHit hit = UI_HIT_NONE;
    int id = ui_window_hit_test(mx, my, &hit);
    if (id < 0) return UI_HIT_NONE;

    switch (hit) {
        case UI_HIT_BTN_CLOSE: ui_window_destroy(id); break;
        case UI_HIT_BTN_MIN:   ui_window_minimize(id); break;
        case UI_HIT_BTN_MAX:   ui_window_toggle_maximize(id); break;
        default: break;
    }
    return hit;
}

// ============================================================
// MOVE / RESIZE
// ============================================================

void ui_window_move(int id, int x, int y) {
    if (!ui__valid(id) || ui_window_is_maximized(id)) return;
    s_windows[id].rect.x = x;
    s_windows[id].rect.y = y;
    s_clamp_rect(&s_windows[id].rect);
    s_windows[id].restore_rect = s_windows[id].rect;
}

void ui_window_resize(int id, int w, int h) {
    if (!ui__valid(id) || ui_window_is_maximized(id)) return;
    s_windows[id].rect.w = w;
    s_windows[id].rect.h = h;
    s_clamp_rect(&s_windows[id].rect);
    s_windows[id].restore_rect = s_windows[id].rect;
}