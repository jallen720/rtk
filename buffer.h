#pragma once

#include "ctk3/ctk3.h"
#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/context.h"
#include "rtk/resources.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct BufferInfo
{
    VkDeviceSize          size;
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

/// Interface
////////////////////////////////////////////////////////////
static void InitBuffer(Buffer* buffer, BufferInfo* info)
{
    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;

    VkBufferCreateInfo create_info =
    {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size                  = info->size,
        .usage                 = info->usage_flags,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = NULL, // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
    };
    res = vkCreateBuffer(device, &create_info, NULL, &buffer->hnd);
    Validate(res, "vkCreateBuffer() failed");

    // Allocate/bind buffer memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device, buffer->hnd, &mem_requirements);

    buffer->mem = AllocateDeviceMemory(mem_requirements, info->mem_property_flags);
    res = vkBindBufferMemory(device, buffer->hnd, buffer->mem, 0);
    Validate(res, "vkBindBufferMemory() failed");

    buffer->size = mem_requirements.size;

    // Map host visible buffer memory.
    if (info->mem_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(device, buffer->mem, 0, buffer->size, 0, (void**)&buffer->mapped_mem);
    }

    // Buffers allocated directly from device memory manage the entire region of memory.
    buffer->offset = 0;
}

static void InitBuffer(Buffer* buffer, Buffer* parent_buffer, VkDeviceSize size)
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

static BufferHnd CreateBuffer(BufferInfo* info)
{
    BufferHnd buffer_hnd = AllocateBuffer();
    InitBuffer(GetBuffer(buffer_hnd), info);
    return buffer_hnd;
}

static BufferHnd CreateBuffer(BufferHnd parent_buffer_hnd, VkDeviceSize size)
{
    BufferHnd buffer_hnd = AllocateBuffer();
    InitBuffer(GetBuffer(buffer_hnd), GetBuffer(parent_buffer_hnd), size);
    return buffer_hnd;
}

static void Write(Buffer* buffer, void* data, VkDeviceSize data_size)
{
    if (buffer->mapped_mem == NULL)
    {
        CTK_FATAL("can't write to buffer: buffer is not host visible (mapped_mem == NULL)");
    }

    if (buffer->index + data_size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to buffer at index %u: write would exceed buffer size of %u", data_size,
                  buffer->index, buffer->size);
    }

    memcpy(buffer->mapped_mem + buffer->index, data, data_size);
    buffer->index += data_size;
}

static void Write(BufferHnd buffer_hnd, void* data, VkDeviceSize data_size)
{
    Buffer* buffer = GetBuffer(buffer_hnd);
    Write(buffer, data, data_size);
}

static void Clear(BufferHnd buffer_hnd)
{
    GetBuffer(buffer_hnd)->index = 0;
}

}
