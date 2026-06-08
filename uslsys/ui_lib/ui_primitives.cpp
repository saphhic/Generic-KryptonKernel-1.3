// written by saphhic.
// ui_primitives.cpp — Implementación de primitivas de renderizado Lawai UI

#include "ui_primitives.h"
#include "ui_theme.h"
#include "../../KryptonKernel/drivers/frameb/framebuffer.hpp"

// ============================================================
// INTERNAL HELPERS
// ============================================================

static inline int ui__min(int a, int b) { return a < b ? a : b; }
static inline int ui__max(int a, int b) { return a > b ? a : b; }
static inline int ui__abs(int a) { return a < 0 ? -a : a; }
static inline int ui__clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Dibuja un pixel con alpha sobre lo que hay en el framebuffer
static void ui__put_alpha(int x, int y, uint32_t color, uint8_t alpha) {
    if (!framebuffer_available()) return;
    if (x < 0 || y < 0 || x >= (int)width || y >= (int)height) return;
    if (alpha == 255) { put_pixel(x, y, color); return; }
    if (alpha == 0) return;

    int idx = y * (int)(pitch / 4) + x;
    uint32_t dst = framebuffer[idx];
    uint8_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
    uint8_t sr = (color >> 16) & 0xFF, sg = (color >> 8) & 0xFF, sb = color & 0xFF;
    uint8_t nr = (uint8_t)(dr + (int)(sr - dr) * alpha / 255);
    uint8_t ng = (uint8_t)(dg + (int)(sg - dg) * alpha / 255);
    uint8_t nb = (uint8_t)(db + (int)(sb - db) * alpha / 255);
    put_pixel(x, y, 0xFF000000u | ((uint32_t)nr << 16) | ((uint32_t)ng << 8) | nb);
}

// Pinta una scanline horizontal con alpha
static void ui__hline_alpha(int x0, int x1, int y, uint32_t color, uint8_t alpha) {
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    for (int x = x0; x <= x1; x++) ui__put_alpha(x, y, color, alpha);
}

// ============================================================
// ROUNDED RECTANGLE FILLED — Algoritmo por scanlines + circle corners
// ============================================================

// Dibuja el cuarto de círculo relleno en la esquina correcta
static void ui__draw_quarter_fill(int cx, int cy, int r, uint32_t color,
                                  int quadrant) {
    // quadrant: 0=TL, 1=TR, 2=BL, 3=BR
    if (r <= 0) return;
    int r2 = r * r;
    for (int dy = 0; dy <= r; dy++) {
        int dx = 0;
        while (dx * dx + dy * dy <= r2) dx++;
        dx--; // último dx dentro del círculo

        int qx, qy;
        int row_start, row_end;
        switch (quadrant) {
            case 0: // top-left
                qy = cy - dy;
                row_start = cx - dx;
                row_end   = cx;
                for (int xx = row_start; xx <= row_end; xx++) put_pixel(xx, qy, color);
                break;
            case 1: // top-right
                qy = cy - dy;
                row_start = cx;
                row_end   = cx + dx;
                for (int xx = row_start; xx <= row_end; xx++) put_pixel(xx, qy, color);
                break;
            case 2: // bottom-left
                qy = cy + dy;
                row_start = cx - dx;
                row_end   = cx;
                for (int xx = row_start; xx <= row_end; xx++) put_pixel(xx, qy, color);
                break;
            case 3: // bottom-right
                qy = cy + dy;
                row_start = cx;
                row_end   = cx + dx;
                for (int xx = row_start; xx <= row_end; xx++) put_pixel(xx, qy, color);
                break;
        }
        (void)qx;
    }
}

void ui_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    if (!framebuffer_available()) return;

    int r = ui__min(radius, ui__min(w / 2, h / 2));
    if (r < 0) r = 0;

    if (r == 0) {
        draw_rect(x, y, w, h, color);
        return;
    }

    // Centro de cada esquina
    int tlx = x + r,       tly = y + r;
    int trx = x + w - r - 1, try_ = y + r;
    int blx = x + r,       bly = y + h - r - 1;
    int brx = x + w - r - 1, bry = y + h - r - 1;

    // Rectángulo central horizontal
    draw_rect(x, y + r, w, h - 2 * r, color);
    // Rectángulo central vertical (parte superior e inferior)
    draw_rect(x + r, y, w - 2 * r, r, color);
    draw_rect(x + r, y + h - r, w - 2 * r, r, color);

    // Cuatro esquinas
    ui__draw_quarter_fill(tlx, tly, r, color, 0);
    ui__draw_quarter_fill(trx, try_, r, color, 1);
    ui__draw_quarter_fill(blx, bly, r, color, 2);
    ui__draw_quarter_fill(brx, bry, r, color, 3);
}

