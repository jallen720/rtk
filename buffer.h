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

struct BufferMemory
{
    VkDeviceSize          size;
    VkMemoryPropertyFlags mem_properties;
    VkBufferUsageFlags    usage;

    VkBuffer              hnd;
    VkDeviceMemory        device_mem;
    uint8*                host_mem;
};

static struct BufferState
{
    uint32        max_buffers;
    uint32        buffer_count;
    uint32        frame_count;

    BufferInfo*   buffer_infos; // size: max_buffers
    uint32*       frame_counts; // size: max_buffers
    uint32*       mem_indexes;  // size: max_buffers
    VkDeviceSize* offsets;      // size: max_buffers * frame_count
    VkDeviceSize* indexes;      // size: max_buffers * frame_count

    FArray<BufferMemory, VK_MAX_MEMORY_TYPES> mems;
}
g_buffer_state;

/// Debug
////////////////////////////////////////////////////////////
static void LogBuffers()
{
    PrintLine("buffers:");
    for (uint32 buffer_index = 0; buffer_index < g_buffer_state.buffer_count; ++buffer_index)
    {
        BufferInfo* buffer_info = g_buffer_state.buffer_infos + buffer_index;
        uint32 frame_count = g_buffer_state.frame_counts[buffer_index];
        PrintLine("   [%2u] size:             %llu", buffer_index, buffer_info->size);
        PrintLine("        offset_alignment: %llu", buffer_info->offset_alignment);
        PrintLine("        per_frame:        %s", buffer_info->per_frame ? "true" : "false");
        PrintLine("        frame_count:      %u", frame_count);
        PrintLine("        mem_properties:   ");
        PrintMemoryPropertyFlags(buffer_info->mem_properties, 3);
        PrintLine("        usage:            ");
        PrintBufferUsageFlags(buffer_info->usage, 3);

        PrintLine("        mem_index:        %u", g_buffer_state.mem_indexes[buffer_index]);
        PrintLine("        offsets:          ");
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            uint32 frame_offset = frame_index * g_buffer_state.max_buffers;
            PrintLine("            [%u] %llu", frame_index, g_buffer_state.offsets[frame_offset + buffer_index]);
        }
        PrintLine();
    }
}

static void LogBufferMemory()
{
    PrintLine("buffer memory:");
    for (uint32 i = 0; i < g_buffer_state.mems.count; ++i)
    {
        BufferMemory* buffer_mem = GetPtr(&g_buffer_state.mems, i);
        PrintLine("   [%2u] hnd:            0x%p", i, buffer_mem->hnd);
        PrintLine("        size:           %llu", buffer_mem->size);
        PrintLine("        device_mem:     0x%p", buffer_mem->device_mem);
        PrintLine("        host_mem:       0x%p", buffer_mem->host_mem);
        PrintLine("        mem_properties: ");
        PrintMemoryPropertyFlags(buffer_mem->mem_properties, 3);
        PrintLine("        usage:          ");
        PrintBufferUsageFlags(buffer_mem->usage, 3);
        PrintLine();
    }
}

/// Utils
////////////////////////////////////////////////////////////
static BufferInfo* GetBufferInfoUtil(BufferHnd hnd)
{
    return g_buffer_state.buffer_infos + hnd.index;
}

static BufferMemory* GetBufferMemoryUtil(BufferHnd hnd)
{
    return GetPtr(&g_buffer_state.mems, g_buffer_state.mem_indexes[hnd.index]);
}

static uint32 GetBufferFrameIndex(BufferHnd hnd, uint32 frame_index)
{
    uint32 buffer_frame_offset = GetBufferInfoUtil(hnd)->per_frame ? frame_index * g_buffer_state.max_buffers : 0;
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
    g_buffer_state.max_buffers  = max_buffers;
    g_buffer_state.buffer_count = 0;
    g_buffer_state.frame_count  = frame_count;

    g_buffer_state.buffer_infos = Allocate<BufferInfo>  (allocator, max_buffers);
    g_buffer_state.frame_counts = Allocate<uint32>      (allocator, max_buffers);
    g_buffer_state.mem_indexes  = Allocate<uint32>      (allocator, max_buffers);
    g_buffer_state.offsets      = Allocate<VkDeviceSize>(allocator, max_buffers * frame_count);
    g_buffer_state.indexes      = Allocate<VkDeviceSize>(allocator, max_buffers * frame_count);
}

static BufferHnd CreateBuffer(BufferInfo* buffer_info)
{
    if (g_buffer_state.buffer_count >= g_buffer_state.max_buffers)
    {
        CTK_FATAL("can't create buffer: already at max of %u", g_buffer_state.max_buffers);
    }

    // Figure out minimum offset alignment if requested.
    // Spec: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VkMemoryRequirements
    if (buffer_info->offset_alignment == USE_MIN_OFFSET_ALIGNMENT)
    {
        VkPhysicalDeviceLimits* physical_device_limits = &GetPhysicalDevice()->properties.limits;
        buffer_info->offset_alignment = 4;

        // Uniform
        if ((buffer_info->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) &&
            buffer_info->offset_alignment < physical_device_limits->minUniformBufferOffsetAlignment)
        {
            buffer_info->offset_alignment = physical_device_limits->minUniformBufferOffsetAlignment;
        }

        // Storage
        if ((buffer_info->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) &&
            buffer_info->offset_alignment < physical_device_limits->minStorageBufferOffsetAlignment)
        {
            buffer_info->offset_alignment = physical_device_limits->minStorageBufferOffsetAlignment;
        }

        // Texel
        if ((buffer_info->usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)) &&
            buffer_info->offset_alignment < physical_device_limits->minTexelBufferOffsetAlignment)
        {
            buffer_info->offset_alignment = physical_device_limits->minTexelBufferOffsetAlignment;
        }
    }

    BufferHnd hnd = { .index = g_buffer_state.buffer_count };
    ++g_buffer_state.buffer_count;
    g_buffer_state.buffer_infos[hnd.index] = *buffer_info;
    g_buffer_state.frame_counts[hnd.index] = buffer_info->per_frame ? g_buffer_state.frame_count : 1;

    return hnd;
}

