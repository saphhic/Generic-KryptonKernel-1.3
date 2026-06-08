//written by saphhic.

#include "framebuffer.hpp"
#include "hal/comparator.h"
#include "font8x8_basic.h"

static const uint32_t FB_INFO_ADDR = 0x00009000;
static const uint32_t FB_INFO_MAGIC = 0x4246524B;

uint32_t FRAMEBUFFER_ADDR = 0;
uint32_t FRAMEBUFFER_WIDTH = 0;
uint32_t FRAMEBUFFER_HEIGHT = 0;
uint32_t FRAMEBUFFER_PITCH = 0;

uint32_t* framebuffer = nullptr;
uint32_t width = 1024;
uint32_t height = 768;
uint32_t pitch = 3072;

static int fb_cursor_x = 0;
static int fb_cursor_y = 0;
static int fb_char_scale = 1;
static uint32_t fb_char_color = 0xFFFFFFFF;
static uint32_t fb_bg_color = 0x00000000;
static const int FB_MARGIN = 8;

uint32_t fb_color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xFF000000u) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    
}

extern "C" void framebuffer_init_stub() {
    volatile uint32_t* info = (volatile uint32_t*)FB_INFO_ADDR;
    if (info[0] != FB_INFO_MAGIC || info[1] == 0 || info[2] == 0 || info[3] == 0 || info[5] != 32) {
        FRAMEBUFFER_ADDR = 0;
        FRAMEBUFFER_WIDTH = 0;
        FRAMEBUFFER_HEIGHT = 0;
        FRAMEBUFFER_PITCH = 0;
        return;
    }

    FRAMEBUFFER_ADDR = info[1];
    FRAMEBUFFER_WIDTH = info[2];
    FRAMEBUFFER_HEIGHT = info[3];
    FRAMEBUFFER_PITCH = info[4];
}

void framebuffer_init() {
    framebuffer_init_stub();

    if (FRAMEBUFFER_ADDR == 0 || FRAMEBUFFER_WIDTH == 0 || FRAMEBUFFER_HEIGHT == 0 || FRAMEBUFFER_PITCH == 0) {
        framebuffer = nullptr;
        width = 0;
        height = 0;
        pitch = 0;
        return;
    }
  
    framebuffer = (uint32_t*)FRAMEBUFFER_ADDR;
    width = FRAMEBUFFER_WIDTH;
    height = FRAMEBUFFER_HEIGHT;
    pitch = FRAMEBUFFER_PITCH;

    if (pitch < width * 4) {
        pitch = width * 4;
    }

    fb_cursor_x = FB_MARGIN;
    fb_cursor_y = FB_MARGIN;
}

bool framebuffer_available() {
    return framebuffer != nullptr && width > 0 && height > 0 && pitch >= width * 4;
}

void fb_set_scale(int scale) {
    if (scale < 1) scale = 1;
    if (scale > 4) scale = 4;
    fb_char_scale = scale;
}

void fb_set_colors(uint32_t fg, uint32_t bg) {
    fb_char_color = fg;
    fb_bg_color = bg;
}

void put_pixel(int x, int y, uint32_t color) {
    if (framebuffer == nullptr) return;
    if (x < 0 || y < 0 || (uint32_t)x >= width || (uint32_t)y >= height) return;
    uint32_t* pixel = (uint32_t*)((uint8_t*)framebuffer + (uint32_t)y * pitch + (uint32_t)x * 4u);
    *pixel = color;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (!framebuffer_available() || w <= 0 || h <= 0) return;

    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int)width) x1 = (int)width;
    if (y1 > (int)height) y1 = (int)height;
    if (x0 >= x1 || y0 >= y1) return;

    uint32_t pixels_per_line = pitch / 4;
    for (int row = y0; row < y1; row++) {
        uint32_t* line_ptr = &framebuffer[row * pixels_per_line + x0];
        int count = x1 - x0;
        for (int i = 0; i < count; i++) line_ptr[i] = color;
    }
}