// ============================================================
// ROUNDED RECT OUTLINE
// ============================================================

static void ui__draw_quarter_arc(int cx, int cy, int r, uint32_t color, int quadrant) {
    if (r <= 0) return;
    int x = 0, y = r, d = 3 - 2 * r;
    while (x <= y) {
        int px, py;
        switch (quadrant) {
            case 0: px = cx - y; py = cy - x; put_pixel(px, py, color);
                    px = cx - x; py = cy - y; put_pixel(px, py, color); break;
            case 1: px = cx + y; py = cy - x; put_pixel(px, py, color);
                    px = cx + x; py = cy - y; put_pixel(px, py, color); break;
            case 2: px = cx - y; py = cy + x; put_pixel(px, py, color);
                    px = cx - x; py = cy + y; put_pixel(px, py, color); break;
            case 3: px = cx + y; py = cy + x; put_pixel(px, py, color);
                    px = cx + x; py = cy + y; put_pixel(px, py, color); break;
        }
        if (d < 0) d += 4 * x + 6; else { d += 4 * (x - y) + 10; y--; }
        x++;
    }
}

void ui_draw_rounded_rect_outline(int x, int y, int w, int h, int radius,
                                  uint32_t color, int border_w) {
    if (w <= 0 || h <= 0) return;
    for (int i = 0; i < border_w; i++) {
        int r = ui__min(radius - i, ui__min((w - i * 2) / 2, (h - i * 2) / 2));
        if (r < 0) r = 0;
        int xi = x + i, yi = y + i, wi = w - i * 2, hi = h - i * 2;

        // Lados
        draw_rect(xi + r, yi, wi - 2 * r, 1, color);          // top
        draw_rect(xi + r, yi + hi - 1, wi - 2 * r, 1, color); // bottom
        draw_rect(xi, yi + r, 1, hi - 2 * r, color);          // left
        draw_rect(xi + wi - 1, yi + r, 1, hi - 2 * r, color); // right

        // Arcos de esquina
        int tlx = xi + r,        tly = yi + r;
        int trx = xi + wi - r - 1, try_ = yi + r;
        int blx = xi + r,        bly = yi + hi - r - 1;
        int brx = xi + wi - r - 1, bry = yi + hi - r - 1;
        ui__draw_quarter_arc(tlx, tly,  r, color, 0);
        ui__draw_quarter_arc(trx, try_, r, color, 1);
        ui__draw_quarter_arc(blx, bly,  r, color, 2);
        ui__draw_quarter_arc(brx, bry,  r, color, 3);
    }
}

// ============================================================
// ROUNDED RECT GRADIENT VERTICAL
// ============================================================

void ui_draw_rounded_rect_vgrad(int x, int y, int w, int h, int radius,
                                uint32_t top_color, uint32_t bottom_color) {
    if (w <= 0 || h <= 0) return;
    // Dibujamos scanline por scanline con color interpolado
    for (int row = 0; row < h; row++) {
        uint32_t c = ui_color_lerp(top_color, bottom_color, (uint8_t)(row * 255 / ui__max(h - 1, 1)));
        // Calcular x_left y x_right para esa fila teniendo en cuenta el radius
        int r = ui__min(radius, ui__min(w / 2, h / 2));
        int xl = x, xr = x + w - 1;
        if (r > 0) {
            if (row < r) {
                // esquina superior
                int dy = r - row;
                int dx = 0;
                while (dx * dx + dy * dy > r * r && dx <= r) dx++;
                xl = x + r - dx + 1;
                xr = x + w - r + dx - 2;
            } else if (row >= h - r) {
                // esquina inferior
                int dy = row - (h - r - 1);
                int dx = 0;
                while (dx * dx + dy * dy > r * r && dx <= r) dx++;
                xl = x + r - dx + 1;
                xr = x + w - r + dx - 2;
            }
        }
        if (xr >= xl)
            draw_rect(xl, y + row, xr - xl + 1, 1, c);
    }
}

// ============================================================
// SOMBRA FAKE (varias capas de rects semitransparentes)
// ============================================================

