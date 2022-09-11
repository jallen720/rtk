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

static void FindPhysicalDevice(Context* rtk, Stack temp_mem, DeviceFeatures* required_features) {
    Array<VkPhysicalDevice>* vk_physical_devices = GetVkPhysicalDevices(&temp_mem, rtk->instance);
    auto discrete_devices = create_array<PhysicalDevice>(&temp_mem, vk_physical_devices->count);
    auto integrated_devices = create_array<PhysicalDevice>(&temp_mem, vk_physical_devices->count);
    for (u32 i = 0; i < vk_physical_devices->count; ++i) {
        PhysicalDevice physical_device = { .hnd = get_copy(vk_physical_devices, i) };
        GetPhysicalDeviceInfo(&physical_device, temp_mem, &rtk->surface);

        if (physical_device.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            push(discrete_devices, physical_device);
        }
        else if (physical_device.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            push(integrated_devices, physical_device);
        }
        else {
            CTK_FATAL("unknown physical device type");
        }
    }

    // Find compatible physical device, prioritizing descrete devices.
    if (!UseCompatiblePhysicalDevice(rtk, discrete_devices, required_features) &&
        !UseCompatiblePhysicalDevice(rtk, integrated_devices, required_features))
    {
        CTK_FATAL("failed to find physical device");
    }

    LogPhysicalDevice(&rtk->physical_device, "selected physical device");
}

static void InitRTK(Context* rtk, Stack* mem, Stack temp_mem, Window* window) {
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
    FindPhysicalDevice(rtk, temp_mem, &required_features);
    InitDevice(rtk, &required_features);
    InitQueues(rtk);
    GetSurfaceInfo(rtk, mem);
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

    auto rtk = allocate<Context>(mem, 1);
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
