#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/get_vk_array.h"
#include "rtk/memory.h"
#include "rtk/device.h"
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/containers.h"
#include "ctk2/file.h"
#include "ctk2/multithreading.h"
#include "stk2/stk.h"

using namespace CTK;
using namespace STK;

/// Macros
////////////////////////////////////////////////////////////
#define RTK_LOAD_INSTANCE_EXTENSION_FUNCTION(INSTANCE, FUNC_NAME) \
    auto FUNC_NAME = (PFN_ ## FUNC_NAME)vkGetInstanceProcAddr(INSTANCE, #FUNC_NAME); \
    if (FUNC_NAME == NULL) \
    { \
        CTK_FATAL("failed to load instance extension function \"%s\"", #FUNC_NAME); \
    }

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
static constexpr uint32 UNSET_INDEX = U32_MAX;

struct InstanceInfo
{
    cstring                             application_name;
    DebugCallback                       debug_callback;
    VkDebugUtilsMessageSeverityFlagsEXT debug_message_severity;
    VkDebugUtilsMessageTypeFlagsEXT     debug_message_type;
};

struct Surface
{
    VkSurfaceKHR               hnd;
    VkSurfaceCapabilitiesKHR   capabilities;
    Array<VkSurfaceFormatKHR>* formats;
    Array<VkPresentModeKHR>*   present_modes;
};

struct ImageInfo
{
    VkImageCreateInfo        image;
    VkImageViewCreateInfo    view;
    VkMemoryPropertyFlagBits mem_property_flags;
};

struct Image
{
    VkImage        hnd;
    VkImageView    view;
    VkDeviceMemory mem;
    VkExtent3D     extent;
};

struct Swapchain
{
    VkSwapchainKHR      hnd;
    Array<VkImageView>* image_views;
    uint32              image_count;
    VkFormat            image_format;
    VkExtent2D          extent;
};

struct RTKContext
{
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    // Device State
    Surface                surface;
    Array<PhysicalDevice>* physical_devices;
    PhysicalDevice*        physical_device;
    VkDevice               device;
    VkQueue                graphics_queue;
    VkQueue                present_queue;
    Buffer                 host_buffer;
    Buffer                 device_buffer;
    VkCommandPool          main_command_pool;
    VkCommandBuffer        temp_command_buffer;

    // Render State
    Swapchain               swapchain;
    VkRenderPass            render_pass;
    Array<VkFramebuffer>*   framebuffers;
    Array<VkCommandPool>*   render_command_pools;
    VkCommandBuffer         primary_render_command_buffer;
    Array<VkCommandBuffer>* render_command_buffers;
    uint32                  swapchain_image_index;
    VkSemaphore             image_acquired;
    VkSemaphore             render_finished;
};

/// Internal
////////////////////////////////////////////////////////////
static QueueFamilies FindQueueFamilies(Stack temp_mem, VkPhysicalDevice physical_device, Surface* surface)
{
    QueueFamilies queue_families =
    {
        .graphics = UNSET_INDEX,
        .present  = UNSET_INDEX,
    };
    Array<VkQueueFamilyProperties>* queue_family_properties = GetVkQueueFamilyProperties(&temp_mem, physical_device);

    // Find first graphics queue family.
    for (uint32 queue_family_index = 0; queue_family_index < queue_family_properties->count; ++queue_family_index)
    {
        // Check if queue supports graphics operations.
        if (GetPtr(queue_family_properties, queue_family_index)->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_families.graphics = queue_family_index;
            break;
        }
    }

    // Find first present queue family.
    for (uint32 queue_family_index = 0; queue_family_index < queue_family_properties->count; ++queue_family_index)
    {
        // Check if queue supports present operations.
        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, surface->hnd, &present_supported);

        if (present_supported)
        {
            queue_families.present = queue_family_index;
            break;
        }
    }

    return queue_families;
}

static VkFormat FindDepthImageFormat(VkPhysicalDevice physical_device)
{
    static constexpr VkFormat DEPTH_IMAGE_FORMATS[] =
    {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
    };

    // Find format that supports depth-stencil attachment feature for physical device.
    for (uint32 i = 0; i < CTK_ARRAY_SIZE(DEPTH_IMAGE_FORMATS); i++)
    {
        VkFormat depth_image_format = DEPTH_IMAGE_FORMATS[i];
        VkFormatProperties format_properties = {};
        vkGetPhysicalDeviceFormatProperties(physical_device, depth_image_format, &format_properties);
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return depth_image_format;
    }

    return VK_FORMAT_UNDEFINED;
}

static VkDeviceQueueCreateInfo GetSingleQueueInfo(uint32 queue_family_index)
{
    static constexpr float32 QUEUE_PRIORITIES[] = { 1.0f };
    VkDeviceQueueCreateInfo info =
    {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .flags            = 0,
        .queueFamilyIndex = queue_family_index,
        .queueCount       = CTK_ARRAY_SIZE(QUEUE_PRIORITIES),
        .pQueuePriorities = QUEUE_PRIORITIES,
    };

    return info;
}

static VkFence CreateFence(VkDevice device)
{
    VkFenceCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkFence fence = VK_NULL_HANDLE;
    VkResult res = vkCreateFence(device, &info, NULL, &fence);
    Validate(res, "failed to create fence");

    return fence;
}

static VkSemaphore CreateSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkResult res = vkCreateSemaphore(device, &info, NULL, &semaphore);
    Validate(res, "failed to create semaphore");

    return semaphore;
}

