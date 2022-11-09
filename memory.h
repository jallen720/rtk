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
    uint8*         mapped_mem;
    VkDeviceSize   offset;
    VkDeviceSize   size;
    VkDeviceSize   index;
};

struct Region
{
    VkBuffer     buffer;
    uint8*       mapped_mem;
    VkDeviceSize size;
    VkDeviceSize offset;
};

struct ImageInfo
{
    VkImageCreateInfo     image;
    VkImageViewCreateInfo view;
    VkMemoryPropertyFlags mem_property_flags;
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
    Validate(res, "vkAllocateMemory() failed");

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
    Validate(res, "vkCreateBuffer() failed");

    // Allocate/bind buffer memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device, buffer->hnd, &mem_requirements);

    buffer->mem = AllocateDeviceMemory(rtk, mem_requirements, info->mem_property_flags);
    res = vkBindBufferMemory(device, buffer->hnd, buffer->mem, 0);
    Validate(res, "vkBindBufferMemory() failed");

    buffer->size = mem_requirements.size;

    // Map host visible buffer memory.
    if (info->mem_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        vkMapMemory(device, buffer->mem, 0, buffer->size, 0, (void**)&buffer->mapped_mem);

    // Buffers allocated directly from device memory manage the entire region of memory.
    buffer->offset = 0;
}

static void Allocate(Buffer* buffer, Buffer* parent_buffer, VkDeviceSize size)
{
    if (parent_buffer->index + size > parent_buffer->size)
    {
        CTK_FATAL("can't allocate %u bytes from parent buffer at index %u: allocation would exceed parent buffer size "
                  "of %u", size, parent_buffer->index, parent_buffer->size);
    }

    buffer->hnd        = parent_buffer->hnd;
    buffer->mem        = parent_buffer->mem;
    buffer->mapped_mem = parent_buffer->mapped_mem == NULL ? NULL : parent_buffer->mapped_mem + parent_buffer->index;
    buffer->offset     = parent_buffer->index;
    buffer->size       = size;
    buffer->index      = 0;

    parent_buffer->index += size;
}

static void Write(Buffer* buffer, void* data, VkDeviceSize data_size)
{
    if (buffer->mapped_mem == NULL)
        CTK_FATAL("can't write to buffer: buffer is not host visible (mapped_mem == NULL)");

    if (buffer->index + data_size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to buffer at index %u: write would exceed buffer size of %u", data_size,
                  buffer->index, buffer->size);
    }

    memcpy(buffer->mapped_mem + buffer->index, data, data_size);
    buffer->index += data_size;
}

static void Clear(Buffer* buffer)
{
    buffer->index = 0;
}

static void Allocate(Region* region, Buffer* buffer, VkDeviceSize size)
{
    if (buffer->index + size > buffer->size)
    {
        CTK_FATAL("can't allocate %u-byte region from buffer at index %u: allocation would exceed buffer size of %u",
                  size, buffer->index, buffer->size);
    }

    region->buffer     = buffer->hnd;
    region->mapped_mem = buffer->mapped_mem == NULL ? NULL : buffer->mapped_mem + buffer->index;
    region->size       = size;
    region->offset     = buffer->offset + buffer->index;

    buffer->index += size;
}

static void Write(Region* region, void* data, VkDeviceSize data_size)
{
    if (region->mapped_mem == NULL)
        CTK_FATAL("can't write to region: region is not host visible (mapped_mem == NULL)");

    if (data_size > region->size)
        CTK_FATAL("can't write %u bytes to region: write would exceed region size of %u", data_size, region->size);

    memcpy(region->mapped_mem, data, data_size);
}

static void InitImage(Image* image, RTKContext* rtk, ImageInfo* info)
{
    VkDevice device = rtk->device;
    VkResult res = VK_SUCCESS;

    res = vkCreateImage(device, &info->image, NULL, &image->hnd);
    Validate(res, "vkCreateImage() failed");

    // Allocate/bind image memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetImageMemoryRequirements(device, image->hnd, &mem_requirements);

    image->mem = AllocateDeviceMemory(rtk, mem_requirements, info->mem_property_flags);
    res = vkBindImageMemory(device, image->hnd, image->mem, 0);
    Validate(res, "vkBindImageMemory() failed");

    image->extent = info->image.extent;


    info->view.image = image->hnd;
    res = vkCreateImageView(device, &info->view, NULL, &image->view);
    Validate(res, "vkCreateImageView() failed");
}

}
