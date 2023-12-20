/// Data
////////////////////////////////////////////////////////////
struct HostBufferWrite
{
    VkDeviceSize size;
    uint8*       src_data;
    VkDeviceSize src_offset;
    BufferHnd    dst_hnd;
    VkDeviceSize dst_offset;
};

struct HostBufferAppend
{
    VkDeviceSize size;
    uint8*       src_data;
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

/// Utils
////////////////////////////////////////////////////////////
static void ValidateBuffer(ResourceGroup* res_group, uint32 buffer_index, const char* action)
{
    if (buffer_index >= res_group->buffer_count)
    {
        CTK_FATAL("%s: buffer index %u exceeds buffer count of %u", action, buffer_index, res_group->buffer_count);
    }
}

static DeviceMemory* GetBufferDeviceMemory(ResourceGroup* res_group, uint32 buffer_index)
{
    uint32 buffer_mem_index = GetBufferState(res_group, buffer_index)->buffer_mem_index;
    return GetDeviceMemory(res_group, GetBufferMemoryState(res_group, buffer_mem_index)->dev_mem_index);
}

/// Interface
////////////////////////////////////////////////////////////
static void WriteHostBuffer(HostBufferWrite* write, uint32 frame_index)
{
    ResourceGroup* res_group = GetResourceGroup(write->dst_hnd.group_index);
    ValidateBuffer(res_group, write->dst_hnd.index, "can't write to destination host buffer");
    CTK_ASSERT(frame_index < res_group->frame_count);

    BufferInfo*       dst_info        = GetBufferInfo      (res_group, write->dst_hnd.index);
    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, write->dst_hnd.index, frame_index);
    if (write->dst_offset + write->size > dst_info->size)
    {
        CTK_FATAL("can't write %u bytes to host buffer at offset %u: write would exceed size of %u",
                  write->size, write->dst_offset, dst_info->size);
    }

    DeviceMemory* dev_mem = GetBufferDeviceMemory(res_group, write->dst_hnd.index);
    CTK_ASSERT(dev_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    uint8* dst = &dev_mem->mapped[dst_frame_state->buffer_mem_offset + write->dst_offset];
    uint8* src = &write->src_data[write->src_offset];
    memcpy(dst, src, write->size);
}

static void AppendHostBuffer(HostBufferAppend* append, uint32 frame_index)
{
    ResourceGroup* res_group = GetResourceGroup(append->dst_hnd.group_index);
    ValidateBuffer(res_group, append->dst_hnd.index, "can't append to destination host buffer");
    CTK_ASSERT(frame_index < res_group->frame_count);

    BufferInfo*       dst_info        = GetBufferInfo      (res_group, append->dst_hnd.index);
    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, append->dst_hnd.index, frame_index);
    if (dst_frame_state->index + append->size > dst_info->size)
    {
        CTK_FATAL("can't append %u bytes to host buffer at index %u: append would exceed size of %u",
                  append->size, dst_frame_state->index, dst_info->size);
    }

    DeviceMemory* dev_mem = GetBufferDeviceMemory(res_group, append->dst_hnd.index);
    CTK_ASSERT(dev_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    uint8* dst = &dev_mem->mapped[dst_frame_state->buffer_mem_offset + dst_frame_state->index];
    uint8* src = &append->src_data[append->src_offset];
    memcpy(dst, src, append->size);
    dst_frame_state->index += append->size;
}

static void WriteDeviceBufferCmd(DeviceBufferWrite* write, uint32 frame_index)
{
    ResourceGroup* res_group = GetResourceGroup(write->dst_hnd.group_index);
    ValidateBuffer(res_group, write->src_hnd.index, "can't write from source buffer to destination device buffer");
    ValidateBuffer(res_group, write->dst_hnd.index, "can't write to destination device buffer");
    CTK_ASSERT(frame_index < res_group->frame_count);

    BufferInfo* dst_info = GetBufferInfo(res_group, write->dst_hnd.index);
    if (write->dst_offset + write->size > dst_info->size)
    {
        CTK_FATAL("can't write %u bytes to device buffer at offset %u: write would exceed size of %u",
                  write->size, write->dst_offset, dst_info->size);
    }

    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, write->dst_hnd.index, frame_index);
    BufferFrameState* src_frame_state = GetBufferFrameState(res_group, write->src_hnd.index, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = src_frame_state->buffer_mem_offset + write->src_offset,
        .dstOffset = dst_frame_state->buffer_mem_offset + write->dst_offset,
        .size      = write->size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBuffer(res_group, write->src_hnd.index),
                    GetBuffer(res_group, write->dst_hnd.index),
                    1, &copy);
}