/// Interface
////////////////////////////////////////////////////////////
static void InitRTKContext(RTKContext* rtk, Stack* mem, uint32 max_physical_devices)
{
    rtk->physical_devices = CreateArray<PhysicalDevice>(mem, max_physical_devices);
};

static void InitInstance(RTKContext* rtk, InstanceInfo info)
{
    VkResult res = VK_SUCCESS;

#ifdef RTK_ENABLE_VALIDATION
    VkDebugUtilsMessengerCreateInfoEXT debug_msgr_info =
    {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = NULL,
        .flags           = 0,
        .messageSeverity = info.debug_message_severity,
        .messageType     = info.debug_message_type,
        .pfnUserCallback = info.debug_callback,
        .pUserData       = NULL,
    };
#endif

    VkApplicationInfo app_info =
    {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .pApplicationName   = info.application_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = info.application_name,
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_0,
    };

    cstring extensions[] =
    {
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef RTK_ENABLE_VALIDATION
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

#ifdef RTK_ENABLE_VALIDATION
    cstring validation_layer = "VK_LAYER_KHRONOS_validation";
#endif

    VkInstanceCreateInfo create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.flags                   = 0;
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = CTK_ARRAY_SIZE(extensions);
    create_info.ppEnabledExtensionNames = extensions;
#ifdef RTK_ENABLE_VALIDATION
    create_info.pNext                   = &debug_msgr_info;
    create_info.enabledLayerCount       = 1;
    create_info.ppEnabledLayerNames     = &validation_layer;
#else
    create_info.pNext                   = NULL;
    create_info.enabledLayerCount       = 0;
    create_info.ppEnabledLayerNames     = NULL;
#endif
    res = vkCreateInstance(&create_info, NULL, &rtk->instance);
    Validate(res, "failed to create Vulkan instance");

#ifdef RTK_ENABLE_VALIDATION
    RTK_LOAD_INSTANCE_EXTENSION_FUNCTION(rtk->instance, vkCreateDebugUtilsMessengerEXT);
    res = vkCreateDebugUtilsMessengerEXT(rtk->instance, &debug_msgr_info, NULL, &rtk->debug_messenger);
    Validate(res, "failed to create debug messenger");
#endif
}

static void InitSurface(RTKContext* rtk, Window* window)
{
    VkWin32SurfaceCreateInfoKHR info =
    {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = window->instance,
        .hwnd      = window->handle,
    };
    VkResult res = vkCreateWin32SurfaceKHR(rtk->instance, &info, NULL, &rtk->surface.hnd);
    Validate(res, "failed to create win32 surface");
}

static void UsePhysicalDevice(RTKContext* rtk, uint32 index)
{
    if (index >= rtk->physical_devices->count)
        CTK_FATAL("physical device index %u is out of bounds: max is %u", index, rtk->physical_devices->count - 1);

    rtk->physical_device = GetPtr(rtk->physical_devices, index);
}

static void LoadCapablePhysicalDevices(RTKContext* rtk, Stack temp_mem, DeviceFeatures* required_features)
{
    // Get system physical devices and ensure there is enough space in the context's physical devices list.
    Array<VkPhysicalDevice>* vk_physical_devices = GetVkPhysicalDevices(&temp_mem, rtk->instance);
    if (vk_physical_devices->size > rtk->physical_devices->size)
    {
        CTK_FATAL("can't load physical devices: max device count is %u, system device count is %u",
                  rtk->physical_devices->size, vk_physical_devices->size);
    }

    // Reset context's physical device list to store new list.
    rtk->physical_devices->count = 0;

    // Check all physical devices, and load the ones capable of rendering into the context's physical device list.
    for (uint32 i = 0; i < vk_physical_devices->count; ++i)
    {
        VkPhysicalDevice vk_physical_device = Get(vk_physical_devices, i);

        // Collect all info about physical device.
        PhysicalDevice physical_device =
        {
            .hnd                = vk_physical_device,
            .queue_families     = FindQueueFamilies(temp_mem, vk_physical_device, &rtk->surface),
            .depth_image_format = FindDepthImageFormat(vk_physical_device),
        };
        vkGetPhysicalDeviceProperties(vk_physical_device, &physical_device.properties);
        vkGetPhysicalDeviceFeatures(vk_physical_device, &physical_device.features.as_struct);
        vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &physical_device.mem_properties);

        // Graphics and present queue families required.
        if (physical_device.queue_families.graphics == UNSET_INDEX ||
            physical_device.queue_families.present == UNSET_INDEX)
        {
            continue;
        }

        // Depth image format required.
        if (physical_device.depth_image_format == VK_FORMAT_UNDEFINED)
            continue;

        // All required features must be supported.
        if (!HasRequiredFeatures(&physical_device, required_features))
            continue;

        Push(rtk->physical_devices, physical_device);
    }

    // Ensure atleast 1 capable physical device was loaded.
    if (rtk->physical_devices->count == 0)
        CTK_FATAL("failed to load any capable physical devices");
}

