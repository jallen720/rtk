/// Data
////////////////////////////////////////////////////////////
struct BufferInfo
{
    VkDeviceSize          size;
    VkDeviceSize          offset_alignment;
    bool                  per_frame;
    VkMemoryPropertyFlags mem_properties;
    VkBufferUsageFlags    usage;
};

struct BufferFrameState
{
    VkDeviceSize offset;
    VkDeviceSize index;
};

struct ImageInfo
{
    VkExtent3D            extent;
    bool                  per_frame;
    VkImageCreateFlags    flags;
    VkImageType           type;
    VkFormat              format;
    uint32                mip_levels;
    uint32                array_layers;
    VkSampleCountFlagBits samples;
    VkImageTiling         tiling;
    VkMemoryPropertyFlags mem_properties;
    VkImageUsageFlags     usage;
};

struct ImageViewInfo
{
    VkImageViewCreateFlags  flags;
    VkImageViewType         type;
    VkFormat                format;
    VkComponentMapping      components;
    VkImageSubresourceRange subresource_range;
};

struct ImageMemoryInfo
{
    VkDeviceSize size;
    VkDeviceSize alignment;
    VkDeviceSize stride;
    uint32       image_index;
};

struct ResourceState
{
    uint32 frame_stride;
    uint32 frame_count;
    uint32 mem_index;
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
    uint32 max_images;
};

struct ResourceGroup
{
    uint32            frame_count;

    uint32            max_buffers;
    uint32            buffer_count;
    BufferInfo*       buffer_infos;                 // size: max_buffers
    ResourceState*    buffer_states;                // size: max_buffers
    BufferFrameState* buffer_frame_states;          // size: max_buffers * frame_count
    VkBuffer          buffers[VK_MAX_MEMORY_TYPES];

    uint32            max_images;
    uint32            image_count;
    ImageInfo*        image_infos;                  // size: max_images
    ResourceState*    image_states;                 // size: max_images
    VkImage*          images;                       // size: max_images * frame_count
    ImageViewInfo*    default_view_infos;           // size: max_images
    VkImageView*      default_views;                // size: max_images * frame_count

