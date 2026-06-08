// written by saphhic.
// ui_desktop.cpp — Implementación del escritorio moderno de Lawai UI

#include "ui_desktop.h"
#include "ui_primitives.h"
#include "ui_font.h"
#include "ui_theme.h"
#include "../../KryptonKernel/drivers/frameb/framebuffer.hpp"

// ============================================================
// ESTADO GLOBAL
// ============================================================

// Pantalla
static int s_screen_w = 1024;
static int s_screen_h = 768;

// Wallpaper
static UiWallpaperMode s_wp_mode     = UI_WALLPAPER_SOLID;
static uint32_t        s_wp_color    = UI_C_DESKTOP_BG;
static uint32_t        s_wp_grad_top = UI_C_DESKTOP_BG;
static uint32_t        s_wp_grad_bot = UI_C_DESKTOP_BG2;
static const uint32_t* s_wp_img_data = nullptr;
static int             s_wp_img_w    = 0;
static int             s_wp_img_h    = 0;

// Taskbar
struct TaskbarIcon {
    bool        used;
    int         app_id;
    char        label[32];
    UiIconDrawCb draw_cb;
};

static TaskbarIcon s_icons[UI_TASKBAR_MAX_ICONS];
static int         s_icon_count = 0;

// ============================================================
// HELPERS INTERNOS
// ============================================================

static inline int ui__max3(int a, int b) { return a > b ? a : b; }

// Formato numérico 2 dígitos en buffer
static void ui__fmt2(char* out, int n) {
    out[0] = (char)('0' + (n / 10) % 10);
    out[1] = (char)('0' + n % 10);
    out[2] = '\0';
}

// Ancho de texto con la font 8x8 (usada en topbar a escala 1)
static inline int ui__text_w8(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n * 8;
}

// Longitud de cadena
static inline int ui__strlen2(const char* s) {
    if (!s) return 0;
    int n = 0;
    while (s[n]) n++;
    return n;
}

// ============================================================
// WALLPAPER
// ============================================================

void ui_wallpaper_set_color(uint32_t color) {
    s_wp_mode  = UI_WALLPAPER_SOLID;
    s_wp_color = color;
}

void ui_wallpaper_set_gradient(uint32_t top, uint32_t bottom) {
    s_wp_mode     = UI_WALLPAPER_GRADIENT;
    s_wp_grad_top = top;
    s_wp_grad_bot = bottom;
}

void ui_wallpaper_set_image(const uint32_t* data, int img_w, int img_h) {
    if (data && img_w > 0 && img_h > 0) {
        s_wp_mode     = UI_WALLPAPER_IMAGE;
        s_wp_img_data = data;
        s_wp_img_w    = img_w;
        s_wp_img_h    = img_h;
    } else {
        s_wp_mode = UI_WALLPAPER_SOLID; // fallback seguro
    }
}

void ui_desktop_draw() {
    if (!framebuffer_available()) return;

    int screen_y_start = UI_TOPBAR_H;
    int screen_y_end   = s_screen_h - UI_TASKBAR_H;
    int draw_h = screen_y_end - screen_y_start;
    if (draw_h <= 0) draw_h = 1;

    switch (s_wp_mode) {

        case UI_WALLPAPER_SOLID:
            // Fondo completo incluyendo las bandas de las barras
            draw_rect(0, 0, s_screen_w, s_screen_h, s_wp_color);
            break;

        case UI_WALLPAPER_GRADIENT: {
            // Degradado vertical para el área del escritorio
            draw_rect(0, 0, s_screen_w, s_screen_h, s_wp_grad_top);
            for (int y = 0; y < draw_h; y++) {
                uint32_t c = ui_color_lerp(s_wp_grad_top, s_wp_grad_bot,
                                           (uint8_t)(y * 255 / ui__max3(draw_h - 1, 1)));
                draw_rect(0, screen_y_start + y, s_screen_w, 1, c);
            }
            break;
        }

        case UI_WALLPAPER_IMAGE: {
            if (!s_wp_img_data) {
                draw_rect(0, 0, s_screen_w, s_screen_h, s_wp_color);
                break;
            }
            // Escalar la imagen para cubrir la pantalla
            // Usamos nearest-neighbor para no usar punto flotante
            int pitch_px = (int)(pitch / 4);
            for (int sy = 0; sy < s_screen_h; sy++) {
                int src_y = sy * s_wp_img_h / s_screen_h;
                if (src_y >= s_wp_img_h) src_y = s_wp_img_h - 1;
                for (int sx = 0; sx < s_screen_w; sx++) {
                    int src_x = sx * s_wp_img_w / s_screen_w;
                    if (src_x >= s_wp_img_w) src_x = s_wp_img_w - 1;
                    uint32_t px = s_wp_img_data[src_y * s_wp_img_w + src_x];
                    framebuffer[sy * pitch_px + sx] = px | 0xFF000000u;
                }
            }
            break;
        }
    }
}