static void InitDevice(RTKContext* rtk, DeviceFeatures* enabled_features)
{
    QueueFamilies* queue_families = &rtk->physical_device->queue_families;

    // Add queue creation info for 1 queue in each queue family.
    FixedArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    Push(&queue_infos, GetSingleQueueInfo(queue_families->graphics));

    // Don't create separate queues if present and graphics queue families are the same.
    if (queue_families->present != queue_families->graphics)
    {
        Push(&queue_infos, GetSingleQueueInfo(queue_families->present));
    }

    // Create device, specifying enabled extensions and features.
    cstring enabled_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo create_info =
    {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .flags                   = 0,
        .queueCreateInfoCount    = queue_infos.count,
        .pQueueCreateInfos       = queue_infos.data,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = NULL,
        .enabledExtensionCount   = CTK_ARRAY_SIZE(enabled_extensions),
        .ppEnabledExtensionNames = enabled_extensions,
        .pEnabledFeatures        = &enabled_features->as_struct,
    };
    VkResult res = vkCreateDevice(rtk->physical_device->hnd, &create_info, NULL, &rtk->device);
    Validate(res, "failed to create device");
}

static void InitQueues(RTKContext* rtk)
{
    QueueFamilies* queue_families = &rtk->physical_device->queue_families;
    vkGetDeviceQueue(rtk->device, queue_families->graphics, 0, &rtk->graphics_queue);
    vkGetDeviceQueue(rtk->device, queue_families->present, 0, &rtk->present_queue);
}

static void GetSurfaceInfo(RTKContext* rtk, Stack* mem)
{
    Surface* surface = &rtk->surface;
    VkPhysicalDevice vk_physical_device = rtk->physical_device->hnd;

    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, surface->hnd, &surface->capabilities);
    Validate(res, "failed to get physical device surface capabilities");

    surface->formats = GetVkSurfaceFormats(mem, vk_physical_device, surface->hnd);
    surface->present_modes = GetVkSurfacePresentModes(mem, vk_physical_device, surface->hnd);
}

static void InitMemory(RTKContext* rtk, uint32 max_host_memory, uint32 max_device_memory)
{
    InitBuffer(&rtk->host_buffer, rtk->device, rtk->physical_device,
    {
        .size               = max_host_memory,
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    });
    InitBuffer(&rtk->device_buffer, rtk->device, rtk->physical_device,
    {
        .size               = max_device_memory,
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    });
}

