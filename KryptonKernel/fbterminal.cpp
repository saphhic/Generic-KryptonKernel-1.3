#include "drivers/frameb/framebuffer.hpp"
#include "hal/fbterminal.h"
#include "drivers/vga/keymap.h"
#include "hal/userf/user.h"
#include "hal/reb_shtdw.h"
#include "filesystem/vfs.h"
#include "../uslsys/ui_lib/fb_externcmds.h" // Usar el nuevo ejecutor de comandos
#include "hal/comparator.h" // Para text_eq_ci, starts_with_ci, skip_spaces, read_token

#if defined(LAWAI_TARGET_BIOS)
  #include "drivers/ethernet/netstack.h"
#endif

extern char* read_line();

// Callback para la salida de comandos en fbterminal
static void fbterminal_output_callback(const char* str) {
    if (str[0] == '\f') { // Manejar Form Feed para limpiar pantalla
        fb_clear_screen();
    } else {
        printf_fb(str);
    }
}

static void fb_print_prompt() {
    const char* username = get_current_username();
    printf_fb(username);
    printf_fb("@krypton:~$ ");
}

void fbterminal_main() {
  framebuffer_init();
  if (!framebuffer_available()) {
    kb_set_framebuffer_echo(false);
    return;
  }

  kb_set_framebuffer_echo(true);
  fb_clear_screen();

  printf_fb("Framebuffer Terminal Initialized.\n");
  printf_fb("Welcome to Krypton OS!\n");
  printf_fb("Type 'help' for a list of commands.\n\n");

  while (true) {
#if defined(LAWAI_TARGET_BIOS)
    netstack_poll();
#endif

    fb_print_prompt();

    char* input = read_line();
    if (input == nullptr || input[0] == '\0') {
        continue; // Ignore empty input
    }

    extern_cmd_execute(input, fbterminal_output_callback);
  }
}