void ui_draw_shadow(int x, int y, int w, int h, int radius,
                    int offset_x, int offset_y, int spread, uint8_t alpha) {
    if (!framebuffer_available()) return;
    int sx = x + offset_x - spread;
    int sy = y + offset_y - spread;
    int sw = w + spread * 2;
    int sh = h + spread * 2;
    int layers = ui__min(spread + 1, 6);
    for (int i = 0; i < layers; i++) {
        uint8_t layer_alpha = (uint8_t)((int)alpha * (layers - i) / layers);
        int lx = sx + i, ly = sy + i;
        int lw = sw - i * 2, lh = sh - i * 2;
        int lr = ui__min(radius + spread - i, ui__min(lw / 2, lh / 2));
        // Dibujamos sólo los bordes del rounded rect con alpha
        if (lw > 0 && lh > 0) {
            // top/bottom bands
            for (int dx = 0; dx < lw; dx++) {
                ui__put_alpha(lx + dx, ly, 0xFF000000u, layer_alpha);
                ui__put_alpha(lx + dx, ly + lh - 1, 0xFF000000u, layer_alpha);
            }
            // left/right bands
            for (int dy = 1; dy < lh - 1; dy++) {
                ui__put_alpha(lx, ly + dy, 0xFF000000u, layer_alpha);
                ui__put_alpha(lx + lw - 1, ly + dy, 0xFF000000u, layer_alpha);
            }
            (void)lr;
        }
    }
}

// ============================================================
// TEXTO ALINEADO
// ============================================================

void ui_draw_text_centered(int x, int y, int area_w, const char* text,
                           uint32_t color, int scale) {
    if (!text) return;
    int len = ui_strlen(text);
    int text_w = len * UI_FONT_W * scale;
    int tx = x + (area_w - text_w) / 2;
    if (tx < x) tx = x;
    print(tx, y, text, color);
}

void ui_draw_text_right(int x, int y, int area_w, const char* text,
                        uint32_t color, int scale) {
    if (!text) return;
    int len = ui_strlen(text);
    int text_w = len * UI_FONT_W * scale;
    int tx = x + area_w - text_w;
    if (tx < x) tx = x;
    print(tx, y, text, color);
}

void ui_draw_text_ellipsis(int x, int y, const char* text, uint32_t color,
                           int scale, int max_chars) {
    if (!text) return;
    int len = ui_strlen(text);
    if (len <= max_chars) {
        print(x, y, text, color);
        return;
    }
    // Copiar primeros (max_chars-3) caracteres + "..."
    char buf[128];
    int copy = max_chars - 3;
    if (copy < 0) copy = 0;
    if (copy > 124) copy = 124;
    int i;
    for (i = 0; i < copy; i++) buf[i] = text[i];
    buf[i++] = '.'; buf[i++] = '.'; buf[i++] = '.'; buf[i] = '\0';
    print(x, y, buf, color);
}

// ============================================================
// ICONOS VECTORIALES
// ============================================================

void ui_icon_close(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 2;
    if (r < 1) r = 1;
    draw_line(cx - r, cy - r, cx + r, cy + r, color);
    draw_line(cx + r, cy - r, cx - r, cy + r, color);
    // Líneas adicionales para dar grosor
    draw_line(cx - r + 1, cy - r, cx + r, cy + r - 1, color);
    draw_line(cx - r, cy - r + 1, cx + r - 1, cy + r, color);
}

void ui_icon_minimize(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 3;
    if (r < 1) r = 1;
    draw_rect(cx - r, cy, r * 2, 2, color);
}

void ui_icon_maximize(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 3;
    if (r < 1) r = 1;
    draw_rect_outline(cx - r, cy - r, r * 2, r * 2, color);
    // Doble línea superior
    draw_rect(cx - r, cy - r, r * 2, 2, color);
}

void ui_icon_restore(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 3;
    if (r < 1) r = 1;
    // Ventana de fondo (desplazada +2, +2)
    draw_rect_outline(cx - r + 2, cy - r, r * 2, r * 2 - 1, color);
    // Ventana frontal
    draw_rect_outline(cx - r, cy - r + 2, r * 2 - 1, r * 2, color);
    draw_rect(cx - r, cy - r + 2, r * 2 - 1, 2, color);
}

void ui_icon_arrow_right(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 2;
    for (int i = 0; i <= r; i++) {
        put_pixel(cx - r + i, cy - i, color);
        put_pixel(cx - r + i, cy + i, color);
    }
}

void ui_icon_menu(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 2;
    draw_rect(cx - r, cy - r,     r * 2, 2, color);
    draw_rect(cx - r, cy - 1,     r * 2, 2, color);
    draw_rect(cx - r, cy + r - 2, r * 2, 2, color);
}

void ui_icon_app_default(int cx, int cy, int size, uint32_t fg, uint32_t bg) {
    int r = size / 2 - 1;
    draw_filled_circle(cx, cy, r, bg);
    draw_circule(cx, cy, r, fg);
}

void ui_icon_terminal(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 2;
    // >
    draw_line(cx - r, cy - r / 2, cx - 1, cy, color);
    draw_line(cx - r, cy + r / 2, cx - 1, cy, color);
    // _
    draw_rect(cx + 1, cy + r / 2, r - 1, 2, color);
}

