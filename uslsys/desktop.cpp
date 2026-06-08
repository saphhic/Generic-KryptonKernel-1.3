// written by saphhic.
// desktop.cpp — Implementación del Bucle Principal del Escritorio

#include "desktop.h"
#include "ui_lib/ui_desktop.h"
#include "ui_lib/ui_window.h"
#include "ui_lib/ui_terminal_window.h" // Nuevo include para la ventana de terminal
#include "ui_lib/ui_font.h" // <-- Añadir esta línea
#include "ui_lib/ui_primitives.h"
#include "ui_lib/ui_theme.h"
#include "../KryptonKernel/drivers/frameb/framebuffer.hpp"
#include "../KryptonKernel/drivers/input/ps2_mouse.h"
#include "../KryptonKernel/hal/userf/kmalloc.h"
#include "../KryptonKernel/drivers/vga/lector.h"
#include "../KryptonKernel/hal/time/rtc.h"
#include "../KryptonKernel/drivers/vga/keymap.h"
#include "../KryptonKernel/hal/kernelpanic.h" // <-- Añadir esta línea

// Callbacks para los iconos del Dock (estilo macOS/Ubuntu)
static void cb_terminal(int cx, int cy, int sz, bool act) { ui_icon_terminal(cx, cy, sz, UI_C_ACCENT); }
static void cb_files(int cx, int cy, int sz, bool act)    { ui_icon_folder(cx, cy, sz, 0xFFF4BF4Fu); } // Amarillo carpeta
static void cb_settings(int cx, int cy, int sz, bool act) { ui_icon_settings(cx, cy, sz, UI_C_TEXT_SECONDARY); }
static void cb_info(int cx, int cy, int sz, bool act)     { ui_icon_system(cx, cy, sz, UI_C_ACCENT2); }

static uint32_t s_last_input_seconds = 0;
static const uint32_t WATCHDOG_TIMEOUT_SECONDS = 5; // 5 segundos de inactividad para el watchdog

