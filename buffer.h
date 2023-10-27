/// Data
////////////////////////////////////////////////////////////
struct BufferHnd { uint32 index; };

static constexpr VkDeviceSize USE_MIN_OFFSET_ALIGNMENT = 0;

/// Utils
////////////////////////////////////////////////////////////
static void ValidateBufferHnd(BufferHnd hnd, const char* action)
{
    uint32 buffer_count = GetResourceGroup()->buffer_count;
    if (hnd.index >= buffer_count)
    {
        CTK_FATAL("%s: buffer handle index %u exceeds max buffer count of %u", action, hnd.index, buffer_count);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static BufferHnd CreateBuffer(BufferInfo* info)
{
    ResourceGroup* res_group = GetResourceGroup();
    if (res_group->buffer_count >= res_group->max_buffers)
    {
        CTK_FATAL("can't create buffer: already at max of %u", res_group->max_buffers);
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

    BufferHnd hnd = { .index = res_group->buffer_count };
    ++res_group->buffer_count;
    *GetBufferInfo(hnd.index) = *info;
    ResourceState* state = GetBufferState(hnd.index);
    if (info->per_frame)
    {
        state->frame_stride = res_group->max_buffers;
        state->frame_count  = res_group->frame_count;
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
    ResourceGroup* res_group = GetResourceGroup();
    VkResult res = VK_SUCCESS;

    // Associate buffers with memory types.
    Array<uint32> mem_buffer_index_arrays[VK_MAX_MEMORY_TYPES] = {};
    uint32 mem_buffer_counts[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 buffer_index = 0; buffer_index < res_group->buffer_count; ++buffer_index)
    {
        GetMemoryRequirements(buffer_index);
        ++mem_buffer_counts[GetBufferState(buffer_index)->mem_index];
    }
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 buffer_count = mem_buffer_counts[mem_index];
        if (buffer_count == 0) { continue; }
        InitArray(&mem_buffer_index_arrays[mem_index], &frame.allocator, buffer_count);
    }
    for (uint32 buffer_index = 0; buffer_index < res_group->buffer_count; ++buffer_index)
    {
        Push(&mem_buffer_index_arrays[GetBufferState(buffer_index)->mem_index], buffer_index);
    }

    // Size buffer memory and calculate buffer offsets.
    VkBufferUsageFlags buffer_usage[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 mem_buffer_count = mem_buffer_counts[mem_index];
        if (mem_buffer_count == 0)
        {
            continue;
        }

        Array<uint32>* mem_buffer_index_array = &mem_buffer_index_arrays[mem_index];
        InsertionSort(mem_buffer_index_array, OffsetAlignmentDesc);

        ResourceMemory* buffer_mem = GetMemory(mem_index);
        for (uint32 i = 0; i < mem_buffer_count; ++i)
        {
            uint32 mem_buffer_index = Get(mem_buffer_index_array, i);
            BufferInfo* info = GetBufferInfo(mem_buffer_index);

            // Size buffer memory and get offsets based on buffer size and alignment memory requirements.
            for (uint32 frame_index = 0; frame_index < GetBufferState(mem_buffer_index)->frame_count; ++frame_index)
            {
                buffer_mem->size = Align(buffer_mem->size, info->offset_alignment);
                GetBufferFrameState(mem_buffer_index, frame_index)->offset = buffer_mem->size;
                buffer_mem->size += info->size;
            }

            // Append buffer usage for creating associated buffer memory.
            buffer_usage[mem_index] |= info->usage;
        }
    }

    // Initialize and allocate buffer memory.
    QueueFamilies* queue_families = &physical_device->queue_families;
    VkMemoryType* memory_types = physical_device->mem_properties.memoryTypes;
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        ResourceMemory* buffer_mem = GetMemory(mem_index);
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
        create_info.usage = buffer_usage[mem_index];
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
        VkBuffer* buffer_ptr = GetBufferPtr(mem_index);
        res = vkCreateBuffer(device, &create_info, NULL, buffer_ptr);
        Validate(res, "vkCreateBuffer() failed");

        // Allocate buffer memory.
        VkMemoryRequirements mem_requirements = {};
        vkGetBufferMemoryRequirements(device, *buffer_ptr, &mem_requirements);
        buffer_mem->size = mem_requirements.size;
        buffer_mem->device = AllocateDeviceMemory(mem_index, buffer_mem->size, NULL);
        buffer_mem->properties = memory_types[mem_index].propertyFlags;

        // Bind buffer memory.
        res = vkBindBufferMemory(device, *buffer_ptr, buffer_mem->device, 0);
        Validate(res, "vkBindBufferMemory() failed");

        // Map host visible buffer memory.
        if (buffer_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            vkMapMemory(device, buffer_mem->device, 0, buffer_mem->size, 0, (void**)&buffer_mem->host);
        }
    }
}

static void WriteHostBuffer(BufferHnd hnd, uint32 frame_index, void* data, VkDeviceSize data_size)
{
    ValidateBufferHnd(hnd, "can't write to host buffer");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count);

    BufferInfo* buffer_info = GetBufferInfo(hnd.index);
    if (data_size > buffer_info->size)
    {
        CTK_FATAL("can't write %u bytes to host buffer: write would exceed size of %u", data_size, buffer_info->size);
    }

    ResourceMemory* buffer_mem = GetBufferMemory(hnd.index);
    CTK_ASSERT(buffer_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    BufferFrameState* frame_state = GetBufferFrameState(hnd.index, frame_index);
    memcpy(&buffer_mem->host[frame_state->offset], data, data_size);
    frame_state->index = data_size;
}

static void AppendHostBuffer(BufferHnd hnd, uint32 frame_index, void* data, VkDeviceSize data_size)
{
    ValidateBufferHnd(hnd, "can't append to host buffer");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count);

    BufferInfo* info = GetBufferInfo(hnd.index);
    BufferFrameState* frame_state = GetBufferFrameState(hnd.index, frame_index);
    if (frame_state->index + data_size > info->size)
    {
        CTK_FATAL("can't append %u bytes to host buffer at index %u: append would exceed size of %u", data_size,
                  frame_state->index, info->size);
    }

    ResourceMemory* buffer_mem = GetBufferMemory(hnd.index);
    CTK_ASSERT(buffer_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    memcpy(&buffer_mem->host[frame_state->offset + frame_state->index], data, data_size);
    frame_state->index += data_size;
}

static void WriteDeviceBufferCmd(BufferHnd dst_buffer_hnd, BufferHnd src_buffer_hnd, uint32 frame_index,
                                 VkDeviceSize offset, VkDeviceSize size)
{
    ValidateBufferHnd(dst_buffer_hnd, "can't write to destination device buffer");
    ValidateBufferHnd(src_buffer_hnd, "can't write from source buffer to destination device buffer");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count);

    BufferInfo* dst_buffer_info = GetBufferInfo(dst_buffer_hnd.index);
    if (size > dst_buffer_info->size)
    {
        CTK_FATAL("can't write %u bytes to device buffer: write would exceed size of %u", size,
                  dst_buffer_info->size);
    }

    BufferFrameState* dst_buffer_frame_state = GetBufferFrameState(dst_buffer_hnd.index, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = GetBufferFrameState(src_buffer_hnd.index, frame_index)->offset + offset,
        .dstOffset = dst_buffer_frame_state->offset,
        .size      = size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBuffer(src_buffer_hnd.index),
                    GetBuffer(dst_buffer_hnd.index),
                    1, &copy);
    dst_buffer_frame_state->index = size;
}

static void AppendDeviceBufferCmd(BufferHnd dst_buffer_hnd, BufferHnd src_buffer_hnd, uint32 frame_index,
                                  VkDeviceSize offset, VkDeviceSize size)
{
    ValidateBufferHnd(dst_buffer_hnd, "can't append to destination device buffer");
    ValidateBufferHnd(src_buffer_hnd, "can't append from source buffer to destination device buffer");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count);

    BufferInfo* dst_buffer_info = GetBufferInfo(dst_buffer_hnd.index);
    BufferFrameState* dst_buffer_frame_state = GetBufferFrameState(dst_buffer_hnd.index, frame_index);
    if (dst_buffer_frame_state->index + size > dst_buffer_info->size)
    {
        CTK_FATAL("can't append %u bytes to device buffer at index %u: append would exceed size of %u", size,
                  dst_buffer_frame_state->index, dst_buffer_info->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = GetBufferFrameState(src_buffer_hnd.index, frame_index)->offset + offset,
        .dstOffset = dst_buffer_frame_state->offset + dst_buffer_frame_state->index,
        .size      = size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBuffer(src_buffer_hnd.index),
                    GetBuffer(dst_buffer_hnd.index),
                    1, &copy);
    dst_buffer_frame_state->index += size;
}

static void Clear(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't clear buffer");
    for (uint32 frame_index = 0; frame_index < GetResourceGroup()->frame_count; ++frame_index)
    {
        GetBufferFrameState(hnd.index, frame_index)->index = 0;
    }
}

static BufferInfo* GetInfo(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't get buffer");
    return GetBufferInfo(hnd.index);
}

static VkBuffer GetBuffer(BufferHnd hnd)
{
    ValidateBufferHnd(hnd, "can't get buffer memory handle");
    return GetBuffer(hnd.index);
}

static VkDeviceSize GetOffset(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer offset");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count);
    return GetBufferFrameState(hnd.index, frame_index)->offset;
}

static VkDeviceSize GetIndex(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer index");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count);
    return GetBufferFrameState(hnd.index, frame_index)->index;
}

static void SetIndex(BufferHnd hnd, uint32 frame_index, VkDeviceSize index)
{
    ValidateBufferHnd(hnd, "can't set buffer index");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count);
    GetBufferFrameState(hnd.index, frame_index)->index = index;
}

template<typename Type>
static Type* GetHostMemory(BufferHnd hnd, uint32 frame_index)
{
    ValidateBufferHnd(hnd, "can't get buffer mapped memory");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count);

    ResourceMemory* mem = GetBufferMemory(hnd.index);
    CTK_ASSERT(mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    return (Type*)&mem->host[GetBufferFrameState(hnd.index, frame_index)->offset];
}