static void InitMainCommandState(RTKContext* rtk)
{
    VkDevice device = rtk->device;
    VkResult res = VK_SUCCESS;

    // Main Command Pool
    VkCommandPoolCreateInfo pool_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = rtk->physical_device->queue_families.graphics,
    };
    res = vkCreateCommandPool(device, &pool_info, NULL, &rtk->main_command_pool);
    Validate(res, "failed to create main_command_pool");

    // Temp Command Buffer
    VkCommandBufferAllocateInfo allocate_info =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = rtk->main_command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    res = vkAllocateCommandBuffers(device, &allocate_info, &rtk->temp_command_buffer);
    Validate(res, "failed to allocate temp_command_buffer");
}

static void InitSwapchain(RTKContext* rtk, Stack* mem, Stack temp_mem)
{
    VkDevice device = rtk->device;
    Surface* surface = &rtk->surface;
    Swapchain* swapchain = &rtk->swapchain;
    VkResult res = VK_SUCCESS;

    // Default to first surface format, then check for preferred 4-component 8-bit BGRA unnormalized format and sRG
    // color space.
    VkSurfaceFormatKHR selected_format = Get(surface->formats, 0);
    for (uint32 i = 0; i < surface->formats->count; ++i)
    {
        VkSurfaceFormatKHR surface_format = Get(surface->formats, i);
        if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM &&
            surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selected_format = surface_format;
            break;
        }
    }

    // Default to FIFO (only present mode with guarenteed availability), check for preferred mailbox present mode.
    VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32 i = 0; i < surface->present_modes->count; ++i)
    {
        VkPresentModeKHR surface_present_mode = Get(surface->present_modes, i);
        if (surface_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            selected_present_mode = surface_present_mode;
            break;
        }
    }

    // Set image count to min image count + 1 or max image count (whichever is smaller).
    uint32 min_image_count = surface->capabilities.minImageCount + 1;
    if (surface->capabilities.maxImageCount > 0 && min_image_count > surface->capabilities.maxImageCount)
    {
        min_image_count = surface->capabilities.maxImageCount;
    }

    // Verify current extent has been set for surface.
    if (surface->capabilities.currentExtent.width == UINT32_MAX)
    {
        CTK_FATAL("current extent not set for surface")
    }

    VkSwapchainCreateInfoKHR info =
    {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .flags            = 0,
        .surface          = surface->hnd,
        .minImageCount    = min_image_count,
        .imageFormat      = selected_format.format,
        .imageColorSpace  = selected_format.colorSpace,
        .imageExtent      = surface->capabilities.currentExtent,
        .imageArrayLayers = 1, // Always 1 for standard images.
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .preTransform     = surface->capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = selected_present_mode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE,
    };

    // Determine image sharing mode based on queue family indexes.
    QueueFamilies* queue_families = &rtk->physical_device->queue_families;
    if (queue_families->graphics != queue_families->present)
    {
        info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(uint32);
        info.pQueueFamilyIndices   = (uint32*)queue_families;
    }
    else
    {
        info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices   = NULL;
    }

    res = vkCreateSwapchainKHR(device, &info, NULL, &swapchain->hnd);
    Validate(res, "failed to create swapchain");

    // Store surface state used to create swapchain for future reference.
    swapchain->image_format = selected_format.format;
    swapchain->extent = surface->capabilities.currentExtent;

    // Create swapchain image views.
    Array<VkImage>* swapchain_images = GetVkSwapchainImages(&temp_mem, device, swapchain->hnd);
    swapchain->image_count = swapchain_images->count;
    swapchain->image_views = CreateArrayFull<VkImageView>(mem, swapchain->image_count);

    for (uint32 i = 0; i < swapchain_images->count; ++i)
    {
        VkImageViewCreateInfo view_info =
        {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .flags    = 0,
            .image    = Get(swapchain_images, i),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = swapchain->image_format,
            .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        res = vkCreateImageView(device, &view_info, NULL, GetPtr(swapchain->image_views, i));
        Validate(res, "failed to create swapchain image view");
    }
}

