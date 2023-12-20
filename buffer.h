/// Data
////////////////////////////////////////////////////////////
struct HostBufferWrite
{
    VkDeviceSize size;
    void*        src_data;
    VkDeviceSize src_offset;
    BufferHnd    dst_hnd;
    VkDeviceSize dst_offset;
};

struct HostBufferAppend
{
    VkDeviceSize size;
    void*        src_data;
    VkDeviceSize src_offset;
    BufferHnd    dst_hnd;
};

struct DeviceBufferWrite
{
    VkDeviceSize size;
    BufferHnd    src_hnd;
    VkDeviceSize src_offset;
    BufferHnd    dst_hnd;
    VkDeviceSize dst_offset;
};

struct DeviceBufferAppend
{
    VkDeviceSize size;
    BufferHnd    src_hnd;
    VkDeviceSize src_offset;
    BufferHnd    dst_hnd;
};

/// Interface
////////////////////////////////////////////////////////////
static void ValidateBuffer(BufferHnd hnd, const char* action)
{
    uint32 res_group_index = GetResourceGroupIndex(hnd.index);
    ValidateResourceGroup(res_group_index, action);

    ResourceGroup* res_group = GetResourceGroup(res_group_index);
    uint32 buffer_index = GetResourceIndex(hnd.index);
    if (buffer_index >= res_group->buffer_count)
    {
        CTK_FATAL("%s: buffer index %u exceeds buffer count of %u", action, buffer_index, res_group->buffer_count);
    }
}

static void WriteHostBuffer(HostBufferWrite* write, uint32 frame_index)
{
    ValidateBuffer(write->dst_hnd, "can't write to destination host buffer");

    ResourceGroup* res_group = GetResourceGroup(GetResourceGroupIndex(write->dst_hnd.index));
    CTK_ASSERT(frame_index < res_group->frame_count);

    uint32 dst_index = GetResourceIndex(write->dst_hnd.index);
    BufferInfo* dst_info = GetBufferInfo(res_group, dst_index);
    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, dst_index, frame_index);
    if (write->dst_offset + write->size > dst_info->size)
    {
        CTK_FATAL("can't write %u bytes to host buffer at offset %u: write would exceed size of %u",
                  write->size, write->dst_offset, dst_info->size);
    }

    DeviceMemory* dev_mem = GetBufferDeviceMemory(res_group, dst_index);
    CTK_ASSERT(dev_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    uint8* dst = &dev_mem->mapped[dst_frame_state->buffer_mem_offset + write->dst_offset];
    uint8* src = &((uint8*)write->src_data)[write->src_offset];
    memcpy(dst, src, write->size);
}

static void AppendHostBuffer(HostBufferAppend* append, uint32 frame_index)
{
    ValidateBuffer(append->dst_hnd, "can't append to destination host buffer");

    ResourceGroup* res_group = GetResourceGroup(GetResourceGroupIndex(append->dst_hnd.index));
    uint32 dst_index = GetResourceIndex(append->dst_hnd.index);
    CTK_ASSERT(frame_index < res_group->frame_count);

    BufferInfo* dst_info = GetBufferInfo(res_group, dst_index);
    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, dst_index, frame_index);
    if (dst_frame_state->index + append->size > dst_info->size)
    {
        CTK_FATAL("can't append %u bytes to host buffer at index %u: append would exceed size of %u",
                  append->size, dst_frame_state->index, dst_info->size);
    }

    DeviceMemory* dev_mem = GetBufferDeviceMemory(res_group, dst_index);
    CTK_ASSERT(dev_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    uint8* dst = &dev_mem->mapped[dst_frame_state->buffer_mem_offset + dst_frame_state->index];
    uint8* src = &((uint8*)append->src_data)[append->src_offset];
    memcpy(dst, src, append->size);
    dst_frame_state->index += append->size;
}

static void WriteDeviceBufferCmd(DeviceBufferWrite* write, uint32 frame_index)
{
    ValidateBuffer(write->src_hnd, "can't write from source buffer to destination device buffer");
    ValidateBuffer(write->dst_hnd, "can't write to destination device buffer");

    ResourceGroup* res_group = GetResourceGroup(GetResourceGroupIndex(write->dst_hnd.index));
    CTK_ASSERT(frame_index < res_group->frame_count);

    uint32 dst_index = GetResourceIndex(write->dst_hnd.index);
    uint32 src_index = GetResourceIndex(write->src_hnd.index);
    BufferInfo* dst_info = GetBufferInfo(res_group, dst_index);
    if (write->dst_offset + write->size > dst_info->size)
    {
        CTK_FATAL("can't write %u bytes to device buffer at offset %u: write would exceed size of %u",
                  write->size, write->dst_offset, dst_info->size);
    }

    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, dst_index, frame_index);
    BufferFrameState* src_frame_state = GetBufferFrameState(res_group, src_index, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = src_frame_state->buffer_mem_offset + write->src_offset,
        .dstOffset = dst_frame_state->buffer_mem_offset + write->dst_offset,
        .size      = write->size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBuffer(res_group, src_index),
                    GetBuffer(res_group, dst_index),
                    1, &copy);
}

