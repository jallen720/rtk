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

struct ImageState
{
    uint32       frame_stride;
    uint32       frame_count;
    uint32       mem_index;
    VkDeviceSize size;
    VkDeviceSize alignment;
};

struct ImageFrameState
{
    VkImage      image;
    VkImageView  view;
    VkDeviceSize offset;
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
    BufferInfo*       buffer_infos;        // size: max_buffers
    BufferState*      buffer_states;       // size: max_buffers
    BufferFrameState* buffer_frame_states; // size: max_buffers * frame_count
    VkBuffer          buffers[VK_MAX_MEMORY_TYPES];

    uint32            max_images;
    uint32            image_count;
    ImageInfo*        image_infos;         // size: max_images
    ImageViewInfo*    image_view_infos;    // size: max_images
    ImageState*       image_states;        // size: max_images
    ImageFrameState*  image_frame_states;  // size: max_images * frame_count

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
static BufferInfo* GetBufferInfo(uint32 buffer_index)
{
    return &g_res_group.buffer_infos[buffer_index];
}

static BufferState* GetBufferState(uint32 buffer_index)
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

static ImageViewInfo* GetImageViewInfo(uint32 image_index)
{
    return &g_res_group.image_view_infos[image_index];
}

static ImageState* GetImageState(uint32 image_index)
{
    return &g_res_group.image_states[image_index];
}

static ImageFrameState* GetImageFrameState(uint32 image_index, uint32 frame_index)
{
    uint32 frame_offset = GetImageState(image_index)->frame_stride * frame_index;
    return &g_res_group.image_frame_states[frame_offset + image_index];
}

static bool AlignmentDesc(uint32* a_image_index, uint32* b_image_index)
{
    return GetImageState(*a_image_index)->alignment >=
           GetImageState(*b_image_index)->alignment;
}

/// Debug
////////////////////////////////////////////////////////////
static void LogBuffers()
{
    PrintLine("buffers:");
    for (uint32 buffer_index = 0; buffer_index < g_res_group.buffer_count; ++buffer_index)
    {
        BufferInfo* info = GetBufferInfo(buffer_index);
        BufferState* state = GetBufferState(buffer_index);
        PrintLine("    buffer %u:", buffer_index);
        PrintLine("        info:");
        PrintLine("            size:             %llu", info->size);
        PrintLine("            offset_alignment: %llu", info->offset_alignment);
        PrintLine("            per_frame:        %s", info->per_frame ? "true" : "false");
        PrintLine("            properties:   ");
        PrintMemoryPropertyFlags(info->mem_properties, 4);
        PrintLine("            usage:            ");
        PrintBufferUsageFlags(info->usage, 4);

        PrintLine("        state:");
        PrintLine("            frame_stride: %u", state->frame_stride);
        PrintLine("            frame_count:  %u", state->frame_count);
        PrintLine("            mem_index:    %u", state->mem_index);
        PrintLine("        frame_states:");
        for (uint32 frame_index = 0; frame_index < state->frame_count; ++frame_index)
        {
            BufferFrameState* frame_state = GetBufferFrameState(buffer_index, frame_index);
            PrintLine("            [%u] offset: %llu", frame_index, frame_state->offset);
            PrintLine("                index:  %llu", frame_state->index);
        }
        PrintLine();
    }
}
static void LogImages()
{
    PrintLine("images:");
    for (uint32 image_index = 0; image_index < GetResourceGroup()->image_count; ++image_index)
    {
        ImageInfo* info = GetImageInfo(image_index);
        ImageViewInfo* view_info = GetImageViewInfo(image_index);
        ImageState* state = GetImageState(image_index);
        PrintLine("    image %u:", image_index);
        PrintLine("        info:");
        PrintLine("            extent:       { w: %u, h: %u, d: %u }",
                  info->extent.width,
                  info->extent.height,
                  info->extent.depth);
        PrintLine("            per_frame:    %s", info->per_frame ? "true" : "false");

        PrintLine("            flags:");
        PrintImageCreateFlags(info->flags, 4);
        PrintLine("            type:         %s", VkImageTypeName(info->type));
        PrintLine("            format:       %s", VkFormatName(info->format));
        PrintLine("            mip_levels:   %u", info->mip_levels);
        PrintLine("            array_layers: %u", info->array_layers);
        PrintLine("            samples:      %s", VkSampleCountName(info->samples));
        PrintLine("            tiling:       %s", VkImageTilingName(info->tiling));
        PrintLine("            mem_properties:");
        PrintMemoryPropertyFlags(info->mem_properties, 4);
        PrintLine("            usage:");
        PrintImageUsageFlags(info->usage, 4);
        PrintLine("        view_info:");
        PrintLine("            flags:");
        PrintImageViewCreateFlags(view_info->flags, 4);
        PrintLine("            type:   %s", VkImageViewTypeName(view_info->type));
        PrintLine("            format: %s", VkFormatName(view_info->format));
        PrintLine("            components:");
        PrintLine("                r: %s", VkComponentSwizzleName(view_info->components.r));
        PrintLine("                g: %s", VkComponentSwizzleName(view_info->components.g));
        PrintLine("                b: %s", VkComponentSwizzleName(view_info->components.b));
        PrintLine("                a: %s", VkComponentSwizzleName(view_info->components.a));
        PrintLine("            subresource_range:");
        PrintLine("                aspectMask:");
        PrintImageAspectFlags(view_info->subresource_range.aspectMask, 5);
        PrintLine("                baseMipLevel:   %u", view_info->subresource_range.baseMipLevel);

        uint32 level_count = view_info->subresource_range.levelCount;
        if (level_count == VK_REMAINING_MIP_LEVELS)
        {
            PrintLine("                levelCount:     VK_REMAINING_MIP_LEVELS");
        }
        else
        {
            PrintLine("                levelCount:     %u", level_count);
        }
        PrintLine("                baseArrayLayer: %u", view_info->subresource_range.baseArrayLayer);

        uint32 layer_count = view_info->subresource_range.layerCount;
        if (layer_count == VK_REMAINING_ARRAY_LAYERS)
        {
            PrintLine("                layerCount:     VK_REMAINING_ARRAY_LAYERS");
        }
        else
        {
            PrintLine("                layerCount:     %u", layer_count);
        }

        PrintLine("        state:");
        PrintLine("            frame_stride: %u", state->frame_stride);
        PrintLine("            frame_count:  %u", state->frame_count);
        PrintLine("            mem_index:    %u", state->mem_index);
        PrintLine("            size:         %llu", state->size);
        PrintLine("            alignment:    %llu", state->alignment);
        PrintLine("        frame_states:");
        for (uint32 frame_index = 0; frame_index < state->frame_count; ++frame_index)
        {
            ImageFrameState* frame_state = GetImageFrameState(image_index, frame_index);
            PrintLine("            [%u] image:  %p", frame_index, frame_state->image);
            PrintLine("                view:   %p", frame_state->view);
            PrintLine("                offset: %llu", frame_state->offset);
        }
        PrintLine();
    }
}

static void LogMemory(uint32 mem_index)
{
    ResourceMemory* mem = GetMemory(mem_index);
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

    g_res_group.frame_count         = frame_count;

    g_res_group.max_buffers         = info->max_buffers;
    g_res_group.buffer_count        = 0;
    g_res_group.buffer_infos        = Allocate<BufferInfo>      (allocator, info->max_buffers);
    g_res_group.buffer_states       = Allocate<BufferState>     (allocator, info->max_buffers);
    g_res_group.buffer_frame_states = Allocate<BufferFrameState>(allocator, info->max_buffers * frame_count);

    g_res_group.max_images          = info->max_images;
    g_res_group.image_count         = 0;
    g_res_group.image_infos         = Allocate<ImageInfo>      (allocator, info->max_images);
    g_res_group.image_view_infos    = Allocate<ImageViewInfo>  (allocator, info->max_images);
    g_res_group.image_states        = Allocate<ImageState>     (allocator, info->max_images);
    g_res_group.image_frame_states  = Allocate<ImageFrameState>(allocator, info->max_images * frame_count);
}

static void AllocateResourceGroup(Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);

