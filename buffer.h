#pragma once

#include "ctk3/ctk3.h"
#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/context.h"
#include "rtk/device.h"

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
    VkDeviceMemory device_mem;
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

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size  = info->size;
    create_info.usage = info->usage_flags;

    // Check if image sharing mode needs to be concurrent due to separate graphics & present queue families.
    QueueFamilies* queue_families = &global_ctx.physical_device->queue_families;
    if (queue_families->graphics != queue_families->present)
    {
        create_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(uint32);
        create_info.pQueueFamilyIndices   = (uint32*)queue_families;
    }
    else
    {
        create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices   = NULL;
    }

    res = vkCreateBuffer(device, &create_info, NULL, &buffer->hnd);
    Validate(res, "vkCreateBuffer() failed");

    // Allocate/bind buffer memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device, buffer->hnd, &mem_requirements);
PrintResourceMemoryInfo("buffer", &mem_requirements, info->mem_property_flags);

    buffer->device_mem = AllocateDeviceMemory(&mem_requirements, info->mem_property_flags, NULL);
    res = vkBindBufferMemory(device, buffer->hnd, buffer->device_mem, 0);
    Validate(res, "vkBindBufferMemory() failed");

    buffer->size = mem_requirements.size;

    // Map host visible buffer memory.
    if (info->mem_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(device, buffer->device_mem, 0, buffer->size, 0, (void**)&buffer->mapped_mem);
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
    buffer->device_mem = parent_buffer->device_mem;
    buffer->mapped_mem = parent_buffer->mapped_mem == NULL ? NULL : parent_buffer->mapped_mem + parent_buffer->index;
    buffer->offset     = parent_buffer->index;
    buffer->size       = size;
    buffer->index      = 0;

    parent_buffer->index += size;
}

static Buffer* CreateBuffer(const Allocator* allocator, BufferInfo* info)
{
    auto buffer = Allocate<Buffer>(allocator, 1);
    InitBuffer(buffer, info);
    return buffer;
}

static Buffer* CreateBuffer(const Allocator* allocator, Buffer* parent_buffer, VkDeviceSize size)
{
    auto buffer = Allocate<Buffer>(allocator, 1);
    InitBuffer(buffer, parent_buffer, size);
    return buffer;
}

static void WriteHostBuffer(Buffer* buffer, void* data, VkDeviceSize data_size)
{
    if (buffer->mapped_mem == NULL)
    {
        CTK_FATAL("can't write to host-buffer: mapped_mem == NULL");
    }

    if (buffer->index + data_size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to host-buffer at index %u: write would exceed buffer size of %u", data_size,
                  buffer->index, buffer->size);
    }

    memcpy(buffer->mapped_mem + buffer->index, data, data_size);
    buffer->index += data_size;
}

static void Clear(Buffer* buffer)
{
    buffer->index = 0;
}

}
