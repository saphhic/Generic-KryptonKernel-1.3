#include "../../hal/io.h"
#include "disable.h"

/**
 * Implementación del apagado del modo texto VGA.
 */
extern "C" void disable_vga() {
    // Solo deshabilitamos el cursor de hardware para que no parpadee sobre el escritorio
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}