    VkDevice device = GetDevice();
    PhysicalDevice* physical_device = GetPhysicalDevice();
    QueueFamilies* queue_families = &physical_device->queue_families;
    VkResult res = VK_SUCCESS;

    // Get resource memory info.
    auto buffer_indexes = CreateArray<uint32>(&frame.allocator, g_res_group.buffer_count);
    VkBufferUsageFlags buffer_usage[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 buffer_index = 0; buffer_index < g_res_group.buffer_count; ++buffer_index)
    {
        BufferInfo* buffer_info = GetBufferInfo(buffer_index);
        BufferState* buffer_state = GetBufferState(buffer_index);

        // Create dummy buffer to get memory requirements.
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size  = buffer_info->size;
        create_info.usage = buffer_info->usage;
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
        res = vkCreateBuffer(device, &create_info, NULL, &temp);
        Validate(res, "vkCreateBuffer() failed");
        VkMemoryRequirements mem_requirements = {};
        vkGetBufferMemoryRequirements(device, temp, &mem_requirements);
        vkDestroyBuffer(device, temp, NULL);

        buffer_state->mem_index = GetCapableMemoryTypeIndex(&mem_requirements, buffer_info->mem_properties);
        buffer_info->size = mem_requirements.size;

        // Add buffer index for sorting.
        Push(buffer_indexes, buffer_index);

        // Append buffer usage for creating associated buffer memory.
        buffer_usage[buffer_state->mem_index] |= buffer_info->usage;
    }
    auto image_indexes = CreateArray<uint32>(&frame.allocator, g_res_group.image_count);
    for (uint32 image_index = 0; image_index < g_res_group.image_count; ++image_index)
    {
        ImageInfo* image_info = GetImageInfo(image_index);
        ImageState* image_state = GetImageState(image_index);

        // Create 1 image per frame.
        VkImageCreateInfo info = {};
        info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext         = NULL;
        info.flags         = 0;
        info.imageType     = image_info->type;
        info.format        = image_info->format;
        info.extent        = image_info->extent;
        info.mipLevels     = image_info->mip_levels;
        info.arrayLayers   = image_info->array_layers;
        info.samples       = image_info->samples;
        info.tiling        = image_info->tiling;
        info.usage         = image_info->usage;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (queue_families->graphics != queue_families->present)
        {
            info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
            info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(uint32);
            info.pQueueFamilyIndices   = (uint32*)queue_families;
        }
        else
        {
            info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
            info.queueFamilyIndexCount = 0;
            info.pQueueFamilyIndices   = NULL;
        }
        for (uint32 frame_index = 0; frame_index < image_state->frame_count; ++frame_index)
        {
            res = vkCreateImage(device, &info, NULL, &GetImageFrameState(image_index, frame_index)->image);
            Validate(res, "vkCreateImage() failed");
        }

        // Get image memory info.
        VkMemoryRequirements mem_requirements = {};
        vkGetImageMemoryRequirements(device, GetImageFrameState(image_index, 0)->image, &mem_requirements);

        uint32 mem_index = GetCapableMemoryTypeIndex(&mem_requirements, image_info->mem_properties);
        image_state->mem_index = mem_index;
        image_state->size      = mem_requirements.size;
        image_state->alignment = mem_requirements.alignment;

        // Add buffer index for sorting.
        Push(image_indexes, image_index);
    }

