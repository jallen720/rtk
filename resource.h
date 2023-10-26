/// Data
////////////////////////////////////////////////////////////
struct BufferHnd { uint32 index; };

struct BufferInfo
{
    VkDeviceSize          size;
    VkDeviceSize          offset_alignment;
    bool                  per_frame;
    VkMemoryPropertyFlags mem_properties;
    VkBufferUsageFlags    usage;
};

struct ResourceState
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

struct ResourceMemory
{
    VkDeviceSize          size;
    VkDeviceMemory        device;
    uint8*                host;
    VkMemoryPropertyFlags properties;
};

struct ResourceGroupInfo
{
    uint32 max_buffers;
};

struct ResourceGroup
{
    uint32            max_buffers;
    uint32            buffer_count;
    uint32            frame_count;

    BufferInfo*       buffer_infos;        // size: max_buffers
    ResourceState*    buffer_states;       // size: max_buffers
    BufferFrameState* buffer_frame_states; // size: max_buffers * frame_count
    VkBuffer          buffers[VK_MAX_MEMORY_TYPES];

    ResourceMemory    mems[VK_MAX_MEMORY_TYPES];
};

static ResourceGroup g_res_group;

/// Utils
////////////////////////////////////////////////////////////
static ResourceGroup* GetResourceGroup()
{
    return &g_res_group;
}

static ResourceMemory* GetMemory(uint32 mem_index)
{
    return &g_res_group.mems[mem_index];
}

/// Buffer Utils
////////////////////////////////////////////////////////////
static VkBuffer* GetBufferPtr(uint32 mem_index)
{
    return &g_res_group.buffers[mem_index];
}

static BufferInfo* GetBufferInfoUtil(uint32 buffer_index)
{
    return &g_res_group.buffer_infos[buffer_index];
}

static ResourceState* GetBufferStateUtil(uint32 buffer_index)
{
    return &g_res_group.buffer_states[buffer_index];
}

static BufferFrameState* GetBufferFrameStateUtil(uint32 buffer_index, uint32 frame_index)
{
    uint32 frame_offset = GetBufferStateUtil(buffer_index)->frame_stride * frame_index;
    return &g_res_group.buffer_frame_states[frame_offset + buffer_index];
}

static VkBuffer GetBufferUtil(uint32 buffer_index)
{
    return g_res_group.buffers[GetBufferStateUtil(buffer_index)->mem_index];
}

static ResourceMemory* GetBufferMemoryUtil(uint32 buffer_index)
{
    return GetMemory(GetBufferStateUtil(buffer_index)->mem_index);
}

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
    return GetBufferInfoUtil(*buffer_index_a)->offset_alignment >=
           GetBufferInfoUtil(*buffer_index_b)->offset_alignment;
}

/// Debug
////////////////////////////////////////////////////////////
static void LogBuffers()
{
    PrintLine("buffers:");
    for (uint32 buffer_index = 0; buffer_index < g_res_group.buffer_count; ++buffer_index)
    {
        BufferInfo* info = GetBufferInfoUtil(buffer_index);
        ResourceState* state = GetBufferStateUtil(buffer_index);
        PrintLine("   [%2u] size:             %llu", buffer_index, info->size);
        PrintLine("        offset_alignment: %llu", info->offset_alignment);
        PrintLine("        per_frame:        %s", info->per_frame ? "true" : "false");
        PrintLine("        properties:   ");
        PrintMemoryPropertyFlags(info->mem_properties, 3);
        PrintLine("        usage:            ");
        PrintBufferUsageFlags(info->usage, 3);

        PrintLine("        frame_stride:     %u", state->frame_stride);
        PrintLine("        frame_count:      %u", state->frame_count);
        PrintLine("        mem_index:        %u", state->mem_index);
        PrintLine("        offsets:          ");
        for (uint32 frame_index = 0; frame_index < state->frame_count; ++frame_index)
        {
            PrintLine("            [%u] %llu", frame_index, GetBufferFrameStateUtil(buffer_index, frame_index)->offset);
        }
        PrintLine();
    }
}

static void LogMemory(uint32 mem_index)
{
    ResourceMemory* mem = &g_res_group.mems[mem_index];
    if (mem->size == 0)
    {
        return;
    }
    PrintLine("   [%2u] size:       %llu", mem_index, mem->size);
    PrintLine("        device:     0x%p", mem->device);
    PrintLine("        host:       0x%p", mem->host);
    PrintLine("        properties: ");
    PrintMemoryPropertyFlags(mem->properties, 3);
}

static void LogMemory()
{
    PrintLine("memory:");
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        LogMemory(mem_index);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitResourceGroup(const Allocator* allocator, ResourceGroupInfo* info)
{
    uint32 frame_count = GetFrameCount();
    g_res_group.max_buffers         = info->max_buffers;
    g_res_group.buffer_count        = 0;
    g_res_group.frame_count         = frame_count;
    g_res_group.buffer_infos        = Allocate<BufferInfo>      (allocator, info->max_buffers);
    g_res_group.buffer_states       = Allocate<ResourceState>   (allocator, info->max_buffers);
    g_res_group.buffer_frame_states = Allocate<BufferFrameState>(allocator, info->max_buffers * frame_count);
}

static void AllocateResourceGroup(Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);

    VkDevice device = GetDevice();
    PhysicalDevice* physical_device = GetPhysicalDevice();
    VkResult res = VK_SUCCESS;

    // Associate buffers with memory types they'll be using.
    Array<uint32> mem_buffer_index_arrays[VK_MAX_MEMORY_TYPES] = {};
    uint32 mem_buffer_counts[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 buffer_index = 0; buffer_index < g_res_group.buffer_count; ++buffer_index)
    {
        uint32 mem_index = GetMemoryIndex(GetBufferInfoUtil(buffer_index));
        GetBufferStateUtil(buffer_index)->mem_index = mem_index;
        ++mem_buffer_counts[mem_index];
    }

    // Associate buffers with memory types.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 buffer_count = mem_buffer_counts[mem_index];
        if (buffer_count == 0) { continue; }
        InitArray(&mem_buffer_index_arrays[mem_index], &frame.allocator, buffer_count);
    }
    for (uint32 buffer_index = 0; buffer_index < g_res_group.buffer_count; ++buffer_index)
    {
        Push(&mem_buffer_index_arrays[GetBufferStateUtil(buffer_index)->mem_index], buffer_index);
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
            BufferInfo* info = GetBufferInfoUtil(mem_buffer_index);

            // Size buffer memory and get offsets based on buffer size and alignment memory requirements.
            for (uint32 frame_index = 0; frame_index < GetBufferStateUtil(mem_buffer_index)->frame_count; ++frame_index)
            {
                buffer_mem->size = Align(buffer_mem->size, info->offset_alignment);
                GetBufferFrameStateUtil(mem_buffer_index, frame_index)->offset = buffer_mem->size;
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
