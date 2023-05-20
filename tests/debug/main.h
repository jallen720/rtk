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

namespace TestDebug
{

static void Run()
{
    Stack* perm_stack = CreateStack(&win32_allocator, Megabyte32<8>());
    Stack* temp_stack = CreateStack(&perm_stack->allocator, Megabyte32<1>());
    FreeList* free_list = CreateFreeList(&perm_stack->allocator, Kilobyte32<16>(), 16, 256);

    ///
    /// Initialization
    ///

    // Make win32 process DPI aware so windows scale properly.
    SetProcessDPIAware();

    WindowInfo window_info =
    {
        .x        = 0,
        .y        = 90, // Taskbar height @ 1.5x zoom (laptop).
        .width    = 1080,
        .height   = 720,
        .title    = "Debug Test",
        .callback = DefaultWindowCallback,
    };
    OpenWindow(&window_info);

    // Init RTK Context + Resources
    ContextInfo context_info = {};
    context_info.instance_info.application_name       = "Debug Test",
    context_info.instance_info.extensions             = {};
#ifdef RTK_ENABLE_VALIDATION
    context_info.instance_info.debug_callback         = DefaultDebugCallback,
    context_info.instance_info.debug_message_severity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    context_info.instance_info.debug_message_type     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
#endif

    context_info.required_features.as_struct =
    {
        .geometryShader    = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };

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

    context_info.render_thread_count = 1,
    InitContext(perm_stack, temp_stack, free_list, &context_info);

    ///
    /// Debugging
    ///

    LogPhysicalDevice(global_ctx.physical_device);

    {
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.flags = 0;
        create_info.size  = Gigabyte32<1>() + 1;
        create_info.usage = /*VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |*/
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        // Check if image sharing mode needs to be concurrent due to separate graphics & present queue families.
        QueueFamilies* queue_families = &GetPhysicalDevice()->queue_families;
        if (queue_families->graphics != queue_families->present)
        {
            create_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(uint32);
            create_info.pQueueFamilyIndices   = (uint32*)queue_families;
        }
        else
        {
            create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices   = NULL;
        }

        VkBuffer buffer = VK_NULL_HANDLE;
        VkResult res = vkCreateBuffer(global_ctx.device, &create_info, NULL, &buffer);
        Validate(res, "vkCreateBuffer() failed");

        VkMemoryRequirements mem_requirements = {};
        vkGetBufferMemoryRequirements(global_ctx.device, buffer, &mem_requirements);
        PrintMemoryRequirements(&mem_requirements, 0);
    }

    {
        VkImageCreateInfo create_info =
        {
            .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext     = NULL,
            .flags     = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            // .format    = GetSwapchain()->surface_format.format,
            .format    = GetPhysicalDevice()->depth_image_format,
            .extent =
            {
                .width  = Megabyte32<1>(),
                .height = 16,
                .depth  = 1
            },
            .mipLevels             = 1,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            // .usage                 = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            .usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                     VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = NULL,
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VkImage image = VK_NULL_HANDLE;
        VkResult res = vkCreateImage(global_ctx.device, &create_info, NULL, &image);
        Validate(res, "vkCreateImage() failed");

        VkMemoryRequirements mem_requirements = {};
        vkGetImageMemoryRequirements(global_ctx.device, image, &mem_requirements);
        PrintMemoryRequirements(&mem_requirements, 0);
    }

    // uint32 mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    //                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

}
