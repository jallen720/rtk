#pragma once

#include "ctk3/ctk3.h"
#include "ctk3/math.h"
#include "ctk3/stack.h"
#include "ctk3/array.h"
#include "ctk3/thread_pool.h"
#include "ctk3/window.h"

using namespace CTK;

/// Data
////////////////////////////////////////////////////////////
template<typename StateType>
struct Job
{
    Array<StateType> states;
    Array<TaskHnd>   tasks;
};

struct Mouse
{
    Vec2<sint32> position;
    Vec2<sint32> delta;
    Vec2<sint32> last_position;
};

/// Interface
////////////////////////////////////////////////////////////
template<typename StateType>
static void InitThreadPoolJob(Job<StateType>* job, Stack* perm_stack, uint32 thread_count)
{
    InitArrayFull(&job->states, &perm_stack->allocator, thread_count);
    InitArrayFull(&job->tasks,  &perm_stack->allocator, thread_count);
}

static void UpdateMouse(Mouse* mouse)
{
    mouse->position = GetMousePosition();
    mouse->delta = mouse->position - mouse->last_position;
    mouse->last_position = mouse->position;
}
