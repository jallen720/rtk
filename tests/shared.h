#pragma once

#include "ctk3/ctk3.h"
#include "ctk3/math.h"
#include "ctk3/window.h"

using namespace CTK;

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
static void UpdateMouse(Mouse* mouse)
{
    mouse->position = GetMousePosition();
    mouse->delta = mouse->position - mouse->last_position;
    mouse->last_position = mouse->position;
}
