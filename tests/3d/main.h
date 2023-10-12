#pragma once

#include <windows.h>
#include "ctk3/ctk3.h"
#include "ctk3/debug.h"
#include "ctk3/math.h"
#include "ctk3/stack.h"
#include "ctk3/array.h"
#include "ctk3/free_list.h"
#include "ctk3/thread_pool.h"
#include "ctk3/profile.h"
#include "ctk3/window.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

#include "rtk/tests/shared.h"
#include "rtk/tests/3d/defs.h"

using namespace CTK;
using namespace RTK;

namespace Test3D
{

#include "rtk/tests/3d/render_state.h"
#include "rtk/tests/3d/game_state.h"

static void Run()
{
    Stack* perm_stack = CreateStack(&win32_allocator, Megabyte32<8>());
    Stack* temp_stack = CreateStack(&perm_stack->allocator, Megabyte32<1>());
    FreeList* free_list = CreateFreeList(&perm_stack->allocator, Kilobyte32<16>(),
                                         { .chunk_byte_size = 16, .max_range_count = 256 });

    // Make win32 process DPI aware so windows scale properly.
    SetProcessDPIAware();

    WindowInfo window_info =
    {
        .x        = 0,
        .y        = 90, // Taskbar height @ 1.5x zoom (laptop).
        .width    = 1080,
        .height   = 720,
        .title    = "3D Test",
        .callback = DefaultWindowCallback,
    };
    OpenWindow(&window_info);

    // Init RTK Context + Resources
    ContextInfo context_info = {};
    context_info.instance_info.application_name = "RTK 3D Test";
    context_info.instance_info.api_version      = VK_API_VERSION_1_3;
    context_info.instance_info.extensions       = {};
#ifdef RTK_ENABLE_VALIDATION
    context_info.instance_info.debug_callback         = DefaultDebugCallback;
    context_info.instance_info.debug_message_severity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    context_info.instance_info.debug_message_type     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
#endif
    context_info.render_thread_count = 6;

    InitDeviceFeatures(&context_info.enabled_features);
    context_info.enabled_features.vulkan_1_0.geometryShader                            = VK_TRUE;
    context_info.enabled_features.vulkan_1_0.samplerAnisotropy                         = VK_TRUE;
    context_info.enabled_features.vulkan_1_2.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    context_info.enabled_features.vulkan_1_2.scalarBlockLayout                         = VK_TRUE;

    InitContext(perm_stack, temp_stack, free_list, &context_info);
LogPhysicalDevice(GetPhysicalDevice());

    // Initialize other test state.
    ThreadPool* thread_pool = CreateThreadPool(&perm_stack->allocator, 8);
    InitRenderState(perm_stack, temp_stack, free_list);
    InitGameState(perm_stack, thread_pool->size);

    // Run game.
    bool recreate_swapchain = false;
    for (;;)
    {
        ProcessWindowEvents();
        if (!WindowIsOpen())
        {
            break; // Quit event closed window.
        }

        UpdateGame();
        if (!WindowIsOpen())
        {
            break; // Game controls closed window.
        }

        if (!WindowIsActive())
        {
            Sleep(1);
            continue;
        }

        NextFrame();
        VkResult next_swapchain_image_res = NextSwapchainImage();
        if (next_swapchain_image_res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // Swapchain image acquisition failed, recreate swapchain, skip rendering and move to next iteration to
            // retry acquiring swapchain image.

            recreate_swapchain = true;
            RecreateSwapchain(temp_stack, free_list);
            continue;
        }
        else if (next_swapchain_image_res == VK_SUBOPTIMAL_KHR)
        {
            // Swapchain image acquisition succeeded but was suboptimal, continue rendering but flag swapchain to be
            // recreated afterwards.

            recreate_swapchain = true;
        }
        else if (next_swapchain_image_res != VK_SUCCESS)
        {
            CTK_FATAL("unhandled NextSwapchainImage() result: %s", VkResultName(next_swapchain_image_res));
        }

        UpdateMVPMatrixes(thread_pool);
        RecordRenderCommands(thread_pool, GetEntityCount());
        VkResult submit_render_commands_res = SubmitRenderCommands(GetRenderTarget());
        if (submit_render_commands_res == VK_ERROR_OUT_OF_DATE_KHR || submit_render_commands_res == VK_SUBOPTIMAL_KHR)
        {
            recreate_swapchain = true;
        }
        else if (submit_render_commands_res != VK_SUCCESS)
        {
            CTK_FATAL("unhandled NextSwapchainImage() result: %s", VkResultName(submit_render_commands_res));
        }

        if (recreate_swapchain)
        {
            RecreateSwapchain(temp_stack, free_list);
            recreate_swapchain = false;
        }
    }
}

}
