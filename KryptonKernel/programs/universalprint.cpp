//written by saphhic.

#include "../drivers/vga/printf.h"
#include "../drivers/frameb/framebuffer.hpp"

void uprint(const char* str) {
    if (framebuffer_available()) {
        printf_fb(str);
    } else {
        printf(str);
    }
}