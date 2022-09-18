#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/get_vk_array.h"
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/containers.h"
#include "ctk/file.h"
#include "stk/stk.h"

using namespace ctk;
using namespace stk;

/// Macros
////////////////////////////////////////////////////////////
#define RTK_LOAD_INSTANCE_EXTENSION_FUNCTION(INSTANCE, FUNC_NAME) \
    auto FUNC_NAME = (PFN_ ## FUNC_NAME)vkGetInstanceProcAddr(INSTANCE, #FUNC_NAME); \
    if (FUNC_NAME == NULL) { \
        CTK_FATAL("failed to load instance extension function \"%s\"", #FUNC_NAME) \
    }

namespace RTK {

/// Data
////////////////////////////////////////////////////////////
static constexpr VkColorComponentFlags COLOR_COMPONENT_RGBA = VK_COLOR_COMPONENT_R_BIT |
                                                              VK_COLOR_COMPONENT_G_BIT |
                                                              VK_COLOR_COMPONENT_B_BIT |
                                                              VK_COLOR_COMPONENT_A_BIT;
static constexpr u32 MAX_DEVICE_FEATURES = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
static constexpr u32 MAX_DEVICES = 8;
static constexpr u32 MAX_FRAMEBUFFER_IMAGES = 8;
static constexpr u32 MAX_RENDER_PASS_ATTACHMENTS = MAX_FRAMEBUFFER_IMAGES;

struct InstanceInfo {
    cstr                                application_name;
    DebugCallback                       debug_callback;
    VkDebugUtilsMessageSeverityFlagsEXT debug_message_severity;
    VkDebugUtilsMessageTypeFlagsEXT     debug_message_type;
};

struct Surface {
    VkSurfaceKHR               hnd;
    VkSurfaceCapabilitiesKHR   capabilities;
    Array<VkSurfaceFormatKHR>* formats;
    Array<VkPresentModeKHR>*   present_modes;
};

struct QueueFamilies {
    static constexpr u32 NO_INDEX = U32_MAX;
    u32 graphics;
    u32 present;
};

union DeviceFeatures {
    VkPhysicalDeviceFeatures as_struct;
    VkBool32                 as_array[MAX_DEVICE_FEATURES];
};

struct PhysicalDevice {
    VkPhysicalDevice                 hnd;
    QueueFamilies                    queue_families;
    VkPhysicalDeviceProperties       properties;
    DeviceFeatures                   features;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkFormat                         depth_image_format;
};

struct BufferInfo {
    VkDeviceSize          size;
    VkSharingMode         sharing_mode;
    VkBufferUsageFlags    usage_flags;
    VkMemoryPropertyFlags mem_property_flags;
};

struct Buffer {
    VkBuffer       hnd;
    VkDeviceMemory mem;
    VkDeviceSize   size;
    u8*            mapped_mem;
};

struct ImageInfo {
    VkImageCreateInfo        image;
    VkImageViewCreateInfo    view;
    VkMemoryPropertyFlagBits mem_property_flags;
};

struct Image {
    VkImage        hnd;
    VkImageView    view;
    VkDeviceMemory mem;
    VkExtent3D     extent;
};

struct Shader {
    VkShaderModule        hnd;
    VkShaderStageFlagBits stage;
};

struct VertexLayout {
    FixedArray<VkVertexInputBindingDescription, 4>    bindings;
    FixedArray<VkVertexInputAttributeDescription, 16> attributes;
    u32                                               attribute_location;
};

struct Swapchain {
    VkSwapchainKHR      hnd;
    Array<VkImageView>* image_views;
    VkFormat            image_format;
    VkExtent2D          extent;
};

struct RenderPassInfo {
    FixedArray<VkAttachmentDescription, MAX_RENDER_PASS_ATTACHMENTS> attachment_descriptions;
    FixedArray<VkAttachmentReference, MAX_RENDER_PASS_ATTACHMENTS>   color_attachment_references;
    FixedArray<VkAttachmentReference, 1>                             depth_attachment_reference;
};

struct RTKContext {
    VkInstance                              instance;
    VkDebugUtilsMessengerEXT                debug_messenger;
    Surface                                 surface;
    FixedArray<PhysicalDevice, MAX_DEVICES> physical_devices;
    PhysicalDevice*                         physical_device;
    VkDevice                                device;
    VkQueue                                 graphics_queue;
    VkQueue                                 present_queue;
    Swapchain                               swapchain;
    VkRenderPass                            render_pass;
};

/// Internal
////////////////////////////////////////////////////////////
static QueueFamilies FindQueueFamilies(Stack temp_mem, VkPhysicalDevice physical_device, Surface* surface) {
    QueueFamilies queue_families = {
        .graphics = QueueFamilies::NO_INDEX,
        .present  = QueueFamilies::NO_INDEX,
    };
    Array<VkQueueFamilyProperties>* queue_family_properties = GetVkQueueFamilyProperties(&temp_mem, physical_device);

    // Find first graphics queue family.
    for (u32 queue_family_index = 0; queue_family_index < queue_family_properties->count; ++queue_family_index) {
        // Check if queue supports graphics operations.
        if (get(queue_family_properties, queue_family_index)->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_families.graphics = queue_family_index;
            break;
        }
    }

    // Find first present queue family.
    for (u32 queue_family_index = 0; queue_family_index < queue_family_properties->count; ++queue_family_index) {
        // Check if queue supports present operations.
        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, surface->hnd, &present_supported);

        if (present_supported) {
            queue_families.present = queue_family_index;
            break;
        }
    }

    return queue_families;
}

static VkFormat FindDepthImageFormat(VkPhysicalDevice physical_device) {
    static constexpr VkFormat DEPTH_IMAGE_FORMATS[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
    };

    // Find format that supports depth-stencil attachment feature for physical device.
    for (u32 i = 0; i < CTK_ARRAY_SIZE(DEPTH_IMAGE_FORMATS); i++) {
        VkFormat depth_image_format = DEPTH_IMAGE_FORMATS[i];
        VkFormatProperties format_properties = {};
        vkGetPhysicalDeviceFormatProperties(physical_device, depth_image_format, &format_properties);
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return depth_image_format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

static VkDeviceQueueCreateInfo GetSingleQueueInfo(u32 queue_family_index) {
    static constexpr f32 QUEUE_PRIORITIES[] = { 1.0f };

    VkDeviceQueueCreateInfo info = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .flags            = 0,
        .queueFamilyIndex = queue_family_index,
        .queueCount       = CTK_ARRAY_SIZE(QUEUE_PRIORITIES),
        .pQueuePriorities = QUEUE_PRIORITIES,
    };

    return info;
}

/// Interface
////////////////////////////////////////////////////////////
static void InitInstance(RTKContext* rtk, InstanceInfo info) {
    VkResult res = VK_SUCCESS;

#ifdef RTK_ENABLE_VALIDATION
    VkDebugUtilsMessengerCreateInfoEXT debug_msgr_info = {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = NULL,
        .flags           = 0,
        .messageSeverity = info.debug_message_severity,
        .messageType     = info.debug_message_type,
        .pfnUserCallback = info.debug_callback,
        .pUserData       = NULL,
    };
#endif

    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .pApplicationName   = info.application_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = info.application_name,
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_0,
    };

    cstr extensions[] = {
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef RTK_ENABLE_VALIDATION
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

#ifdef RTK_ENABLE_VALIDATION
    cstr validation_layer = "VK_LAYER_KHRONOS_validation";
#endif

    VkInstanceCreateInfo create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    create_info.flags                   = 0,
    create_info.pApplicationInfo        = &app_info,
    create_info.enabledExtensionCount   = CTK_ARRAY_SIZE(extensions),
    create_info.ppEnabledExtensionNames = extensions,
#ifdef RTK_ENABLE_VALIDATION
    create_info.pNext                   = &debug_msgr_info,
    create_info.enabledLayerCount       = 1,
    create_info.ppEnabledLayerNames     = &validation_layer,
#else
    create_info.pNext                   = NULL,
    create_info.enabledLayerCount       = 0,
    create_info.ppEnabledLayerNames     = NULL,
#endif

    res = vkCreateInstance(&create_info, NULL, &rtk->instance);
    Validate(res, "failed to create Vulkan instance");

#ifdef RTK_ENABLE_VALIDATION
    RTK_LOAD_INSTANCE_EXTENSION_FUNCTION(rtk->instance, vkCreateDebugUtilsMessengerEXT);
    res = vkCreateDebugUtilsMessengerEXT(rtk->instance, &debug_msgr_info, NULL, &rtk->debug_messenger);
    Validate(res, "failed to create debug messenger");
#endif
}

static void InitSurface(RTKContext* rtk, Window* window) {
    VkWin32SurfaceCreateInfoKHR info = {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = window->instance,
        .hwnd      = window->handle,
    };

    VkResult res = vkCreateWin32SurfaceKHR(rtk->instance, &info, NULL, &rtk->surface.hnd);
    Validate(res, "failed to create win32 surface");
}

static bool HasRequiredFeatures(PhysicalDevice* physical_device, DeviceFeatures* required_features) {
    for (u32 i = 0; i < MAX_DEVICE_FEATURES; ++i) {
        if (required_features->as_array[i] == VK_TRUE && physical_device->features.as_array[i] == VK_FALSE) {
            return false;
        }
    }

    return true;
}

static void UsePhysicalDevice(RTKContext* rtk, u32 index) {
    if (index >= rtk->physical_devices.count) {
        CTK_FATAL("physical device index %u is out of bounds: max is %u", index, rtk->physical_devices.count - 1);
    }

    rtk->physical_device = get(&rtk->physical_devices, index);
}

static void LoadCapablePhysicalDevices(RTKContext* rtk, Stack temp_mem, DeviceFeatures* required_features) {
    // Get system physical devices and ensure there is enough space in the context's physical devices list.
    Array<VkPhysicalDevice>* vk_physical_devices = GetVkPhysicalDevices(&temp_mem, rtk->instance);
    if (vk_physical_devices->count > MAX_DEVICES) {
        CTK_FATAL("can't load physical devices: max device count is %u, system device count is %u", MAX_DEVICES,
                  vk_physical_devices->count);
    }

    // Reset context's physical device list to store new list.
    rtk->physical_devices.count = 0;

    // Check all physical devices, and load the ones capable of rendering into the context's physical device list.
    for (u32 i = 0; i < vk_physical_devices->count; ++i) {
        VkPhysicalDevice vk_physical_device = get_copy(vk_physical_devices, i);

        // Collect all info about physical device.
        PhysicalDevice physical_device = {
            .hnd                = vk_physical_device,
            .queue_families     = FindQueueFamilies(temp_mem, vk_physical_device, &rtk->surface),
            .depth_image_format = FindDepthImageFormat(vk_physical_device),
        };
        vkGetPhysicalDeviceProperties(vk_physical_device, &physical_device.properties);
        vkGetPhysicalDeviceFeatures(vk_physical_device, &physical_device.features.as_struct);
        vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &physical_device.mem_properties);

        // Graphics and present queue families required.
        if (physical_device.queue_families.graphics == QueueFamilies::NO_INDEX ||
            physical_device.queue_families.present == QueueFamilies::NO_INDEX)
        {
            continue;
        }

        // Depth image format required.
        if (physical_device.depth_image_format == VK_FORMAT_UNDEFINED) {
            continue;
        }

        // All required features must be supported.
        if (!HasRequiredFeatures(&physical_device, required_features)) {
            continue;
        }

        push(&rtk->physical_devices, physical_device);
    }

    // Ensure atleast 1 capable physical device was loaded.
    if (rtk->physical_devices.count == 0) {
        CTK_FATAL("failed to load any capable physical devices");
    }
}

static void LogPhysicalDevice(PhysicalDevice* physical_device, cstr message) {
    VkPhysicalDeviceType type = physical_device->properties.deviceType;
    VkFormat depth_image_format = physical_device->depth_image_format;
    print_line("%s:", message);
    print_line("    name: %s", physical_device->properties.deviceName);
    print_line("    type: %s",
        type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU" :
        type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU" :
        "invalid");
    print_line("    queue_families:");
    print_line("        graphics: %u", physical_device->queue_families.graphics);
    print_line("        present: %u", physical_device->queue_families.present);
    print_line("    depth_image_format: %s",
        depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT ? "VK_FORMAT_D32_SFLOAT_S8_UINT" :
        depth_image_format == VK_FORMAT_D32_SFLOAT ? "VK_FORMAT_D32_SFLOAT" :
        depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT ? "VK_FORMAT_D24_UNORM_S8_UINT" :
        depth_image_format == VK_FORMAT_D16_UNORM_S8_UINT ? "VK_FORMAT_D16_UNORM_S8_UINT" :
        depth_image_format == VK_FORMAT_D16_UNORM ? "VK_FORMAT_D16_UNORM" :
        "UNKNWON");
}

static void InitDevice(RTKContext* rtk, DeviceFeatures* enabled_features) {
    QueueFamilies* queue_families = &rtk->physical_device->queue_families;

    // Add queue creation info for 1 queue in each queue family.
    FixedArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    push(&queue_infos, GetSingleQueueInfo(queue_families->graphics));

    // Don't create separate queues if present and graphics queue families are the same.
    if (queue_families->present != queue_families->graphics) {
        push(&queue_infos, GetSingleQueueInfo(queue_families->present));
    }

    // Create device, specifying enabled extensions and features.
    cstr enabled_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo create_info = {
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

static void InitQueues(RTKContext* rtk) {
    QueueFamilies* queue_families = &rtk->physical_device->queue_families;
    vkGetDeviceQueue(rtk->device, queue_families->graphics, 0, &rtk->graphics_queue);
    vkGetDeviceQueue(rtk->device, queue_families->present, 0, &rtk->present_queue);
}

static void GetSurfaceInfo(RTKContext* rtk, Stack* mem) {
    Surface* surface = &rtk->surface;
    VkPhysicalDevice vk_physical_device = rtk->physical_device->hnd;

    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, surface->hnd, &surface->capabilities);
    Validate(res, "failed to get physical device surface capabilities");

    surface->formats = GetVkPhysicalDeviceSurfaceFormats(mem, vk_physical_device, surface->hnd);
    surface->present_modes = GetVkPhysicalDeviceSurfacePresentModes(mem, vk_physical_device, surface->hnd);
}

static void InitSwapchain(RTKContext* rtk, Stack* mem, Stack temp_mem) {
    VkDevice device = rtk->device;
    Surface* surface = &rtk->surface;
    Swapchain* swapchain = &rtk->swapchain;
    VkResult res = VK_SUCCESS;

    // Default to first surface format, then check for preferred 4-component 8-bit BGRA unnormalized format and sRG
    // color space.
    VkSurfaceFormatKHR selected_format = get_copy(surface->formats, 0);
    for (u32 i = 0; i < surface->formats->count; ++i) {
        VkSurfaceFormatKHR surface_format = get_copy(surface->formats, i);
        if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM &&
            surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selected_format = surface_format;
            break;
        }
    }

    // Default to FIFO (only present mode with guarenteed availability), check for preferred mailbox present mode.
    VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < surface->present_modes->count; ++i) {
        VkPresentModeKHR surface_present_mode = get_copy(surface->present_modes, i);
        if (surface_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            selected_present_mode = surface_present_mode;
            break;
        }
    }

    // Set image count to min image count + 1 or max image count (whichever is smaller).
    u32 min_image_count = surface->capabilities.minImageCount + 1;
    if (surface->capabilities.maxImageCount > 0 && min_image_count > surface->capabilities.maxImageCount) {
        min_image_count = surface->capabilities.maxImageCount;
    }

    // Verify current extent has been set for surface.
    if (surface->capabilities.currentExtent.width == UINT32_MAX) {
        CTK_FATAL("current extent not set for surface")
    }

    VkSwapchainCreateInfoKHR info = {
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
    if (queue_families->graphics != queue_families->present) {
        info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(u32);
        info.pQueueFamilyIndices   = (u32*)queue_families;
    }
    else {
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
    swapchain->image_views = create_array_full<VkImageView>(mem, swapchain_images->count);

    for (u32 i = 0; i < swapchain_images->count; ++i) {
        VkImageViewCreateInfo view_info = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .flags    = 0,
            .image    = get_copy(swapchain_images, i),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = swapchain->image_format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        res = vkCreateImageView(device, &view_info, NULL, get(swapchain->image_views, i));
        Validate(res, "failed to create swapchain image view");
    }
}

static void PushColorAttachment(RenderPassInfo* info, VkAttachmentDescription description) {
    u32 attachment_index = info->attachment_descriptions.count;
    if (attachment_index == get_size(&info->attachment_descriptions)) {
        CTK_FATAL("cannot push color attachment: already at max attachment count of %u",
                  MAX_RENDER_PASS_ATTACHMENTS);
    }

    push(&info->color_attachment_references, {
        .attachment = attachment_index,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    });
    push(&info->attachment_descriptions, description);
}

static void PushDepthAttachment(RenderPassInfo* info, VkAttachmentDescription description) {
    u32 attachment_index = info->attachment_descriptions.count;
    if (attachment_index == get_size(&info->attachment_descriptions)) {
        CTK_FATAL("cannot push depth attachment: already at max attachment count of %u", MAX_RENDER_PASS_ATTACHMENTS);
    }

    if (info->depth_attachment_reference.count == 1) {
        CTK_FATAL("cannot push depth attachment: one has already been pushed (only one allowed)");
    }

    push(&info->depth_attachment_reference, {
        .attachment = attachment_index,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    });
    push(&info->attachment_descriptions, description);
}

static void InitRenderPass(RTKContext* rtk, RenderPassInfo* info) {
    VkSubpassDescription subpass_description = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = info->color_attachment_references.count,
        .pColorAttachments       = info->color_attachment_references.data,
        .pDepthStencilAttachment = info->depth_attachment_reference.count == 1
                                   ? info->depth_attachment_reference.data
                                   : NULL,
    };

    VkRenderPassCreateInfo create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = info->attachment_descriptions.count,
        .pAttachments    = info->attachment_descriptions.data,
        .subpassCount    = 1,
        .pSubpasses      = &subpass_description,
        .dependencyCount = 0,
        .pDependencies   = NULL,
    };
    VkResult res = vkCreateRenderPass(rtk->device, &create_info, NULL, &rtk->render_pass);
    Validate(res, "failed to create render pass");
}

// static VkDeviceMemory AllocateDeviceMemory(VkDevice device, PhysicalDevice* physical_device,
//                                            VkMemoryRequirements mem_requirements,
//                                            VkMemoryPropertyFlags mem_property_flags)
// {
//     // Find memory type index in memory properties based on memory property flags.
//     VkPhysicalDeviceMemoryProperties mem_properties = physical_device->mem_properties;
//     u32 mem_type_index = U32_MAX;
//     for (u32 i = 0; i < mem_properties.memoryTypeCount; ++i) {
//         // Ensure memory type at mem_properties.memoryTypes[i] is supported by mem_requirements.
//         if ((mem_requirements.memoryTypeBits & (1 << i)) == 0) {
//             continue;
//         }

//         // Check if memory at index has all property flags.
//         if ((mem_properties.memoryTypes[i].propertyFlags & mem_property_flags) == mem_property_flags) {
//             mem_type_index = i;
//             break;
//         }
//     }

//     if (mem_type_index == U32_MAX) {
//         CTK_FATAL("failed to find memory type that satisfies memory requirements");
//     }

//     // Allocate memory using selected memory type index.
//     VkMemoryAllocateInfo info = {
//         .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
//         .allocationSize = mem_requirements.size,
//         .memoryTypeIndex = mem_type_index,
//     };
//     VkDeviceMemory mem = VK_NULL_HANDLE;
//     VkResult res = vkAllocateMemory(device, &info, NULL, &mem);
//     Validate(res, "failed to allocate memory");

//     return mem;
// }

// static void InitBuffer(Buffer* buffer, VkDevice device, PhysicalDevice* physical_device, BufferInfo info) {
//     VkResult res = VK_SUCCESS;

//     VkBufferCreateInfo create_info = {
//         .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
//         .size = info.size,
//         .usage = info.usage_flags,
//         .sharingMode = info.sharing_mode,
//         .queueFamilyIndexCount = 0,
//         .pQueueFamilyIndices = NULL, // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
//     };
//     res = vkCreateBuffer(device, &create_info, NULL, &buffer->hnd);
//     Validate(res, "failed to create buffer");

//     // Allocate/bind buffer memory.
//     VkMemoryRequirements mem_requirements = {};
//     vkGetBufferMemoryRequirements(device, buffer->hnd, &mem_requirements);

//     buffer->mem = AllocateDeviceMemory(device, physical_device, mem_requirements, info.mem_property_flags);
//     res = vkBindBufferMemory(device, buffer->hnd, buffer->mem, 0);
//     Validate(res, "failed to bind buffer memory");

//     buffer->size = mem_requirements.size;

//     // Map host visible buffer memory.
//     if (info.mem_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
//         vkMapMemory(device, buffer->mem, 0, buffer->size, 0, (void**)&buffer->mapped_mem);
//     }
// }

// static void InitImage(Image* image, VkDevice device, PhysicalDevice* physical_device, ImageInfo info) {
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

// static void LoadShader(Shader* shader, Stack temp_mem, VkDevice device, cstr path, VkShaderStageFlagBits stage) {
//     Array<u32>* bytecode = read_file<u32>(&temp_mem, path);
//     if (bytecode == NULL) {
//         CTK_FATAL("failed to load bytecode from \"%s\"", path);
//     }

//     shader->stage = stage;
//     VkShaderModuleCreateInfo info = {
//         .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
//         .pNext = NULL,
//         .flags = 0,
//         .codeSize = byte_size(bytecode),
//         .pCode = bytecode->data,
//     };
//     VkResult res = vkCreateShaderModule(device, &info, NULL, &shader->hnd);
//     Validate(res, "failed to create shader from SPIR-V file \"%p\"", path);
// }

// static VkSemaphore CreateSemaphore(VkDevice device) {
//     VkSemaphoreCreateInfo info = {
//         .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
//         .pNext = NULL,
//         .flags = 0,
//     };
//     VkSemaphore semaphore = VK_NULL_HANDLE;
//     VkResult res = vkCreateSemaphore(device, &info, NULL, &semaphore);
//     Validate(res, "failed to create semaphore");

//     return semaphore;
// }

// static VkFence CreateFence(VkDevice device) {
//     VkFenceCreateInfo info = {
//         .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
//         .pNext = NULL,
//         .flags = VK_FENCE_CREATE_SIGNALED_BIT,
//     };
//     VkFence fence = VK_NULL_HANDLE;
//     VkResult res = vkCreateFence(device, &info, NULL, &fence);
//     Validate(res, "failed to create fence");

//     return fence;
// }

// static void PushBinding(VertexLayout* layout, VkVertexInputRate input_rate) {
//     push(&layout->bindings, {
//         .binding = layout->bindings.count,
//         .stride = 0,
//         .inputRate = input_rate,
//     });

//     layout->attribute_location = 0; // Reset attribute location for new binding.
// }

// static void PushAttribute(VertexLayout* layout, u32 field_count) {
//     static constexpr VkFormat FORMATS[] = {
//         VK_FORMAT_R32_SFLOAT,
//         VK_FORMAT_R32G32_SFLOAT,
//         VK_FORMAT_R32G32B32_SFLOAT,
//         VK_FORMAT_R32G32B32A32_SFLOAT,
//     };

//     CTK_ASSERT(layout->bindings.count > 0);
//     CTK_ASSERT(field_count > 0);
//     CTK_ASSERT(field_count <= 4);

//     u32 current_binding_index = layout->bindings.count - 1;
//     VkVertexInputBindingDescription* current_binding = get(&layout->bindings, current_binding_index);

//     push(&layout->attributes, {
//         .location = layout->attribute_location,
//         .binding = current_binding_index,
//         .format = FORMATS[field_count - 1],
//         .offset = current_binding->stride,
//     });

//     // Update binding state for future attributes.
//     current_binding->stride += field_count * 4;
//     layout->attribute_location++;
// }

// static void BeginTempCommandBuffer(VkCommandBuffer command_buffer) {
//     VkCommandBufferBeginInfo info = {
//         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//         .pNext = NULL,
//         .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
//         .pInheritanceInfo = NULL,
//     };
//     vkBeginCommandBuffer(command_buffer, &info);
// }

// static void SubmitTempCommandBuffer(VkCommandBuffer command_buffer, VkQueue queue) {
//     vkEndCommandBuffer(command_buffer);

//     VkSubmitInfo submit_info = {
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

}
