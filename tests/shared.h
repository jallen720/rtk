#pragma once

#include "ctk/ctk.h"
#include "ctk/math.h"
#include "stk/stk.h"

using namespace ctk;
using namespace stk;

/// Data
////////////////////////////////////////////////////////////
struct Mouse {
    Vec2<s32> position;
    Vec2<s32> delta;
    Vec2<s32> last_position;
};

/// Interface
////////////////////////////////////////////////////////////
static void UpdateMouse(Mouse* mouse, Window* window) {
    mouse->position = get_mouse_position(window);
    mouse->delta = mouse->position - mouse->last_position;
    mouse->last_position = mouse->position;
}
