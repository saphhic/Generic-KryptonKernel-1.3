#include "kernelpanic.h"
#include "../drivers/frameb/framebuffer.hpp"
#include "../drivers/vga/lector.h"
#include "reb_shtdw.h"
#include "startup.h"
#include "time/rtc.h" // Para rtc_get_total_seconds
#include "../drivers/vga/keymap.h" // Para kb_poll_key


void kernel_panic(const char* message) {
    // Deshabilitar interrupciones por si acaso
    __asm__ volatile("cli");

    if (!framebuffer_available()) {
        // Sin framebuffer, no podemos hacer nada visual
        __asm__ volatile("cli; hlt");
    }

    // 1. Fondo azul total
    draw_rect(0, 0, (int)width, (int)height, 0xFF0000AA);

    // 2. Panel central más oscuro
    int panel_w = (int)width  - 200;
    int panel_h = 180;
    int panel_x = 100;
    int panel_y = ((int)height - panel_h) / 2;
    draw_rect(panel_x, panel_y, panel_w, panel_h, 0xFF00007A);

    // 3. Texto
    // Título
    int title_x = panel_x + 20;
    int title_y = panel_y + 20;
    print(title_x, title_y, "KERNEL PANIC", 0xFFFFFFFF);

    // Línea separadora (rectángulo fino)
    draw_rect(panel_x + 10, title_y + 16, panel_w - 20, 1, 0xFF4444CC);

    // Mensaje de error
    print(title_x, title_y + 30, message, 0xFFFFFF88);

    // Mensaje inferior
    print(title_x, panel_y + panel_h - 30, 
          "El sistema se ha detenido. presiona cualquier tecla para reiniciar", 0xFFAAAAAA);

    // Esperar por una tecla o reiniciar automáticamente después de 3 segundos
    uint32_t start_time = rtc_get_total_seconds();
    bool key_pressed = false;
    while (rtc_get_total_seconds() < start_time + 3) {
        if (kb_poll_key(nullptr, nullptr)) { // Comprobar si se ha pulsado cualquier tecla
            key_pressed = true;
            break;
        }
        for (volatile int i = 0; i < 100000; i++) { __asm__ volatile("pause"); } // Pequeño delay
    }

    if (key_pressed) {
        startup(); // Intentar reiniciar normalmente
    } else {
        reboot();
    }
}

void safety_crash(const char* message) {
    // Deshabilitar interrupciones para tomar control total
    __asm__ volatile("cli");

    if (!framebuffer_available()) { __asm__ volatile("hlt"); }

    // IMPORTANTE: Si el escritorio estaba usando doble buffer, forzamos el puntero
    // a la memoria de video real para que el crash sea visible inmediatamente.
    framebuffer = (uint32_t*)FRAMEBUFFER_ADDR;

    // 1. Fondo rojo oscuro (Pantalla de Seguridad)
    draw_rect(0, 0, (int)width, (int)height, 0xFF880000);

    int panel_w = (int)width - 200;
    int panel_h = 220;
    int panel_x = 100;
    int panel_y = ((int)height - panel_h) / 2;
    draw_rect(panel_x, panel_y, panel_w, panel_h, 0xFF550000);

    int tx = panel_x + 20;
    int ty = panel_y + 20;

    print(tx, ty, "VIOLACION DE SEGURIDAD DETECTADA", 0xFFFFFFFF);
    draw_rect(panel_x + 10, ty + 16, panel_w - 20, 1, 0xFFFF0000);
    
    print(tx, ty + 40, "Motivo:", 0xFFFFFF88);
    print(tx, ty + 60, message, 0xFFFFFFFF);

    // 2. Countdown de 5 segundos
    for (int i = 5; i > 0; i--) {
        char count_msg[] = "Reiniciando en 5...  ";
        count_msg[15] = '0' + i;
        
        // Limpiar area del mensaje inferior
        draw_rect(tx, panel_y + panel_h - 40, panel_w - 40, 20, 0xFF550000);
        print(tx, panel_y + panel_h - 40, count_msg, 0xFFAAAAAA);

        // Esperar 1 segundo aproximadamente
        uint32_t start_s = rtc_get_total_seconds();
        // Si el RTC no avanza, usamos un loop de fallback
        if (start_s == 0) {
            for (volatile int j = 0; j < 50000000; j++) { __asm__ volatile("pause"); }
        } else {
            while (rtc_get_total_seconds() < start_s + 1) { __asm__ volatile("pause"); }
        }
    }

    // 3. Reinicio de hardware y VM directo
    reboot();
}