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
    VkBuffer              buffer;
    VkDeviceMemory        device_mem;
    uint8*                host_mem;
    VkMemoryPropertyFlags mem_properties;
    VkBufferUsageFlags    usage;
};

struct BufferState
{
    uint32        max_buffers;
    uint32        buffer_count;
    uint32        frame_count;

    BufferInfo*   buffer_infos;  // size: max_buffers
    uint32*       frame_strides; // size: max_buffers
    uint32*       mem_indexes;   // size: max_buffers
    VkDeviceSize* offsets;       // size: max_buffers * frame_count
    VkDeviceSize* indexes;       // size: max_buffers * frame_count

    BufferMemory  mems[VK_MAX_MEMORY_TYPES];
};

static BufferState g_buffer_state;

/// Utils
////////////////////////////////////////////////////////////
static uint32 GetMemoryIndex(BufferInfo* buffer_info)
{
    VkDevice device = GetDevice();
    QueueFamilies* queue_families = &GetPhysicalDevice()->queue_families;

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size  = buffer_info->size;
    create_info.usage = buffer_info->usage;

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
    VkBuffer temp = VK_NULL_HANDLE;
    VkResult res = vkCreateBuffer(device, &create_info, NULL, &temp);
    Validate(res, "vkCreateBuffer() failed");
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device, temp, &mem_requirements);
    vkDestroyBuffer(device, temp, NULL);

    return GetCapableMemoryTypeIndex(&mem_requirements, buffer_info->mem_properties);
}

static bool OffsetAlignmentDesc(uint32* buffer_index_a, uint32* buffer_index_b)
{
    return g_buffer_state.buffer_infos[*buffer_index_a].offset_alignment >=
           g_buffer_state.buffer_infos[*buffer_index_b].offset_alignment;
}

static uint32 GetBufferFrameIndex(uint32 buffer_index, uint32 frame_index)
{
    return (g_buffer_state.frame_strides[buffer_index] * frame_index) + buffer_index;
}

static uint32 GetBufferFrameIndex(BufferHnd hnd, uint32 frame_index)
{
    return GetBufferFrameIndex(hnd.index, frame_index);
}

static BufferInfo* GetBufferInfoUtil(BufferHnd hnd)
{
    return &g_buffer_state.buffer_infos[hnd.index];
}

static BufferMemory* GetBufferMemoryUtil(BufferHnd hnd)
{
    return &g_buffer_state.mems[g_buffer_state.mem_indexes[hnd.index]];
}

static void ValidateBufferHnd(BufferHnd hnd, const char* action)
{
    if (hnd.index >= g_buffer_state.buffer_count)
    {
        CTK_FATAL("%s: buffer handle index %u exceeds max buffer count of %u",
                  action, hnd.index, g_buffer_state.buffer_count);
    }
}

/// Debug
////////////////////////////////////////////////////////////
static void LogBuffers()
{
    PrintLine("buffers:");
    for (uint32 buffer_index = 0; buffer_index < g_buffer_state.buffer_count; ++buffer_index)
    {
        BufferInfo* buffer_info = g_buffer_state.buffer_infos + buffer_index;
        PrintLine("   [%2u] size:             %llu", buffer_index, buffer_info->size);
        PrintLine("        offset_alignment: %llu", buffer_info->offset_alignment);
        PrintLine("        per_frame:        %s", buffer_info->per_frame ? "true" : "false");
        PrintLine("        mem_properties:   ");
        PrintMemoryPropertyFlags(buffer_info->mem_properties, 3);
        PrintLine("        usage:            ");
        PrintBufferUsageFlags(buffer_info->usage, 3);

        PrintLine("        frame_stride:     %u", g_buffer_state.frame_strides[buffer_index]);
        PrintLine("        mem_index:        %u", g_buffer_state.mem_indexes[buffer_index]);
        PrintLine("        offsets:          ");
        uint32 frame_count = buffer_info->per_frame ? g_buffer_state.frame_count : 1;
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            PrintLine("            [%u] %llu", frame_index,
                      g_buffer_state.offsets[GetBufferFrameIndex(buffer_index, frame_index)]);
        }
        PrintLine();
    }
}

static void LogBufferMemory(BufferMemory* buffer_mem)
{
    PrintLine("   [%2u] size:           %llu", buffer_mem - g_buffer_state.mems, buffer_mem->size);
    PrintLine("        buffer:         0x%p", buffer_mem->buffer);
    PrintLine("        device_mem:     0x%p", buffer_mem->device_mem);
    PrintLine("        host_mem:       0x%p", buffer_mem->host_mem);
    PrintLine("        mem_properties:   ");
    PrintMemoryPropertyFlags(buffer_mem->mem_properties, 3);
    PrintLine("        usage:            ");
    PrintBufferUsageFlags(buffer_mem->usage, 3);
}

