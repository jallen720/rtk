#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/vk_array.h"
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
static constexpr uint32 UNSET_INDEX = UINT32_MAX;

struct RTKInfo
{
    cstring                             application_name;
    DebugCallback                       debug_callback;
    VkDebugUtilsMessageSeverityFlagsEXT debug_message_severity;
    VkDebugUtilsMessageTypeFlagsEXT     debug_message_type;
    DeviceFeatures                      required_features;
    uint32                              max_physical_devices;
    uint32                              render_thread_count;
    Array<VkDescriptorPoolSize>         descriptor_pool_sizes;
};

struct Surface
{
    VkSurfaceKHR              hnd;
    VkSurfaceCapabilitiesKHR  capabilities;
    Array<VkSurfaceFormatKHR> formats;
    Array<VkPresentModeKHR>   present_modes;
};

struct Swapchain
{
    VkSwapchainKHR     hnd;
    Array<VkImageView> image_views;
    uint32             image_count;
    VkFormat           image_format;
    VkExtent2D         extent;
};

struct Frame
{
    // Sync State
    VkSemaphore image_acquired;
    VkSemaphore render_finished;
    VkFence     in_progress;

    // Render State
    VkCommandBuffer        primary_render_command_buffer;
    Array<VkCommandBuffer> render_command_buffers;
    uint32                 swapchain_image_index;
};

struct RTKContext
{
    // Instance State
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    // Device State
    Surface               surface;
    Array<PhysicalDevice> physical_devices;
    PhysicalDevice*       physical_device;
    VkDevice              device;
    VkQueue               graphics_queue;
    VkQueue               present_queue;
    VkCommandPool         main_command_pool;
    VkCommandBuffer       temp_command_buffer;

    // Render State
    Swapchain            swapchain;
    Array<VkCommandPool> render_command_pools;
    RingBuffer<Frame>    frames;
    Frame*               frame;
    VkDescriptorPool     descriptor_pool;
};

/// Instance
////////////////////////////////////////////////////////////
static RTKContext global_ctx;

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

