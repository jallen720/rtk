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

struct BufferState
{
    uint32 frame_stride;
    uint32 frame_count;
    uint32 mem_index;
};

struct BufferFrameState
{
    VkDeviceSize offset;
    VkDeviceSize index;
};

struct BufferGroup
{
    uint32            max_buffers;
    uint32            buffer_count;
    uint32            frame_count;

    BufferInfo*       infos;        // size: max_buffers
    BufferState*      states;       // size: max_buffers
    BufferFrameState* frame_states; // size: max_buffers * frame_count
    VkBuffer          buffers[VK_MAX_MEMORY_TYPES];

    ResourceMemory    mems[VK_MAX_MEMORY_TYPES];
};

static BufferGroup g_buf_group;

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
    return g_buf_group.infos[*buffer_index_a].offset_alignment >=
           g_buf_group.infos[*buffer_index_b].offset_alignment;
}

static BufferInfo* GetInfoUtil(uint32 index)
{
    return &g_buf_group.infos[index];
}

static BufferInfo* GetInfoUtil(BufferHnd hnd)
{
    return GetInfoUtil(hnd.index);
}

static BufferState* GetStateUtil(uint32 index)
{
    return &g_buf_group.states[index];
}

static BufferState* GetStateUtil(BufferHnd hnd)
{
    return GetStateUtil(hnd.index);
}

static uint32 GetBufferFrameIndex(uint32 buffer_index, uint32 frame_index)
{
    return (GetStateUtil(buffer_index)->frame_stride * frame_index) + buffer_index;
}

static uint32 GetBufferFrameIndex(BufferHnd hnd, uint32 frame_index)
{
    return GetBufferFrameIndex(hnd.index, frame_index);
}

static BufferFrameState* GetFrameStateUtil(uint32 buffer_index, uint32 frame_index)
{
    return &g_buf_group.frame_states[GetBufferFrameIndex(buffer_index, frame_index)];
}

static BufferFrameState* GetFrameStateUtil(BufferHnd hnd, uint32 frame_index)
{
    return GetFrameStateUtil(hnd.index, frame_index);
}

static VkBuffer GetBufferUtil(BufferHnd hnd)
{
    return g_buf_group.buffers[GetStateUtil(hnd)->mem_index];
}

static ResourceMemory* GetMemoryUtil(BufferHnd hnd)
{
    return &g_buf_group.mems[GetStateUtil(hnd)->mem_index];
}

static void ValidateBufferHnd(BufferHnd hnd, const char* action)
{
    if (hnd.index >= g_buf_group.buffer_count)
    {
        CTK_FATAL("%s: buffer handle index %u exceeds max buffer count of %u",
                  action, hnd.index, g_buf_group.buffer_count);
    }
}

/// Debug
////////////////////////////////////////////////////////////
static void LogBuffers()
{
    PrintLine("buffers:");
    for (uint32 buffer_index = 0; buffer_index < g_buf_group.buffer_count; ++buffer_index)
    {
        BufferInfo* info = GetInfoUtil(buffer_index);
        BufferState* state = GetStateUtil(buffer_index);
        PrintLine("   [%2u] size:             %llu", buffer_index, info->size);
        PrintLine("        offset_alignment: %llu", info->offset_alignment);
        PrintLine("        per_frame:        %s", info->per_frame ? "true" : "false");
        PrintLine("        mem_properties:   ");
        PrintMemoryPropertyFlags(info->mem_properties, 3);
        PrintLine("        usage:            ");
        PrintBufferUsageFlags(info->usage, 3);

        PrintLine("        frame_stride:     %u", state->frame_stride);
        PrintLine("        frame_count:      %u", state->frame_count);
        PrintLine("        mem_index:        %u", state->mem_index);
        PrintLine("        offsets:          ");
        for (uint32 frame_index = 0; frame_index < state->frame_count; ++frame_index)
        {
            PrintLine("            [%u] %llu", frame_index, GetFrameStateUtil(buffer_index, frame_index)->offset);
        }
        PrintLine();
    }
}