static void InitRenderPass(RTKContext* rtk)
{
    VkAttachmentDescription attachments[] =
    {
        // Swapchain Image
        {
            .flags          = 0,
            .format         = rtk->swapchain.image_format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,

            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        }
    };

    VkAttachmentReference swapchain_attachment_reference =
    {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass_description =
    {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &swapchain_attachment_reference,
        .pDepthStencilAttachment = NULL,
    };

    VkRenderPassCreateInfo create_info =
    {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = CTK_ARRAY_SIZE(attachments),
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass_description,
        .dependencyCount = 0,
        .pDependencies   = NULL,
    };
    VkResult res = vkCreateRenderPass(rtk->device, &create_info, NULL, &rtk->render_pass);
    Validate(res, "failed to create render pass");
}

static void InitFramebuffers(RTKContext* rtk, Stack* mem)
{
    Swapchain* swapchain = &rtk->swapchain;
    rtk->framebuffers = CreateArray<VkFramebuffer>(mem, rtk->swapchain.image_count);
    for (uint32 i = 0; i < rtk->swapchain.image_count; ++i)
    {
        VkImageView attachments[] =
        {
            Get(swapchain->image_views, i),
        };
        VkFramebufferCreateInfo info =
        {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = rtk->render_pass,
            .attachmentCount = CTK_ARRAY_SIZE(attachments),
            .pAttachments    = attachments,
            .width           = swapchain->extent.width,
            .height          = swapchain->extent.height,
            .layers          = 1,
        };
        VkResult res = vkCreateFramebuffer(rtk->device, &info, NULL, Push(rtk->framebuffers));
        Validate(res, "failed to create framebuffer");
    }
}

static void InitRenderCommandState(RTKContext* rtk, Stack* mem, uint32 render_thread_count)
{
    VkResult res = VK_SUCCESS;
    VkDevice device = rtk->device;

    // primary_render_command_buffer
    {
        VkCommandBufferAllocateInfo allocate_info =
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = rtk->main_command_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        res = vkAllocateCommandBuffers(device, &allocate_info, &rtk->primary_render_command_buffer);
        Validate(res, "failed to allocate primary_render_command_buffer");
    }

    // render_command_pools
    {
        rtk->render_command_pools = CreateArray<VkCommandPool>(mem, render_thread_count);
        VkCommandPoolCreateInfo info =
        {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = rtk->physical_device->queue_families.graphics,
        };
        for (uint32 i = 0; i < render_thread_count; ++i)
        {
            res = vkCreateCommandPool(device, &info, NULL, Push(rtk->render_command_pools));
            Validate(res, "failed to create render_command_pools");
        }
    }

    // render_command_buffers
    {
        rtk->render_command_buffers = CreateArray<VkCommandBuffer>(mem, render_thread_count);
        for (uint32 i = 0; i < render_thread_count; ++i)
        {
            VkCommandBufferAllocateInfo allocate_info =
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = Get(rtk->render_command_pools, i),
                .level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                .commandBufferCount = 1,
            };
            res = vkAllocateCommandBuffers(device, &allocate_info, Push(rtk->render_command_buffers));
            Validate(res, "failed to allocate render_command_buffers[%u]", i);
        }
    }
}

static void InitSyncState(RTKContext* rtk)
{
    rtk->image_acquired = CreateSemaphore(rtk->device);
    rtk->render_finished = CreateSemaphore(rtk->device);
}

// static void InitImage(Image* image, VkDevice device, PhysicalDevice* physical_device, ImageInfo info)
// {
//     VkResult res = VK_SUCCESS;

//     res = vkCreateImage(device, &info.image, NULL, &image->hnd);
//     Validate(res, "failed to create image");

//     // Allocate/bind image memory.
//     VkMemoryRequirements mem_requirements = {};
//     vkGetImageMemoryRequirements(device, image->hnd, &mem_requirements);

//     image->mem = AllocateDeviceMemory(device, physical_device, mem_requirements, info.mem_property_flags);
//     res = vkBindImageMemory(device, image->hnd, image->mem, 0);
//     Validate(res, "failed to bind image memory");

//     info.view.image = image->hnd;
//     res = vkCreateImageView(device, &info.view, NULL, &image->view);
//     Validate(res, "failed to create image view");

//     image->extent = info.image.extent;
// }