static void InitInstance(RTKInfo* info)
{
    VkResult res = VK_SUCCESS;

#ifdef RTK_ENABLE_VALIDATION
    VkDebugUtilsMessengerCreateInfoEXT debug_msgr_info =
    {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = NULL,
        .flags           = 0,
        .messageSeverity = info->debug_message_severity,
        .messageType     = info->debug_message_type,
        .pfnUserCallback = info->debug_callback,
        .pUserData       = NULL,
    };
#endif

    VkApplicationInfo app_info =
    {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .pApplicationName   = info->application_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = info->application_name,
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
    res = vkCreateInstance(&create_info, NULL, &global_ctx.instance);
    Validate(res, "vkCreateInstance() failed");

#ifdef RTK_ENABLE_VALIDATION
    RTK_LOAD_INSTANCE_EXTENSION_FUNCTION(global_ctx.instance, vkCreateDebugUtilsMessengerEXT);
    res = vkCreateDebugUtilsMessengerEXT(global_ctx.instance, &debug_msgr_info, NULL, &global_ctx.debug_messenger);
    Validate(res, "vkCreateDebugUtilsMessengerEXT() failed");
#endif
}

static void InitSurface(Window* window)
{
    VkWin32SurfaceCreateInfoKHR info =
    {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = window->instance,
        .hwnd      = window->handle,
    };
    VkResult res = vkCreateWin32SurfaceKHR(global_ctx.instance, &info, NULL, &global_ctx.surface.hnd);
    Validate(res, "vkCreateWin32SurfaceKHR() failed");
}

static void UsePhysicalDevice(uint32 index)
{
    if (index >= global_ctx.physical_devices.count)
        CTK_FATAL("physical device index %u is out of bounds: max is %u", index, global_ctx.physical_devices.count - 1);

    global_ctx.physical_device = GetPtr(&global_ctx.physical_devices, index);
}

static void LoadCapablePhysicalDevices(Stack temp_mem, DeviceFeatures* required_features)
{
    // Get system physical devices and ensure there is enough space in the context's physical devices list.
    Array<VkPhysicalDevice>* vk_physical_devices = GetVkPhysicalDevices(&temp_mem, global_ctx.instance);
    if (vk_physical_devices->size > global_ctx.physical_devices.size)
    {
        CTK_FATAL("can't load physical devices: max device count is %u, system device count is %u",
                  global_ctx.physical_devices.size, vk_physical_devices->size);
    }

    // Reset context's physical device list to store new list.
    global_ctx.physical_devices.count = 0;

    // Check all physical devices, and load the ones capable of rendering into the context's physical device list.
    for (uint32 i = 0; i < vk_physical_devices->count; ++i)
    {
        VkPhysicalDevice vk_physical_device = Get(vk_physical_devices, i);

        // Collect all info about physical device.
        PhysicalDevice physical_device =
        {
            .hnd                = vk_physical_device,
            .queue_families     = FindQueueFamilies(temp_mem, vk_physical_device, &global_ctx.surface),
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

        Push(&global_ctx.physical_devices, physical_device);
    }

    // Ensure atleast 1 capable physical device was loaded.
    if (global_ctx.physical_devices.count == 0)
        CTK_FATAL("failed to load any capable physical devices");
}

static void InitDevice(DeviceFeatures* enabled_features)
{
    QueueFamilies* queue_families = &global_ctx.physical_device->queue_families;

    // Add queue creation info for 1 queue in each queue family.
    FixedArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    Push(&queue_infos, GetSingleQueueInfo(queue_families->graphics));

    // Don't create separate queues if present and graphics queue families are the same.
    if (queue_families->present != queue_families->graphics)
        Push(&queue_infos, GetSingleQueueInfo(queue_families->present));

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
    VkResult res = vkCreateDevice(global_ctx.physical_device->hnd, &create_info, NULL, &global_ctx.device);
    Validate(res, "vkCreateDevice() failed");
}

static void InitQueues()
{
    QueueFamilies* queue_families = &global_ctx.physical_device->queue_families;
    vkGetDeviceQueue(global_ctx.device, queue_families->graphics, 0, &global_ctx.graphics_queue);
    vkGetDeviceQueue(global_ctx.device, queue_families->present, 0, &global_ctx.present_queue);
}

static void GetSurfaceInfo(Stack* mem)
{
    Surface* surface = &global_ctx.surface;
    VkPhysicalDevice vk_physical_device = global_ctx.physical_device->hnd;

    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, surface->hnd, &surface->capabilities);
    Validate(res, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed");

    LoadVkSurfaceFormats(&surface->formats, mem, vk_physical_device, surface->hnd);
    LoadVkSurfacePresentModes(&surface->present_modes, mem, vk_physical_device, surface->hnd);
}

static void InitMainCommandState()
{
    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;

    // Main Command Pool
    VkCommandPoolCreateInfo pool_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = global_ctx.physical_device->queue_families.graphics,
    };
    res = vkCreateCommandPool(device, &pool_info, NULL, &global_ctx.main_command_pool);
    Validate(res, "vkCreateCommandPool() failed");

    // Temp Command Buffer
    VkCommandBufferAllocateInfo allocate_info =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = global_ctx.main_command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    res = vkAllocateCommandBuffers(device, &allocate_info, &global_ctx.temp_command_buffer);
    Validate(res, "vkAllocateCommandBuffers() failed");
}

static void InitSwapchain(Stack* mem, Stack temp_mem)
{
    VkDevice device = global_ctx.device;
    Surface* surface = &global_ctx.surface;
    Swapchain* swapchain = &global_ctx.swapchain;
    VkResult res = VK_SUCCESS;

    // Default to first surface format, then check for preferred 4-component 8-bit BGRA unnormalized format and sRG
    // color space.
    VkSurfaceFormatKHR selected_format = Get(&surface->formats, 0);
    for (uint32 i = 0; i < surface->formats.count; ++i)
    {
        VkSurfaceFormatKHR surface_format = Get(&surface->formats, i);
        if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM &&
            surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selected_format = surface_format;
            break;
        }
    }

    // Default to FIFO (only present mode with guarenteed availability), check for preferred mailbox present mode.
    VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32 i = 0; i < surface->present_modes.count; ++i)
    {
        VkPresentModeKHR surface_present_mode = Get(&surface->present_modes, i);
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
        CTK_FATAL("current extent not set for surface");
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
    QueueFamilies* queue_families = &global_ctx.physical_device->queue_families;
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
    Validate(res, "vkCreateSwapchainKHR() failed");

    // Store surface state used to create swapchain for future reference.
    swapchain->image_format = selected_format.format;
    swapchain->extent = surface->capabilities.currentExtent;

    // Create swapchain image views.
    Array<VkImage>* swapchain_images = GetVkSwapchainImages(&temp_mem, device, swapchain->hnd);
    swapchain->image_count = swapchain_images->count;
    InitArrayFull(&swapchain->image_views, mem, swapchain->image_count);

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
        res = vkCreateImageView(device, &view_info, NULL, GetPtr(&swapchain->image_views, i));
        Validate(res, "vkCreateImageView() failed");
    }
}

static void InitRenderCommandPools(Stack* mem, uint32 render_thread_count)
{
    InitArray(&global_ctx.render_command_pools, mem, render_thread_count);
    VkCommandPoolCreateInfo info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = global_ctx.physical_device->queue_families.graphics,
    };
    for (uint32 i = 0; i < render_thread_count; ++i)
    {
        VkResult res = vkCreateCommandPool(global_ctx.device, &info, NULL, Push(&global_ctx.render_command_pools));
        Validate(res, "vkCreateCommandPool() failed");
    }
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
    Validate(res, "vkCreateFence() failed");

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
    Validate(res, "vkCreateSemaphore() failed");

    return semaphore;
}

