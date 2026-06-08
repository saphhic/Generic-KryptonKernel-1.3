// written by saphhic.
#include "hal/startup.h"
#include "hal/comparator.h"
#include "ulsysid/ulsys.h"
#include "hal/startup.h"
#include "hal/terminal.h"
#include <stdint.h>

extern "C" void kernel_main() {
    startup(); 
    while (true) {
        // Kernel main loop
        __asm__ volatile ("hlt"); // Halt the CPU until the next interrupt
    }
}