static void AppendDeviceBufferCmd(DeviceBufferAppend* append, uint32 frame_index)
{
    ResourceGroup* res_group = GetResourceGroup(append->dst_hnd.group_index);
    ValidateBuffer(res_group, append->src_hnd.index, "can't append from source buffer to destination device buffer");
    ValidateBuffer(res_group, append->dst_hnd.index, "can't append to destination device buffer");
    CTK_ASSERT(frame_index < res_group->frame_count);

    BufferInfo* dst_info = GetBufferInfo(res_group, append->dst_hnd.index);
    BufferFrameState* dst_frame_state = GetBufferFrameState(res_group, append->dst_hnd.index, frame_index);
    if (dst_frame_state->index + append->size > dst_info->size)
    {
        CTK_FATAL("can't append %u bytes to device buffer at index %u: append would exceed size of %u",
                  append->size, dst_frame_state->index, dst_info->size);
    }

    BufferFrameState* src_frame_state = GetBufferFrameState(res_group, append->src_hnd.index, frame_index);
    VkBufferCopy copy =
    {
        .srcOffset = src_frame_state->buffer_mem_offset + append->src_offset,
        .dstOffset = dst_frame_state->buffer_mem_offset + dst_frame_state->index,
        .size      = append->size,
    };
    vkCmdCopyBuffer(GetTempCommandBuffer(),
                    GetBuffer(res_group, append->src_hnd.index),
                    GetBuffer(res_group, append->dst_hnd.index),
                    1, &copy);
    dst_frame_state->index += append->size;
}

static void Clear(BufferHnd buffer_hnd)
{
    ResourceGroup* res_group = GetResourceGroup(buffer_hnd.group_index);
    ValidateBuffer(res_group, buffer_hnd.index, "can't clear buffer");

    for (uint32 frame_index = 0; frame_index < GetBufferState(res_group, buffer_hnd.index)->frame_count; ++frame_index)
    {
        GetBufferFrameState(res_group, buffer_hnd.index, frame_index)->index = 0;
    }
}

static BufferInfo* GetBufferInfo(BufferHnd buffer_hnd)
{
    ResourceGroup* res_group = GetResourceGroup(buffer_hnd.group_index);
    ValidateBuffer(res_group, buffer_hnd.index, "can't get buffer info");
    return GetBufferInfo(res_group, buffer_hnd.index);
}

static BufferState* GetBufferState(BufferHnd buffer_hnd)
{
    ResourceGroup* res_group = GetResourceGroup(buffer_hnd.group_index);
    ValidateBuffer(res_group, buffer_hnd.index, "can't get buffer state");
    return GetBufferState(res_group, buffer_hnd.index);
}

static BufferFrameState* GetBufferFrameState(BufferHnd buffer_hnd, uint32 frame_index)
{
    ResourceGroup* res_group = GetResourceGroup(buffer_hnd.group_index);
    ValidateBuffer(res_group, buffer_hnd.index, "can't get buffer frame state");
    CTK_ASSERT(frame_index < res_group->frame_count);

    return GetBufferFrameState(res_group, buffer_hnd.index, frame_index);
}

template<typename Type>
static Type* GetMappedMemory(BufferHnd buffer_hnd, uint32 frame_index)
{
    ResourceGroup* res_group = GetResourceGroup(buffer_hnd.group_index);
    ValidateBuffer(res_group, buffer_hnd.index, "can't get buffer mapped memory");
    CTK_ASSERT(frame_index < res_group->frame_count);

    DeviceMemory* dev_mem = GetBufferDeviceMemory(res_group, buffer_hnd.index);
    CTK_ASSERT(dev_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    return (Type*)&dev_mem->mapped[GetBufferFrameState(res_group, buffer_hnd.index, frame_index)->buffer_mem_offset];
}

static VkBuffer GetBuffer(BufferHnd buffer_hnd)
{
    ResourceGroup* res_group = GetResourceGroup(buffer_hnd.group_index);
    ValidateBuffer(res_group, buffer_hnd.index, "can't get buffer memory handle");
    return GetBuffer(res_group, buffer_hnd.index);
}
