/// Data
////////////////////////////////////////////////////////////
struct BufferHnd { uint32 index; };

static constexpr VkDeviceSize USE_MIN_OFFSET_ALIGNMENT = 0;

struct BufferInfo
{
    VkDeviceSize          size;
    VkDeviceSize          offset_alignment;
    bool                  per_frame;
    VkBufferUsageFlags    usage;
    VkMemoryPropertyFlags mem_properties;
};

struct Buffer
{
    VkDeviceSize          size;
    VkDeviceSize          offset_alignment;
    bool                  per_frame;
    VkBufferUsageFlags    usage;
    VkMemoryPropertyFlags mem_properties;
    uint32                mem_index;
};

struct BufferMemory
{
    VkBuffer             hnd;
    VkMemoryRequirements mem_requirements;
    VkDeviceMemory       mem;
    uint8*               mapped_mem;
    VkDeviceSize         size;
};

static struct BufferState
{
    Buffer*       buffers; // size: size
    VkDeviceSize* offsets; // size: size * frame_count
    VkDeviceSize* indexes; // size: size * frame_count
    uint32        count;
    uint32        size;
    uint32        frame_count;
    BufferMemory  mem[VK_MAX_MEMORY_TYPES];
}
g_buffer_state;

/// Interface
////////////////////////////////////////////////////////////
static void InitBufferModule(const Allocator* allocator, uint32 max_buffers)
{
    uint32 frame_count = GetFrameCount();
    g_buffer_state.buffers     = Allocate<Buffer>(allocator, max_buffers);
    g_buffer_state.offsets     = Allocate<VkDeviceSize>(allocator, max_buffers * frame_count);
    g_buffer_state.indexes     = Allocate<VkDeviceSize>(allocator, max_buffers * frame_count);
    g_buffer_state.count       = 0;
    g_buffer_state.size        = max_buffers;
    g_buffer_state.frame_count = frame_count;
}

static BufferHnd CreateBuffer(BufferInfo* info)
{
    if (g_buffer_state.count >= g_buffer_state.size)
    {
        CTK_FATAL("can't create buffer: already at max buffer count of %u", g_buffer_state.size);
    }

    BufferHnd hnd = { .index = g_buffer_state.count };
    ++g_buffer_state.count;

    Buffer* buffer = g_buffer_state.buffers + hnd.index;
    buffer->size             = info->size;
    buffer->offset_alignment = info->offset_alignment;
    buffer->per_frame        = info->per_frame;
    buffer->usage            = info->usage;
    buffer->mem_properties   = info->mem_properties;

    return hnd;
}

static void BackBuffersWithMemory()
{
    PhysicalDevice* physical_device = GetPhysicalDevice();
    VkDevice device = GetDevice();
    VkResult res = VK_SUCCESS;



//     VkBufferCreateInfo create_info = {};
//     create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//     create_info.size  = info->size;
//     create_info.usage = info->usage;

//     // Check if sharing mode needs to be concurrent due to separate graphics & present queue families.
//     QueueFamilies* queue_families = &physical_device->queue_families;
//     if (queue_families->graphics != queue_families->present)
//     {
//         create_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
//         create_info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(uint32);
//         create_info.pQueueFamilyIndices   = (uint32*)queue_families;
//     }
//     else
//     {
//         create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
//         create_info.queueFamilyIndexCount = 0;
//         create_info.pQueueFamilyIndices   = NULL;
//     }

//     res = vkCreateBuffer(device, &create_info, NULL, &buffer_stack->hnd);
//     Validate(res, "vkCreateBuffer() failed");

//     // Allocate/bind buffer_stack memory.
//     VkMemoryRequirements mem_requirements = {};
//     vkGetBufferMemoryRequirements(device, buffer_stack->hnd, &mem_requirements);
// PrintResourceMemoryInfo("buffer-stack", &mem_requirements, info->mem_properties);

//     buffer_stack->mem = AllocateDeviceMemory(&mem_requirements, info->mem_properties, NULL);
//     res = vkBindBufferMemory(device, buffer_stack->hnd, buffer_stack->mem, 0);
//     Validate(res, "vkBindBufferMemory() failed");

//     buffer_stack->size  = mem_requirements.size;
//     buffer_stack->index = 0;

//     // Map host visible buffer_stack memory.
//     if (info->mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
//     {
//         vkMapMemory(device, buffer_stack->mem, 0, buffer_stack->size, 0, (void**)&buffer_stack->mapped_mem);
//     }
}