static void LogBufferMemory(uint32 mem_index)
{
    ResourceMemory* buffer_mem = &g_buf_group.mems[mem_index];
    if (buffer_mem->size == 0)
    {
        return;
    }
    PrintLine("   [%2u] size:           %llu", mem_index, buffer_mem->size);
    PrintLine("        device_mem:     0x%p", buffer_mem->device_mem);
    PrintLine("        host_mem:       0x%p", buffer_mem->host_mem);
    PrintLine("        mem_properties:   ");
    PrintMemoryPropertyFlags(buffer_mem->mem_properties, 3);
    PrintLine("        hnd   :         0x%p", g_buf_group.buffers[mem_index]);
}

static void LogBufferMemory()
{
    PrintLine("buffer memory:");
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        LogBufferMemory(mem_index);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitBufferModule(const Allocator* allocator, uint32 max_buffers)
{
    uint32 frame_count = GetFrameCount();
    g_buf_group.max_buffers  = max_buffers;
    g_buf_group.buffer_count = 0;
    g_buf_group.frame_count  = frame_count;
    g_buf_group.infos        = Allocate<BufferInfo>      (allocator, max_buffers);
    g_buf_group.states       = Allocate<BufferState>     (allocator, max_buffers);
    g_buf_group.frame_states = Allocate<BufferFrameState>(allocator, max_buffers * frame_count);
}

static BufferHnd CreateBuffer(BufferInfo* info)
{
    if (g_buf_group.buffer_count >= g_buf_group.max_buffers)
    {
        CTK_FATAL("can't create buffer: already at max of %u", g_buf_group.max_buffers);
    }

    // Figure out minimum offset alignment if requested.
    // Spec: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VkMemoryRequirements
    if (info->offset_alignment == USE_MIN_OFFSET_ALIGNMENT)
    {
        VkPhysicalDeviceLimits* physical_device_limits = &GetPhysicalDevice()->properties.limits;
        info->offset_alignment = 4;

        // Uniform
        if ((info->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) &&
            info->offset_alignment < physical_device_limits->minUniformBufferOffsetAlignment)
        {
            info->offset_alignment = physical_device_limits->minUniformBufferOffsetAlignment;
        }

        // Storage
        if ((info->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) &&
            info->offset_alignment < physical_device_limits->minStorageBufferOffsetAlignment)
        {
            info->offset_alignment = physical_device_limits->minStorageBufferOffsetAlignment;
        }

        // Texel
        if ((info->usage & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)) &&
            info->offset_alignment < physical_device_limits->minTexelBufferOffsetAlignment)
        {
            info->offset_alignment = physical_device_limits->minTexelBufferOffsetAlignment;
        }
    }

    BufferHnd hnd = { .index = g_buf_group.buffer_count };
    ++g_buf_group.buffer_count;
    g_buf_group.infos[hnd.index] = *info;
    BufferState* state = GetStateUtil(hnd);
    if (info->per_frame)
    {
        state->frame_stride = g_buf_group.max_buffers;
        state->frame_count  = g_buf_group.frame_count;
    }
    else
    {
        state->frame_stride = 0;
        state->frame_count  = 1;
    }

    return hnd;
}