static uint32_t blend_pixel(uint32_t dst, uint32_t src, uint8_t alpha) {
    uint32_t inv = 255u - (uint32_t)alpha;
    uint32_t sr = (src >> 16) & 0xFFu;
    uint32_t sg = (src >> 8) & 0xFFu;
    uint32_t sb = src & 0xFFu;
    uint32_t dr = (dst >> 16) & 0xFFu;
    uint32_t dg = (dst >> 8) & 0xFFu;
    uint32_t db = dst & 0xFFu;
    uint32_t r = (sr * alpha + dr * inv) / 255u;
    uint32_t g = (sg * alpha + dg * inv) / 255u;
    uint32_t b = (sb * alpha + db * inv) / 255u;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha) {
    if (!framebuffer_available() || w <= 0 || h <= 0 || alpha == 0) return;
    if (alpha == 255) {
        draw_rect(x, y, w, h, color);
        return;
    }

    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int)width) x1 = (int)width;
    if (y1 > (int)height) y1 = (int)height;
    if (x0 >= x1 || y0 >= y1) return;

    for (int row = y0; row < y1; row++) {
        uint32_t* line = (uint32_t*)((uint8_t*)framebuffer + (uint32_t)row * pitch);
        for (int col = x0; col < x1; col++) {
            line[col] = blend_pixel(line[col], color, alpha);
        }
    }
}

static uint32_t mix_fb_color(uint32_t a, uint32_t b, int t) {
    int inv = 255 - t;
    uint32_t ar = (a >> 16) & 0xFFu;
    uint32_t ag = (a >> 8) & 0xFFu;
    uint32_t ab = a & 0xFFu;
    uint32_t br = (b >> 16) & 0xFFu;
    uint32_t bg = (b >> 8) & 0xFFu;
    uint32_t bb = b & 0xFFu;
    uint32_t r = (ar * (uint32_t)inv + br * (uint32_t)t) / 255u;
    uint32_t g = (ag * (uint32_t)inv + bg * (uint32_t)t) / 255u;
    uint32_t bl = (ab * (uint32_t)inv + bb * (uint32_t)t) / 255u;
    return 0xFF000000u | (r << 16) | (g << 8) | bl;
}

void draw_rect_vgradient(int x, int y, int w, int h, uint32_t top, uint32_t bottom) {
    if (!framebuffer_available() || w <= 0 || h <= 0) return;

    int x0 = x < 0 ? 0 : x;
    int x1 = x + w;
    if (x1 > (int)width) x1 = (int)width;
    if (x0 >= x1) return;

    int y0 = y < 0 ? 0 : y;
    int y1 = y + h;
    if (y1 > (int)height) y1 = (int)height;
    if (y0 >= y1) return;

    uint32_t r_top = (top >> 16) & 0xFF, g_top = (top >> 8) & 0xFF, b_top = top & 0xFF;
    uint32_t r_bot = (bottom >> 16) & 0xFF, g_bot = (bottom >> 8) & 0xFF, b_bot = bottom & 0xFF;

    for (int row = y0; row < y1; row++) {
        int rel = row - y;
        int t = h > 1 ? (rel * 255) / (h - 1) : 0;
        int inv = 255 - t;

        uint32_t r = (r_top * inv + r_bot * t) / 255;
        uint32_t g = (g_top * inv + g_bot * t) / 255;
        uint32_t b = (b_top * inv + b_bot * t) / 255;
        uint32_t color = 0xFF000000u | (r << 16) | (g << 8) | b;

        uint32_t* line_ptr = &framebuffer[row * (pitch / 4) + x0];
        for (int col = x0; col < x1; col++) line_ptr[col - x0] = color;
    }
}