// ============================================================
// TOPBAR
// ============================================================

void ui_topbar_draw(int hour, int minute, const char* username) {
    if (!framebuffer_available()) return;

    // Fondo de la topbar
    draw_rect(0, 0, s_screen_w, UI_TOPBAR_H, UI_C_TOPBAR_BG);

    // Línea inferior separadora con degradado
    ui_draw_separator_h(0, UI_TOPBAR_H - 1, s_screen_w, UI_C_TOPBAR_SEPARATOR);

    // ── Menú izquierdo: logo / nombre del sistema ──────────
    // Pequeño bloque de acento a la izquierda
    draw_rect(0, 0, 3, UI_TOPBAR_H, UI_C_TOPBAR_ACCENT);

    // Texto del usuario (A la izquierda, reemplazando a Lawai)
    const char* display_name = username ? username : "Invitado";
    int sys_x = 10;
    int sys_y = (UI_TOPBAR_H - 8) / 2;  // centrado vertical con font 8px
    
    // Dibujar icono de usuario pequeño antes del nombre
    draw_filled_circle(sys_x + 5, UI_TOPBAR_H / 2, 5, UI_C_ACCENT);
    print(sys_x + 15, sys_y, display_name, UI_C_TOPBAR_TEXT);

    // Separador pequeño
    ui_draw_separator_v(sys_x + ui__text_w8(display_name) + 20, 4,
                        UI_TOPBAR_H - 8, UI_C_TOPBAR_SEPARATOR);

    // ── Centro: reloj ───────────────────────────────────────
    char clock_buf[8];  // "HH:MM\0"
    if (hour >= 0 && minute >= 0) {
        char hh[3], mm[3];
        ui__fmt2(hh, hour);
        ui__fmt2(mm, minute);
        clock_buf[0] = hh[0]; clock_buf[1] = hh[1];
        clock_buf[2] = ':';
        clock_buf[3] = mm[0]; clock_buf[4] = mm[1];
        clock_buf[5] = '\0';
    } else {
        clock_buf[0] = '-'; clock_buf[1] = '-'; clock_buf[2] = ':';
        clock_buf[3] = '-'; clock_buf[4] = '-'; clock_buf[5] = '\0';
    }

    // Usar font 16x16 para el reloj (más elegante)
    int clock_w = ui_font16_text_width(clock_buf);
    int clock_x = (s_screen_w - clock_w) / 2;
    int clock_y = (UI_TOPBAR_H - UI_FONT16_H) / 2;
    if (clock_y < 0) clock_y = 0;

    // Cápsula sutil detrás del reloj
    ui_draw_rounded_rect(clock_x - 6, 4, clock_w + 12, UI_TOPBAR_H - 8,
                         3, UI_C_SURFACE_2);
    ui_font16_print(clock_x, clock_y, clock_buf,
                    UI_C_TOPBAR_TEXT, 0, true);

    // ── Derecha: nombre de usuario ──────────────────────────
    const char* user = username ? username : "user";
    int user_len = ui__strlen2(user);
    int user_w   = user_len * 8;  // font 8px
    int user_x   = s_screen_w - user_w - 14;
    if (user_x < 0) user_x = 0;
    int user_y   = (UI_TOPBAR_H - 8) / 2;

    // Ícono de usuario (círculo pequeño)
    int icon_cx = user_x - 10;
    int icon_cy = UI_TOPBAR_H / 2;
    draw_filled_circle(icon_cx, icon_cy, 5, UI_C_ACCENT);
    draw_filled_circle(icon_cx, icon_cy, 4, UI_C_SURFACE_ELEVATED);
    put_pixel(icon_cx, icon_cy - 1, UI_C_ACCENT);

    print(user_x, user_y, user, UI_C_TOPBAR_TEXT);

    // Punto de batería / estado (decorativo)
    ui_draw_indicator_dot(s_screen_w - 6, UI_TOPBAR_H / 2, 3, UI_C_BTN_MAX);
}