    // Sort resource indexes by descending order of alignement before calculating memory size and offsets.
    InsertionSort(buffer_indexes, OffsetAlignmentDesc);
    InsertionSort(image_indexes, AlignmentDesc);

    // Adjust memory size, calculate offsets, and create buffer for each memory type.
    for (uint32 i = 0; i < buffer_indexes->count; ++i)
    {
        uint32 buffer_index = Get(buffer_indexes, i);
        BufferInfo* buffer_info = GetBufferInfo(buffer_index);
        BufferState* buffer_state = GetBufferState(buffer_index);
        ResourceMemory* mem = GetMemory(buffer_state->mem_index);
        for (uint32 frame_index = 0; frame_index < buffer_state->frame_count; ++frame_index)
        {
            mem->size = Align(mem->size, buffer_info->offset_alignment);
            GetBufferFrameState(buffer_index, frame_index)->offset = mem->size;
            mem->size += buffer_info->size;
        }
    }
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        ResourceMemory* mem = GetMemory(mem_index);
        if (mem->size == 0) { continue; }

        // Create buffer.
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.pNext = NULL;
        create_info.flags = 0;
        create_info.size  = mem->size;
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
        res = vkCreateBuffer(device, &create_info, NULL, &g_res_group.buffers[mem_index]);
        Validate(res, "vkCreateBuffer() failed");
    }
    for (uint32 i = 0; i < image_indexes->count; ++i)
    {
        uint32 image_index = Get(image_indexes, i);
        ImageState* image_state = GetImageState(image_index);
        ResourceMemory* mem = GetMemory(image_state->mem_index);
        for (uint32 frame_index = 0; frame_index < image_state->frame_count; ++frame_index)
        {
            mem->size = Align(mem->size, image_state->alignment);
            GetImageFrameState(image_index, frame_index)->offset = mem->size;
            mem->size += image_state->size;
        }
    }

    // Allocate memory.
    VkMemoryType* memory_types = physical_device->mem_properties.memoryTypes;
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        ResourceMemory* mem = GetMemory(mem_index);
        if (mem->size == 0) { continue; }

        // Allocate buffer memory.
        mem->device = AllocateDeviceMemory(mem_index, mem->size, NULL);
        mem->properties = memory_types[mem_index].propertyFlags;

        // Map host visible buffer memory.
        if (mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            vkMapMemory(device, mem->device, 0, mem->size, 0, (void**)&mem->host);
        }
    }

    // Bind buffers to memory.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        ResourceMemory* mem = GetMemory(mem_index);
        if (mem->size == 0) { continue; }

        res = vkBindBufferMemory(device, g_res_group.buffers[mem_index], mem->device, 0);
        Validate(res, "vkBindBufferMemory() failed");
    }

    // Bind images to memory.
    for (uint32 i = 0; i < g_res_group.image_count; ++i)
    {
        uint32 image_index = Get(image_indexes, i);
        ImageState* image_state = GetImageState(image_index);
        ResourceMemory* mem = GetMemory(image_state->mem_index);
        for (uint32 frame_index = 0; frame_index < image_state->frame_count; ++frame_index)
        {
            ImageFrameState* image_frame_state = GetImageFrameState(image_index, frame_index);
            res = vkBindImageMemory(device, image_frame_state->image, mem->device, image_frame_state->offset);
            Validate(res, "vkBindImageMemory() failed");
        }
    }

    // Create image views.
    for (uint32 image_index = 0; image_index < g_res_group.image_count; ++image_index)
    {
        ImageViewInfo* view_info = GetImageViewInfo(image_index);
        for (uint32 frame_index = 0; frame_index < GetImageState(image_index)->frame_count; ++frame_index)
        {
            ImageFrameState* image_frame_state = GetImageFrameState(image_index, frame_index);
            VkImageViewCreateInfo view_create_info =
            {
                .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext            = NULL,
                .flags            = view_info->flags,
                .image            = image_frame_state->image,
                .viewType         = view_info->type,
                .format           = view_info->format,
                .components       = view_info->components,
                .subresourceRange = view_info->subresource_range,
            };
            res = vkCreateImageView(device, &view_create_info, NULL, &image_frame_state->view);
            Validate(res, "vkCreateImageView() failed");
        }
    }
}
