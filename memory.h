#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/device.h"
#include "rtk/rtk_context.h"
#include "ctk2/ctk.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct BufferInfo
{
    VkDeviceSize          size;
    VkSharingMode         sharing_mode;
    VkBufferUsageFlags    usage_flags;
    VkMemoryPropertyFlags mem_property_flags;
};

struct Buffer
{
    VkBuffer       hnd;
    VkDeviceMemory mem;
    VkDeviceSize   size;
    VkDeviceSize   offset;
    uint8*         mapped_mem;
};

struct ImageInfo
{
    VkImageCreateInfo        image;
    VkImageViewCreateInfo    view;
    VkMemoryPropertyFlagBits mem_property_flags;
};

struct Image
{
    VkImage        hnd;
    VkImageView    view;
    VkDeviceMemory mem;
    VkExtent3D     extent;
};

/// Utils
////////////////////////////////////////////////////////////
static VkDeviceMemory AllocateDeviceMemory(RTKContext* rtk, VkMemoryRequirements mem_requirements,
                                           VkMemoryPropertyFlags mem_property_flags)
{
    // Find memory type index in memory properties based on memory property flags.
    VkPhysicalDeviceMemoryProperties* mem_properties = &rtk->physical_device->mem_properties;
    uint32 mem_type_index = U32_MAX;
    for (uint32 i = 0; i < mem_properties->memoryTypeCount; ++i)
    {
        // Ensure memory type at mem_properties->memoryTypes[i] is supported by mem_requirements.
        if ((mem_requirements.memoryTypeBits & (1 << i)) == 0)
            continue;

        // Check if memory at index has all property flags.
        if ((mem_properties->memoryTypes[i].propertyFlags & mem_property_flags) == mem_property_flags)
        {
            mem_type_index = i;
            break;
        }
    }

    if (mem_type_index == U32_MAX)
        CTK_FATAL("failed to find memory type that satisfies memory requirements");

    // Allocate memory using selected memory type index.
    VkMemoryAllocateInfo info =
    {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_requirements.size,
        .memoryTypeIndex = mem_type_index,
    };
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkResult res = vkAllocateMemory(rtk->device, &info, NULL, &mem);
    Validate(res, "failed to allocate memory");

    return mem;
}

/// Interface
////////////////////////////////////////////////////////////
static void InitBuffer(Buffer* buffer, RTKContext* rtk, BufferInfo* info)
{
    VkDevice device = rtk->device;
    VkResult res = VK_SUCCESS;

    VkBufferCreateInfo create_info =
    {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size                  = info->size,
        .usage                 = info->usage_flags,
        .sharingMode           = info->sharing_mode,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = NULL, // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
    };
    res = vkCreateBuffer(device, &create_info, NULL, &buffer->hnd);
    Validate(res, "failed to create buffer");

    // Allocate/bind buffer memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device, buffer->hnd, &mem_requirements);

    buffer->mem = AllocateDeviceMemory(rtk, mem_requirements, info->mem_property_flags);
    res = vkBindBufferMemory(device, buffer->hnd, buffer->mem, 0);
    Validate(res, "failed to bind buffer memory");

    buffer->size = mem_requirements.size;

    // Map host visible buffer memory.
    if (info->mem_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        vkMapMemory(device, buffer->mem, 0, buffer->size, 0, (void**)&buffer->mapped_mem);
}

static void InitBuffer(Buffer* buffer, RTKContext* rtk, BufferInfo info)
{
    InitBuffer(buffer, rtk, &info);
}

static void InitImage(Image* image, RTKContext* rtk, ImageInfo* info)
{
    VkDevice device = rtk->device;
    VkResult res = VK_SUCCESS;

    res = vkCreateImage(device, &info->image, NULL, &image->hnd);
    Validate(res, "failed to create image");

    // Allocate/bind image memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetImageMemoryRequirements(device, image->hnd, &mem_requirements);

    image->mem = AllocateDeviceMemory(rtk, mem_requirements, info->mem_property_flags);
    res = vkBindImageMemory(device, image->hnd, image->mem, 0);
    Validate(res, "failed to bind image memory");

    info->view.image = image->hnd;
    res = vkCreateImageView(device, &info->view, NULL, &image->view);
    Validate(res, "failed to create image view");

    image->extent = info->image.extent;
}

static void InitImage(Image* image, RTKContext* rtk, ImageInfo info)
{
    InitImage(image, rtk, &info);
}

}
