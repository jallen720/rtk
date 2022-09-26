#pragma once

#include "rtk/vulkan.h"
#include "ctk2/ctk.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
static constexpr uint32 MAX_DEVICE_FEATURES = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

struct QueueFamilies
{
    uint32 graphics;
    uint32 present;
};

union DeviceFeatures
{
    VkPhysicalDeviceFeatures as_struct;
    VkBool32                 as_array[MAX_DEVICE_FEATURES];
};

struct PhysicalDevice
{
    VkPhysicalDevice                 hnd;
    QueueFamilies                    queue_families;
    VkPhysicalDeviceProperties       properties;
    DeviceFeatures                   features;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkFormat                         depth_image_format;
};

/// Interface
////////////////////////////////////////////////////////////
static bool HasRequiredFeatures(PhysicalDevice* physical_device, DeviceFeatures* required_features)
{
    for (uint32 i = 0; i < MAX_DEVICE_FEATURES; ++i)
        if (required_features->as_array[i] == VK_TRUE && physical_device->features.as_array[i] == VK_FALSE)
            return false;

    return true;
}

static void LogPhysicalDevice(PhysicalDevice* physical_device, cstring message)
{
    VkPhysicalDeviceType type = physical_device->properties.deviceType;
    VkFormat depth_image_format = physical_device->depth_image_format;

    PrintLine("%s:", message);
    PrintLine("    name: %s", physical_device->properties.deviceName);
    PrintLine("    type: %s",
        type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU   ? "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU" :
        type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU" :
        "invalid");
    PrintLine("    queue_families:");
    PrintLine("        graphics: %u", physical_device->queue_families.graphics);
    PrintLine("        present: %u", physical_device->queue_families.present);
    PrintLine("    depth_image_format: %s",
        depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT ? "VK_FORMAT_D32_SFLOAT_S8_UINT" :
        depth_image_format == VK_FORMAT_D32_SFLOAT         ? "VK_FORMAT_D32_SFLOAT"         :
        depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT  ? "VK_FORMAT_D24_UNORM_S8_UINT"  :
        depth_image_format == VK_FORMAT_D16_UNORM_S8_UINT  ? "VK_FORMAT_D16_UNORM_S8_UINT"  :
        depth_image_format == VK_FORMAT_D16_UNORM          ? "VK_FORMAT_D16_UNORM"          :
        "UNKNWON");
}

}