// static void BeginTempCommandBuffer(VkCommandBuffer command_buffer)
// {
//     VkCommandBufferBeginInfo info =
//     {
//         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//         .pNext = NULL,
//         .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
//         .pInheritanceInfo = NULL,
//     };
//     vkBeginCommandBuffer(command_buffer, &info);
// }

// static void SubmitTempCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue)
// {
//     vkEndCommandBuffer(command_buffer);

//     VkSubmitInfo submit_info =
//     {
//         .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//         .pNext = NULL,
//         .waitSemaphoreCount = 0,
//         .pWaitSemaphores = NULL,
//         .pWaitDstStageMask = NULL,
//         .commandBufferCount = 1,
//         .pCommandBuffers = &command_buffer,
//         .signalSemaphoreCount = 0,
//         .pSignalSemaphores = NULL,
//     };
//     VkResult res = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
//     Validate(res, "failed to submit temp command buffer");

//     vkQueueWaitIdle(queue);
// }

static void NextFrame(RTKContext* rtk)
{
    // Get next swapchain image's index.
    VkResult res = vkAcquireNextImageKHR(rtk->device, rtk->swapchain.hnd, U64_MAX, rtk->image_acquired, VK_NULL_HANDLE,
                                         &rtk->swapchain_image_index);
    Validate(res, "failed to aquire next swapchain image");
}

static VkCommandBuffer BeginRecordingRenderCommands(RTKContext* rtk, uint32 render_thread_index)
{
    VkCommandBuffer render_command_buffer = Get(rtk->render_command_buffers, render_thread_index);

    VkCommandBufferInheritanceInfo inheritance_info =
    {
        .sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .pNext                = NULL,
        .renderPass           = rtk->render_pass,
        .subpass              = 0,
        .framebuffer          = Get(rtk->framebuffers, rtk->swapchain_image_index),
        .occlusionQueryEnable = VK_FALSE,
        .queryFlags           = 0,
        .pipelineStatistics   = 0,
    };
    VkCommandBufferBeginInfo command_buffer_begin_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritance_info,
    };
    VkResult res = vkBeginCommandBuffer(render_command_buffer, &command_buffer_begin_info);
    Validate(res, "failed to begin recording command buffer");

    return render_command_buffer;
}

static void SubmitRenderCommands(RTKContext* rtk)
{
    VkResult res = VK_SUCCESS;

    // Record current frame's primary command buffer, including render command buffers.
    VkCommandBuffer command_buffer = rtk->primary_render_command_buffer;

    // Begin command buffer.
    VkCommandBufferBeginInfo command_buffer_begin_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = 0,
        .pInheritanceInfo = NULL,
    };
    res = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    Validate(res, "failed to begin recording command buffer");

    // Begin render pass.
    VkClearValue clear_value = { 0.0, 0.1, 0.2, 1 };
    VkRenderPassBeginInfo render_pass_begin_info =
    {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext       = NULL,
        .renderPass  = rtk->render_pass,
        .framebuffer = Get(rtk->framebuffers, rtk->swapchain_image_index),
        .renderArea =
        {
            .offset = { 0, 0 },
            .extent = rtk->swapchain.extent,
        },
        .clearValueCount = 1,
        .pClearValues    = &clear_value,
    };
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // Execute entity render commands.
    vkCmdExecuteCommands(command_buffer, rtk->render_command_buffers->count, rtk->render_command_buffers->data);

    vkCmdEndRenderPass(command_buffer);
    vkEndCommandBuffer(command_buffer);

    // Submit commands for rendering to graphics queue.
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info =
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = NULL,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &rtk->image_acquired,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &rtk->render_finished,
    };
    res = vkQueueSubmit(rtk->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    Validate(res, "vkQueueSubmit() failed");

    // Queue swapchain image for presentation.
    VkPresentInfoKHR present_info =
    {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &rtk->render_finished,
        .swapchainCount     = 1,
        .pSwapchains        = &rtk->swapchain.hnd,
        .pImageIndices      = &rtk->swapchain_image_index,
        .pResults           = NULL,
    };
    res = vkQueuePresentKHR(rtk->present_queue, &present_info);
    Validate(res, "vkQueuePresentKHR() failed");

    vkDeviceWaitIdle(rtk->device);
}

}
