#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "stk/stk.h"
#include "rtk/rtk.h"

using namespace ctk;
using namespace stk;
using namespace rtk;

static constexpr u32 TASKBAR_HEIGHT = 60;
static constexpr u32 WINDOW_WIDTH = 1080;
static constexpr u32 WINDOW_HEIGHT = 720;

s32 main() {
    Stack* mem = create_stack(megabyte(4));

    win32_init();
    Window* window = create_window(mem, {
        .surface = {
            .x = 0,
            .y = TASKBAR_HEIGHT,
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
        },
        .title = L"Rend Test",
        .callback = stk::default_window_callback,
    });

    auto ctx = allocate<Context>(mem, 1);
    InitRend(ctx, mem);
}