int ui_topbar_hit(int mx, int my) {
    if (my < 0 || my >= UI_TOPBAR_H) return -1;
    if (mx < s_screen_w / 3) return 0;           // zona izquierda
    if (mx < 2 * s_screen_w / 3) return 1;       // zona central (reloj)
    return 2;                                      // zona derecha
}

// ============================================================
// TASKBAR
// ============================================================

void ui_taskbar_add_icon_internal(int app_id, const char* label, UiIconDrawCb draw_cb) {
    if (s_icon_count >= UI_TASKBAR_MAX_ICONS) return;
    TaskbarIcon* ic = &s_icons[s_icon_count];
    ic->used    = true;
    ic->app_id  = app_id;
    ic->draw_cb = draw_cb;
    int ni = 0;
    if (label) {
        while (label[ni] && ni < 31) { ic->label[ni] = label[ni]; ni++; }
    }
    ic->label[ni] = '\0';
    s_icon_count++;
}

int ui_taskbar_add_icon(int app_id, const char* label, UiIconDrawCb draw_cb) {
    if (s_icon_count >= UI_TASKBAR_MAX_ICONS) return -1;
    int idx = s_icon_count;
    ui_taskbar_add_icon_internal(app_id, label, draw_cb);
    return idx;
}

void ui_taskbar_remove_icon(int index) {
    if (index < 0 || index >= s_icon_count) return;
    for (int i = index; i < s_icon_count - 1; i++) s_icons[i] = s_icons[i + 1];
    s_icon_count--;
    s_icons[s_icon_count].used = false;
}

void ui_taskbar_clear() {
    for (int i = 0; i < UI_TASKBAR_MAX_ICONS; i++) s_icons[i].used = false;
    s_icon_count = 0;
}

int ui_taskbar_icon_count() { return s_icon_count; }

// Calcula la posición X del centro del icono en el dock
static int s_icon_cx(int index) {
    // Iconos centrados en el taskbar
    int total_w = s_icon_count * (UI_TASKBAR_ICON_SZ + UI_TASKBAR_ICON_GAP)
                  - UI_TASKBAR_ICON_GAP;
    int start_x = (s_screen_w - total_w) / 2;
    return start_x + index * (UI_TASKBAR_ICON_SZ + UI_TASKBAR_ICON_GAP)
           + UI_TASKBAR_ICON_SZ / 2;
}

// Y del centro del icono
static int s_icon_cy() {
    return s_screen_h - UI_TASKBAR_H / 2 - 2;
}

