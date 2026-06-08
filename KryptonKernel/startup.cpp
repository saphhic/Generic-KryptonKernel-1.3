// written by saphhic.
#include "drivers/vga/keymap.h"
#include "drivers/vga/printf.h"
#include "drivers/vga/disable.h"
#include "hal/userf/user.h"
#include "hal/startup.h"
#include "hal/userf/kmalloc.h"
#include "hal/comparator.h"
#include "ulsysid/ulsys.h"
#include "filesystem/vfs.h"

#if defined(LAWAI_TARGET_BIOS)
#include "drivers/ethernet/ethernet.h"
#include "drivers/ethernet/netstack.h"
#endif

#if defined(LAWAI_ENABLE_FRAMEBUFFER)
#include "drivers/frameb/framebuffer.hpp" // Incluir framebuffer.hpp aquí
#include "hal/fbterminal.h" // Para fbterminal_main()
#endif

#if defined(LAWAI_ENABLE_FRAMEBUFFER)
static bool use_framebuffer_console = false;
static bool want_desktop = true;
#endif

static void startup_print(const char* text) {
#if defined(LAWAI_ENABLE_FRAMEBUFFER)
    if (use_framebuffer_console && framebuffer_available()) {
        printf_fb(text);
        return;
    }
#endif
    printf(text);
}

static void startup_clear() {
#if defined(LAWAI_ENABLE_FRAMEBUFFER)
    if (use_framebuffer_console && framebuffer_available()) {
        fb_clear_screen();
        return;
    }
#endif
    clear_screen();
}

static void init_startup_console() {
#if defined(LAWAI_ENABLE_FRAMEBUFFER)
    framebuffer_init();
    use_framebuffer_console = framebuffer_available();
    kb_set_framebuffer_echo(use_framebuffer_console);
    startup_clear();
#else
    kb_set_framebuffer_echo(false);
    clear_screen();
#endif
}

static LoginResult do_login() {
    startup_print("Username: ");
    char* username = kstrdup(read_line());

    startup_print("Password: ");
    char* password = kstrdup(read_line());

    LoginResult result = LOGIN_ERROR;
    if (username && password) {
        result = login_user(username, password);
    }
    kfree(username);
    kfree(password);
    return result;
}

static void do_create_user() {
    startup_print("Enter new username: ");
    char* new_username = kstrdup(read_line());

    startup_print("Enter new password: ");
    char* new_password = kstrdup(read_line());

    if (!new_username || !new_password) {
        startup_print("Failed to read username or password.\n");
        kfree(new_username);
        kfree(new_password);
        return;
    }

    UserCreationResult r = create_normal_user(new_username, new_password);

    switch (r) {
        case CREATE_OK:
            startup_print("User created successfully, logging in..\n");
            login_user(new_username, new_password);
            break;
        case CREATE_EXISTS:
            startup_print("ERROR: User already exists.\n");
            break;
        case CREATE_FULL:
            startup_print("ERROR: User limit reached.\n");
            break;
        case CREATE_BAD_NAME:
            startup_print("ERROR: Invalid username. (only a-z, A-Z, 0-9, _ allowed\n");
            break;
        case CREATE_BAD_PASS:
            startup_print("ERROR: Invalid password.\n");
            break;
        default:
            startup_print("An error occurred while creating the user.\n");
            break;
    }

    kfree(new_username);
    kfree(new_password);
}

static void run_login_phase() {
    while (!check_login()) {
        startup_print("Login or type 'new' to create an account.\n");

        LoginResult result = do_login();
        switch (result) {
            case LOGIN_OK:
                startup_print("Login successful!\n");
                 break;

            case LOGIN_NO_USER: {
                startup_print("No such user. Type 'new' to create an account.\n");

                char* choice = read_line();
                if (compare(choice, "new") || compare(choice, "NEW")) {
                    do_create_user();
                } 
                break;
            }

            case LOGIN_BAD_PASS:
                startup_print("Incorrect password.\n");
                break;
            case LOGIN_LOCKED:
                startup_print("Account locked due to too many failed attempts. Reboot to try again\n");
                while (true) { __asm__ volatile("hlt"); }
                break;

            default:
                startup_print("An error occurred during login. try again\n");
                break;
        }
    }
}


static void run_hardware_init() {
    #if defined(LAWAI_TARGET_BIOS)
        ethernet_init();
        netstack_init();
    #else
        startup_print("Ethernet failed to initialize: not supported on this chip.\n");
    #endif
}


static void run_output_phase() {
#if defined(LAWAI_ENABLE_FRAMEBUFFER)
    if (use_framebuffer_console && framebuffer_available()) {
        if (want_desktop) {
            ulsys_main();
        } else {
            // El usuario eligió no ir al escritorio, iniciar fbterminal
            fbterminal_main();
        }
        return; // ulsys_main() es un bucle infinito, este return es teórico.
    }
    // Framebuffer no disponible o no inicializado, y no hay fallback a terminal antiguo.
    startup_print("Framebuffer no disponible o no inicializado. El sistema se detendra.\n");
#endif
    while(true) { __asm__ volatile ("hlt"); } // Detener si el escritorio no puede iniciarse
}

static void quest_phase() {
    startup_print("Welcome to KryptonKernel!\n");
    startup_print("do you wish to continue to desktop? (y/n)\n");
    char* input = kstrdup(read_line());
    if (compare(input, "y") || compare(input, "Y")) {
        startup_print("Continuing to desktop...\n");
        want_desktop = true;
    }
    else {
        want_desktop = false;
    }
    kfree(input);
}

void startup() {
    init_startup_console();
    kb_init();
    startup_print("KryptonKernel starting...\n");

    init_default_user();
    startup_clear();
    run_login_phase();

    quest_phase();

    startup_clear();
    startup_print("kernel started successfully!\n");
    fs_init();
    run_hardware_init();
    run_output_phase();
}

   
