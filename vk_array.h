#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "ctk3/ctk3.h"
#include "ctk3/stack.h"
#include "ctk3/array.h"

using namespace CTK;

namespace RTK
{

/// Interface
////////////////////////////////////////////////////////////
template<typename Type, typename VkFunc, typename... Args>
static void LoadVkArray(Array<Type>* array, Stack* stack, VkFunc vk_func, Args... args)
{
    VkResult res = VK_SUCCESS;

    // Get number of elements for array, returning without initializing array if none were found.
    uint32 count = 0;
    res = vk_func(args..., &count, NULL);
    Validate(res, "failed to get vulkan array count");
    if (count == 0)
    {
        return;
    }

    // Initialize and fill array.
    InitArrayFull(array, stack, count);
    res = vk_func(args..., &array->count, array->data);
    Validate(res, "failed to get vulkan array values");
}

template<typename Type, typename VkFunc, typename... Args>
static void LoadVkArrayUnchecked(Array<Type>* array, Stack* stack, VkFunc vk_func, Args... args)
{
    // Get number of elements for array, returning without initializing array if none were found.
    uint32 count = 0;
    vk_func(args..., &count, NULL);
    if (count == 0)
    {
        return;
    }

    // Initialize and fill array.
    InitArrayFull(array, stack, count);
    vk_func(args..., &array->count, array->data);
}

static void LoadVkSwapchainImages(Array<VkImage>* array, Stack* stack, VkDevice device, VkSwapchainKHR swapchain)
{
    LoadVkArray(array, stack, vkGetSwapchainImagesKHR, device, swapchain);
}

static void LoadVkQueueFamilyProperties(Array<VkQueueFamilyProperties>* array, Stack* stack,
                                        VkPhysicalDevice physical_device)
{
    LoadVkArrayUnchecked(array, stack, vkGetPhysicalDeviceQueueFamilyProperties, physical_device);
}

static void LoadVkPhysicalDevices(Array<VkPhysicalDevice>* array, Stack* stack, VkInstance instance)
{
    LoadVkArray(array, stack, vkEnumeratePhysicalDevices, instance);
}

static void LoadVkSurfaceFormats(Array<VkSurfaceFormatKHR>* array, Stack* stack, VkPhysicalDevice physical_device,
                                 VkSurfaceKHR surface)
{
    LoadVkArray(array, stack, vkGetPhysicalDeviceSurfaceFormatsKHR, physical_device, surface);
}

static void LoadVkSurfacePresentModes(Array<VkPresentModeKHR>* array, Stack* stack, VkPhysicalDevice physical_device,
                                      VkSurfaceKHR surface)
{
    LoadVkArray(array, stack, vkGetPhysicalDeviceSurfacePresentModesKHR, physical_device, surface);
}

}
