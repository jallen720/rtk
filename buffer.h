/// Data
////////////////////////////////////////////////////////////
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
