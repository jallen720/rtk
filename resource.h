/// Data
////////////////////////////////////////////////////////////
struct BufferHnd        { uint32 index; };
struct ImageHnd         { uint32 index; };
struct ResourceGroupHnd { uint32 index; };

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
    VkImageLayout         initial_layout;
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

static Array<ResourceGroup> g_res_groups;

/// Utils
////////////////////////////////////////////////////////////
static uint32 GetResourceIndex(uint32 res_hnd_index)
{
    return 0x00FFFFFF & res_hnd_index;
}

static uint32 GetResourceGroupIndex(uint32 res_hnd_index)
{
    return (0xFF000000 & res_hnd_index) >> 24;
}

static uint32 GetResourceHndIndex(ResourceGroupHnd res_group_hnd, uint32 res_index)
{
    return (res_group_hnd.index << 24) | res_index;
}

static ResourceGroup* GetResourceGroup(uint32 index)
{
    return GetPtr(&g_res_groups, index);
}

static ResourceGroup* GetResourceGroup(ResourceGroupHnd hnd)
{
    return GetResourceGroup(hnd.index);
}

static ResourceMemory* GetMemory(ResourceGroup* res_group, uint32 mem_index)
{
    return &res_group->mems[mem_index];
}

/// Buffer Utils
////////////////////////////////////////////////////////////
static uint32 GetBufferIndex(BufferHnd hnd)
{
    return GetResourceIndex(hnd.index);
}

static ResourceGroup* GetResourceGroup(BufferHnd hnd)
{
    return GetResourceGroup(GetResourceGroupIndex(hnd.index));
}

static BufferInfo* GetBufferInfo(ResourceGroup* res_group, uint32 buffer_index)
{
    return &res_group->buffer_infos[buffer_index];
}

static BufferState* GetBufferState(ResourceGroup* res_group, uint32 buffer_index)
{
    return &res_group->buffer_states[buffer_index];
}

static BufferFrameState* GetBufferFrameState(ResourceGroup* res_group, uint32 buffer_index, uint32 frame_index)
{
    uint32 frame_offset = GetBufferState(res_group, buffer_index)->frame_stride * frame_index;
    return &res_group->buffer_frame_states[frame_offset + buffer_index];
}

static VkBuffer GetBuffer(ResourceGroup* res_group, uint32 buffer_index)
{
    return res_group->buffers[GetBufferState(res_group, buffer_index)->mem_index];
}

static ResourceMemory* GetBufferMemory(ResourceGroup* res_group, uint32 buffer_index)
{
    return GetMemory(res_group, GetBufferState(res_group, buffer_index)->mem_index);
}

static bool OffsetAlignmentDesc(uint32* a_buffer_index, uint32* b_buffer_index, ResourceGroup* res_group)
{
    return GetBufferInfo(res_group, *a_buffer_index)->offset_alignment >=
           GetBufferInfo(res_group, *b_buffer_index)->offset_alignment;
}

/// Image Utils
////////////////////////////////////////////////////////////
static uint32 GetImageIndex(ImageHnd hnd)
{
    return GetResourceIndex(hnd.index);
}

static ResourceGroup* GetResourceGroup(ImageHnd hnd)
{
    return GetResourceGroup(GetResourceGroupIndex(hnd.index));
}

static ImageInfo* GetImageInfo(ResourceGroup* res_group, uint32 image_index)
{
    return &res_group->image_infos[image_index];
}

static ImageViewInfo* GetImageViewInfo(ResourceGroup* res_group, uint32 image_index)
{
    return &res_group->image_view_infos[image_index];
}

static ImageState* GetImageState(ResourceGroup* res_group, uint32 image_index)
{
    return &res_group->image_states[image_index];
}

static ImageFrameState* GetImageFrameState(ResourceGroup* res_group, uint32 image_index, uint32 frame_index)
{
    uint32 frame_offset = GetImageState(res_group, image_index)->frame_stride * frame_index;
    return &res_group->image_frame_states[frame_offset + image_index];
}

static bool AlignmentDesc(uint32* a_image_index, uint32* b_image_index, ResourceGroup* res_group)
{
    return GetImageState(res_group, *a_image_index)->alignment >=
           GetImageState(res_group, *b_image_index)->alignment;
}

/// Interface
////////////////////////////////////////////////////////////
static void InitResourceGroups(const Allocator* allocator, uint32 max_resource_groups)
{
    InitArray(&g_res_groups, allocator, max_resource_groups);
}

