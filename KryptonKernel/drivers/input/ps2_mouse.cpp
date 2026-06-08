#include "ps2_mouse.h"
#include "../../hal/io.h"
#include "../vga/keymap.h"

static const uint16_t PS2_DATA = 0x60;
static const uint16_t PS2_STATUS = 0x64;
static const uint16_t PS2_COMMAND = 0x64;

static const uint8_t STATUS_OUT_FULL = 0x01;
static const uint8_t STATUS_IN_FULL = 0x02;
static const uint8_t STATUS_AUX_DATA = 0x20;

static const uint8_t CMD_READ_CONFIG = 0x20;
static const uint8_t CMD_WRITE_CONFIG = 0x60;
static const uint8_t CMD_ENABLE_AUX = 0xA8;
static const uint8_t CMD_WRITE_AUX = 0xD4;

static const uint8_t MOUSE_ACK = 0xFA;
static const uint8_t MOUSE_RESEND = 0xFE;

static MouseState state = { false, 0, 0, 0, 0, false, false, false, false, false };
static int max_x = 1023;
static int max_y = 767;
static uint8_t packet[3];
static int packet_index = 0;

static bool wait_input_clear() {
    for (int i = 0; i < 10000; i++) {
        if ((inb(PS2_STATUS) & STATUS_IN_FULL) == 0) return true;
    }
    return false;
}

static bool wait_output_full() {
    for (int i = 0; i < 10000; i++) {
        if ((inb(PS2_STATUS) & STATUS_OUT_FULL) != 0) return true;
    }
    return false;
}

static void flush_output() {
    for (int i = 0; i < 32; i++) {
        uint8_t status = inb(PS2_STATUS);
        if ((status & STATUS_OUT_FULL) == 0) return;

        uint8_t value = inb(PS2_DATA);
        if ((status & STATUS_AUX_DATA) == 0) {
            kb_queue_scancode(value);
        }
    }
}

static bool write_command(uint8_t command) {
    if (!wait_input_clear()) return false;
    outb(PS2_COMMAND, command);
    return true;
}

static bool write_data(uint8_t data) {
    if (!wait_input_clear()) return false;
    outb(PS2_DATA, data);
    return true;
}

static bool read_data(uint8_t* data) {
    if (!data || !wait_output_full()) return false;
    *data = inb(PS2_DATA);
    return true;
}

static bool read_aux_data(uint8_t* data) {
    if (!data) return false;

    for (int i = 0; i < 1000; i++) { // REDUCIDO de 5000 a 1000 para evitar bloqueos largos
        uint8_t status = inb(PS2_STATUS);
        if ((status & STATUS_OUT_FULL) == 0) {
            __asm__ volatile("pause");
            continue;
        }

        uint8_t value = inb(PS2_DATA);
        if ((status & STATUS_AUX_DATA) != 0) {
            *data = value;
            return true;
        }

        kb_queue_scancode(value);
    }

    return false;
}

static bool read_mouse_response(uint8_t* data) {
    if (!data) return false;

    for (int i = 0; i < 1000; i++) { // REDUCIDO de 5000 a 1000 para evitar bloqueos largos
        uint8_t status = inb(PS2_STATUS);
        if ((status & STATUS_OUT_FULL) == 0) {
            __asm__ volatile("pause");
            continue;
        }

        uint8_t value = inb(PS2_DATA);
        if ((status & STATUS_AUX_DATA) != 0 || value == MOUSE_ACK || value == MOUSE_RESEND) {
            *data = value;
            return true;
        }

        kb_queue_scancode(value);
    }

    return false;
}

static bool mouse_write(uint8_t data) {
    for (int attempt = 0; attempt < 3; attempt++) {
        if (!write_command(CMD_WRITE_AUX)) return false;
        if (!write_data(data)) return false;

        uint8_t response = 0;
        if (!read_mouse_response(&response)) return false;
        if (response == MOUSE_ACK) return true;
        if (response != MOUSE_RESEND) return false;
    }

    return false;
}

static int sign_extend(uint8_t value, bool negative) {
    int out = (int)value;
    if (negative) out -= 256;
    return out;
}

void mouse_set_bounds(int screen_width, int screen_height) {
    if (screen_width > 0) max_x = screen_width - 1;
    if (screen_height > 0) max_y = screen_height - 1;

    if (state.x > max_x) state.x = max_x;
    if (state.y > max_y) state.y = max_y;
    if (state.x < 0) state.x = 0;
    if (state.y < 0) state.y = 0;
}


