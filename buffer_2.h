/// Data
////////////////////////////////////////////////////////////
static constexpr VkDeviceSize USE_MIN_OFFSET_ALIGNMENT = 0;

struct BufferStackInfo
{
    VkDeviceSize          size;
    VkDeviceSize          offset_alignment;
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

enum struct BufferType
{
    BUFFER,
    UNIFORM,
    STORAGE,
};

struct BufferInfo
{
    BufferType   type;
    VkDeviceSize size;
    VkDeviceSize offset_alignment;
    uint32       instance_count;
};

struct Buffer
{
    VkBuffer     hnd;
    uint8*       mapped_mem;
    VkDeviceSize size;
    uint32       instance_count;
    VkDeviceSize offsets[MAX_FRAME_COUNT];
    VkDeviceSize indexes[MAX_FRAME_COUNT];
};

/// Interface
////////////////////////////////////////////////////////////
static void InitBufferStack(BufferStack* buffer_stack, BufferStackInfo* info)
{
    PhysicalDevice* physical_device = GetPhysicalDevice();
    VkDevice device = GetDevice();
    VkResult res = VK_SUCCESS;

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size  = info->size;
    create_info.usage = info->usage_flags;

    // Check if sharing mode needs to be concurrent due to separate graphics & present queue families.
    QueueFamilies* queue_families = &physical_device->queue_families;
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

static void InitBuffer(Buffer* buffer, BufferStack* buffer_stack, BufferInfo* info)
{
    CTK_ASSERT(info->instance_count > 0);
    CTK_ASSERT(info->instance_count <= MAX_FRAME_COUNT);

    VkPhysicalDeviceLimits* limits = &GetPhysicalDevice()->properties.limits;
    if (info->offset_alignment == USE_MIN_OFFSET_ALIGNMENT)
    {
        info->offset_alignment = info->type == BufferType::UNIFORM ? limits->minUniformBufferOffsetAlignment :
                                 info->type == BufferType::STORAGE ? limits->minStorageBufferOffsetAlignment :
                                 4;
    }

    VkDeviceSize aligned_offset = MultipleOf(buffer_stack->index, info->offset_alignment);
    VkDeviceSize aligned_size = MultipleOf(info->size, info->offset_alignment);
    uint32 total_aligned_size = aligned_size * info->instance_count;
    if (aligned_offset + total_aligned_size > buffer_stack->size)
    {
        CTK_FATAL("can't sub-allocate %u offset-aligned %u-byte buffer(s) (%u-bytes total) from buffer-stack at "
                  "offset-aligned index %u: allocation would exceed buffer-stack size of %u",
                  info->instance_count, aligned_size, total_aligned_size, info->offset_alignment, aligned_offset,
                  buffer_stack->size);
    }

    buffer->hnd            = buffer_stack->hnd;
    buffer->mapped_mem     = buffer_stack->mapped_mem;
    buffer->size           = aligned_size;
    buffer->instance_count = info->instance_count;
    for (uint32 i = 0; i < info->instance_count; ++i)
    {
        buffer->offsets[i] = aligned_offset + (aligned_size * i);
        buffer->indexes[i] = 0;
    }

    buffer_stack->index = aligned_offset + total_aligned_size;
}

static Buffer* CreateBuffer(const Allocator* allocator, BufferStack* buffer_stack, BufferInfo* info)
{
    auto buffer = Allocate<Buffer>(allocator, 1);
    InitBuffer(buffer, buffer_stack, info);
    return buffer;
}

static void WriteHostBuffer(Buffer* buffer, uint32 instance_index, void* data, VkDeviceSize data_size)
{
    CTK_ASSERT(buffer->mapped_mem != NULL);
    CTK_ASSERT(instance_index < buffer->instance_count);

    if (data_size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to host-buffer: write would exceed size of %u", data_size, buffer->size);
    }

    memcpy(buffer->mapped_mem + buffer->offsets[instance_index], data, data_size);
    buffer->indexes[instance_index] = data_size;
}

static void AppendHostBuffer(Buffer* buffer, uint32 instance_index, void* data, VkDeviceSize data_size)
{
    CTK_ASSERT(buffer->mapped_mem != NULL);
    CTK_ASSERT(instance_index < buffer->instance_count);

    if (buffer->index + data_size > buffer->size)
    {
        CTK_FATAL("can't append %u bytes to host-buffer at index %u: append would exceed size of %u", data_size,
                  buffer->index, buffer->size);
    }

    memcpy(buffer->mapped_mem + buffer->offsets[instance_index] + buffer->index, data, data_size);
    buffer->indexes[instance_index] += data_size;
}

static void WriteDeviceBufferCmd(Buffer* buffer, uint32 instance_index,
                                 Buffer* src_buffer, uint32 src_instance_index,
                                 VkDeviceSize offset, VkDeviceSize size)
{
    CTK_ASSERT(host_buffer->instance_count == 1);
    CTK_ASSERT(instance_index < buffer->instance_count);

    if (size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to device-buffer: write would exceed size of %u", size,
                  buffer->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = host_buffer->offsets[src_instance_index] + offset,
        .dstOffset = buffer->offsets[instance_index],
        .size      = size,
    };
    vkCmdCopyBuffer(global_ctx.temp_command_buffer, host_buffer->hnd, buffer->hnd, 1, &copy);
    buffer->index = size;
}

static void AppendDeviceBufferCmd(Buffer* buffer, uint32 instance_index,
                                  Buffer* src_buffer, uint32 src_instance_index,
                                  VkDeviceSize offset, VkDeviceSize size)
{
    CTK_ASSERT(src_buffer->instance_count == 1);
    CTK_ASSERT(instance_index < buffer->instance_count);

    if (buffer->index + size > buffer->size)
    {
        CTK_FATAL("can't append %u bytes to device-buffer at index %u: append would exceed size of %u",
                  size, buffer->index, buffer->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = src_buffer->offsets[src_instance_index] + offset,
        .dstOffset = buffer->offsets[instance_index] + buffer->index,
        .size      = size,
    };
    vkCmdCopyBuffer(global_ctx.temp_command_buffer, src_buffer->hnd, buffer->hnd, 1, &copy);
    buffer->index += size;
}

template<typename Type>
static Type* GetMappedMem(Buffer* buffer, uint32 instance_index)
{
    CTK_ASSERT(buffer->mapped_mem != NULL);
    CTK_ASSERT(instance_index < buffer->instance_count);
    return (Type*)(buffer->mapped_mem + buffer->offsets[instance_index]);
}

static void Clear(Buffer* buffer)
{
    for (uint32 i = 0; i < buffer->instance_count; ++i)
    {
        buffer->indexes[i] = 0;
    }
}
