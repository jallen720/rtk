#pragma once

#include "rtk/vulkan.h"
#include "ctk2/ctk.h"

using namespace CTK;

/// Macros
////////////////////////////////////////////////////////////
#define RTK_VK_RESULT_NAME(VK_RESULT) VK_RESULT, #VK_RESULT

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
using DebugCallback = VKAPI_ATTR VkBool32 (VKAPI_CALL*)(VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity_flag_bit,
                                                        VkDebugUtilsMessageTypeFlagsEXT msg_type_flags,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                        void* user_data);

struct VkResultInfo
{
    VkResult result;
    cstring  name;
    cstring  message;
};

/// Interface
////////////////////////////////////////////////////////////
static void PrintResult(VkResult result)
{
    static constexpr VkResultInfo VK_RESULT_DEBUG_INFOS[] =
    {
        {
            RTK_VK_RESULT_NAME(VK_SUCCESS),
            "Command successfully completed."
        },
        {
            RTK_VK_RESULT_NAME(VK_NOT_READY),
            "A fence or query has not yet completed."
        },
        {
            RTK_VK_RESULT_NAME(VK_TIMEOUT),
            "A wait operation has not completed in the specified time."
        },
        {
            RTK_VK_RESULT_NAME(VK_EVENT_SET),
            "An event is signaled."
        },
        {
            RTK_VK_RESULT_NAME(VK_EVENT_RESET),
            "An event is unsignaled."
        },
        {
            RTK_VK_RESULT_NAME(VK_INCOMPLETE),
            "A return array was too small for the result."
        },
        {
            RTK_VK_RESULT_NAME(VK_SUBOPTIMAL_KHR),
            "A swapchain no longer matches the surface properties exactly, but can still be used to present to the "
            "surface successfully."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_OUT_OF_HOST_MEMORY),
            "A host memory allocation has failed."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_OUT_OF_DEVICE_MEMORY),
            "A device memory allocation has failed."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INITIALIZATION_FAILED),
            "Initialization of an object could not be completed for implementation-specific reasons."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_DEVICE_LOST),
            "The logical or physical device has been lost."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_MEMORY_MAP_FAILED),
            "Mapping of a memory object has failed."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_LAYER_NOT_PRESENT),
            "A requested layer is not present or could not be loaded."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_EXTENSION_NOT_PRESENT),
            "A requested extension is not supported."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_FEATURE_NOT_PRESENT),
            "A requested feature is not supported."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INCOMPATIBLE_DRIVER),
            "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for "
            "implementation-specific reasons."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_TOO_MANY_OBJECTS),
            "Too many objects of the type have already been created."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_FORMAT_NOT_SUPPORTED),
            "A requested format is not supported on this device."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_FRAGMENTED_POOL),
            "A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no "
            "attempt to allocate host or device memory was made to accommodate the new allocation. This should be "
            "returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the "
            "pool allocation failure was due to fragmentation."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_SURFACE_LOST_KHR),
            "A surface is no longer available."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR),
            "The requested window is already in use by Vulkan or another API in a manner which prevents it from being "
            "used again."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_OUT_OF_DATE_KHR),
            "A surface has changed in such a way that it is no longer compatible with the swapchain, and further "
            "presentation requests using the swapchain will fail. Applications must query the new surface properties "
            "and recreate their swapchain if they wish to continue presenting to the surface."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR),
            "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a "
            "way that prevents sharing an image."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INVALID_SHADER_NV),
            "One or more shaders failed to compile or link."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_OUT_OF_POOL_MEMORY),
            "A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device "
            "memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of "
            "the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INVALID_EXTERNAL_HANDLE),
            "An external handle is not a valid handle of the specified type."
        },
        // {
        //     RTK_VK_RESULT_NAME(VK_ERROR_FRAGMENTATION),
        //     "A descriptor pool creation has failed due to fragmentation."
        // },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT),
            "A buffer creation failed because the requested address is not available."
        },
        // {
        //     RTK_VK_RESULT_NAME(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS),
        //     "A buffer creation or memory allocation failed because the requested address is not available."
        // },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT),
            "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it "
            "did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside "
            "of the application’s control."
        },
        // {
        //     RTK_VK_RESULT_NAME(VK_ERROR_UNKNOWN),
        //     "An unknown error has occurred; either the application has provided invalid input, or an implementation "
        //     "failure has occurred."
        // },
    };

    const VkResultInfo* res_info = NULL;
    for (uint32 i = 0; i < CTK_ARRAY_SIZE(VK_RESULT_DEBUG_INFOS); ++i)
    {
        res_info = VK_RESULT_DEBUG_INFOS + i;
        if (res_info->result == result)
            break;
    }

    if (!res_info)
    {
        CTK_FATAL("failed to find debug info for VkResult %d", result);
    }

    if (res_info->result == 0)
        PrintInfo("vulkan function returned %s: %s", res_info->name, res_info->message);
    else if (res_info->result > 0)
        PrintWarning("vulkan function returned %s: %s", res_info->name, res_info->message);
    else
        PrintError("vulkan function returned %s: %s", res_info->name, res_info->message);
}

template<typename ...Args>
static void Validate(VkResult result, cstring fail_message, Args... args)
{
    if (result != VK_SUCCESS)
    {
        PrintResult(result);
        CTK_FATAL(fail_message, args...)
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_flag_bit,
                     VkDebugUtilsMessageTypeFlagsEXT message_type_flags,
                     const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    CTK_UNUSED(message_type_flags)
    CTK_UNUSED(user_data)
    cstring message_id = callback_data->pMessageIdName ? callback_data->pMessageIdName : "";
    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT & message_severity_flag_bit)
    {
        CTK_FATAL("%s\n", callback_data->pMessage);
    }
    else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT & message_severity_flag_bit)
    {
        PrintWarning("%s\n", callback_data->pMessage);
    }
    else
    {
        PrintInfo("%s\n", callback_data->pMessage);
    }

    return VK_FALSE;
}

static cstring VkPresentModeName(VkPresentModeKHR present_mode)
{
    static constexpr Pair<VkPresentModeKHR, cstring> NAMES[] =
    {
        CTK_ENUM_NAME_PAIR(VK_PRESENT_MODE_IMMEDIATE_KHR),
        CTK_ENUM_NAME_PAIR(VK_PRESENT_MODE_MAILBOX_KHR),
        CTK_ENUM_NAME_PAIR(VK_PRESENT_MODE_FIFO_KHR),
        CTK_ENUM_NAME_PAIR(VK_PRESENT_MODE_FIFO_RELAXED_KHR),
        CTK_ENUM_NAME_PAIR(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR),
        CTK_ENUM_NAME_PAIR(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR),
    };

    cstring name = NULL;
    if (!FindValue(NAMES, CTK_ARRAY_SIZE(NAMES), present_mode, &name))
        CTK_FATAL("can't find name for present mode %u", present_mode);

    return name;
}

}
