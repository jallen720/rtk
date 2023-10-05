/// Data
////////////////////////////////////////////////////////////
struct BufferHnd { uint32 index; };

static constexpr VkDeviceSize USE_MIN_OFFSET_ALIGNMENT = 0;

struct BufferInfo
{
    VkDeviceSize          size;
    VkDeviceSize          offset_alignment;
    bool                  per_frame;
    VkMemoryPropertyFlags mem_properties;
    VkBufferUsageFlags    usage;
};
using Buffer = BufferInfo; // Stored information for Buffer is same as information used to create it.

struct BufferMemory
{
    VkBuffer              hnd;
    VkDeviceSize          size;
    VkDeviceMemory        device_mem;
    uint8*                host_mem;
    VkMemoryPropertyFlags mem_properties;
    VkBufferUsageFlags    usage;
};

static struct BufferState
{
    Buffer*       buffers;     // size: max_buffers
    uint32*       mem_indexes; // size: max_buffers
    VkDeviceSize* offsets;     // size: max_buffers * frame_count
    VkDeviceSize* indexes;     // size: max_buffers * frame_count
    uint32        max_buffers;
    uint32        buffer_count;
    uint32        frame_count;

    FArray<BufferMemory, VK_MAX_MEMORY_TYPES> buffer_mems;
}
g_buffer_state;

/// Debug
////////////////////////////////////////////////////////////
static void LogBuffers()
{
    PrintLine("buffers:");
    for (uint32 buffer_index = 0; buffer_index < g_buffer_state.buffer_count; ++buffer_index)
    {
        Buffer* buffer = g_buffer_state.buffers + buffer_index;
        PrintLine("   [%2u] mem_properties:", buffer_index);
        PrintMemoryProperties(buffer->mem_properties, 3);
        PrintLine("        usage:");
        PrintBufferUsage(buffer->usage, 3);
        PrintLine("        size:             %u", buffer->size);
        PrintLine("        offset_alignment: %u", buffer->offset_alignment);
        PrintLine("        per_frame:        %s", buffer->per_frame ? "true" : "false");
        PrintLine("        mem_index:        %u", g_buffer_state.mem_indexes[buffer_index]);
        PrintLine("        offsets:");
        if (buffer->per_frame)
        {
            for (uint32 frame_index = 0; frame_index < GetFrameCount(); ++frame_index)
            {
                uint32 frame_offset = frame_index * g_buffer_state.buffer_count;
                PrintLine("            [%u] %u", frame_index, g_buffer_state.offsets[frame_offset + buffer_index]);
            }
        }
        else
        {
            PrintLine("            [0] %u", g_buffer_state.offsets[buffer_index]);
        }
        PrintLine();
    }
}

static void LogBufferMems()
{
    PrintLine("buffer mems:");
    for (uint32 i = 0; i < g_buffer_state.buffer_mems.count; ++i)
    {
        BufferMemory* buffer_mem = GetPtr(&g_buffer_state.buffer_mems, i);
        PrintLine("   [%2u] mem_properties:", i);
        PrintMemoryProperties(buffer_mem->mem_properties, 3);
        PrintLine("        usage:");
        PrintBufferUsage(buffer_mem->usage, 3);
        PrintLine("        size: %u", buffer_mem->size);
        PrintLine();
    }
}

/// Utils
////////////////////////////////////////////////////////////
static Buffer* GetBufferUtil(BufferHnd hnd)
{
    return g_buffer_state.buffers + hnd.index;
}

static BufferMemory* GetBufferMemoryUtil(BufferHnd hnd)
{
    return GetPtr(&g_buffer_state.buffer_mems, g_buffer_state.mem_indexes[hnd.index]);
}

static uint32 GetBufferFrameIndex(BufferHnd hnd, uint32 frame_index)
{
    uint32 buffer_frame_offset = GetBufferUtil(hnd)->per_frame ? frame_index * g_buffer_state.max_buffers : 0;
    return buffer_frame_offset + hnd.index;
}

