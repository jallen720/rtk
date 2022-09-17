#include <windows.h>
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "stk/stk.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

using namespace ctk;
using namespace stk;
using namespace RTK;

static void Controls(Window* window) {
    if (key_down(window, Key::ESCAPE)) {
        window->open = false;
    }
}

static void SelectPhysicalDevice(RTKContext* rtk) {
    // Default to first physical device.
    UsePhysicalDevice(rtk, 0);

    // Use first discrete device if any are available.
    for (u32 i = 0; i < rtk->physical_devices.count; ++i) {
        if (get(&rtk->physical_devices, i)->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            UsePhysicalDevice(rtk, i);
            break;
        }
    }

    LogPhysicalDevice(rtk->physical_device, "selected physical device");
}

static void InitRTK(RTKContext* rtk, Stack* mem, Stack temp_mem, Window* window) {
    // Initialize RTK instance.
    InitInstance(rtk, {
        .application_name = "RTK Test",
        .debug_callback = DefaultDebugCallback,
    });
    InitSurface(rtk, window);

    // Initialize devices and queues.
    DeviceFeatures required_features = {
        .as_struct = {
            .geometryShader = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
        },
    };
    LoadCapablePhysicalDevices(rtk, temp_mem, &required_features);
    SelectPhysicalDevice(rtk);
    InitDevice(rtk, &required_features);
    InitQueues(rtk);
    GetSurfaceInfo(rtk, mem);

    // Initialize rendering state.
    InitSwapchain(rtk, mem, temp_mem);
}

s32 main() {
    Stack* mem = create_stack(megabyte(4));
    Stack* temp_mem = create_stack(mem, megabyte(1));

    win32_init();
    Window* window = create_window(mem, {
        .surface = {
            .x = 0,
            .y = 60, // Taskbar height.
            .width = 1080,
            .height = 720,
        },
        .title = L"RTK Test",
        .callback = default_window_callback,
    });

    auto rtk = allocate<RTKContext>(mem, 1);
    InitRTK(rtk, mem, *temp_mem, window);

    // Run test.
    while (1) {
        process_events(window);
        if (!window->open) {
            // Quit event closed window.
            break;
        }

        Controls(window);
        if (!window->open) {
            // Controls closed window.
            break;
        }

        if (window_is_active(window)) {
            // // Update game state.
            // update(game, window, renderer);
            // if (!window->open) {
            //     // Game update closed window.
            //     break;
            // }

            // // Start frame for rendering UI.
            // next_ui_frame();

            // // Render game UI.
            // render_game_ui(game, renderer);
            // if (!window->open) {
            //     // UI interaction closed window.
            //     break;
            // }

            // // Render debug UI.
            // render_debug_ui(&frame_metrics);

            // // Record and submit rendering commands based on updated game and UI state.
            // Frame* frame = next_frame(renderer);
            // record_entity_render_commands(renderer, frame, *temp_mem, &game->entities, &game->view);
            // record_ui_render_commands(renderer, frame);
            // submit_render_commands(renderer, frame);
        }
        else {
            Sleep(1);
        }
    }
}