void desktop_main() {
    // 1. Inicialización de sistemas UI con la resolución actual
    int screen_w = (int)width;
    int screen_h = (int)height;

    ui_desktop_init(screen_w, screen_h);
    ui_win_system_init(screen_w, screen_h, UI_TOPBAR_H, UI_TASKBAR_H);
    mouse_init(screen_w, screen_h);

    // Inicializar el sistema de terminales
    ui_terminal_init_system();
    
    // 1.1 Crear Backbuffer para evitar parpadeo (Doble Buffering)
    // Usamos pitch * height para asegurar que el buffer sea del tamaño exacto de la VRAM
    uint32_t buffer_size = pitch * screen_h;
    uint32_t* back_buffer = (uint32_t*)kmalloc(buffer_size);
    if (!back_buffer) kernel_panic("Desktop: Fallo al asignar backbuffer");

    uint32_t* real_fb = framebuffer;

    // 2. Configurar Dock estilo moderno
    ui_taskbar_clear();
    ui_taskbar_add_icon(1, "Terminal", cb_terminal);
    ui_taskbar_add_icon(2, "Archivos", cb_files);
    ui_taskbar_add_icon(3, "Ajustes",  cb_settings);
    ui_taskbar_add_icon(4, "Información",  cb_info);

    // Crear una ventana de bienvenida por defecto
    int win_welcome = ui_window_create("Bienvenido a KryptonOS", -1, -1, 500, 300);
    
    bool running = true;
    int active_app = -1;
    
    // Mapeo para no duplicar ventanas y poder restaurar minimizadas
    static int app_windows[5] = {-1, -1, -1, -1, -1};

    const char* current_user = "saphhic"; // Esto vendría del sistema de login

    // Inicializar el watchdog
    s_last_input_seconds = rtc_get_total_seconds();

    // Bucle de renderizado y eventos
    while (running) {
        // --- A. Entrada de datos (Polling) ---
        mouse_poll();
        const MouseState& ms = mouse_get_state();
        
        // --- B. Lógica de Interacción ---
        
        uint32_t current_total_seconds = rtc_get_total_seconds();

        // Manejar ventanas primero (el foco lo consume la ventana)
        bool mouse_on_win = ui_window_handle_mouse(ms.x, ms.y, ms.left);
        
        // Usamos una variable local para el click y lo "limpiamos" inmediatamente
        bool was_clicked = ms.clicked;
        if (was_clicked) {
            UiWinHit hit_type = ui_window_handle_click(ms.x, ms.y);
            
            // Si no hicimos click en una ventana, probamos el Dock
            if (!mouse_on_win) {
                int app_hit = ui_taskbar_hit(ms.x, ms.y);
                if (app_hit != -1) {
                    active_app = app_hit;
                    
                    int existing_win = app_windows[app_hit];
                    // Si la ventana existe, la restauramos y subimos al frente
                    if (ui_window_is_open(existing_win) || ui_window_is_minimized(existing_win)) {
                        ui_window_restore(existing_win);
                        ui_window_raise(existing_win);
                    } else {
                        // Si no existe, la creamos
                        if (app_hit == 1) app_windows[1] = ui_terminal_create("Terminal de Krypton", 50, 80, 600, 400);
                        else if (app_hit == 2) app_windows[2] = ui_window_create("Explorador de Archivos", 100, 120, 500, 350);
                        else if (app_hit == 3) app_windows[3] = ui_window_create("Configuracion", 150, 160, 400, 450);
                        else if (app_hit == 4) app_windows[4] = ui_window_create("Acerca del Sistema", -1, -1, 350, 200);
                    }
                }

                // Click en Topbar (Menú)
                int top_hit = ui_topbar_hit(ms.x, ms.y);
                if (top_hit == 0) {
                    // Abrir menú de aplicaciones...
                }
            }
        }

        // --- C. Dibujado (En orden de capas) ---
        framebuffer = back_buffer;
        
        // 1. Fondo de pantalla (Wallpaper)
        ui_desktop_draw();
        
        // 2. Ventanas
        ui_window_draw_all();

        // 2.1 Dibujar contenido dentro de las ventanas
        for (int i = 0; i < UI_WIN_MAX; i++) {
            if (ui_window_is_open(i)) {
                UiRect body = ui_window_body_rect(i);
                if (i == app_windows[1]) {
                    ui_terminal_draw(i);
                } else {
                    // Placeholder de contenido funcional para otras ventanas
                    ui_font16_print(body.x + 15, body.y + 15, "> System Ready.", UI_C_ACCENT, 0, true);
                    ui_font16_print(body.x + 15, body.y + 35, ui_window_name(i), UI_C_TEXT_SECONDARY, 0, true);
                    draw_rect(body.x + 15, body.y + 60, body.w - 30, 1, UI_C_WIN_BORDER);
                }
            }
        }
        
        // 3. UI del Sistema (Barras siempre arriba)
        RtcDateTime now;
        if (rtc_get_local_time(&now)) {
            ui_topbar_draw(now.hour, now.minute, current_user);
        }
        
        // Dibujar el Dock con hover effect
        ui_taskbar_draw(active_app, ms.x, ms.y);
        
        // 4. Cursor del ratón (Lo último siempre)
        ui_draw_cursor(ms.x, ms.y);

        // --- D. Swap Buffers (Copiar Backbuffer a la pantalla real) ---
        framebuffer = real_fb;
        // Copia directa de memoria (se puede optimizar con memcpy si lo tienes)
        // Si tienes una función memcpy optimizada en tu kernel, puedes usarla aquí:
        // if (memcpy_available) {
        //     memcpy(real_fb, back_buffer, screen_w * screen_h * 4);
        // } else {
        //     for (int i = 0; i < screen_w * screen_h; i++) {
        //         real_fb[i] = back_buffer[i];
        //     }
        // }
        for (int i = 0; i < screen_w * screen_h; i++) {
            real_fb[i] = back_buffer[i];
        }

        // Pequeño delay para no saturar la CPU en el emulador
        // En un SO real usarías un timer o vsync
        for (volatile int i = 0; i < 100000; i++) { __asm__ volatile("pause"); }

        // Tecla de pánico/salida (Esc)
        /*
        if (kb_peek_scancode() == 0x01) {
            running = false;
        }
        */
        // Manejo de entrada de teclado real para la terminal
        char k = 0;
        SpecialKey sk = KEY_NONE;
        if (kb_poll_key(&k, &sk)) {
            int focused = ui_window_focused();
            if (focused != -1 && focused == app_windows[1]) {
                if (k != 0) ui_terminal_handle_key(focused, k);
            }
        }

        // --- Watchdog Check ---
        // Si el tiempo actual es menor al anterior (medianoche) o el RTC falló (0),
        // simplemente reseteamos el marcador sin entrar en pánico.
        if (current_total_seconds < s_last_input_seconds || current_total_seconds == 0) {
            s_last_input_seconds = current_total_seconds;
        } else {
            int diff = (int)(current_total_seconds - s_last_input_seconds);
            // Solo entramos en pánico si una sola vuelta del bucle tarda más de 5 segundos.
            // Esto indica que el sistema está colgado en una operación (deadlock).
            if (diff > (int)WATCHDOG_TIMEOUT_SECONDS) {
                kernel_panic("Watchdog: El sistema se ha bloqueado.");
            }
        }

        // Marcamos que el bucle ha completado una vuelta exitosa
        s_last_input_seconds = current_total_seconds;
    }
}