static void AllocateBuffers(Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);

    VkDevice device = GetDevice();
    PhysicalDevice* physical_device = GetPhysicalDevice();
    VkResult res = VK_SUCCESS;

    // Associate buffers with memory types they'll be using.
    Array<uint32> mem_buffer_index_arrays[VK_MAX_MEMORY_TYPES] = {};
    uint32 mem_buffer_counts[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 buffer_index = 0; buffer_index < g_buf_group.buffer_count; ++buffer_index)
    {
        uint32 mem_index = GetMemoryIndex(GetInfoUtil(buffer_index));
        GetStateUtil(buffer_index)->mem_index = mem_index;
        ++mem_buffer_counts[mem_index];
    }

    // Associate buffers with memory types.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 buffer_count = mem_buffer_counts[mem_index];
        if (buffer_count == 0) { continue; }
        InitArray(&mem_buffer_index_arrays[mem_index], &frame.allocator, buffer_count);
    }
    for (uint32 buffer_index = 0; buffer_index < g_buf_group.buffer_count; ++buffer_index)
    {
        Push(&mem_buffer_index_arrays[GetStateUtil(buffer_index)->mem_index], buffer_index);
    }

    // Size buffer memory and calculate buffer offsets.
    VkBufferUsageFlags usage[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 mem_buffer_count = mem_buffer_counts[mem_index];
        if (mem_buffer_count == 0)
        {
            continue;
        }

        Array<uint32>* mem_buffer_index_array = &mem_buffer_index_arrays[mem_index];
        InsertionSort(mem_buffer_index_array, OffsetAlignmentDesc);

        ResourceMemory* buffer_mem = &g_buf_group.mems[mem_index];
        for (uint32 i = 0; i < mem_buffer_count; ++i)
        {
            uint32 mem_buffer_index = Get(mem_buffer_index_array, i);
            BufferInfo* info = GetInfoUtil(mem_buffer_index);

            // Size buffer memory and get offsets based on buffer size and alignment memory requirements.
            for (uint32 frame_index = 0; frame_index < GetStateUtil(mem_buffer_index)->frame_count; ++frame_index)
            {
                buffer_mem->size = Align(buffer_mem->size, info->offset_alignment);
                GetFrameStateUtil(mem_buffer_index, frame_index)->offset = buffer_mem->size;
                buffer_mem->size += info->size;
            }

            // Append buffer usage for creating associated buffer memory.
            usage[mem_index] |= info->usage;
        }
    }

    // Initialize and allocate buffer memory.
    QueueFamilies* queue_families = &physical_device->queue_families;
    VkMemoryType* memory_types = physical_device->mem_properties.memoryTypes;
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        ResourceMemory* buffer_mem = &g_buf_group.mems[mem_index];
        if (buffer_mem->size == 0)
        {
            continue;
        }

        // Create buffer.
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.pNext = NULL;
        create_info.flags = 0;
        create_info.size  = buffer_mem->size;
        create_info.usage = usage[mem_index];
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
        res = vkCreateBuffer(device, &create_info, NULL, &g_buf_group.buffers[mem_index]);
        Validate(res, "vkCreateBuffer() failed");

        // Allocate buffer memory.
        VkMemoryRequirements mem_requirements = {};
        vkGetBufferMemoryRequirements(device, g_buf_group.buffers[mem_index], &mem_requirements);
        buffer_mem->size = mem_requirements.size;
        buffer_mem->device_mem = AllocateDeviceMemory(mem_index, buffer_mem->size, NULL);
        buffer_mem->mem_properties = memory_types[mem_index].propertyFlags;

        // Bind buffer memory.
        res = vkBindBufferMemory(device, g_buf_group.buffers[mem_index], buffer_mem->device_mem, 0);
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
    CTK_ASSERT(frame_index < g_buf_group.frame_count);

    BufferInfo* buffer_info = GetInfoUtil(hnd);
    if (data_size > buffer_info->size)
    {
        CTK_FATAL("can't write %u bytes to host buffer: write would exceed size of %u", data_size, buffer_info->size);
    }

    ResourceMemory* buffer_mem = GetMemoryUtil(hnd);
    CTK_ASSERT(buffer_mem->mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    BufferFrameState* frame_state = GetFrameStateUtil(hnd, frame_index);
    memcpy(&buffer_mem->host_mem[frame_state->offset], data, data_size);
    frame_state->index = data_size;
}

