//written by shappic.
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

extern uint32_t FRAMEBUFFER_ADDR;
extern uint32_t FRAMEBUFFER_WIDTH;
extern uint32_t FRAMEBUFFER_HEIGHT;
extern uint32_t FRAMEBUFFER_PITCH;

extern uint32_t* framebuffer;
extern uint32_t width;
extern uint32_t height;
extern uint32_t pitch;

extern "C" void framebuffer_init_stub();

uint32_t fb_color(uint8_t r, uint8_t g, uint8_t b);
void framebuffer_init();
bool framebuffer_available();
void fb_set_scale(int scale);
void fb_set_colors(uint32_t fg, uint32_t bg);
int fb_get_cursor_x();
int fb_get_cursor_y();
void fb_set_cursor(int x, int y);



//color table (format 0xAARRGGBB):
#define FB_BLACK       fb_color(0,   0,   0  )
#define FB_WHITE       fb_color(255, 255, 255)
#define FB_RED         fb_color(220, 50,  50 )
#define FB_GREEN       fb_color(50,  200, 80 )
#define FB_BLUE        fb_color(60,  120, 220)
#define FB_YELLOW      fb_color(240, 220, 60 )
#define FB_CYAN        fb_color(60,  200, 220)
#define FB_MAGENTA     fb_color(200, 60,  200)
#define FB_ORANGE      fb_color(240, 140, 40 )
#define FB_GRAY        fb_color(128, 128, 128)
#define FB_DARK_GRAY   fb_color(64,  64,  64 )
#define FB_LIGHT_GRAY  fb_color(192, 192, 192)
#define FB_PURPLE      fb_color(120, 60,  200)
#define FB_PINK        fb_color(220, 120, 180)
#define FB_LIME        fb_color(120, 220, 60 )
#define FB_BROWN       fb_color(140, 80,  40 )

void put_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha);
void draw_rect_vgradient(int x, int y, int w, int h, uint32_t top, uint32_t bottom);
void draw_rect_outline(int x, int y, int w, int h, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_circule(int cx, int cy, int r, uint32_t color);
void draw_filled_circle(int cx, int cy, int r, uint32_t color);

void draw_char(int px, int py, char c, uint32_t fg, uint32_t bg, int scale);
void print(int x, int y, const char* str, uint32_t color);
void putchar_fb(char c);
void fb_clear_screen();
void printf_fb(const char* str);

void printf_fb_s(const char* fmt, const char* arg);
void printf_fb_d(const char* fmt, int32_t arg);

#endif // FRAMEBUFFER_H