static ResourceGroupHnd CreateResourceGroup(const Allocator* allocator, ResourceGroupInfo* info)
{
    ResourceGroupHnd hnd = { .index = g_res_groups.count };
    ++g_res_groups.count;

    ResourceGroup* res_group = GetResourceGroup(hnd);
    uint32 frame_count = GetFrameCount();

    res_group->frame_count         = frame_count;

    res_group->max_buffers         = info->max_buffers;
    res_group->buffer_count        = 0;
    res_group->buffer_infos        = Allocate<BufferInfo>      (allocator, info->max_buffers);
    res_group->buffer_states       = Allocate<BufferState>     (allocator, info->max_buffers);
    res_group->buffer_frame_states = Allocate<BufferFrameState>(allocator, info->max_buffers * frame_count);

    res_group->max_images          = info->max_images;
    res_group->image_count         = 0;
    res_group->image_infos         = Allocate<ImageInfo>      (allocator, info->max_images);
    res_group->image_view_infos    = Allocate<ImageViewInfo>  (allocator, info->max_images);
    res_group->image_states        = Allocate<ImageState>     (allocator, info->max_images);
    res_group->image_frame_states  = Allocate<ImageFrameState>(allocator, info->max_images * frame_count);

    return hnd;
}

static void InitResources(ResourceGroupHnd res_group_hnd, Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);

    VkDevice device = GetDevice();
    PhysicalDevice* physical_device = GetPhysicalDevice();
    QueueFamilies* queue_families = &physical_device->queue_families;
    ResourceGroup* res_group = GetResourceGroup(res_group_hnd);
    VkResult res = VK_SUCCESS;

    // Initialize resource state and sort into arrays per memory type.
    auto mem_buffer_indexes = CreateArray2D<uint32>(&frame.allocator, VK_MAX_MEMORY_TYPES, res_group->buffer_count);
    VkBufferUsageFlags buffer_usage[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 buffer_index = 0; buffer_index < res_group->buffer_count; ++buffer_index)
    {
        BufferInfo* buffer_info = GetBufferInfo(res_group, buffer_index);
        BufferState* buffer_state = GetBufferState(res_group, buffer_index);

        if (buffer_info->per_frame)
        {
            buffer_state->frame_stride = res_group->max_buffers;
            buffer_state->frame_count  = res_group->frame_count;
        }
        else
        {
            buffer_state->frame_stride = 0;
            buffer_state->frame_count  = 1;
        }

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

        Push(mem_buffer_indexes, buffer_state->mem_index, buffer_index);

        // Append buffer usage for creating associated buffer memory.
        buffer_usage[buffer_state->mem_index] |= buffer_info->usage;
    }
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 buffer_count = GetCount(mem_buffer_indexes, mem_index);
        if (buffer_count == 0) { continue; }
        InsertionSortColumn(mem_buffer_indexes, mem_index, OffsetAlignmentDesc, res_group);
    }

    auto mem_image_indexes = CreateArray2D<uint32>(&frame.allocator, VK_MAX_MEMORY_TYPES, res_group->image_count);
    for (uint32 image_index = 0; image_index < res_group->image_count; ++image_index)
    {
        ImageInfo* image_info = GetImageInfo(res_group, image_index);
        ImageState* image_state = GetImageState(res_group, image_index);

        if (image_info->per_frame)
        {
            image_state->frame_stride = res_group->max_images;
            image_state->frame_count  = res_group->frame_count;
        }
        else
        {
            image_state->frame_stride = 0;
            image_state->frame_count  = 1;
        }

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
        info.initialLayout = image_info->initial_layout;
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
            res = vkCreateImage(device, &info, NULL, &GetImageFrameState(res_group, image_index, frame_index)->image);
            Validate(res, "vkCreateImage() failed");
        }

        // Get image memory info.
        VkMemoryRequirements mem_requirements = {};
        vkGetImageMemoryRequirements(device, GetImageFrameState(res_group, image_index, 0)->image, &mem_requirements);

        uint32 mem_index = GetCapableMemoryTypeIndex(&mem_requirements, image_info->mem_properties);
        image_state->mem_index = mem_index;
        image_state->size      = mem_requirements.size;
        image_state->alignment = mem_requirements.alignment;

        Push(mem_image_indexes, image_state->mem_index, image_index);
    }
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 image_count = GetCount(mem_image_indexes, mem_index);
        if (image_count == 0) { continue; }
        InsertionSortColumn(mem_image_indexes, mem_index, AlignmentDesc, res_group);
    }

    // Adjust memory size, calculate offsets, and create buffer for each memory type.
    VkMemoryType* memory_types = physical_device->mem_properties.memoryTypes;
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 buffer_count = GetCount(mem_buffer_indexes, mem_index);
        uint32 image_count  = GetCount(mem_image_indexes,  mem_index);
        if (buffer_count == 0 && image_count == 0)
        {
            continue;
        }

        uint32* buffer_indexes = GetColumn(mem_buffer_indexes, mem_index);
        uint32* image_indexes  = GetColumn(mem_image_indexes,  mem_index);
        ResourceMemory* mem = GetMemory(res_group, mem_index);

        // Adjust memory size & calculate offsets for buffers.
        CTK_ITER_PTR(buffer_index, buffer_indexes, buffer_count)
        {
            BufferInfo* buffer_info = GetBufferInfo(res_group, *buffer_index);
            BufferState* buffer_state = GetBufferState(res_group, *buffer_index);
            for (uint32 frame_index = 0; frame_index < buffer_state->frame_count; ++frame_index)
            {
                mem->size = Align(mem->size, buffer_info->offset_alignment);
                GetBufferFrameState(res_group, *buffer_index, frame_index)->offset = mem->size;
                mem->size += buffer_info->size;
            }
        }

        // Create Vulkan buffer to be used by all buffers for this memory type.
        if (buffer_count > 0)
        {
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
            res = vkCreateBuffer(device, &create_info, NULL, &res_group->buffers[mem_index]);
            Validate(res, "vkCreateBuffer() failed");
        }

        // Adjust memory size & calculate offsets for images.
        CTK_ITER_PTR(image_index, image_indexes, image_count)
        {
            ImageState* image_state = GetImageState(res_group, *image_index);
            for (uint32 frame_index = 0; frame_index < image_state->frame_count; ++frame_index)
            {
                mem->size = Align(mem->size, image_state->alignment);
                GetImageFrameState(res_group, *image_index, frame_index)->offset = mem->size;
                mem->size += image_state->size;
            }
        }

        // Allocate memory.
        mem->device = AllocateDeviceMemory(mem_index, mem->size, NULL);
        mem->properties = memory_types[mem_index].propertyFlags;

        // Map host visible memory.
        if (mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            vkMapMemory(device, mem->device, 0, mem->size, 0, (void**)&mem->host);
        }

        // Bind buffers to memory.
        if (buffer_count > 0)
        {
            res = vkBindBufferMemory(device, res_group->buffers[mem_index], mem->device, 0);
            Validate(res, "vkBindBufferMemory() failed");
        }

        // Bind images to memory.
        CTK_ITER_PTR(image_index, image_indexes, image_count)
        {
            ImageState* image_state = GetImageState(res_group, *image_index);
            for (uint32 frame_index = 0; frame_index < image_state->frame_count; ++frame_index)
            {
                ImageFrameState* image_frame_state = GetImageFrameState(res_group, *image_index, frame_index);
                res = vkBindImageMemory(device, image_frame_state->image, mem->device, image_frame_state->offset);
                Validate(res, "vkBindImageMemory() failed");
            }
        }
    }

    // Create image views.
    for (uint32 image_index = 0; image_index < res_group->image_count; ++image_index)
    {
        ImageViewInfo* image_view_info = GetImageViewInfo(res_group, image_index);
        ImageState* image_state = GetImageState(res_group, image_index);
        for (uint32 frame_index = 0; frame_index < image_state->frame_count; ++frame_index)
        {
            ImageFrameState* image_frame_state = GetImageFrameState(res_group, image_index, frame_index);
            VkImageViewCreateInfo view_create_info =
            {
                .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext            = NULL,
                .flags            = image_view_info->flags,
                .image            = image_frame_state->image,
                .viewType         = image_view_info->type,
                .format           = image_view_info->format,
                .components       = image_view_info->components,
                .subresourceRange = image_view_info->subresource_range,
            };
            res = vkCreateImageView(device, &view_create_info, NULL, &image_frame_state->view);
            Validate(res, "vkCreateImageView() failed");
        }
    }
}

