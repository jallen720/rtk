/// Data
////////////////////////////////////////////////////////////
struct DeviceStackInfo
{
    VkDeviceSize          size;
    VkBufferUsageFlags    usage_flags;
    VkMemoryPropertyFlags mem_property_flags;
};

struct DeviceStack
{
    VkBuffer       hnd;
    VkDeviceMemory mem;
    uint8*         mapped_mem;
    VkDeviceSize   size;
    VkDeviceSize   index;
};

struct Buffer
{
    VkBuffer     hnd;
    uint8*       mapped_mem;
    VkDeviceSize offset;
    VkDeviceSize size;
    VkDeviceSize index;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitDeviceStack(DeviceStack* device_stack, DeviceStackInfo* info)
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

    res = vkCreateBuffer(device, &create_info, NULL, &device_stack->hnd);
    Validate(res, "vkCreateBuffer() failed");

    // Allocate/bind device_stack memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device, device_stack->hnd, &mem_requirements);
PrintResourceMemoryInfo("device_stack", &mem_requirements, info->mem_property_flags);

    device_stack->mem = AllocateDeviceMemory(&mem_requirements, info->mem_property_flags, NULL);
    res = vkBindBufferMemory(device, device_stack->hnd, device_stack->mem, 0);
    Validate(res, "vkBindBufferMemory() failed");

    device_stack->size = mem_requirements.size;

    // Map host visible device_stack memory.
    if (info->mem_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(device, device_stack->mem, 0, device_stack->size, 0, (void**)&device_stack->mapped_mem);
    }
}

static DeviceStack* CreateDeviceStack(const Allocator* allocator, DeviceStackInfo* info)
{
    auto device_stack = Allocate<DeviceStack>(allocator, 1);
    InitDeviceStack(device_stack, info);
    return device_stack;
}

static void InitBuffer(Buffer* buffer, DeviceStack* device_stack, VkDeviceSize size)
{
    VkDeviceSize aligned_size =
        MultipleOf(size, global_ctx.physical_device->properties.limits.minUniformBufferOffsetAlignment);

    if (device_stack->index + size > device_stack->size)
    {
        CTK_FATAL("can't allocate %u-byte buffer from device-stack at index %u: allocation would exceed device-stack "
                  "size of %u", aligned_size, device_stack->index, device_stack->size);
    }

    buffer->hnd        = device_stack->hnd;
    buffer->mapped_mem = device_stack->mapped_mem == NULL ? NULL : device_stack->mapped_mem + device_stack->index;
    buffer->offset     = device_stack->index;
    buffer->size       = aligned_size;
    buffer->index      = 0;

    device_stack->index += aligned_size;
}

static Buffer* CreateBuffer(const Allocator* allocator, DeviceStack* device_stack, VkDeviceSize size)
{
    auto buffer = Allocate<Buffer>(allocator, 1);
    InitBuffer(buffer, device_stack, size);
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
