#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "ctk3/ctk3.h"
#include "ctk3/allocator.h"
#include "ctk3/array.h"

using namespace CTK;

namespace RTK
{

/// Interface
////////////////////////////////////////////////////////////
template<typename Type, typename VkFunc, typename... Args>
static void LoadVkArray(Array<Type>* array, const Allocator* allocator, VkFunc vk_func, Args... args)
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
    InitArrayFull(array, allocator, count);
    res = vk_func(args..., &array->count, array->data);
    Validate(res, "failed to get vulkan array values");
}

template<typename Type, typename VkFunc, typename... Args>
static void LoadVkArrayUnchecked(Array<Type>* array, const Allocator* allocator, VkFunc vk_func, Args... args)
{
    // Get number of elements for array, returning without initializing array if none were found.
    uint32 count = 0;
    vk_func(args..., &count, NULL);
    if (count == 0)
    {
        return;
    }

    // Initialize and fill array.
    InitArrayFull(array, allocator, count);
    vk_func(args..., &array->count, array->data);
}

static void LoadVkSwapchainImages(Array<VkImage>* array, const Allocator* allocator, VkDevice device,
                                  VkSwapchainKHR swapchain)
{
    LoadVkArray(array, allocator, vkGetSwapchainImagesKHR, device, swapchain);
}

static void LoadVkQueueFamilyProperties(Array<VkQueueFamilyProperties>* array, const Allocator* allocator,
                                        VkPhysicalDevice physical_device)
{
    LoadVkArrayUnchecked(array, allocator, vkGetPhysicalDeviceQueueFamilyProperties, physical_device);
}

static void LoadVkPhysicalDevices(Array<VkPhysicalDevice>* array, const Allocator* allocator, VkInstance instance)
{
    LoadVkArray(array, allocator, vkEnumeratePhysicalDevices, instance);
}

static void LoadVkSurfaceFormats(Array<VkSurfaceFormatKHR>* array, const Allocator* allocator,
                                 VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    LoadVkArray(array, allocator, vkGetPhysicalDeviceSurfaceFormatsKHR, physical_device, surface);
}

static void LoadVkSurfacePresentModes(Array<VkPresentModeKHR>* array, const Allocator* allocator,
                                      VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    LoadVkArray(array, allocator, vkGetPhysicalDeviceSurfacePresentModesKHR, physical_device, surface);
}

}
