#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/containers.h"

using namespace CTK;

namespace RTK
{

/// Interface
////////////////////////////////////////////////////////////
template<typename Type, typename VkFunc, typename... Args>
static Array<Type>* GetVkArray(Stack* mem, VkFunc vk_func, Args... args)
{
    VkResult res = VK_SUCCESS;

    uint32 count = 0;
    res = vk_func(args..., &count, NULL);
    Validate(res, "failed to get vulkan array count");

    auto array = CreateArrayFull<Type>(mem, count);
    res = vk_func(args..., &array->count, array->data);
    Validate(res, "failed to get vulkan array values");

    return array;
}

template<typename Type, typename VkFunc, typename... Args>
static void LoadVkArray(Array<Type>* array, Stack* mem, VkFunc vk_func, Args... args)
{
    VkResult res = VK_SUCCESS;

    uint32 count = 0;
    res = vk_func(args..., &count, NULL);
    Validate(res, "failed to get vulkan array count");

    InitArrayFull(array, mem, count);
    res = vk_func(args..., &array->count, array->data);
    Validate(res, "failed to get vulkan array values");
}

template<typename Type, typename VkFunc, typename... Args>
static Array<Type>* GetVkArrayUnchecked(Stack* mem, VkFunc vk_func, Args... args)
{
    uint32 count = 0;
    vk_func(args..., &count, NULL);

    auto array = CreateArrayFull<Type>(mem, count);
    vk_func(args..., &array->count, array->data);

    return array;
}

static Array<VkImage>* GetVkSwapchainImages(Stack* mem, VkDevice device, VkSwapchainKHR swapchain)
{
    return GetVkArray<VkImage>(mem, vkGetSwapchainImagesKHR, device, swapchain);
}

static Array<VkQueueFamilyProperties>* GetVkQueueFamilyProperties(Stack* mem, VkPhysicalDevice physical_device)
{
    return GetVkArrayUnchecked<VkQueueFamilyProperties>(mem, vkGetPhysicalDeviceQueueFamilyProperties, physical_device);
}

static Array<VkPhysicalDevice>* GetVkPhysicalDevices(Stack* mem, VkInstance instance)
{
    return GetVkArray<VkPhysicalDevice>(mem, vkEnumeratePhysicalDevices, instance);
}

static void LoadVkSurfaceFormats(Array<VkSurfaceFormatKHR>* array, Stack* mem, VkPhysicalDevice physical_device,
                                 VkSurfaceKHR surface)
{
    return LoadVkArray(array, mem, vkGetPhysicalDeviceSurfaceFormatsKHR, physical_device, surface);
}

static void LoadVkSurfacePresentModes(Array<VkPresentModeKHR>* array, Stack* mem, VkPhysicalDevice physical_device,
                                      VkSurfaceKHR surface)
{
    return LoadVkArray(array, mem, vkGetPhysicalDeviceSurfacePresentModesKHR, physical_device, surface);
}

}