static void ValidateBufferHnd(BufferHnd hnd, const char* action)
{
    if (hnd.index >= g_buffer_state.buffer_count)
    {
        CTK_FATAL("%s: buffer handle index %u exceeds max buffer count of %u",
                  action, hnd.index, g_buffer_state.buffer_count);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitBufferModule(const Allocator* allocator, uint32 max_buffers)
{
    uint32 frame_count = GetFrameCount();
    g_buffer_state.buffers      = Allocate<Buffer>      (allocator, max_buffers);
    g_buffer_state.mem_indexes  = Allocate<uint32>      (allocator, max_buffers);
    g_buffer_state.offsets      = Allocate<VkDeviceSize>(allocator, max_buffers * frame_count);
    g_buffer_state.indexes      = Allocate<VkDeviceSize>(allocator, max_buffers * frame_count);
    g_buffer_state.max_buffers  = max_buffers;
    g_buffer_state.buffer_count = 0;
    g_buffer_state.frame_count  = frame_count;
}

static BufferHnd CreateBuffer(BufferInfo* info)
{
    if (g_buffer_state.buffer_count >= g_buffer_state.max_buffers)
    {
        CTK_FATAL("can't create buffer: already at max buffer count of %u", g_buffer_state.max_buffers);
    }

    BufferHnd hnd = { .index = g_buffer_state.buffer_count };
    ++g_buffer_state.buffer_count;
    Buffer* buffer = g_buffer_state.buffers + hnd.index;
    *buffer = *info; // Buffer is exact copy of info passed to CreateBuffer().

    // Figure out minimum offset alignment if requested.
    if (buffer->offset_alignment == USE_MIN_OFFSET_ALIGNMENT)
    {
        VkPhysicalDeviceLimits* physical_device_limits = &GetPhysicalDevice()->properties.limits;
        buffer->offset_alignment = 4;

        // Uniform
        if ((buffer->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) &&
            buffer->offset_alignment < physical_device_limits->minUniformBufferOffsetAlignment)
        {
            buffer->offset_alignment = physical_device_limits->minUniformBufferOffsetAlignment;
        }

        // Storage
        if ((buffer->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) &&
            buffer->offset_alignment < physical_device_limits->minStorageBufferOffsetAlignment)
        {
            buffer->offset_alignment = physical_device_limits->minStorageBufferOffsetAlignment;
        }

        // Texel
        if ((buffer->usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)) &&
            buffer->offset_alignment < physical_device_limits->minTexelBufferOffsetAlignment)
        {
            buffer->offset_alignment = physical_device_limits->minTexelBufferOffsetAlignment;
        }
    }

    return hnd;
}

static void BackBuffersWithMemory()
{
    PhysicalDevice* physical_device = GetPhysicalDevice();
    VkDevice device = GetDevice();
    VkResult res = VK_SUCCESS;

    // Calculate buffer memory information and buffer offsets.
    for (uint32 buffer_index = 0; buffer_index < g_buffer_state.buffer_count; ++buffer_index)
    {
        Buffer* buffer = g_buffer_state.buffers + buffer_index;

        CTK_TODO("shouldn't use aligned size");
        uint32 buffer_aligned_size = MultipleOf(buffer->size, buffer->offset_alignment);
        uint32 buffer_aligned_total_size = buffer_aligned_size * (buffer->per_frame ? g_buffer_state.frame_count : 1);

        // Get mem index and calculate base offset.
        bool buffer_mem_exists = false;
        uint32 buffer_aligned_base_offset = UINT32_MAX;
        uint32 mem_index = UINT32_MAX;
        for (uint32 buffer_mem_index = 0; buffer_mem_index < g_buffer_state.buffer_mems.count; ++buffer_mem_index)
        {
            BufferMemory* buffer_mem = GetPtr(&g_buffer_state.buffer_mems, buffer_mem_index);
            if (buffer_mem->mem_properties == buffer->mem_properties)
            {
                buffer_aligned_base_offset = MultipleOf(buffer_mem->size, buffer->offset_alignment);
                mem_index = buffer_mem_index;
                buffer_mem->size = buffer_aligned_base_offset + buffer_aligned_total_size;
                buffer_mem->usage |= buffer->usage;
                buffer_mem_exists = true;
                break;
            }
        }
        if (!buffer_mem_exists)
        {
            buffer_aligned_base_offset = 0;
            mem_index = g_buffer_state.buffer_mems.count;
            Push(&g_buffer_state.buffer_mems,
                 {
                     .size           = buffer_aligned_total_size,
                     .mem_properties = buffer->mem_properties,
                     .usage          = buffer->usage,
                 });
        }
        g_buffer_state.mem_indexes[buffer_index] = mem_index;

        // Calculate offsets (if per-frame).
        if (buffer->per_frame)
        {
            for (uint32 frame_index = 0; frame_index < GetFrameCount(); ++frame_index)
            {
                uint32 frame_offset = frame_index * g_buffer_state.buffer_count;
                uint32 offset = buffer_aligned_base_offset + (buffer_aligned_size * frame_index);
                g_buffer_state.offsets[frame_offset + buffer_index] = offset;
            }
        }
        else
        {
            uint32 frame_offset = 0;
            uint32 offset = buffer_aligned_base_offset;
            g_buffer_state.offsets[frame_offset + buffer_index] = offset;
        }

    }

    // Initialize and allocate buffer memory.
    QueueFamilies* queue_families = &GetPhysicalDevice()->queue_families;
    CTK_ITER(buffer_mem, &g_buffer_state.buffer_mems)
    {
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size  = buffer_mem->size;
        create_info.usage = buffer_mem->usage;

        // Check if sharing mode needs to be concurrent due to separate graphics & present queue families.
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

        res = vkCreateBuffer(device, &create_info, NULL, &buffer_mem->hnd);
        Validate(res, "vkCreateBuffer() failed");

        // Allocate/bind buffer memory.
        VkMemoryRequirements mem_requirements = {};
        vkGetBufferMemoryRequirements(device, buffer_mem->hnd, &mem_requirements);
        buffer_mem->size = mem_requirements.size;
PrintResourceMemoryInfo("buffer-mem", &mem_requirements, buffer_mem->mem_properties);

        buffer_mem->device_mem = AllocateDeviceMemory(&mem_requirements, buffer_mem->mem_properties, NULL);
        res = vkBindBufferMemory(device, buffer_mem->hnd, buffer_mem->device_mem, 0);
        Validate(res, "vkBindBufferMemory() failed");

        // Map host visible buffer memory.
        if (buffer_mem->mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            vkMapMemory(device, buffer_mem->device_mem, 0, buffer_mem->size, 0, (void**)&buffer_mem->host_mem);
        }
    }
}

static void WriteHostBuffer(BufferHnd hnd, uint32 frame_index, void* data, VkDeviceSize data_size)
{
    ValidateBufferHnd(hnd, "can't write to host buffer");
    CTK_ASSERT(frame_index < g_buffer_state.frame_count);

    Buffer* buffer = GetBufferUtil(hnd);
    if (data_size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to host buffer: write would exceed size of %u", data_size, buffer->size);
    }

    BufferMemory* buffer_mem = GetBufferMemoryUtil(hnd);
    CTK_ASSERT(buffer_mem->mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    uint32 buffer_frame_index = GetBufferFrameIndex(hnd, frame_index);
    memcpy(buffer_mem->host_mem + g_buffer_state.offsets[buffer_frame_index], data, data_size);
    g_buffer_state.indexes[buffer_frame_index] = data_size;
}

static void AppendHostBuffer(BufferHnd hnd, uint32 frame_index, void* data, VkDeviceSize data_size)
{
    ValidateBufferHnd(hnd, "can't append to host buffer");
    CTK_ASSERT(frame_index < g_buffer_state.frame_count);

    Buffer* buffer = GetBufferUtil(hnd);
    uint32 buffer_index = g_buffer_state.indexes[GetBufferFrameIndex(hnd, frame_index)];
    if (buffer_index + data_size > buffer->size)
    {
        CTK_FATAL("can't append %u bytes to host buffer at index %u: append would exceed size of %u", data_size,
                  buffer_index, buffer->size);
    }

    BufferMemory* buffer_mem = GetBufferMemoryUtil(hnd);
    CTK_ASSERT(buffer_mem->mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    uint32 buffer_frame_index = GetBufferFrameIndex(hnd, frame_index);
    memcpy(buffer_mem->host_mem + g_buffer_state.offsets[buffer_frame_index] + buffer_index, data, data_size);
    g_buffer_state.indexes[buffer_frame_index] += data_size;
}

static void WriteDeviceBufferCmd(BufferHnd buffer_hnd, uint32 frame_index,
                                 BufferHnd src_buffer_hnd, uint32 src_frame_index,
                                 VkDeviceSize offset, VkDeviceSize size)
{
    ValidateBufferHnd(buffer_hnd, "can't write to device buffer");
    ValidateBufferHnd(src_buffer_hnd, "can't write to device buffer");
    CTK_ASSERT(frame_index < g_buffer_state.frame_count);

    Buffer* buffer = GetBufferUtil(buffer_hnd);
    if (size > buffer->size)
    {
        CTK_FATAL("can't write %u bytes to device buffer: write would exceed size of %u", size,
                  buffer->size);
    }

    uint32 buffer_frame_index = GetBufferFrameIndex(buffer_hnd, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = g_buffer_state.offsets[GetBufferFrameIndex(src_buffer_hnd, frame_index)] + offset,
        .dstOffset = g_buffer_state.offsets[buffer_frame_index],
        .size      = size,
    };
    vkCmdCopyBuffer(global_ctx.temp_command_buffer,
                    GetBufferMemoryUtil(src_buffer_hnd)->hnd,
                    GetBufferMemoryUtil(buffer_hnd)->hnd,
                    1, &copy);
    g_buffer_state.indexes[buffer_frame_index] = size;
}

static void AppendDeviceBufferCmd(BufferHnd buffer_hnd, uint32 frame_index,
                                  BufferHnd src_buffer_hnd, uint32 src_frame_index,
                                  VkDeviceSize offset, VkDeviceSize size)
{
    ValidateBufferHnd(buffer_hnd, "can't append to device buffer");
    ValidateBufferHnd(src_buffer_hnd, "can't append to device buffer");
    CTK_ASSERT(frame_index < g_buffer_state.frame_count);

    Buffer* buffer = GetBufferUtil(buffer_hnd);
    uint32 buffer_frame_index = GetBufferFrameIndex(buffer_hnd, frame_index);
    uint32 buffer_index = g_buffer_state.indexes[buffer_frame_index];
    if (buffer_index + size > buffer->size)
    {
        CTK_FATAL("can't append %u bytes to device buffer at index %u: append would exceed size of %u", size,
                  buffer_index, buffer->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = g_buffer_state.offsets[GetBufferFrameIndex(src_buffer_hnd, frame_index)] + offset,
        .dstOffset = g_buffer_state.offsets[buffer_frame_index] + buffer_index,
        .size      = size,
    };
    vkCmdCopyBuffer(global_ctx.temp_command_buffer,
                    GetBufferMemoryUtil(src_buffer_hnd)->hnd,
                    GetBufferMemoryUtil(buffer_hnd)->hnd,
                    1, &copy);
    g_buffer_state.indexes[buffer_frame_index] += size;
}

static Buffer* GetBuffer(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't get buffer");
    return GetBufferUtil(hnd);
}

static BufferMemory* GetBufferMemory(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't get buffer memory");
    return GetBufferMemoryUtil(hnd);
}

static VkDeviceSize GetOffset(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer offset");
    CTK_ASSERT(frame_index < g_buffer_state.frame_count);

    return g_buffer_state.offsets[GetBufferFrameIndex(hnd, frame_index)];
}

static VkDeviceSize GetIndex(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer index");
    CTK_ASSERT(frame_index < g_buffer_state.frame_count);

    return g_buffer_state.indexes[GetBufferFrameIndex(hnd, frame_index)];
}

static void SetIndex(BufferHnd hnd, uint32 frame_index, VkDeviceSize index)
{
    ValidateBufferHnd(hnd, "can't set buffer index");
    CTK_ASSERT(frame_index < g_buffer_state.frame_count);

    g_buffer_state.indexes[GetBufferFrameIndex(hnd, frame_index)] = index;
}

template<typename Type>
static Type* GetHostMemory(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer mapped memory");
    CTK_ASSERT(frame_index < g_buffer_state.frame_count);

    BufferMemory* buffer_mem = GetBufferMemoryUtil(hnd);
    CTK_ASSERT(buffer_mem->mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    return (Type*)(buffer_mem->host_mem + g_buffer_state.offsets[GetBufferFrameIndex(hnd, frame_index)]);
}

static void Clear(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't clear buffer");
    for (uint32 frame_index = 0; frame_index < g_buffer_state.frame_count; ++frame_index)
    {
        SetIndex(hnd, frame_index, 0);
    }
}