void draw_rect_outline(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < h; i++) {
        put_pixel(x, y + i, color);
        put_pixel(x + w - 1, y + i, color);
    }
    for (int i = 0; i < w; i++) {
        put_pixel(x + i, y, color);
        put_pixel(x + i, y + h - 1, color);
    }
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0 ? x1 - x0 : x0 - x1);
    int dy = -(y1 > y0 ? y1 - y0 : y0 - y1);
    int sx = (x1 > x0) ? 1 : -1;
    int sy = (y1 > y0) ? 1 : -1;
    int err = dx + dy;
    while (true) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_circule(int cx, int cy, int r, uint32_t color) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    while (x <= y) {
        put_pixel(cx + x, cy + y, color);
        put_pixel(cx + x, cy - y, color);
        put_pixel(cx - x, cy + y, color);
        put_pixel(cx - x, cy - y, color);
        put_pixel(cx + y, cy + x, color);
        put_pixel(cx + y, cy - x, color);
        put_pixel(cx - y, cy + x, color);
        put_pixel(cx - y, cy - x, color);
        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void draw_filled_circle(int cx, int cy, int r, uint32_t color) {
    if (!framebuffer_available()) return;
    if (r <= 0) return;
    int r2 = r * r;
    for (int y = -r; y <= r; y++) {
        int y_pos = cy + y;
        if (y_pos < 0 || y_pos >= (int)height) continue;
        int yy = y * y;
        for (int x = -r; x <= r; x++) {
            int x_pos = cx + x;
            if (x_pos < 0 || x_pos >= (int)width) continue;
            if (x * x + yy <= r2) {
                framebuffer[y_pos * (pitch / 4) + x_pos] = color;
            }
        }
    }
}

void draw_char(int px, int py, char c, uint32_t fg, uint32_t bg, int scale) {
    uint8_t idx = (uint8_t)c;
    if (idx >= 128) idx = '?';
    bool transparent_bg = ((bg >> 24) == 0);
    for (int row = 0; row < 8; row++) {
        uint8_t bits = font8x8_basic[idx][row];
        for (int col = 0; col < 8; col++) {
            bool glyph_pixel = (bits & (1 << col)) != 0;
            if (!glyph_pixel && transparent_bg) continue;
            uint32_t color = glyph_pixel ? fg : bg;
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    put_pixel(px + col * scale + sx, py + row * scale + sy, color);
                }
            }
        }
    }
}

void print(int x, int y, const char* str, uint32_t color) {
    int px = x;
    while(*str) {
        char c = *str++;
        if (c == '\n') {
            y += 8 * fb_char_scale;
            px = x;
            continue;
        }
        draw_char(px, y, c, color, fb_bg_color, fb_char_scale);
        px += 8 * fb_char_scale;
    }
}

static void fb_scroll_up() {
    if (!framebuffer_available()) return;

    int char_h = 8 * fb_char_scale;
    uint32_t row_bytes = (uint32_t)(width) * 4u;

    for (uint32_t row = 0; row < (uint32_t)(height - char_h); row++) {
        uint8_t* dst = (uint8_t*)framebuffer + row * pitch;
        uint8_t* src = (uint8_t*)framebuffer + (row + (uint32_t)char_h) * pitch;
        for (uint32_t i = 0; i < row_bytes; i++) {
            dst[i] = src[i];
        }
    }

    for (int row = height - char_h; row < (int)height; row++) {
        for (int col = 0; col < (int)width; col++) {
            put_pixel(col, row, fb_bg_color);
        }
    }

}

void fb_clear_screen() {
    if (!framebuffer_available()) return;

    for (uint32_t row = 0; row < height; row++) {
        uint32_t* line = (uint32_t*)((uint8_t*)framebuffer + row * pitch);
        for (uint32_t col = 0; col < width; col++) {
            line[col] = fb_bg_color;
        }
        fb_cursor_x = FB_MARGIN;
        fb_cursor_y = FB_MARGIN;
    }
}

