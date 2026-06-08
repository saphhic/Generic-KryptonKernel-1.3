
// written by saphhic.
// Updated version by saphhic, 2024-06-15.
// Added Spanish layout and special key handling.

#include "lector.h"
#include "printf.h"
#include "keymap.h"
#include "../../hal/io.h"
#include "../input/ps2_mouse.h"

#if defined(LAWAI_ENABLE_FRAMEBUFFER)
#include "../frameb/framebuffer.hpp"
#endif

#define BUF_SIZE 256
#define SC_RELEASE 0x80
#define SC_EXTENDED 0xE0

#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_CAPS 0x3A
#define SC_LCTRL 0x1D
#define SC_LATL 0x38
#define SC_BACKSPACE 0x0E
#define SC_ENTER 0x1C
#define SC_TAB 0x0F
#define SC_SPACE 0x39
#define SC_ESCAPE 0x01

#define EX_UP 0x48
#define EX_DOWN 0x50
#define EX_LEFT 0x4B
#define EX_RIGHT 0x4D
#define EX_HOME 0x47
#define EX_END 0x4F
#define EX_PAGEUP 0x49
#define EX_PAGEDOWN 0x51
#define EX_INSERT 0x52
#define EX_DELETE 0x53


//--STANDARD US ENGLISH QWERTY KEYMAP--
static const char en_normal[58] = {
    /*00*/ 0, 0, '1', '2', '3', '4', '5', '6', '7', '8',
    /*10*/ '9', '0', '-', '=', 0, 0, 'q', 'w', 'e', 'r',
    /*20*/ 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
    /*30*/ 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    /*40*/ '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    /*50*/ 'm', ',', '.', '/', 0, '*', 0, ' '
};

static const char en_shift[58] = {
    /*00*/ 0, 0, '!', '@', '#', '$', '%', '^', '&', '*',
    /*10*/ '(', ')', '_', '+', 0, 0, 'Q', 'W', 'E', 'R',
    /*20*/ 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0,
    /*30*/ 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    /*40*/ '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
    /*50*/ 'M', '<', '>', '?', 0, '*', 0, ' '
};

//--STANDARD ES ESPANOL QWERTY KEYMAP--
static const char es_normal[58] = {
    /*00*/ 0, 0, '1', '2', '3', '4', '5', '6', '7', '8',
    /*10*/ '9', '0', '\'', 0, 0, 0, 'q', 'w', 'e', 'r',
    /*20*/ 't', 'y', 'u', 'i', 'o', 'p', '`', '+', 0, 0,
    /*30*/ 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '\xf1',
    /*40*/ '\'', '\xfa', 0, '<', 'z', 'x', 'c', 'v', 'b',
    /*50*/ 'm', ',', '.', '-', 0, '*', 0, ' '
};

static const char es_shift[58] = {
    /*00*/ 0, 0, '!', '"', '#', '$', '%', '&', '/', '(', 
    /*10*/ ')', '=', '?', '|', 0, 0, 'Q', 'W', 'E', 'R',
    /*20*/ 'T', 'Y', 'U', 'I', 'O', 'P', '^', '*', 0, 0,
    /*30*/ 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '\xd1',
    /*40*/ '{', '}', 0, '>', 'Z', 'X', 'C', 'V', 'B', 'N',
    /*50*/ 'M', ';', ':', '_', 0, '*', 0, ' '
};

//--FN F1-F12 KEYMAP--
static const char* fn_keys[12] = {
    "[F1]", "[F2]", "[F3]", "[F4]", "[F5]", "[F6]",
    "[F7]", "[F8]", "[F9]", "[F10]", "[F11]", "[F12]"
};

static bool shift_down = false;
static bool caps_lock = false;
static bool ctrl_down = false;
static bool alt_down = false;
static bool extended = false;

//--DEFAULT KEYMAP (US ENGLISH)--
static KeyLayout current_layout = LAYOUT_EN;

static char line_buf[BUF_SIZE];
static int line_len = 0;
static bool echo_to_framebuffer = false;

static SpecialKey last_special = KEY_NONE;