static void InitBuffer(BufferHnd buffer_hnd, BufferStack* buffer_stack, BufferInfo* info)
{
    // CTK_ASSERT(info->frame_count > 0);
    // CTK_ASSERT(info->frame_count <= MAX_FRAME_COUNT);

    // VkPhysicalDeviceLimits* limits = &GetPhysicalDevice()->properties.limits;
    // if (info->offset_alignment == USE_MIN_OFFSET_ALIGNMENT)
    // {
    //     info->offset_alignment = info->type == BufferType::UNIFORM ? limits->minUniformBufferOffsetAlignment :
    //                              info->type == BufferType::STORAGE ? limits->minStorageBufferOffsetAlignment :
    //                              4;
    // }

    // VkDeviceSize aligned_offset = MultipleOf(buffer_stack->index, info->offset_alignment);
    // VkDeviceSize aligned_size = MultipleOf(info->size, info->offset_alignment);
    // uint32 total_aligned_size = aligned_size * info->frame_count;
    // if (aligned_offset + total_aligned_size > buffer_stack->size)
    // {
    //     CTK_FATAL("can't sub-allocate %u offset-aligned %u-byte buffer(s) (%u-bytes total) from buffer-stack at "
    //               "offset-aligned index %u: allocation would exceed buffer-stack size of %u",
    //               info->frame_count, aligned_size, total_aligned_size, info->offset_alignment, aligned_offset,
    //               buffer_stack->size);
    // }

    // buffer->hnd            = buffer_stack->hnd;
    // buffer->mapped_mem     = buffer_stack->mapped_mem;
    // buffer->size           = aligned_size;
    // buffer->frame_count = info->frame_count;
    // for (uint32 i = 0; i < info->frame_count; ++i)
    // {
    //     buffer->offsets[i] = aligned_offset + (aligned_size * i);
    //     buffer->indexes[i] = 0;
    // }

    // buffer_stack->index = aligned_offset + total_aligned_size;
}

static void WriteHostBuffer(BufferHnd buffer_hnd, uint32 instance_index, void* data, VkDeviceSize data_size)
{
    // CTK_ASSERT(buffer->mapped_mem != NULL);
    // CTK_ASSERT(instance_index < buffer->frame_count);

    // if (data_size > buffer->size)
    // {
    //     CTK_FATAL("can't write %u bytes to host-buffer: write would exceed size of %u", data_size, buffer->size);
    // }

    // memcpy(buffer->mapped_mem + buffer->offsets[instance_index], data, data_size);
    // buffer->indexes[instance_index] = data_size;
}

static void AppendHostBuffer(BufferHnd buffer_hnd, uint32 instance_index, void* data, VkDeviceSize data_size)
{
    // CTK_ASSERT(buffer->mapped_mem != NULL);
    // CTK_ASSERT(instance_index < buffer->frame_count);

    // if (buffer->indexes[instance_index] + data_size > buffer->size)
    // {
    //     CTK_FATAL("can't append %u bytes to host-buffer at index %u: append would exceed size of %u", data_size,
    //               buffer->indexes[instance_index], buffer->size);
    // }

    // memcpy(buffer->mapped_mem + buffer->offsets[instance_index] + buffer->indexes[instance_index], data, data_size);
    // buffer->indexes[instance_index] += data_size;
}

static void WriteDeviceBufferCmd(BufferHnd buffer_hnd, uint32 instance_index,
                                 Buffer* src_buffer, uint32 src_instance_index,
                                 VkDeviceSize offset, VkDeviceSize size)
{
    // CTK_ASSERT(src_buffer->frame_count == 1);
    // CTK_ASSERT(instance_index < buffer->frame_count);

    // if (size > buffer->size)
    // {
    //     CTK_FATAL("can't write %u bytes to device-buffer: write would exceed size of %u", size,
    //               buffer->size);
    // }

    // VkBufferCopy copy =
    // {
    //     .srcOffset = src_buffer->offsets[src_instance_index] + offset,
    //     .dstOffset = buffer->offsets[instance_index],
    //     .size      = size,
    // };
    // vkCmdCopyBuffer(g_ctx.temp_command_buffer, src_buffer->hnd, buffer->hnd, 1, &copy);
    // buffer->indexes[instance_index] = size;
}

static void AppendDeviceBufferCmd(BufferHnd buffer_hnd, uint32 instance_index,
                                  BufferHnd src_buffer_hnd, uint32 src_instance_index,
                                  VkDeviceSize offset, VkDeviceSize size)
{
    // CTK_ASSERT(src_buffer->frame_count == 1);
    // CTK_ASSERT(instance_index < buffer->frame_count);

    // if (buffer->indexes[instance_index] + size > buffer->size)
    // {
    //     CTK_FATAL("can't append %u bytes to device-buffer at index %u: append would exceed size of %u",
    //               size, buffer->indexes[instance_index], buffer->size);
    // }

    // VkBufferCopy copy =
    // {
    //     .srcOffset = src_buffer->offsets[src_instance_index] + offset,
    //     .dstOffset = buffer->offsets[instance_index] + buffer->indexes[instance_index],
    //     .size      = size,
    // };
    // vkCmdCopyBuffer(g_ctx.temp_command_buffer, src_buffer->hnd, buffer->hnd, 1, &copy);
    // buffer->indexes[instance_index] += size;
}

template<typename Type>
static Type* GetMappedMem(BufferHnd buffer_hnd, uint32 instance_index)
{
    // CTK_ASSERT(buffer->mapped_mem != NULL);
    // CTK_ASSERT(instance_index < buffer->frame_count);
    // return (Type*)(buffer->mapped_mem + buffer->offsets[instance_index]);
}

static void Clear(BufferHnd buffer_hnd)
{
    // for (uint32 i = 0; i < buffer->frame_count; ++i)
    // {
    //     buffer->indexes[i] = 0;
    // }
}
