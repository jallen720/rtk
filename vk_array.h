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
static Array<Type>* GetVkArray(Stack* stack, VkFunc vk_func, Args... args)
{
    VkResult res = VK_SUCCESS;

    uint32 count = 0;
    res = vk_func(args..., &count, NULL);
    Validate(res, "failed to get vulkan array count");

    auto array = CreateArrayFull<Type>(stack, count);
    res = vk_func(args..., &array->count, array->data);
    Validate(res, "failed to get vulkan array values");

    return array;
}

template<typename Type, typename VkFunc, typename... Args>
static void LoadVkArray(Array<Type>* array, Stack* stack, VkFunc vk_func, Args... args)
{
    VkResult res = VK_SUCCESS;

    uint32 count = 0;
    res = vk_func(args..., &count, NULL);
    Validate(res, "failed to get vulkan array count");

    InitArrayFull(array, stack, count);
    res = vk_func(args..., &array->count, array->data);
    Validate(res, "failed to get vulkan array values");
}

template<typename Type, typename VkFunc, typename... Args>
static Array<Type>* GetVkArrayUnchecked(Stack* stack, VkFunc vk_func, Args... args)
{
    uint32 count = 0;
    vk_func(args..., &count, NULL);

    auto array = CreateArrayFull<Type>(stack, count);
    vk_func(args..., &array->count, array->data);

    return array;
}

static Array<VkImage>* GetVkSwapchainImages(Stack* stack, VkDevice device, VkSwapchainKHR swapchain)
{
    return GetVkArray<VkImage>(stack, vkGetSwapchainImagesKHR, device, swapchain);
}

static Array<VkQueueFamilyProperties>* GetVkQueueFamilyProperties(Stack* stack, VkPhysicalDevice physical_device)
{
    return GetVkArrayUnchecked<VkQueueFamilyProperties>(stack, vkGetPhysicalDeviceQueueFamilyProperties,
                                                        physical_device);
}

static Array<VkPhysicalDevice>* GetVkPhysicalDevices(Stack* stack, VkInstance instance)
{
    return GetVkArray<VkPhysicalDevice>(stack, vkEnumeratePhysicalDevices, instance);
}

static void LoadVkSurfaceFormats(Array<VkSurfaceFormatKHR>* array, Stack* stack, VkPhysicalDevice physical_device,
                                 VkSurfaceKHR surface)
{
    return LoadVkArray(array, stack, vkGetPhysicalDeviceSurfaceFormatsKHR, physical_device, surface);
}

static void LoadVkSurfacePresentModes(Array<VkPresentModeKHR>* array, Stack* stack, VkPhysicalDevice physical_device,
                                      VkSurfaceKHR surface)
{
    return LoadVkArray(array, stack, vkGetPhysicalDeviceSurfacePresentModesKHR, physical_device, surface);
}

}