static void AllocateBuffers()
{
    VkDevice device = GetDevice();
    QueueFamilies* queue_families = &GetPhysicalDevice()->queue_families;
    VkResult res = VK_SUCCESS;

    // Calculate buffer memory information and buffer offsets.
    for (uint32 buffer_index = 0; buffer_index < g_buffer_state.buffer_count; ++buffer_index)
    {
        BufferInfo* buffer_info = g_buffer_state.buffer_infos + buffer_index;

        uint32 buffer_aligned_size = MultipleOf(buffer_info->size, buffer_info->offset_alignment);
        uint32 buffer_aligned_total_size = buffer_aligned_size * g_buffer_state.frame_counts[buffer_index];

        // Get mem index and calculate base offset.
        bool buffer_mem_exists = false;
        uint32 buffer_aligned_base_offset = UINT32_MAX;
        uint32 mem_index = UINT32_MAX;
        for (uint32 buffer_mem_index = 0; buffer_mem_index < g_buffer_state.mems.count; ++buffer_mem_index)
        {
            BufferMemory* buffer_mem = GetPtr(&g_buffer_state.mems, buffer_mem_index);
            if (buffer_mem->mem_properties == buffer_info->mem_properties)
            {
                buffer_aligned_base_offset = MultipleOf(buffer_mem->size, buffer_info->offset_alignment);
                mem_index = buffer_mem_index;
                buffer_mem->size = buffer_aligned_base_offset + buffer_aligned_total_size;
                buffer_mem->usage |= buffer_info->usage;
                buffer_mem_exists = true;
                break;
            }
        }
        if (!buffer_mem_exists)
        {
            buffer_aligned_base_offset = 0;
            mem_index = g_buffer_state.mems.count;
            Push(&g_buffer_state.mems,
                 {
                     .size           = buffer_aligned_total_size,
                     .mem_properties = buffer_info->mem_properties,
                     .usage          = buffer_info->usage,
                 });
        }
        g_buffer_state.mem_indexes[buffer_index] = mem_index;

        // Calculate offsets.
        for (uint32 frame_index = 0; frame_index < g_buffer_state.frame_counts[buffer_index]; ++frame_index)
        {
            uint32 frame_offset = frame_index * g_buffer_state.max_buffers;
            uint32 offset = buffer_aligned_base_offset + (buffer_aligned_size * frame_index);
            g_buffer_state.offsets[frame_offset + buffer_index] = offset;
        }
    }

    // Initialize and allocate buffer memory.
    CTK_ITER(buffer_mem, &g_buffer_state.mems)
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

    BufferInfo* buffer_info = GetBufferInfoUtil(hnd);
    if (data_size > buffer_info->size)
    {
        CTK_FATAL("can't write %u bytes to host buffer: write would exceed size of %u", data_size, buffer_info->size);
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

    BufferInfo* buffer_info = GetBufferInfoUtil(hnd);
    uint32 buffer_index = g_buffer_state.indexes[GetBufferFrameIndex(hnd, frame_index)];
    if (buffer_index + data_size > buffer_info->size)
    {
        CTK_FATAL("can't append %u bytes to host buffer at index %u: append would exceed size of %u", data_size,
                  buffer_index, buffer_info->size);
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

    BufferInfo* buffer_info = GetBufferInfoUtil(buffer_hnd);
    if (size > buffer_info->size)
    {
        CTK_FATAL("can't write %u bytes to device buffer: write would exceed size of %u", size,
                  buffer_info->size);
    }

    uint32 buffer_frame_index = GetBufferFrameIndex(buffer_hnd, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = g_buffer_state.offsets[GetBufferFrameIndex(src_buffer_hnd, frame_index)] + offset,
        .dstOffset = g_buffer_state.offsets[buffer_frame_index],
        .size      = size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
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

    BufferInfo* buffer_info = GetBufferInfoUtil(buffer_hnd);
    uint32 buffer_frame_index = GetBufferFrameIndex(buffer_hnd, frame_index);
    uint32 buffer_index = g_buffer_state.indexes[buffer_frame_index];
    if (buffer_index + size > buffer_info->size)
    {
        CTK_FATAL("can't append %u bytes to device buffer at index %u: append would exceed size of %u", size,
                  buffer_index, buffer_info->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = g_buffer_state.offsets[GetBufferFrameIndex(src_buffer_hnd, frame_index)] + offset,
        .dstOffset = g_buffer_state.offsets[buffer_frame_index] + buffer_index,
        .size      = size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBufferMemoryUtil(src_buffer_hnd)->hnd,
                    GetBufferMemoryUtil(buffer_hnd)->hnd,
                    1, &copy);
    g_buffer_state.indexes[buffer_frame_index] += size;
}

static BufferInfo* GetBufferInfo(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't get buffer");
    return GetBufferInfoUtil(hnd);
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
