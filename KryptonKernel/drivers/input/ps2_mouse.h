#ifndef PS2_MOUSE_H
#define PS2_MOUSE_H

#include <stdint.h>

struct MouseState {
    bool present;
    int x;
    int y;
    int dx;
    int dy;
    bool left;
    bool right;
    bool middle;
    bool moved;
    bool clicked;
};

bool mouse_init(int screen_width, int screen_height);
bool mouse_poll();
const MouseState& mouse_get_state();
void mouse_set_bounds(int screen_width, int screen_height);
void mouse_drain_aux();

#endif