static const int SCANCODE_QUEUE_SIZE = 64;
static uint8_t scancode_queue[SCANCODE_QUEUE_SIZE];
static int scancode_head = 0;
static int scancode_tail = 0;

void kb_init() {
    // Vaciar el buffer del KBC
    for (int i = 0; i < 32; i++) {
        if ((inb(0x64) & 0x01) == 0) break;
        inb(0x60);
    }
}

void kb_set_layout(KeyLayout layout) {
    current_layout = layout;
}

KeyLayout kb_get_layout() {
    return current_layout;
}

SpecialKey kb_last_special() {
    SpecialKey k = last_special;
    last_special = KEY_NONE;
    return k;
}

void kb_set_framebuffer_echo(bool enabled) {
    echo_to_framebuffer = enabled;
}

void kb_queue_scancode(uint8_t scancode) {
    int next_tail = (scancode_tail + 1) % SCANCODE_QUEUE_SIZE;
    if (next_tail == scancode_head) {
        return;
    }

    scancode_queue[scancode_tail] = scancode;
    scancode_tail = next_tail;
}

static bool kb_pop_scancode(uint8_t* out_scancode) {
    if (!out_scancode || scancode_head == scancode_tail) return false;

    *out_scancode = scancode_queue[scancode_head];
    scancode_head = (scancode_head + 1) % SCANCODE_QUEUE_SIZE;
    return true;
}

static uint8_t kb_read_scancode_blocking() {
    uint8_t scancode = 0;
    int timeout = 1000000; // Evitar cuelgue total si el hardware falla
    while (true) {
        if (kb_pop_scancode(&scancode)) return scancode;
        
        uint8_t status = inb(0x64);
        if (status & 0x01) {
            uint8_t data = inb(0x60); // LEER SIEMPRE para vaciar el buffer
            if (!(status & 0x20)) {
                return data; 
            }
            // Si es del ratón, lo ignoramos aquí para no colgar read_line()
            // pero el puerto ya quedó limpio para el siguiente ciclo.
        }
        __asm__ volatile("pause");
    }
}

static void echo_char(char c) {
#if defined(LAWAI_ENABLE_FRAMEBUFFER)
    if (echo_to_framebuffer && framebuffer_available()) {
        putchar_fb(c);
        return;
    }
#endif
    putchar(c);
}

static void update_leds() {
    auto wait_ibe = []() {
        for (int i = 0; i < 10000; i++) {
            uint8_t s;
            asm volatile("inb $0x64, %0" : "=a"(s));
            if (!(s & 0x02)) return;
        }
    };

    uint8_t led_byte = caps_lock ? 0x04 : 0x00;
    wait_ibe();
    asm volatile("outb %0, $0x60" : : "a"((uint8_t)0xED));
    wait_ibe();
    asm volatile("outb %0, $0x60" : : "a"(led_byte));
}