static void DestroyResources(ResourceGroupHnd res_group_hnd)
{
    VkDevice device = GetDevice();
    ResourceGroup* res_group = GetResourceGroup(res_group_hnd);

    // Destroy resources.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        if (GetMemory(res_group, mem_index)->size == 0) { continue; }
        vkDestroyBuffer(device, res_group->buffers[mem_index], NULL);
    }
    for (uint32 image_index = 0; image_index < res_group->image_count; ++image_index)
    {
        for (uint32 frame_index = 0; frame_index < GetImageState(res_group, image_index)->frame_count; ++frame_index)
        {
            ImageFrameState* image_frame_state = GetImageFrameState(res_group, image_index, frame_index);
            vkDestroyImageView(device, image_frame_state->view, NULL);
            vkDestroyImage(device, image_frame_state->image, NULL);
        }
    }

    // Free memory.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        ResourceMemory* mem = GetMemory(res_group, mem_index);
        if (mem->size == 0) { continue; }

        if (mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            vkUnmapMemory(device, mem->device);
        }
        vkFreeMemory(device, mem->device, NULL);
    }

    // Clear resource group.
    uint32 max_buffers = res_group->max_buffers;
    uint32 max_images  = res_group->max_images;
    uint32 frame_count = res_group->frame_count;

    res_group->buffer_count = 0;
    res_group->image_count  = 0;

    memset(res_group->buffer_infos,        0, sizeof(BufferInfo)       * max_buffers);
    memset(res_group->buffer_states,       0, sizeof(BufferState)      * max_buffers);
    memset(res_group->buffer_frame_states, 0, sizeof(BufferFrameState) * max_buffers * frame_count);
    memset(res_group->buffers,             0, sizeof(VkBuffer)         * VK_MAX_MEMORY_TYPES);

    memset(res_group->image_infos,         0, sizeof(ImageInfo)        * max_images);
    memset(res_group->image_view_infos,    0, sizeof(ImageViewInfo)    * max_images);
    memset(res_group->image_states,        0, sizeof(ImageState)       * max_images);
    memset(res_group->image_frame_states,  0, sizeof(ImageFrameState)  * max_images * frame_count);

    memset(res_group->mems,                0, sizeof(ResourceMemory)   * VK_MAX_MEMORY_TYPES);
}

