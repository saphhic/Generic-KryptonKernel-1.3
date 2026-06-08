#include "../KryptonKernel/drivers/headers.h"
#include "ui_lib/ustartup.h"
#include "desktop.h"

#include "../KryptonKernel/drivers/vga/keymap.h"
#include "../KryptonKernel/drivers/vga/printf.h"
#include "../KryptonKernel/hal/terminal.h"
#include "../KryptonKernel/hal/userf/user.h"
#include "../KryptonKernel/hal/startup.h"
#include "../KryptonKernel/hal/userf/kmalloc.h"
#include "../KryptonKernel/hal/comparator.h"
#include "../KryptonKernel/programs/universalprint.h"
#include "../KryptonKernel/hal/kernelpanic.h"

void ulsys_start() {
    disable_vga();
    fb_clear_screen();

    for (int attempt = 0; attempt < 3; attempt++) {
        desktop_main();
        
        // Si retorna, es un crash suave. Esperar un poco antes de reintentar.
        for (volatile int i = 0; i < 10000000; i++) { __asm__ volatile("pause"); }
    }

    // Tres intentos fallidos
    kernel_panic("Desktop: pantalla negra persistente tras 3 intentos");
}