static void LogBufferMemory()
{
    PrintLine("buffer memory:");
    for (uint32 i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
    {
        BufferMemory* buffer_mem = &g_buffer_state.mems[i];
        if (buffer_mem->size == 0)
        {
            continue;
        }
        LogBufferMemory(buffer_mem);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitBufferModule(const Allocator* allocator, uint32 max_buffers)
{
    uint32 frame_count = GetFrameCount();
    g_buffer_state.max_buffers   = max_buffers;
    g_buffer_state.buffer_count  = 0;
    g_buffer_state.frame_count   = frame_count;

    g_buffer_state.buffer_infos  = Allocate<BufferInfo>  (allocator, max_buffers);
    g_buffer_state.frame_strides = Allocate<uint32>      (allocator, max_buffers);
    g_buffer_state.mem_indexes   = Allocate<uint32>      (allocator, max_buffers);
    g_buffer_state.offsets       = Allocate<VkDeviceSize>(allocator, max_buffers * frame_count);
    g_buffer_state.indexes       = Allocate<VkDeviceSize>(allocator, max_buffers * frame_count);
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
    g_buffer_state.frame_strides[hnd.index] = buffer_info->per_frame ? g_buffer_state.max_buffers : 0;

    return hnd;
}

struct BufferMemoryInfo
{
    VkDeviceSize size;
    VkDeviceSize offset_alignment;
    uint32       buffer_index;
};

static void AllocateBuffers(Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);

    VkDevice device = GetDevice();
    PhysicalDevice* physical_device = GetPhysicalDevice();
    QueueFamilies* queue_families = &physical_device->queue_families;
    VkMemoryType* memory_types = physical_device->mem_properties.memoryTypes;
    VkResult res = VK_SUCCESS;

    // Associate buffers with memory types they'll be using.
    Array<uint32> mem_buffer_index_arrays[VK_MAX_MEMORY_TYPES] = {};
    uint32 mem_buffer_counts[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 buffer_index = 0; buffer_index < g_buffer_state.buffer_count; ++buffer_index)
    {
        uint32 mem_index = GetMemoryIndex(&g_buffer_state.buffer_infos[buffer_index]);
        g_buffer_state.mem_indexes[buffer_index] = mem_index;
        ++mem_buffer_counts[mem_index];
    }

    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 buffer_count = mem_buffer_counts[mem_index];
        if (buffer_count > 0)
        {
            InitArray(&mem_buffer_index_arrays[mem_index], &frame.allocator, buffer_count);
        }
    }

    for (uint32 buffer_index = 0; buffer_index < g_buffer_state.buffer_count; ++buffer_index)
    {
        Push(&mem_buffer_index_arrays[g_buffer_state.mem_indexes[buffer_index]], buffer_index);
    }

    // Size buffer memory and calculate buffer offsets.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 buffer_count = mem_buffer_counts[mem_index];
        if (buffer_count == 0)
        {
            continue;
        }

        Array<uint32>* mem_buffer_index_array = &mem_buffer_index_arrays[mem_index];
        InsertionSort(mem_buffer_index_array, OffsetAlignmentDesc);

        BufferMemory* buffer_mem = &g_buffer_state.mems[mem_index];
        for (uint32 i = 0; i < buffer_count; ++i)
        {
            uint32 buffer_index = Get(mem_buffer_index_array, i);
            BufferInfo* buffer_info = &g_buffer_state.buffer_infos[buffer_index];

            // Size buffer memory and generate offsets based on buffer size and alignment memory requirements.
            uint32 frame_count = buffer_info->per_frame ? g_buffer_state.frame_count : 1;
            for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
            {
                buffer_mem->size = MultipleOf(buffer_mem->size, buffer_info->offset_alignment);
                g_buffer_state.offsets[GetBufferFrameIndex(buffer_index, frame_index)] = buffer_mem->size;
                buffer_mem->size += buffer_info->size;
            }

            // Append buffer usage for creating associated buffer memory.
            buffer_mem->usage |= buffer_info->usage;
        }
    }

    // Initialize and allocate buffer memory.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        BufferMemory* buffer_mem = &g_buffer_state.mems[mem_index];
        if (buffer_mem->size == 0)
        {
            continue;
        }

        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.pNext = NULL;
        create_info.flags = 0;
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

        res = vkCreateBuffer(device, &create_info, NULL, &buffer_mem->buffer);
        Validate(res, "vkCreateBuffer() failed");

        // Allocate buffer memory.
        VkMemoryRequirements mem_requirements = {};
        vkGetBufferMemoryRequirements(device, buffer_mem->buffer, &mem_requirements);
        buffer_mem->size = mem_requirements.size;
        buffer_mem->device_mem = AllocateDeviceMemory(mem_index, buffer_mem->size, NULL);
        buffer_mem->mem_properties = memory_types[mem_index].propertyFlags;

        // Bind buffer memory.
        res = vkBindBufferMemory(device, buffer_mem->buffer, buffer_mem->device_mem, 0);
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
                    GetBufferMemoryUtil(src_buffer_hnd)->buffer,
                    GetBufferMemoryUtil(buffer_hnd)->buffer,
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
                    GetBufferMemoryUtil(src_buffer_hnd)->buffer,
                    GetBufferMemoryUtil(buffer_hnd)->buffer,
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
