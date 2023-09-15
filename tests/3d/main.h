#pragma once

#include <windows.h>
#include "ctk3/ctk3.h"
#include "ctk3/stack.h"
#include "ctk3/free_list.h"
#include "ctk3/thread_pool.h"
#include "ctk3/profile.h"
#include "ctk3/window.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

#include "rtk/tests/shared.h"
#include "rtk/tests/3d/defs.h"
#include "rtk/tests/3d/game_state.h"
#include "rtk/tests/3d/render_state.h"

using namespace CTK;
using namespace RTK;

namespace Test3D
{

static void Run()
{
    Stack* perm_stack = CreateStack(&win32_allocator, Megabyte32<8>());
    Stack* temp_stack = CreateStack(&perm_stack->allocator, Megabyte32<1>());
    FreeList* free_list = CreateFreeList(&perm_stack->allocator, Kilobyte32<16>(), 16, 256);

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
    context_info.instance_info.application_name       = "RTK 3D Test";
    context_info.instance_info.api_version            = VK_API_VERSION_1_3;
    context_info.instance_info.extensions             = {};
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
    InitDeviceFeatures(&context_info.enabled_features);
    context_info.enabled_features.vulkan_1_0.geometryShader                            = VK_TRUE;
    context_info.enabled_features.vulkan_1_0.samplerAnisotropy                         = VK_TRUE;
    context_info.enabled_features.vulkan_1_2.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    context_info.enabled_features.vulkan_1_2.scalarBlockLayout                         = VK_TRUE;

    VkDescriptorPoolSize descriptor_pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                1024 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1024 },
    };
    context_info.descriptor_pool_sizes = CTK_WRAP_ARRAY(descriptor_pool_sizes),

    context_info.render_thread_count = RENDER_THREAD_COUNT,
    InitContext(perm_stack, temp_stack, free_list, &context_info);
LogPhysicalDevice(global_ctx.physical_device);

    // Initialize other test state.
    ThreadPool* thread_pool = CreateThreadPool(&perm_stack->allocator, 8);
    InitGameState();
    InitRenderState(perm_stack, temp_stack, free_list, thread_pool);

    // Run game.
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

        // NextFrame() will only return VK_SUCCESS, VK_SUBOPTIMAL_KHR or VK_ERROR_OUT_OF_DATE_KHR.
        VkResult next_frame_result = NextFrame();
        if (next_frame_result == VK_SUBOPTIMAL_KHR || next_frame_result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            WaitIdle();
            UpdateSwapchainSurfaceExtent(temp_stack, free_list);
            UpdateAllPipelineViewports(free_list);
            UpdateAllRenderTargetAttachments(temp_stack, free_list);
            continue;
        }

        UpdateMVPMatrixes(thread_pool);
        RecordRenderCommands(thread_pool);
        SubmitRenderCommands(GetRenderTarget());
    }
}

}
