#pragma once

#include "ctk2/ctk.h"
#include "ctk2/math.h"
#include "stk2/stk.h"

using namespace CTK;
using namespace STK;

/// Data
////////////////////////////////////////////////////////////
struct Mouse
{
    Vec2<sint32> position;
    Vec2<sint32> delta;
    Vec2<sint32> last_position;
};

/// Interface
////////////////////////////////////////////////////////////
static void UpdateMouse(Mouse* mouse, Window* window)
{
    mouse->position = GetMousePosition(window);
    mouse->delta = mouse->position - mouse->last_position;
    mouse->last_position = mouse->position;
}