static void AppendHostBuffer(BufferHnd hnd, uint32 frame_index, void* data, VkDeviceSize data_size)
{
    ValidateBufferHnd(hnd, "can't append to host buffer");
    CTK_ASSERT(frame_index < g_buf_group.frame_count);

    BufferInfo* info = GetInfoUtil(hnd);
    BufferFrameState* frame_state = GetFrameStateUtil(hnd, frame_index);
    if (frame_state->index + data_size > info->size)
    {
        CTK_FATAL("can't append %u bytes to host buffer at index %u: append would exceed size of %u", data_size,
                  frame_state->index, info->size);
    }

    ResourceMemory* buffer_mem = GetMemoryUtil(hnd);
    CTK_ASSERT(buffer_mem->mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    memcpy(&buffer_mem->host_mem[frame_state->offset + frame_state->index], data, data_size);
    frame_state->index += data_size;
}

static void WriteDeviceBufferCmd(BufferHnd dst_buffer_hnd, uint32 frame_index,
                                 BufferHnd src_buffer_hnd, uint32 src_frame_index,
                                 VkDeviceSize offset, VkDeviceSize size)
{
    ValidateBufferHnd(dst_buffer_hnd, "can't write to device buffer");
    ValidateBufferHnd(src_buffer_hnd, "can't write to device buffer");
    CTK_ASSERT(frame_index < g_buf_group.frame_count);

    BufferInfo* dst_buffer_info = GetInfoUtil(dst_buffer_hnd);
    if (size > dst_buffer_info->size)
    {
        CTK_FATAL("can't write %u bytes to device buffer: write would exceed size of %u", size,
                  dst_buffer_info->size);
    }

    BufferFrameState* dst_buffer_frame_state = GetFrameStateUtil(dst_buffer_hnd, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = GetFrameStateUtil(src_buffer_hnd, frame_index)->offset + offset,
        .dstOffset = dst_buffer_frame_state->offset,
        .size      = size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBufferUtil(src_buffer_hnd),
                    GetBufferUtil(dst_buffer_hnd),
                    1, &copy);
    dst_buffer_frame_state->index = size;
}

static void AppendDeviceBufferCmd(BufferHnd dst_buffer_hnd, uint32 frame_index,
                                  BufferHnd src_buffer_hnd, uint32 src_frame_index,
                                  VkDeviceSize offset, VkDeviceSize size)
{
    ValidateBufferHnd(dst_buffer_hnd, "can't append to device buffer");
    ValidateBufferHnd(src_buffer_hnd, "can't append to device buffer");
    CTK_ASSERT(frame_index < g_buf_group.frame_count);

    BufferInfo* dst_buffer_info = GetInfoUtil(dst_buffer_hnd);
    BufferFrameState* dst_buffer_frame_state = GetFrameStateUtil(dst_buffer_hnd, frame_index);
    if (dst_buffer_frame_state->index + size > dst_buffer_info->size)
    {
        CTK_FATAL("can't append %u bytes to device buffer at index %u: append would exceed size of %u", size,
                  dst_buffer_frame_state->index, dst_buffer_info->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = GetFrameStateUtil(src_buffer_hnd, frame_index)->offset + offset,
        .dstOffset = dst_buffer_frame_state->offset + dst_buffer_frame_state->index,
        .size      = size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBufferUtil(src_buffer_hnd),
                    GetBufferUtil(dst_buffer_hnd),
                    1, &copy);
    dst_buffer_frame_state->index += size;
}

static void Clear(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't clear buffer");
    for (uint32 frame_index = 0; frame_index < g_buf_group.frame_count; ++frame_index)
    {
        GetFrameStateUtil(hnd, frame_index)->index = 0;
    }
}

static BufferInfo* GetInfo(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't get buffer");
    return GetInfoUtil(hnd);
}

static VkBuffer GetBuffer(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't get buffer memory handle");
    return GetBufferUtil(hnd);
}

static VkDeviceSize GetOffset(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer offset");
    CTK_ASSERT(frame_index < g_buf_group.frame_count);
    return GetFrameStateUtil(hnd, frame_index)->offset;
}

static VkDeviceSize GetIndex(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer index");
    CTK_ASSERT(frame_index < g_buf_group.frame_count);
    return GetFrameStateUtil(hnd, frame_index)->index;
}

static void SetIndex(BufferHnd hnd, uint32 frame_index, VkDeviceSize index)
{
    ValidateBufferHnd(hnd, "can't set buffer index");
    CTK_ASSERT(frame_index < g_buf_group.frame_count);
    GetFrameStateUtil(hnd, frame_index)->index = index;
}

template<typename Type>
static Type* GetHostMemory(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer mapped memory");
    CTK_ASSERT(frame_index < g_buf_group.frame_count);

    ResourceMemory* mem = GetMemoryUtil(hnd);
    CTK_ASSERT(mem->mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    return (Type*)&mem->host_mem[GetFrameStateUtil(hnd, frame_index)->offset];
}