    ResourceMemory    mems[VK_MAX_MEMORY_TYPES];
    ResourceMemory    image_mems[VK_MAX_MEMORY_TYPES];
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

static BufferInfo* GetBufferInfo(uint32 buffer_index)
{
    return &g_res_group.buffer_infos[buffer_index];
}

static ResourceState* GetBufferState(uint32 buffer_index)
{
    return &g_res_group.buffer_states[buffer_index];
}

static BufferFrameState* GetBufferFrameState(uint32 buffer_index, uint32 frame_index)
{
    uint32 frame_offset = GetBufferState(buffer_index)->frame_stride * frame_index;
    return &g_res_group.buffer_frame_states[frame_offset + buffer_index];
}

static VkBuffer GetBuffer(uint32 buffer_index)
{
    return g_res_group.buffers[GetBufferState(buffer_index)->mem_index];
}

static ResourceMemory* GetBufferMemory(uint32 buffer_index)
{
    return GetMemory(GetBufferState(buffer_index)->mem_index);
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
    return GetBufferInfo(*buffer_index_a)->offset_alignment >=
           GetBufferInfo(*buffer_index_b)->offset_alignment;
}

/// Image Utils
////////////////////////////////////////////////////////////
static ImageInfo* GetImageInfo(uint32 image_index)
{
    return &g_res_group.image_infos[image_index];
}

static ResourceState* GetImageState(uint32 image_index)
{
    return &g_res_group.image_states[image_index];
}

static uint32 GetImageFrameIndex(uint32 image_index, uint32 frame_index)
{
    return (GetImageState(image_index)->frame_stride * frame_index) + image_index;
}

static VkImage GetImage(uint32 image_index, uint32 frame_index)
{
    return g_res_group.images[GetImageFrameIndex(image_index, frame_index)];
}

static VkImage* GetImagePtr(uint32 image_index, uint32 frame_index)
{
    return &g_res_group.images[GetImageFrameIndex(image_index, frame_index)];
}

static ImageViewInfo* GetDefaultViewInfo(uint32 image_index)
{
    return &g_res_group.default_view_infos[image_index];
}

static VkImageView GetDefaultView(uint32 image_index, uint32 frame_index)
{
    return g_res_group.default_views[GetImageFrameIndex(image_index, frame_index)];
}

static VkImageView* GetDefaultViewPtr(uint32 image_index, uint32 frame_index)
{
    return &g_res_group.default_views[GetImageFrameIndex(image_index, frame_index)];
}

static ResourceMemory* GetImageMemory(uint32 mem_index)
{
    return &g_res_group.image_mems[mem_index];
}

static bool AlignmentDesc(ImageMemoryInfo* a, ImageMemoryInfo* b)
{
    return a->alignment >= b->alignment;
}

static VkDeviceSize TotalSize(ImageMemoryInfo* mem_info)
{
    return (mem_info->stride * (GetImageState(mem_info->image_index)->frame_count - 1)) + mem_info->size;
}

/// Debug
////////////////////////////////////////////////////////////
static void LogBuffers()
{
    PrintLine("buffers:");
    for (uint32 buffer_index = 0; buffer_index < g_res_group.buffer_count; ++buffer_index)
    {
        BufferInfo* info = GetBufferInfo(buffer_index);
        ResourceState* state = GetBufferState(buffer_index);
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
            PrintLine("            [%u] %llu", frame_index, GetBufferFrameState(buffer_index, frame_index)->offset);
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

static void LogImageMemoryInfos(Array<ImageMemoryInfo>* mem_infos,
                                Array<uint32> mem_info_index_arrays[VK_MAX_MEMORY_TYPES])
{
    for (uint32 mem_type_index = 0; mem_type_index < VK_MAX_MEMORY_TYPES; ++mem_type_index)
    {
        Array<uint32>* mem_info_indexes = &mem_info_index_arrays[mem_type_index];
        if (mem_info_indexes->count == 0)
        {
            continue;
        }
        PrintLine("mem-type %u:", mem_type_index);
        for (uint32 i = 0; i < mem_info_indexes->count; ++i)
        {
            ImageMemoryInfo* mem_info = GetPtr(mem_infos, Get(mem_info_indexes, i));
            PrintLine("    [%2u] size:        %llu", i, mem_info->size);
            PrintLine("         alignment:   %llu", mem_info->alignment);
            PrintLine("         stride:      %u", mem_info->stride);
            PrintLine("         image_index: %u", mem_info->image_index);
            PrintLine();
        }
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitResourceGroup(const Allocator* allocator, ResourceGroupInfo* info)
{
    uint32 frame_count = GetFrameCount();

    g_res_group.frame_count         = frame_count;

    g_res_group.max_buffers         = info->max_buffers;
    g_res_group.buffer_count        = 0;
    g_res_group.buffer_infos        = Allocate<BufferInfo>      (allocator, info->max_buffers);
    g_res_group.buffer_states       = Allocate<ResourceState>   (allocator, info->max_buffers);
    g_res_group.buffer_frame_states = Allocate<BufferFrameState>(allocator, info->max_buffers * frame_count);

    g_res_group.max_images          = info->max_images;
    g_res_group.image_count         = 0;
    g_res_group.image_infos         = Allocate<ImageInfo>    (allocator, info->max_images);
    g_res_group.image_states        = Allocate<ResourceState>(allocator, info->max_images);
    g_res_group.images              = Allocate<VkImage>      (allocator, info->max_images * frame_count);
    g_res_group.default_view_infos  = Allocate<ImageViewInfo>(allocator, info->max_images);
    g_res_group.default_views       = Allocate<VkImageView>  (allocator, info->max_images * frame_count);
}