/// Debug
////////////////////////////////////////////////////////////
static void LogResourceGroups()
{
    for (uint32 res_group_index = 0; res_group_index < g_res_groups.count; ++res_group_index)
    {
        ResourceGroup* res_group = GetResourceGroup(res_group_index);
        PrintLine("resource group %u:", res_group_index);

        for (uint32 buffer_index = 0; buffer_index < res_group->buffer_count; ++buffer_index)
        {
            BufferInfo* info = GetBufferInfo(res_group, buffer_index);
            BufferState* state = GetBufferState(res_group, buffer_index);
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
                BufferFrameState* frame_state = GetBufferFrameState(res_group, buffer_index, frame_index);
                PrintLine("            frame %u:", frame_index);
                PrintLine("                offset: %llu", frame_state->offset);
                PrintLine("                index:  %llu", frame_state->index);
            }
            PrintLine();
        }

        for (uint32 image_index = 0; image_index < res_group->image_count; ++image_index)
        {
            ImageInfo* info = GetImageInfo(res_group, image_index);
            ImageViewInfo* view_info = GetImageViewInfo(res_group, image_index);
            ImageState* state = GetImageState(res_group, image_index);
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

            PrintLine("            state:");
            PrintLine("                frame_stride: %u", state->frame_stride);
            PrintLine("                frame_count:  %u", state->frame_count);
            PrintLine("                mem_index:    %u", state->mem_index);
            PrintLine("                size:         %llu", state->size);
            PrintLine("                alignment:    %llu", state->alignment);
            PrintLine("            frame_states:");
            for (uint32 frame_index = 0; frame_index < state->frame_count; ++frame_index)
            {
                ImageFrameState* frame_state = GetImageFrameState(res_group, image_index, frame_index);
                PrintLine("                frame %u:", frame_index);
                PrintLine("                    image:  %p", frame_state->image);
                PrintLine("                    view:   %p", frame_state->view);
                PrintLine("                    offset: %llu", frame_state->offset);
            }
            PrintLine();
        }

        for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
        {
            ResourceMemory* mem = GetMemory(res_group, mem_index);
            if (mem->size == 0)
            {
                return;
            }
            PrintLine("   memory %u:", mem_index);
            PrintLine("        size:   %llu", mem->size);
            PrintLine("        device: 0x%p", mem->device);
            PrintLine("        host:   0x%p", mem->host);
            PrintLine("        properties: ");
            PrintMemoryPropertyFlags(mem->properties, 3);
        }

        PrintLine();
    }
}