static void InitFrames(Stack* mem)
{
    VkResult res = VK_SUCCESS;
    VkDevice device = global_ctx.device;
    uint32 frame_count = global_ctx.swapchain.image_count + 1;

    InitRingBuffer(&global_ctx.frames, mem, frame_count);
    for (uint32 i = 0; i < frame_count; ++i)
    {
        Frame* frame = GetPtr(&global_ctx.frames, i);

        // Sync State
        frame->image_acquired  = CreateSemaphore(device);
        frame->render_finished = CreateSemaphore(device);
        frame->in_progress     = CreateFence(device);

        // primary_render_command_buffer
        {
            VkCommandBufferAllocateInfo allocate_info =
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = global_ctx.main_command_pool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            res = vkAllocateCommandBuffers(device, &allocate_info, &frame->primary_render_command_buffer);
            Validate(res, "vkAllocateCommandBuffers() failed");
        }

        // render_command_buffers
        InitArray(&frame->render_command_buffers, mem, global_ctx.render_command_pools.count);
        for (uint32 i = 0; i < global_ctx.render_command_pools.count; ++i)
        {
            VkCommandBufferAllocateInfo allocate_info =
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = Get(&global_ctx.render_command_pools, i),
                .level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                .commandBufferCount = 1,
            };
            res = vkAllocateCommandBuffers(device, &allocate_info, Push(&frame->render_command_buffers));
            Validate(res, "vkAllocateCommandBuffers() failed");
        }
    }
}

static void InitDescriptorPool(Array<VkDescriptorPoolSize>* descriptor_pool_sizes)
{
    // Count total descriptors from pool sizes.
    uint32 total_descriptors = 0;
    for (uint32 i = 0; i < descriptor_pool_sizes->count; ++i)
        total_descriptors += GetPtr(descriptor_pool_sizes, i)->descriptorCount;

    VkDescriptorPoolCreateInfo pool_info =
    {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = 0,
        .maxSets       = total_descriptors,
        .poolSizeCount = descriptor_pool_sizes->count,
        .pPoolSizes    = descriptor_pool_sizes->data,
    };
    VkResult res = vkCreateDescriptorPool(global_ctx.device, &pool_info, NULL, &global_ctx.descriptor_pool);
    Validate(res, "vkCreateDescriptorPool() failed");
}

/// Interface
////////////////////////////////////////////////////////////
static void InitContext(Stack* mem, Stack temp_mem, Window* window, RTKInfo* info)
{
    InitInstance(info);
    InitSurface(window);

    // Initialize device state.
    InitArray(&global_ctx.physical_devices, mem, info->max_physical_devices);
    LoadCapablePhysicalDevices(temp_mem, &info->required_features);
    UsePhysicalDevice(0); // Default to first capable device found. Required by InitDevice().
    InitDevice(&info->required_features);
    InitQueues();
    GetSurfaceInfo(mem);
    InitMainCommandState();

    // Initialize rendering state.
    InitSwapchain(mem, temp_mem);
    InitRenderCommandPools(mem, info->render_thread_count);
    InitFrames(mem);
    InitDescriptorPool(&info->descriptor_pool_sizes);
};

static void NextFrame()
{
    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;

    // Get next frame and wait for it to be finished (if still in-progress) before proceeding.
    Frame* frame = Next(&global_ctx.frames);

    res = vkWaitForFences(device, 1, &frame->in_progress, VK_TRUE, UINT64_MAX);
    Validate(res, "vkWaitForFences() failed");

    res = vkResetFences(device, 1, &frame->in_progress);
    Validate(res, "vkResetFences() failed");

    // Once frame is ready, aquire next swapchain image's index.
    res = vkAcquireNextImageKHR(device, global_ctx.swapchain.hnd, UINT64_MAX, frame->image_acquired, VK_NULL_HANDLE,
                                &frame->swapchain_image_index);
    Validate(res, "vkAcquireNextImageKHR() failed");

    global_ctx.frame = frame;
}

static void BeginTempCommandBuffer()
{
    VkCommandBufferBeginInfo info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };
    vkBeginCommandBuffer(global_ctx.temp_command_buffer, &info);
}

static void SubmitTempCommandBuffer()
{
    vkEndCommandBuffer(global_ctx.temp_command_buffer);

    VkSubmitInfo submit_info =
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = NULL,
        .waitSemaphoreCount   = 0,
        .pWaitSemaphores      = NULL,
        .pWaitDstStageMask    = NULL,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &global_ctx.temp_command_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores    = NULL,
    };
    VkResult res = vkQueueSubmit(global_ctx.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    Validate(res, "vkQueueSubmit() failed");

    vkQueueWaitIdle(global_ctx.graphics_queue);
}

}
