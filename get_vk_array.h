#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/containers.h"

using namespace ctk;

namespace RTK {

template<typename Type, typename VkArrayFunc, typename... Args>
static Array<Type>* GetVkArray(Stack* mem, VkArrayFunc vk_array_func, Args... args) {
    VkResult res = VK_SUCCESS;

    u32 count = 0;
    res = vk_array_func(args..., &count, NULL);
    Validate(res, "failed to get vulkan array count");

    auto array = create_array_full<Type>(mem, count);
    res = vk_array_func(args..., &array->count, array->data);
    Validate(res, "failed to get vulkan array values");

    return array;
}

template<typename Type, typename VkArrayFunc, typename... Args>
static Array<Type>* GetVkArrayUnchecked(Stack* mem, VkArrayFunc vk_array_func, Args... args) {
    u32 count = 0;
    vk_array_func(args..., &count, NULL);

    auto array = create_array_full<Type>(mem, count);
    vk_array_func(args..., &array->count, array->data);

    return array;
}

static Array<VkImage>*
GetVkSwapchainImages(Stack* mem, VkDevice device, VkSwapchainKHR swapchain) {
    return GetVkArray<VkImage>(mem, vkGetSwapchainImagesKHR, device, swapchain);
}

static Array<VkQueueFamilyProperties>*
GetVkQueueFamilyProperties(Stack* mem, VkPhysicalDevice physical_device) {
    return GetVkArrayUnchecked<VkQueueFamilyProperties>(mem, vkGetPhysicalDeviceQueueFamilyProperties, physical_device);
}

static Array<VkPhysicalDevice>*
GetVkPhysicalDevices(Stack* mem, VkInstance instance) {
    return GetVkArray<VkPhysicalDevice>(mem, vkEnumeratePhysicalDevices, instance);
}

static Array<VkSurfaceFormatKHR>*
GetVkPhysicalDeviceSurfaceFormats(Stack* mem, VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    return GetVkArray<VkSurfaceFormatKHR>(mem, vkGetPhysicalDeviceSurfaceFormatsKHR, physical_device, surface);
}

static Array<VkPresentModeKHR>*
GetVkPhysicalDeviceSurfacePresentModes(Stack* mem, VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    return GetVkArray<VkPresentModeKHR>(mem, vkGetPhysicalDeviceSurfacePresentModesKHR, physical_device, surface);
}

}
