#pragma once

#include "ctk/ctk.h"
#include "ctk/containers.h"
#include "ctk/memory.h"
#include "vtk/vtk.h"

using namespace ctk;

namespace Rend {

struct InstanceInfo {
    cstr application_name;
    vtk::DebugCallback debug_callback;
};

struct Surface {
    VkSurfaceKHR hnd;
    VkSurfaceCapabilitiesKHR capabilities;
    Array<VkSurfaceFormatKHR>* formats;
    Array<VkPresentModeKHR>* present_modes;
};

struct Context {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    Surface surface;
};

static void InitInstance(Context* ctx, InstanceInfo info) {
    VkResult res = VK_SUCCESS;

#ifdef REND_ENABLE_VALIDATION
    VkDebugUtilsMessengerCreateInfoEXT debug_msgr_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .messageSeverity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = info.debug_callback,
        .pUserData = NULL,
    };
#endif

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = info.application_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = info.application_name,
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    cstr extensions[] = {
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef REND_ENABLE_VALIDATION
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

#ifdef REND_ENABLE_VALIDATION
    cstr validation_layer = "VK_LAYER_KHRONOS_validation";
#endif

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    create_info.flags = 0,
    create_info.pApplicationInfo = &app_info,
    create_info.enabledExtensionCount = CTK_ARRAY_SIZE(extensions),
    create_info.ppEnabledExtensionNames = extensions,
#ifdef REND_ENABLE_VALIDATION
    create_info.pNext = &debug_msgr_info,
    create_info.enabledLayerCount = 1,
    create_info.ppEnabledLayerNames = &validation_layer,
#else
    create_info.pNext = NULL,
    create_info.enabledLayerCount = 0,
    create_info.ppEnabledLayerNames = NULL,
#endif

    res = vkCreateInstance(&create_info, NULL, &ctx->instance);
    vtk::validate(res, "failed to create Vulkan instance");

#ifdef REND_ENABLE_VALIDATION
    VTK_LOAD_INSTANCE_EXTENSION_FUNCTION(ctx->instance, vkCreateDebugUtilsMessengerEXT);
    res = vkCreateDebugUtilsMessengerEXT(ctx->instance, &debug_msgr_info, NULL, &ctx->debug_messenger);
    vtk::validate(res, "failed to create debug messenger");
#endif
}

static void InitRend(Context* ctx, Stack* mem) {
    InitInstance(ctx, {
        .application_name = "Rend",
        .debug_callback = vtk::debug_callback,
    });
}

}