void ui_icon_folder(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 1;
    // Tab superior izquierdo
    draw_rect(cx - r, cy - r, r, 3, color);
    // Cuerpo de la carpeta
    draw_rect_outline(cx - r, cy - r + 2, r * 2, r * 2 - 1, color);
}

void ui_icon_settings(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 1;
    draw_circule(cx, cy, r / 2, color);
    int pts = 6;
    for (int i = 0; i < pts; i++) {
        // 6 puntos al exterior del círculo
        int angle_step = 360 / pts;
        // Aproximamos con líneas en 8 posiciones cardinales
        (void)angle_step;
    }
    // Simplificado: círculo exterior + interior
    draw_circule(cx, cy, r, color);
    draw_filled_circle(cx, cy, r / 2, color);
    draw_filled_circle(cx, cy, r / 2 - 1, 0xFF0A0A0Fu);
}

void ui_icon_system(int cx, int cy, int size, uint32_t color) {
    int r = size / 2 - 1;
    draw_filled_circle(cx, cy, r, color);
    // "i" en blanco
    put_pixel(cx, cy - r / 3, 0xFF0A0A0Fu);
    draw_rect(cx - 1, cy - r / 4 + 3, 3, r / 2, 0xFF0A0A0Fu);
}

// ============================================================
// CURSOR DEL MOUSE
// ============================================================

// Forma de flecha clásica 11×18
static const uint8_t CURSOR_SHAPE[18] = {
    0b11000000,  // ##
    0b11100000,  // ###
    0b11110000,  // ####
    0b11111000,  // #####
    0b11111100,  // ######
    0b11111110,  // #######
    0b11111111,  // ########
    0b11111110,  // #######  (8+)
    0b11110000,  // ####
    0b11011000,  // ## ##
    0b10001100,  // #   ##
    0b00001100,  //     ##
    0b00000110,  //      ##
    0b00000110,  //      ##
    0b00000011,  //       ##
    0b00000011,  //       ##
    0b00000000,
    0b00000000,
};

void ui_draw_cursor(int x, int y) {
    // Sombra (desplazada 1,1)
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 8; col++) {
            if (CURSOR_SHAPE[row] & (0x80 >> col)) {
                ui__put_alpha(x + col + 1, y + row + 1, 0xFF000000u, 80);
            }
        }
    }
    // Píxeles del cursor (blanco)
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 8; col++) {
            if (CURSOR_SHAPE[row] & (0x80 >> col)) {
                put_pixel(x + col, y + row, UI_C_CURSOR);
            }
        }
    }
    // Borde negro (1 px de outline)
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 8; col++) {
            bool cur = (CURSOR_SHAPE[row] & (0x80 >> col)) != 0;
            if (!cur) {
                // Ver si algún vecino es cursor → borde
                bool has_neighbor = false;
                if (row > 0  && (CURSOR_SHAPE[row-1] & (0x80 >> col))) has_neighbor = true;
                if (row < 15 && (CURSOR_SHAPE[row+1] & (0x80 >> col))) has_neighbor = true;
                if (col > 0  && (CURSOR_SHAPE[row] & (0x80 >> (col-1)))) has_neighbor = true;
                if (col < 7  && (CURSOR_SHAPE[row] & (0x80 >> (col+1)))) has_neighbor = true;
                if (has_neighbor)
                    put_pixel(x + col, y + row, 0xFF101010u);
            }
        }
    }
}

// ============================================================
// SEPARADORES
// ============================================================

void ui_draw_separator_h(int x, int y, int w, uint32_t color) {
    // Fade en los extremos (primer y último cuarto)
    int fade = w / 6;
    for (int i = 0; i < w; i++) {
        uint8_t a = 200;
        if (i < fade) a = (uint8_t)(200 * i / ui__max(fade, 1));
        else if (i > w - fade) a = (uint8_t)(200 * (w - i) / ui__max(fade, 1));
        ui__put_alpha(x + i, y, color, a);
    }
}

void ui_draw_separator_v(int x, int y, int h, uint32_t color) {
    int fade = h / 6;
    for (int i = 0; i < h; i++) {
        uint8_t a = 180;
        if (i < fade) a = (uint8_t)(180 * i / ui__max(fade, 1));
        else if (i > h - fade) a = (uint8_t)(180 * (h - i) / ui__max(fade, 1));
        ui__put_alpha(x, y + i, color, a);
    }
}

// ============================================================
// INDICADORES
// ============================================================

void ui_draw_indicator_dot(int cx, int cy, int radius, uint32_t color) {
    draw_filled_circle(cx, cy, radius, color);
}

void ui_draw_indicator_bar(int cx, int y, int bar_w, int bar_h, uint32_t color) {
    int bx = cx - bar_w / 2;
    ui_draw_rounded_rect(bx, y, bar_w, bar_h, bar_h / 2, color);
}