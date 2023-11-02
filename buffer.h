/// Data
////////////////////////////////////////////////////////////
static constexpr VkDeviceSize USE_MIN_OFFSET_ALIGNMENT = 0;

/// Utils
////////////////////////////////////////////////////////////
static void ValidateHnd(BufferHnd hnd, const char* action)
{
    ResourceGroup* res_group = GetResourceGroup(hnd);
    uint32 buffer_index = GetBufferIndex(hnd);
    if (buffer_index >= res_group->buffer_count)
    {
        CTK_FATAL("%s: buffer index %u exceeds buffer count of %u", action, buffer_index, res_group->buffer_count);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static BufferHnd CreateBuffer(ResourceGroupHnd res_group_hnd, BufferInfo* info)
{
    ResourceGroup* res_group = GetResourceGroup(res_group_hnd);
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

    uint32 buffer_index = res_group->buffer_count;
    BufferHnd hnd = { .index = GetResourceHndIndex(res_group_hnd, buffer_index) };
    ++res_group->buffer_count;
    *GetBufferInfo(res_group, buffer_index) = *info;

    return hnd;
}

static void WriteHostBuffer(BufferHnd hnd, uint32 frame_index, void* data, VkDeviceSize data_size)
{
    ValidateHnd(hnd, "can't write to host buffer");

    ResourceGroup* res_group = GetResourceGroup(hnd);
    CTK_ASSERT(frame_index < res_group->frame_count);

    uint32 buffer_index = GetBufferIndex(hnd);
    BufferInfo* buffer_info = GetBufferInfo(res_group, buffer_index);
    BufferFrameState* buffer_frame_state = GetBufferFrameState(res_group, buffer_index, frame_index);
    if (data_size > buffer_info->size)
    {
        CTK_FATAL("can't write %u bytes to host buffer: write would exceed size of %u", data_size, buffer_info->size);
    }

    ResourceMemory* buffer_mem = GetBufferMemory(res_group, buffer_index);
    CTK_ASSERT(buffer_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    memcpy(&buffer_mem->host[buffer_frame_state->offset], data, data_size);
    buffer_frame_state->index = data_size;
}

static void AppendHostBuffer(BufferHnd hnd, uint32 frame_index, void* data, VkDeviceSize data_size)
{
    ValidateHnd(hnd, "can't append to host buffer");

    ResourceGroup* res_group = GetResourceGroup(hnd);
    CTK_ASSERT(frame_index < res_group->frame_count);

    uint32 buffer_index = GetBufferIndex(hnd);
    BufferInfo* buffer_info = GetBufferInfo(res_group, buffer_index);
    BufferFrameState* buffer_frame_state = GetBufferFrameState(res_group, buffer_index, frame_index);
    if (buffer_frame_state->index + data_size > buffer_info->size)
    {
        CTK_FATAL("can't append %u bytes to host buffer at index %u: append would exceed size of %u", data_size,
                  buffer_frame_state->index, buffer_info->size);
    }

    ResourceMemory* buffer_mem = GetBufferMemory(res_group, buffer_index);
    CTK_ASSERT(buffer_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    memcpy(&buffer_mem->host[buffer_frame_state->offset + buffer_frame_state->index], data, data_size);
    buffer_frame_state->index += data_size;
}

static void WriteDeviceBufferCmd(BufferHnd dst_hnd, BufferHnd src_hnd, uint32 frame_index, VkDeviceSize offset,
                                 VkDeviceSize size)
{
    ValidateHnd(dst_hnd, "can't write to destination device buffer");
    ValidateHnd(src_hnd, "can't write from source buffer to destination device buffer");

    ResourceGroup* res_group = GetResourceGroup(dst_hnd);
    CTK_ASSERT(frame_index < res_group->frame_count);

    uint32 dst_index = GetBufferIndex(dst_hnd);
    uint32 src_index = GetBufferIndex(src_hnd);
    BufferInfo* dst_info = GetBufferInfo(res_group, dst_index);
    if (size > dst_info->size)
    {
        CTK_FATAL("can't write %u bytes to device buffer: write would exceed size of %u", size,
                  dst_info->size);
    }

    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, dst_index, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = GetBufferFrameState(res_group, src_index, frame_index)->offset + offset,
        .dstOffset = dst_frame_state->offset,
        .size      = size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBuffer(res_group, src_index),
                    GetBuffer(res_group, dst_index),
                    1, &copy);
    dst_frame_state->index = size;
}

static void AppendDeviceBufferCmd(BufferHnd dst_hnd, BufferHnd src_hnd, uint32 frame_index, VkDeviceSize offset,
                                  VkDeviceSize size)
{
    ValidateHnd(dst_hnd, "can't append to destination device buffer");
    ValidateHnd(src_hnd, "can't append from source buffer to destination device buffer");

    ResourceGroup* res_group = GetResourceGroup(dst_hnd);
    CTK_ASSERT(frame_index < res_group->frame_count);

    uint32 dst_index = GetBufferIndex(dst_hnd);
    uint32 src_index = GetBufferIndex(src_hnd);
    BufferInfo* dst_info = GetBufferInfo(res_group, dst_index);
    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, dst_index, frame_index);
    if (dst_frame_state->index + size > dst_info->size)
    {
        CTK_FATAL("can't append %u bytes to device buffer at index %u: append would exceed size of %u", size,
                  dst_frame_state->index, dst_info->size);
    }

    VkBufferCopy copy =
    {
        .srcOffset = GetBufferFrameState(res_group, src_index, frame_index)->offset + offset,
        .dstOffset = dst_frame_state->offset + dst_frame_state->index,
        .size      = size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBuffer(res_group, src_index),
                    GetBuffer(res_group, dst_index),
                    1, &copy);
    dst_frame_state->index += size;
}

static void Clear(BufferHnd hnd)
{
    ValidateHnd(hnd, "can't clear buffer");
    ResourceGroup* res_group = GetResourceGroup(hnd);
    uint32 buffer_index = GetBufferIndex(hnd);
    for (uint32 frame_index = 0; frame_index < GetBufferState(res_group, buffer_index)->frame_count; ++frame_index)
    {
        GetBufferFrameState(res_group, buffer_index, frame_index)->index = 0;
    }
}

static BufferInfo* GetInfo(BufferHnd hnd)
{
    ValidateHnd(hnd, "can't get buffer");
    return GetBufferInfo(GetResourceGroup(hnd), GetBufferIndex(hnd));
}

static VkDeviceSize GetSize(BufferHnd hnd)
{
    ValidateHnd(hnd, "can't get buffer size");
    return GetBufferInfo(GetResourceGroup(hnd), GetBufferIndex(hnd))->size;
}

static VkDeviceSize GetOffset(BufferHnd hnd, uint32 frame_index)
{
    ValidateHnd(hnd, "can't get buffer offset");

    ResourceGroup* res_group = GetResourceGroup(hnd);
    CTK_ASSERT(frame_index < res_group->frame_count);

    return GetBufferFrameState(res_group, GetBufferIndex(hnd), frame_index)->offset;
}

static VkDeviceSize GetIndex(BufferHnd hnd, uint32 frame_index)
{
    ValidateHnd(hnd, "can't get buffer index");

    ResourceGroup* res_group = GetResourceGroup(hnd);
    CTK_ASSERT(frame_index < res_group->frame_count);

    return GetBufferFrameState(res_group, GetBufferIndex(hnd), frame_index)->index;
}

static void SetIndex(BufferHnd hnd, uint32 frame_index, VkDeviceSize index)
{
    ValidateHnd(hnd, "can't set buffer index");

    ResourceGroup* res_group = GetResourceGroup(hnd);
    CTK_ASSERT(frame_index < res_group->frame_count);

    GetBufferFrameState(res_group, GetBufferIndex(hnd), frame_index)->index = index;
}

template<typename Type>
static Type* GetHostMemory(BufferHnd hnd, uint32 frame_index)
{
    ValidateHnd(hnd, "can't get buffer mapped memory");

    ResourceGroup* res_group = GetResourceGroup(hnd);
    CTK_ASSERT(frame_index < res_group->frame_count);

    uint32 buffer_index = GetBufferIndex(hnd);
    ResourceMemory* mem = GetBufferMemory(res_group, buffer_index);
    CTK_ASSERT(mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    return (Type*)&mem->host[GetBufferFrameState(res_group, buffer_index, frame_index)->offset];
}

static VkBuffer GetBuffer(BufferHnd hnd)
{
    ValidateHnd(hnd, "can't get buffer memory handle");
    return GetBuffer(GetResourceGroup(hnd), GetBufferIndex(hnd));
}