void ui_taskbar_draw(int active_app_id, int hover_mx, int hover_my) {
    if (!framebuffer_available()) return;

    int bar_y = s_screen_h - UI_TASKBAR_H;

    // Fondo del taskbar (panel oscuro semitransparente)
    // Simulamos transparencia dibujando rect sólido — sin blending real
    draw_rect(0, bar_y, s_screen_w, UI_TASKBAR_H, UI_C_TASKBAR_BG);

    // Borde superior
    draw_rect(0, bar_y, s_screen_w, 1, UI_C_TASKBAR_BORDER);

    // Decoración de separadores laterales suaves
    ui_draw_separator_v(0, bar_y + 1, UI_TASKBAR_H - 1, UI_C_TASKBAR_BORDER);
    ui_draw_separator_v(s_screen_w - 1, bar_y + 1, UI_TASKBAR_H - 1, UI_C_TASKBAR_BORDER);

    if (s_icon_count == 0) return;

    int cy = s_icon_cy();

    for (int i = 0; i < s_icon_count; i++) {
        TaskbarIcon* ic = &s_icons[i];
        if (!ic->used) continue;

        int cx = s_icon_cx(i);
        int ix = cx - UI_TASKBAR_ICON_SZ / 2;
        int iy = s_screen_h - UI_TASKBAR_H + UI_TASKBAR_ICON_PAD;
        int iw = UI_TASKBAR_ICON_SZ;
        int ih = UI_TASKBAR_ICON_SZ;

        bool active  = (ic->app_id == active_app_id);
        bool hovered = (hover_mx >= ix && hover_mx < ix + iw &&
                        hover_my >= iy && hover_my < iy + ih);

        // Fondo del icono (cápsula redondeada)
        uint32_t bg_c;
        if (active)       bg_c = UI_C_TASKBAR_ITEM_ACT;
        else if (hovered) bg_c = UI_C_TASKBAR_ITEM_HOV;
        else              bg_c = UI_C_TASKBAR_ITEM;

        ui_draw_rounded_rect(ix, iy, iw, ih, 8, bg_c);

        // Borde sutil
        if (active) {
            ui_draw_rounded_rect_outline(ix, iy, iw, ih, 8,
                                         UI_C_TASKBAR_INDICATOR, 1);
        } else {
            ui_draw_rounded_rect_outline(ix, iy, iw, ih, 8,
                                         UI_C_TASKBAR_BORDER, 1);
        }

        // Icono de la aplicación
        int icon_size = iw - 10;
        if (ic->draw_cb) {
            ic->draw_cb(cx, cy, icon_size, active);
        } else {
            // Icono genérico por defecto
            ui_icon_app_default(cx, cy, icon_size,
                                active ? UI_C_ACCENT : UI_C_TEXT_SECONDARY,
                                bg_c);
        }

        // Indicador de app activa (barra inferior)
        if (active) {
            int bar_w = iw - 10;
            ui_draw_indicator_bar(cx, s_screen_h - UI_TASKBAR_INDICATOR_H - 2,
                                  bar_w, UI_TASKBAR_INDICATOR_H,
                                  UI_C_TASKBAR_INDICATOR);
        }

        // Tooltip-like: etiqueta bajo hover (usando font pequeña)
        if (hovered && ic->label[0]) {
            int lbl_len = ui__strlen2(ic->label);
            int lbl_w = lbl_len * 8 + 8;
            int lbl_x = cx - lbl_w / 2;
            int lbl_y = iy - 20;
            if (lbl_x < 2) lbl_x = 2;
            if (lbl_x + lbl_w > s_screen_w - 2) lbl_x = s_screen_w - 2 - lbl_w;

            // Fondo del tooltip
            ui_draw_rounded_rect(lbl_x - 4, lbl_y - 2, lbl_w + 4, 14, 4,
                                 UI_C_SURFACE_ELEVATED);
            ui_draw_rounded_rect_outline(lbl_x - 4, lbl_y - 2, lbl_w + 4, 14, 4,
                                         UI_C_WIN_BORDER, 1);
            print(lbl_x, lbl_y, ic->label, UI_C_TEXT_PRIMARY);
        }
    }
}

int ui_taskbar_hit(int mx, int my) {
    int bar_y = s_screen_h - UI_TASKBAR_H;
    if (my < bar_y || my >= s_screen_h) return -1;

    for (int i = 0; i < s_icon_count; i++) {
        if (!s_icons[i].used) continue;
        int cx = s_icon_cx(i);
        int ix = cx - UI_TASKBAR_ICON_SZ / 2;
        int iy = s_screen_h - UI_TASKBAR_H + UI_TASKBAR_ICON_PAD;
        int iw = UI_TASKBAR_ICON_SZ;
        int ih = UI_TASKBAR_ICON_SZ;
        if (mx >= ix && mx < ix + iw && my >= iy && my < iy + ih)
            return s_icons[i].app_id;
    }
    return -1;
}

// ============================================================
// INICIALIZACIÓN
// ============================================================

void ui_desktop_init(int screen_w, int screen_h) {
    s_screen_w = screen_w;
    s_screen_h = screen_h;

    // Wallpaper por defecto: degradado oscuro moderno
    ui_wallpaper_set_gradient(0xFF0F0F18u, 0xFF181828u);

    // Limpiar iconos
    ui_taskbar_clear();
}

void ui_desktop_set_screen(int screen_w, int screen_h) {
    s_screen_w = screen_w;
    s_screen_h = screen_h;
}