static void AppendDeviceBufferCmd(DeviceBufferAppend* append, uint32 frame_index)
{
    ValidateBuffer(append->src_hnd, "can't append from source buffer to destination device buffer");
    ValidateBuffer(append->dst_hnd, "can't append to destination device buffer");

    ResourceGroup* res_group = GetResourceGroup(GetResourceGroupIndex(append->dst_hnd.index));
    uint32 dst_index = GetResourceIndex(append->dst_hnd.index);
    uint32 src_index = GetResourceIndex(append->src_hnd.index);
    CTK_ASSERT(frame_index < res_group->frame_count);

    BufferInfo* dst_info = GetBufferInfo(res_group, dst_index);
    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, dst_index, frame_index);
    if (dst_frame_state->index + append->size > dst_info->size)
    {
        CTK_FATAL("can't append %u bytes to device buffer at index %u: append would exceed size of %u",
                  append->size, dst_frame_state->index, dst_info->size);
    }

    BufferFrameState* src_frame_state = GetBufferFrameState(res_group, src_index, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = src_frame_state->buffer_mem_offset + append->src_offset,
        .dstOffset = dst_frame_state->buffer_mem_offset + dst_frame_state->index,
        .size      = append->size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBuffer(res_group, src_index),
                    GetBuffer(res_group, dst_index),
                    1, &copy);
    dst_frame_state->index += append->size;
}

static void Clear(BufferHnd hnd)
{
    ValidateBuffer(hnd, "can't clear buffer");
    ResourceGroup* res_group = GetResourceGroup(GetResourceGroupIndex(hnd.index));
    uint32 buffer_index = GetResourceIndex(hnd.index);
    for (uint32 frame_index = 0; frame_index < GetBufferState(res_group, buffer_index)->frame_count; ++frame_index)
    {
        GetBufferFrameState(res_group, buffer_index, frame_index)->index = 0;
    }
}

static BufferInfo* GetBufferInfo(BufferHnd hnd)
{
    ValidateBuffer(hnd, "can't get buffer info");
    return GetBufferInfo(GetResourceGroup(GetResourceGroupIndex(hnd.index)), GetResourceIndex(hnd.index));
}

static BufferState* GetBufferState(BufferHnd hnd)
{
    ValidateBuffer(hnd, "can't get buffer state");
    return GetBufferState(GetResourceGroup(GetResourceGroupIndex(hnd.index)), GetResourceIndex(hnd.index));
}

static BufferFrameState* GetBufferFrameState(BufferHnd hnd, uint32 frame_index)
{
    ValidateBuffer(hnd, "can't get buffer frame state");

    ResourceGroup* res_group = GetResourceGroup(GetResourceGroupIndex(hnd.index));
    CTK_ASSERT(frame_index < res_group->frame_count);

    return GetBufferFrameState(res_group, GetResourceIndex(hnd.index), frame_index);
}

template<typename Type>
static Type* GetMappedMemory(BufferHnd hnd, uint32 frame_index)
{
    ValidateBuffer(hnd, "can't get buffer mapped memory");

    ResourceGroup* res_group = GetResourceGroup(GetResourceGroupIndex(hnd.index));
    CTK_ASSERT(frame_index < res_group->frame_count);

    uint32 buffer_index = GetResourceIndex(hnd.index);
    DeviceMemory* dev_mem = GetBufferDeviceMemory(res_group, buffer_index);
    CTK_ASSERT(dev_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    return (Type*)&dev_mem->mapped[GetBufferFrameState(res_group, buffer_index, frame_index)->buffer_mem_offset];
}

static VkBuffer GetBuffer(BufferHnd hnd)
{
    ValidateBuffer(hnd, "can't get buffer memory handle");
    return GetBuffer(GetResourceGroup(GetResourceGroupIndex(hnd.index)), GetResourceIndex(hnd.index));
}
