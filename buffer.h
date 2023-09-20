/// Data
////////////////////////////////////////////////////////////
struct BufferStackInfo
{
    VkDeviceSize          size;
    VkBufferUsageFlags    usage_flags;
    VkMemoryPropertyFlags mem_property_flags;
};

struct BufferStack
{
    VkBuffer       hnd;
    VkDeviceMemory mem;
    VkDeviceSize   size;
    VkDeviceSize   index;
    uint8*         mapped_mem;
};

struct Buffer
{
    VkBuffer     hnd;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkDeviceSize index;
    uint8*       mapped_mem;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitBufferStack(BufferStack* buffer_stack, BufferStackInfo* info)
{
    VkDevice device = GetDevice();
    VkResult res = VK_SUCCESS;

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size  = info->size;
    create_info.usage = info->usage_flags;

    // Check if sharing mode needs to be concurrent due to separate graphics & present queue families.
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

    res = vkCreateBuffer(device, &create_info, NULL, &buffer_stack->hnd);
    Validate(res, "vkCreateBuffer() failed");

    // Allocate/bind buffer_stack memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device, buffer_stack->hnd, &mem_requirements);
PrintResourceMemoryInfo("buffer-stack", &mem_requirements, info->mem_property_flags);

    buffer_stack->mem = AllocateDeviceMemory(&mem_requirements, info->mem_property_flags, NULL);
    res = vkBindBufferMemory(device, buffer_stack->hnd, buffer_stack->mem, 0);
    Validate(res, "vkBindBufferMemory() failed");

    buffer_stack->size  = mem_requirements.size;
    buffer_stack->index = 0;

    // Map host visible buffer_stack memory.
    if (info->mem_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(device, buffer_stack->mem, 0, buffer_stack->size, 0, (void**)&buffer_stack->mapped_mem);
    }
}

static BufferStack* CreateBufferStack(const Allocator* allocator, BufferStackInfo* info)
{
    auto buffer_stack = Allocate<BufferStack>(allocator, 1);
    InitBufferStack(buffer_stack, info);
    return buffer_stack;
}

static void InitBuffer(Buffer* buffer, BufferStack* buffer_stack, VkDeviceSize size)
{
    CTK_TODO("fix to align buffer-stack index (should be named offset) before allocation instead of using aligned size");
    VkDeviceSize min_offset_alignment = global_ctx.physical_device->properties.limits.minUniformBufferOffsetAlignment;
    VkDeviceSize aligned_size = MultipleOf(size, min_offset_alignment);

    if (buffer_stack->index + aligned_size > buffer_stack->size)
    {
        CTK_FATAL("can't sub-allocate %u-byte buffer for from buffer-stack at index %u: allocation would exceed "
                  "buffer-stack size of %u", aligned_size, buffer_stack->index, buffer_stack->size);
    }

    buffer->hnd        = buffer_stack->hnd;
    buffer->offset     = buffer_stack->index;
    buffer->size       = aligned_size;
    buffer->index      = 0;
    buffer->mapped_mem = buffer_stack->mapped_mem == NULL ? NULL : buffer_stack->mapped_mem + buffer->offset;

    buffer_stack->index += aligned_size;
}

static Buffer* CreateBuffer(const Allocator* allocator, BufferStack* buffer_stack, VkDeviceSize size)
{
    auto buffer = Allocate<Buffer>(allocator, 1);
    InitBuffer(buffer, buffer_stack, size);
    return buffer;
}

static void WriteHostBuffer(Buffer* buffer, void* data, VkDeviceSize data_size)
{
    CTK_ASSERT(buffer->mapped_mem != NULL);

    if (data_size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to host-buffer: write would exceed size of %u", data_size, buffer->size);
    }

    memcpy(buffer->mapped_mem, data, data_size);
    buffer->index = data_size;
}

static void AppendHostBuffer(Buffer* buffer, void* data, VkDeviceSize data_size)
{
    CTK_ASSERT(buffer->mapped_mem != NULL);

    if (buffer->index + data_size > buffer->size)
    {
        CTK_FATAL("can't append %u bytes to host-buffer at index %u: append would exceed size of %u", data_size,
                  buffer->index, buffer->size);
    }

    memcpy(buffer->mapped_mem + buffer->index, data, data_size);
    buffer->index += data_size;
}

static void WriteDeviceBufferCmd(Buffer* buffer, Buffer* staging_buffer, VkDeviceSize offset = 0, VkDeviceSize size = 0)
{
    if (size == 0)
    {
        size = staging_buffer->index;
    }

    if (size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to device-buffer: write would exceed size of %u", size,
                  buffer->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = staging_buffer->offset + offset,
        .dstOffset = buffer->offset,
        .size      = size,
    };
    vkCmdCopyBuffer(global_ctx.temp_command_buffer, staging_buffer->hnd, buffer->hnd, 1, &copy);
    buffer->index = size;
}

static void AppendDeviceBufferCmd(Buffer* buffer, Buffer* staging_buffer, VkDeviceSize offset = 0,
                                  VkDeviceSize size = 0)
{
    if (size == 0)
    {
        size = staging_buffer->index;
    }

    if (buffer->index + size > buffer->size)
    {
        CTK_FATAL("can't append %u bytes to device-buffer at index %u: append would exceed size of %u",
                  size, buffer->index, buffer->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = staging_buffer->offset + offset,
        .dstOffset = buffer->offset + buffer->index,
        .size      = size,
    };
    vkCmdCopyBuffer(global_ctx.temp_command_buffer, staging_buffer->hnd, buffer->hnd, 1, &copy);
    buffer->index += size;
}

static void Clear(Buffer* buffer)
{
    buffer->index = 0;
}