static char translate(uint8_t sc) {
    last_special = KEY_NONE;

    if (extended) {
        extended = false;
        bool released = (sc & SC_RELEASE);
        uint8_t base = sc & ~SC_RELEASE;

        if (base == SC_LCTRL) { ctrl_down = !released; return '\0'; }
        if (base == SC_LATL) { alt_down = !released; return '\0'; }

        if (released) return '\0';

        switch (base) {
            case EX_UP: last_special = KEY_UP; return '\0';
            case EX_DOWN: last_special = KEY_DOWN; return '\0';
            case EX_LEFT: last_special = KEY_LEFT; return '\0';
            case EX_RIGHT: last_special = KEY_RIGHT; return '\0';
            case EX_HOME: last_special = KEY_HOME; return '\0';
            case EX_END: last_special = KEY_END; return '\0';
            case EX_PAGEUP: last_special = KEY_PGUP; return '\0';
            case EX_PAGEDOWN: last_special = KEY_PGDN; return '\0';
            case EX_INSERT: last_special = KEY_INS; return '\0';
            case EX_DELETE: last_special = KEY_DEL; return '\0';

            case 0x1C: return '\n';
            default: return '\0';
        }
    }

    if (sc == SC_EXTENDED) {
        extended = true;
        return '\0';
    }

    if (sc & SC_RELEASE) {
        uint8_t base = sc & ~SC_RELEASE;
        if (base == SC_LSHIFT || base == SC_RSHIFT) shift_down = false;
        if (base == SC_LCTRL) ctrl_down = false;
        if (base == SC_LATL) alt_down = false;
        return '\0';
    }

    if (sc == SC_LSHIFT || sc == SC_RSHIFT) {
        shift_down = true;
        return '\0';
    }

    if (sc == SC_LCTRL) {
        ctrl_down = true;
        return '\0';
    }

    if (sc == SC_LATL) {
        alt_down = true;
        return '\0';
    }

    if (sc == SC_CAPS) {
        caps_lock = !caps_lock;
        update_leds();
        return '\0';
    }

    if (sc >= 0x3B && sc <= 0x44) {
        last_special = (SpecialKey)(KEY_F1 + (sc - 0x3B));
        return '\0';
    }
    if (sc == 0x57) {
        last_special = KEY_F11;
        return '\0';
    }
    if (sc == 0x58) {
        last_special = KEY_F12;
        return '\0';
    }

    if (sc == SC_BACKSPACE) return '\b';
    if (sc == SC_ENTER) return '\n';
    if (sc == SC_TAB) return '\t';
    if (sc == SC_SPACE) return ' ';

    if (sc >= 58) return '\0';

    const char* normal_map = (current_layout == LAYOUT_EN) ? en_normal : es_normal;
    const char* shift_map = (current_layout == LAYOUT_EN) ? en_shift : es_shift;

    bool use_shift = shift_down;

    char base_char = normal_map[sc];
    if (caps_lock && base_char >= 'a' && base_char <= 'z') {
        use_shift = !use_shift;
    }

    char c = use_shift ? shift_map[sc] : normal_map[sc];
    return c ? c : '\0';
}

char* read_line() {
    line_len = 0;
    line_buf[0] = '\0';

  while(true) {
    uint8_t sc = kb_read_scancode_blocking();
    char c = translate(sc);
    
    if (c == '\0') continue;

    if (c == '\b') {
        if (line_len > 0) {
            line_len--;
            line_buf[line_len] = '\0';
            echo_char('\b');
        }
        continue;
    }
    
    if (c == '\n') {
        line_buf[line_len] = '\0';
        echo_char('\n');
        return line_buf;
    } 

    if (c == '\t') {
        int spaces = 4 - (line_len % 4);
        for (int i = 0; i < spaces && line_len < BUF_SIZE - 1; i++) {
            line_buf[line_len++] = ' ';
            echo_char(' ');
        }
        line_buf[line_len] = '\0';
        continue;
    }

    if (line_len < BUF_SIZE - 1) {
        line_buf[line_len++] = c;
        line_buf[line_len] = '\0';
        echo_char(c);
    }
  }
}

char read_key() {
    while (true) {
        uint8_t sc = kb_read_scancode_blocking();
        char c = translate(sc);
        if (c != '\0') return c;
        if (last_special != KEY_NONE) return '\0';
    }
}

bool kb_poll_key(char* out_key, SpecialKey* out_special) {
    if (out_key) *out_key = '\0';
    if (out_special) *out_special = KEY_NONE;

    uint8_t sc = 0;

    // Primero intentar desde la cola software (llenada por mouse_poll)
    if (!kb_pop_scancode(&sc)) {
        // Cola vacía: leer directamente del hardware
        uint8_t status = inb(0x64);
        if (!(status & 0x01)) return false;  // nada en el buffer

        // Si es mouse, no consumir el byte: mouse_poll() mantiene el paquete alineado.
        if (status & 0x20) {
            // Es dato del mouse — descartarlo limpiamente y salir
            // mouse_poll() lo procesará en la siguiente iteración
            return false;
        }

        uint8_t data = inb(0x60);
        sc = data;
    }

    char c = translate(sc);
    SpecialKey special = kb_last_special();
    if (c == '\0' && special == KEY_NONE) return false;
    if (out_key) *out_key = c;
    if (out_special) *out_special = special;
    return true;
}