void putchar_fb(char c) {
    int char_w = 8 * fb_char_scale;
    int char_h = 8 * fb_char_scale;
    if (c == '\n') {
        fb_cursor_y += char_h;
        fb_cursor_x = FB_MARGIN;
    }
    else if (c == '\r') {
        fb_cursor_x = FB_MARGIN;
    }
    else if (c == '\b') {
        fb_cursor_x -= char_w;
        if (fb_cursor_x < FB_MARGIN) fb_cursor_x = FB_MARGIN;
        draw_rect(fb_cursor_x, fb_cursor_y, char_w, char_h, fb_bg_color);

        return;
    }
    else if (c == '\t') {
        int tab_w = char_w * 4;
        fb_cursor_x = ((fb_cursor_x - FB_MARGIN) / tab_w + 1) * tab_w + FB_MARGIN;
    }
    else {
        draw_char(fb_cursor_x, fb_cursor_y, c, fb_char_color, fb_bg_color, fb_char_scale);
        fb_cursor_x += char_w;
    }

    if (fb_cursor_x + char_w > (int)width - FB_MARGIN) {
        fb_cursor_x = FB_MARGIN;
        fb_cursor_y += char_h;
    }

    if (fb_cursor_y + char_h > (int)height - FB_MARGIN) {
        fb_scroll_up();
        fb_cursor_y -= char_h;
        if (fb_cursor_y < FB_MARGIN) fb_cursor_y = FB_MARGIN;
    }
        
}

static int int_to_str(char* buf, int bufsize, uint32_t val, int base, bool uppercase) {
    const char* digits_lo = "0123456789abcdef";
    const char* digital_hi = "0123456789ABCDEF";
    const char* digitds = uppercase ? digital_hi : digits_lo;

    if (base < 2 || base > 16) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }

    char tmp[32];
    int len = 0;
    while (val > 0 && len < 31) {
        tmp[len++] = digitds[val % (uint32_t)base];
        val /= (uint32_t)base;
    }

    int out = 0;
    for (int i = len - 1; i >= 0; i--) {
        buf[out++] = tmp[i];
    }
    buf[out] = '\0';
    return out;
}

void printf_fb(const char* fmt) {
    for (int i = 0; fmt[i] != '\0'; i++) {
        putchar_fb(fmt[i]);
    }
}

void printf_fb_s(const char* fmt, const char* arg) {
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%' && fmt[i + 1] == 's') {
            for (int j = 0; arg[j] != '\0'; j++) {
                putchar_fb(arg[j]);
            }
            i++;
        }
        else {
            putchar_fb(fmt[i]);
        }
    }
}

void printf_fb_d(const char* fmt, int32_t arg) {
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%' && (fmt[i + 1] == 'd' || fmt[i + 1] == 'i')) {
            char buf[16];
            uint32_t uval = (uint32_t)arg;
          if (arg < 0) {
            putchar_fb('-');
            uval = (uint32_t)(-arg);
          }
          int_to_str(buf, 16, uval, 10, false);
          for (int j = 0; buf[j]; j++) {
            putchar_fb(buf[j]);
          }
          i++;
        }
        else if (fmt[i] == '%' && fmt[i + 1] == 'x') {
            char buf[16];
            int_to_str(buf, 16, (uint32_t)arg, 16, false);
            for (int j = 0; buf[j]; j++) putchar_fb(buf[j]);
            i++;
        }
        else if (fmt[i] == '%' && fmt[i + 1] == 'X') {
            char buf[16];
            int_to_str(buf, 16, (uint32_t)arg, 16, true);
            for ( int j = 0; buf[j]; j++) putchar_fb(buf[j]);
            i++;
        }
        else {
            putchar_fb(fmt[i]);
        }

    }    
}

int fb_get_cursor_x() { return fb_cursor_x; }
int fb_get_cursor_y() { return fb_cursor_y; }
void fb_set_cursor(int x, int y) {
    fb_cursor_x = x;
    fb_cursor_y = y;
}
