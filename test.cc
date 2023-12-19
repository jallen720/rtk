#include "ctk3/ctk.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

using namespace CTK;
using namespace RTK;

#include "rtk/tests/defs.h"
#include "rtk/tests/render_state.h"
#include "rtk/tests/game_state.h"

sint32 main()
{
    Stack*    perm_stack = CreateStack   (&std_allocator,         Megabyte32<8>());
    Stack*    temp_stack = CreateStack   (&perm_stack->allocator, Megabyte32<1>());
    FreeList* free_list  = CreateFreeList(&perm_stack->allocator, Kilobyte32<16>(), { .max_range_count = 256 });

    ThreadPool* thread_pool = CreateThreadPool(&perm_stack->allocator, 8);

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
// LogPhysicalDevice(GetPhysicalDevice());

    // Initialize other test state.
    InitRenderState(perm_stack, temp_stack, free_list, thread_pool->size);
    InitGameState(perm_stack);

    // Run game.
    bool recreate_swapchain = false;
    for (;;)
    {
        ProcessWindowEvents();
        if (!WindowIsOpen())
        {
            break; // Quit event closed window.
        }
        else if (!WindowIsActive())
        {
            Sleep(1);
            continue;
        }

        UpdateGame();
        if (!WindowIsOpen())
        {
            break; // Game controls closed window.
        }

        // When window is re-focused after being minimized, surface extent is 0,0 for a short period of time. Skip
        // rendering until surface extent > 0,0.
        VkSurfaceCapabilitiesKHR surface_capabilities = {};
        GetSurfaceCapabilities(&surface_capabilities);
        VkExtent2D current_surface_extent = surface_capabilities.currentExtent;
        if (current_surface_extent.width == 0 || current_surface_extent.height == 0)
        {
            continue;
        }

        NextFrame();
        VkResult acquire_swapchain_image_res = AcquireSwapchainImage();
        if (acquire_swapchain_image_res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // Swapchain image acquisition failed, recreate swapchain, skip rendering and move to next iteration to
            // retry acquiring swapchain image.

            RecreateSwapchain(temp_stack, free_list);
            continue;
        }
        else if (acquire_swapchain_image_res == VK_SUBOPTIMAL_KHR)
        {
            // Swapchain image acquisition succeeded but was suboptimal, continue rendering but flag swapchain to be
            // recreated afterwards.

            recreate_swapchain = true;
        }

        EntityData* entity_data = GetEntityData();
        UpdateMVPMatrixes(thread_pool, GetView(), entity_data->transforms, entity_data->count);
        RecordRenderCommands(thread_pool, entity_data->count);
        VkResult submit_render_commands_res = SubmitRenderCommands(GetRenderTarget());

        if (recreate_swapchain ||
            submit_render_commands_res == VK_ERROR_OUT_OF_DATE_KHR ||
            submit_render_commands_res == VK_SUBOPTIMAL_KHR)
        {
            RecreateSwapchain(temp_stack, free_list);
            recreate_swapchain = false;
        }
    }
}
