#include <windows.h>
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/multithreading.h"
#include "stk/stk.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

using namespace ctk;
using namespace stk;
using namespace RTK;

static constexpr u32 MAX_PHYSICAL_DEVICES = 8;
static constexpr u32 MAX_RENDER_PASS_ATTACHMENTS = 8;
static constexpr u32 MAX_HOST_MEMORY = megabyte(128);
static constexpr u32 MAX_DEVICE_MEMORY = megabyte(256);

struct Test {
    VertexLayout vertex_layout;
    Array<Shader>* shaders;
    Pipeline pipeline;
};

static void Controls(Window* window) {
    if (key_down(window, Key::ESCAPE)) {
        window->open = false;
    }
}

static void SelectPhysicalDevice(RTKContext* rtk) {
    // Default to first physical device.
    UsePhysicalDevice(rtk, 0);

    // Use first discrete device if any are available.
    for (u32 i = 0; i < rtk->physical_devices->count; ++i) {
        if (get(rtk->physical_devices, i)->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            UsePhysicalDevice(rtk, i);
            break;
        }
    }

    LogPhysicalDevice(rtk->physical_device, "selected physical device");
}

static void InitRTK(RTKContext* rtk, Stack* mem, Stack temp_mem, Window* window, ThreadPool* thread_pool) {
    InitRTKContext(rtk, mem, thread_pool, MAX_PHYSICAL_DEVICES);

    // Initialize RTK instance.
    InitInstance(rtk, {
        .application_name = "RTK Test",
#ifdef RTK_ENABLE_VALIDATION
        .debug_callback         = DefaultDebugCallback,
        .debug_message_severity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .debug_message_type     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
#endif
    });
    InitSurface(rtk, window);

    // Initialize device state.
    DeviceFeatures required_features = {
        .as_struct = {
            .geometryShader    = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
        },
    };
    LoadCapablePhysicalDevices(rtk, temp_mem, &required_features);
    SelectPhysicalDevice(rtk);
    InitDevice(rtk, &required_features);
    InitQueues(rtk);
    GetSurfaceInfo(rtk, mem);
    InitMemory(rtk, MAX_HOST_MEMORY, MAX_DEVICE_MEMORY);
    InitMainCommandState(rtk);

    // Initialize rendering state.
    InitSwapchain(rtk, mem, temp_mem);
    InitRenderPass(rtk);
    InitFramebuffers(rtk, mem);
    InitRenderCommandState(rtk, mem);
}

static void InitTest(Test* test, Stack* mem, Stack temp_mem, RTKContext* rtk) {
    InitVertexLayout(&test->vertex_layout, mem, 1, 4);
    PushBinding(&test->vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);
    PushAttribute(&test->vertex_layout, 3); // Position
    PushAttribute(&test->vertex_layout, 3); // Color

    test->shaders = create_array<Shader>(mem, 2);
    LoadShader(push(test->shaders), temp_mem, rtk->device, "shaders/test.vs", VK_SHADER_STAGE_VERTEX_BIT);
    LoadShader(push(test->shaders), temp_mem, rtk->device, "shaders/test.fs", VK_SHADER_STAGE_FRAGMENT_BIT);

    InitPipeline(&test->pipeline, temp_mem, rtk, &test->vertex_layout, test->shaders);
}

s32 main() {
    Stack* mem = create_stack(megabyte(4));
    Stack* temp_mem = create_stack(mem, megabyte(1));

    win32_init();
    Window* window = create_window(mem, {
        .surface = {
            .x      = 0,
            .y      = 60, // Taskbar height.
            .width  = 1080,
            .height = 720,
        },
        .title    = L"RTK Test",
        .callback = default_window_callback,
    });

    // Create threadpool.
    SYSTEM_INFO system_info = {};
    GetSystemInfo(&system_info);
    ThreadPool* thread_pool = create_thread_pool(mem, system_info.dwNumberOfProcessors);

    auto rtk = allocate<RTKContext>(mem, 1);
    InitRTK(rtk, mem, *temp_mem, window, thread_pool);

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