bool mouse_init(int screen_width, int screen_height) {
    state.present = false;
    state.dx = 0;
    state.dy = 0;
    state.moved = false;
    state.clicked = false;
    state.left = false;
    state.right = false;
    state.middle = false;
    packet_index = 0;
    mouse_set_bounds(screen_width, screen_height);
    state.x = max_x / 2;
    state.y = max_y / 2;

    flush_output(); // limpiar basura primero

    if (!write_command(CMD_ENABLE_AUX)) return false;

    // Leer config byte - si falla, usar un valor seguro por defecto en vez de abortar
    uint8_t status_byte = 0x47; // valor típico: KB clock on, mouse clock on, traducción activa
    if (write_command(CMD_READ_CONFIG)) {
        uint8_t tmp;
        if (read_data(&tmp)) status_byte = tmp;
    }

    status_byte &= ~0x02; // deshabilitar IRQ mouse (polling)
    // NO tocar bit 0x01 (IRQ teclado) - dejarlo como está
    status_byte &= ~0x20; // habilitar reloj del ratón

    if (!write_command(CMD_WRITE_CONFIG)) return false;
    if (!write_data(status_byte)) return false;

    // ... resto igual
    // Resetear el ratón a valores de fábrica
    bool reset_ok = mouse_write(0xFF);
    uint8_t self_test = 0, mouse_id = 0;
    bool got_aa = read_mouse_response(&self_test) && (self_test == 0xAA);
    read_mouse_response(&mouse_id);

    if (!reset_ok || !got_aa) {
        state.present = false;
        return false; // Fallar limpiamente si el mouse no está
    }

    // 3. Activar el muestreo de datos (Streaming mode)
    if (!mouse_write(0xF4)) return false;

    mouse_set_bounds(screen_width, screen_height);
    state.x = max_x / 2;
    state.y = max_y / 2;
    state.dx = 0;
    state.dy = 0;
    state.moved = false;
    state.clicked = false;
    state.left = false;
    state.right = false;
    state.middle = false;
    packet_index = 0;
    state.present = true;
    
    return true;
}

static bool process_packet() {
    uint8_t flags = packet[0];
    if ((flags & 0x08) == 0) {
        packet_index = 0;
        return false;
    }

    if ((flags & 0x40) || (flags & 0x80)) {
        state.dx = 0;
        state.dy = 0;
        state.moved = false;
        return false;
    }

    int dx = sign_extend(packet[1], (flags & 0x10) != 0);
    int dy = sign_extend(packet[2], (flags & 0x20) != 0);
    bool was_left = state.left;
    bool was_right = state.right;
    bool was_middle = state.middle;

    state.dx = dx;
    state.dy = -dy;
    state.x += dx;
    state.y -= dy;

    if (state.x < 0) state.x = 0;
    if (state.y < 0) state.y = 0;
    if (state.x > max_x) state.x = max_x;
    if (state.y > max_y) state.y = max_y;

    state.left = (flags & 0x01) != 0;
    state.right = (flags & 0x02) != 0;
    state.middle = (flags & 0x04) != 0;
    state.clicked |= (state.left && !was_left); // Acumular clicks procesados en este poll
    state.moved = dx != 0 || dy != 0;

    return state.moved ||
           state.clicked ||
           state.left != was_left ||
           state.right != was_right ||
           state.middle != was_middle;
}

bool mouse_poll() {
    // Drenamos el buffer siempre, incluso si el ratón no está "presente",
    // para evitar que los datos del ratón bloqueen al teclado.
    state.clicked = false;

    bool changed = false;

    // Drenar un lote completo evita que el KBC se llene si el raton se mueve rapido.
    for (int i = 0; i < 32; i++) {
        uint8_t status = inb(PS2_STATUS);
        if ((status & STATUS_OUT_FULL) == 0) break;

        if ((status & STATUS_AUX_DATA) == 0) {
            // Es teclado: lo mandamos a la cola para que kb_poll_key lo vea
            uint8_t data = inb(PS2_DATA);
            kb_queue_scancode(data);
            continue;
        }

        // Si llegamos aquí y el ratón no está listo, simplemente tiramos el byte
        if (!state.present) { inb(PS2_DATA); continue; }

        uint8_t data = inb(PS2_DATA);
        if (packet_index == 0 && (data & 0x08) == 0) {
            continue;
        }

        packet[packet_index++] = data;
        if (packet_index == 3) {
            packet_index = 0;
            changed = process_packet() || changed;
        }
    }

    return changed;
}

const MouseState& mouse_get_state() {
    return state;
}

void mouse_drain_aux() {
    for (int i = 0; i < 16; i++) {
        uint8_t status = inb(PS2_STATUS);
        if ((status & STATUS_OUT_FULL) == 0) return;

        uint8_t value = inb(PS2_DATA);
        if ((status & STATUS_AUX_DATA) == 0) {
            kb_queue_scancode(value);
            return;
        }
    }
}
