#pragma once

#include "ctk3/ctk3.h"
#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/context.h"

using namespace CTK;

namespace RTK
{

/// Interface
////////////////////////////////////////////////////////////
static VkDeviceMemory
AllocateDeviceMemory(VkMemoryRequirements mem_requirements, VkMemoryPropertyFlags mem_property_flags)
{
    // Find memory type index in memory properties based on memory property flags.
    VkPhysicalDeviceMemoryProperties* mem_properties = &global_ctx.physical_device->mem_properties;
    uint32 mem_type_index = UINT32_MAX;
    for (uint32 i = 0; i < mem_properties->memoryTypeCount; ++i)
    {
        // Ensure memory type at mem_properties->memoryTypes[i] is supported by mem_requirements.
        if ((mem_requirements.memoryTypeBits & (1 << i)) == 0)
        {
            continue;
        }

        // Check if memory at index has all property flags.
        if ((mem_properties->memoryTypes[i].propertyFlags & mem_property_flags) == mem_property_flags)
        {
            mem_type_index = i;
            break;
        }
    }

    if (mem_type_index == UINT32_MAX)
    {
        CTK_FATAL("failed to find memory type that satisfies memory requirements");
    }

    // Allocate memory using selected memory type index.
    VkMemoryAllocateInfo info =
    {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_requirements.size,
        .memoryTypeIndex = mem_type_index,
    };
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkResult res = vkAllocateMemory(global_ctx.device, &info, NULL, &mem);
    Validate(res, "vkAllocateMemory() failed");

    return mem;
}

}
