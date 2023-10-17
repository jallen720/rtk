/// Data
////////////////////////////////////////////////////////////
struct ImageHnd { uint32 index; };

static constexpr VkComponentMapping RGBA_COMPONENT_SWIZZLE_IDENTITY =
{
    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
};

struct ImageInfo
{
    VkExtent3D            extent;
    VkImageUsageFlags     usage;
    bool                  per_frame;
    VkMemoryPropertyFlags mem_properties;
    VkImageCreateFlags    flags;
    VkImageType           type;
    VkFormat              format;
    uint32                mip_levels;
    uint32                array_layers;
    VkSampleCountFlagBits samples;
    VkImageTiling         tiling;
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
    VkDeviceSize offset;
    VkImage      hnd;
};

struct ImageMemory
{
    VkDeviceSize   size;
    VkDeviceMemory device_mem;
};

struct ImageState
{
    uint32         max_images;
    uint32         image_count;
    uint32         frame_count;

    ImageInfo*     image_infos;        // size: max_images
    uint32*        mem_indexes;        // size: max_images
    VkImage*       image_hnds;         // size: max_images * frame_count

    ImageViewInfo* default_view_infos; // size: max_images
    VkImageView*   default_view_hnds;  // size: max_images * frame_count

    ImageMemory    mems[VK_MAX_MEMORY_TYPES];
};

struct ImageData
{
    sint32 width;
    sint32 height;
    sint32 channel_count;
    sint32 size;
    uint8* data;
};

static ImageState g_image_state;

/// Utils
////////////////////////////////////////////////////////////
static uint32 GetFrameOffset(uint32 frame_index)
{
    return frame_index * g_image_state.max_images;
}

static bool AlignmentDesc(ImageMemoryInfo* a, ImageMemoryInfo* b)
{
    return a->alignment >= b->alignment;
}

/// Debug
////////////////////////////////////////////////////////////
static void LogImages()
{
    PrintLine("images:");
    for (uint32 image_index = 0; image_index < g_image_state.image_count; ++image_index)
    {
        ImageInfo* image_info = &g_image_state.image_infos[image_index];
        uint32 frame_count = image_info->per_frame ? GetFrameCount() : 1;

        PrintLine("   [%2u] extent:       { w: %u, h: %u, d: %u }",
                  image_index,
                  image_info->extent.width,
                  image_info->extent.height,
                  image_info->extent.depth);
        PrintLine("        per_frame:    %s", image_info->per_frame ? "true" : "false");

        PrintLine("        type:         %s", VkImageTypeName(image_info->type));
        PrintLine("        format:       %s", VkFormatName(image_info->format));
        PrintLine("        mip_levels:   %u", image_info->mip_levels);
        PrintLine("        array_layers: %u", image_info->array_layers);
        PrintLine("        samples:      %s", VkSampleCountName(image_info->samples));
        PrintLine("        tiling:       %s", VkImageTilingName(image_info->tiling));
        PrintLine("        mem_index:    %u", g_image_state.mem_indexes[image_index]);

        PrintLine("        mem_properties:");
        PrintMemoryPropertyFlags(image_info->mem_properties, 3);
        PrintLine("        flags:");
        PrintImageCreateFlags(image_info->flags, 3);
        PrintLine("        usage:");
        PrintImageUsageFlags(image_info->usage, 3);
        PrintLine("        image_hnds:");
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            PrintLine("            [%u] %p", frame_index,
                      g_image_state.image_hnds[GetFrameOffset(frame_index) + image_index]);
        }
        PrintLine();
    }
}

static void LogDefaultViews()
{
    PrintLine("default views:");
    for (uint32 image_index = 0; image_index < g_image_state.image_count; ++image_index)
    {
        ImageViewInfo* default_view_info = &g_image_state.default_view_infos[image_index];
        uint32 layer_count = default_view_info->subresource_range.layerCount;
        uint32 level_count = default_view_info->subresource_range.levelCount;
        PrintLine("   [%2u] type:   %s", image_index, VkImageViewTypeName(default_view_info->type));
        PrintLine("        format: %s", VkFormatName(default_view_info->format));
        PrintLine("        components:");
        PrintLine("            r: %s", VkComponentSwizzleName(default_view_info->components.r));
        PrintLine("            g: %s", VkComponentSwizzleName(default_view_info->components.g));
        PrintLine("            b: %s", VkComponentSwizzleName(default_view_info->components.b));
        PrintLine("            a: %s", VkComponentSwizzleName(default_view_info->components.a));
        PrintLine("        subresource_range:");
        PrintLine("            aspectMask:");
        PrintImageAspectFlags(default_view_info->subresource_range.aspectMask, 4);
        PrintLine("            baseMipLevel:   %u", default_view_info->subresource_range.baseMipLevel);
        if (level_count == VK_REMAINING_MIP_LEVELS)
        {
            PrintLine("            levelCount:     VK_REMAINING_MIP_LEVELS");
        }
        else
        {
            PrintLine("            levelCount:     %u", level_count);
        }
        PrintLine("            baseArrayLayer: %u", default_view_info->subresource_range.baseArrayLayer);
        if (layer_count == VK_REMAINING_ARRAY_LAYERS)
        {
            PrintLine("            layerCount:     VK_REMAINING_ARRAY_LAYERS");
        }
        else
        {
            PrintLine("            layerCount:     %u", layer_count);
        }

        PrintLine("        flags:");
        PrintImageViewCreateFlags(default_view_info->flags, 3);

        PrintLine("        default_view_hnds:");
        uint32 frame_count = g_image_state.image_infos[image_index].per_frame ? g_image_state.frame_count : 1;
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            PrintLine("            [%u] %p", frame_index,
                      g_image_state.default_view_hnds[GetFrameOffset(frame_index) + image_index]);
        }
        PrintLine();
    }
}

static void LogImageMemory()
{
    PrintLine("image memory:");
    for (uint32 i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
    {
        ImageMemory* image_mem = g_image_state.mems + i;
        if (image_mem->size == 0) { continue; }

        PrintLine("   [%2u] size:       %llu", i, image_mem->size);
        PrintLine("        device_mem: 0x%p", image_mem->device_mem);
        PrintLine();
    }
}

static void LogImageMemoryInfoArrays(Array<ImageMemoryInfo> mem_info_arrays[VK_MAX_MEMORY_TYPES])
{
    for (uint32 mem_type_index = 0; mem_type_index < VK_MAX_MEMORY_TYPES; ++mem_type_index)
    {
        Array<ImageMemoryInfo>* mem_infos = &mem_info_arrays[mem_type_index];
        if (mem_infos->count == 0)
        {
            continue;
        }
        PrintLine("mem-type %u:", mem_type_index);
        for (uint32 mem_info_index = 0; mem_info_index < mem_infos->count; ++mem_info_index)
        {
            ImageMemoryInfo* mem_info = GetPtr(mem_infos, mem_info_index);
            PrintLine("    [%2u] size:      %llu", mem_info_index, mem_info->size);
            PrintLine("         alignment: %llu", mem_info->alignment);
            PrintLine("         offset:    %llu", mem_info->offset);
            PrintLine("         hnd:       %p", mem_info->hnd);
            PrintLine();
        }
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitImageModule(const Allocator* allocator, uint32 max_images)
{
    CTK_ASSERT(max_images > 0);

    uint32 frame_count = GetFrameCount();

    g_image_state.max_images         = max_images;
    g_image_state.image_count        = 0;
    g_image_state.frame_count        = frame_count;

    g_image_state.image_infos        = Allocate<ImageInfo>    (allocator, max_images);
    g_image_state.mem_indexes        = Allocate<uint32>       (allocator, max_images);
    g_image_state.image_hnds         = Allocate<VkImage>      (allocator, max_images * frame_count);

    g_image_state.default_view_infos = Allocate<ImageViewInfo>(allocator, max_images);
    g_image_state.default_view_hnds  = Allocate<VkImageView>  (allocator, max_images * frame_count);

}

static ImageHnd CreateImage(ImageInfo* image_info, ImageViewInfo* default_view_info)
{
    CTK_ASSERT(g_image_state.image_count < g_image_state.max_images)

    // Copy info.
    ImageHnd hnd = { .index = g_image_state.image_count };
    ++g_image_state.image_count;
    g_image_state.image_infos[hnd.index] = *image_info;
    g_image_state.default_view_infos[hnd.index] = *default_view_info;

    return hnd;
}

static void AllocateImages(Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);

    VkDevice device = GetDevice();
    QueueFamilies* queue_families = &GetPhysicalDevice()->queue_families;
    VkResult res = VK_SUCCESS;

    // Create images and get their memory information.
    Array<ImageMemoryInfo> mem_info_arrays[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 image_index = 0; image_index < g_image_state.image_count; ++image_index)
    {
        ImageInfo* image_info = g_image_state.image_infos + image_index;

        // Create image.
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

        // Check if sharing mode needs to be concurrent due to separate graphics & present queue families.
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

        uint32 frame_count = image_info->per_frame ? g_image_state.frame_count : 1;
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            res = vkCreateImage(device, &info, NULL, &g_image_state.image_hnds[GetFrameOffset(frame_index) + image_index]);
            Validate(res, "vkCreateImage() failed");
        }
        // if (image_info->per_frame)
        // {
        //     for (uint32 frame_index = 0; frame_index < g_image_state.frame_count; ++frame_index)
        //     {
        //         res = vkCreateImage(device, &info, NULL, &g_image_state.image_hnds[GetFrameOffset(frame_index) + image_index]);
        //         Validate(res, "vkCreateImage() failed");
        //     }
        // }
        // else
        // {
        //     VkImage image_hnd = VK_NULL_HANDLE;
        //     res = vkCreateImage(device, &info, NULL, &image_hnd);
        //     Validate(res, "vkCreateImage() failed");
        //     for (uint32 frame_index = 0; frame_index < g_image_state.frame_count; ++frame_index)
        //     {
        //         g_image_state.image_hnds[GetFrameOffset(frame_index) + image_index] = image_hnd;
        //     }
        // }

        // Get image memory info.
        VkMemoryRequirements mem_requirements = {};
        vkGetImageMemoryRequirements(device, g_image_state.image_hnds[0 + image_index], &mem_requirements);
        uint32 mem_index = GetCapableMemoryTypeIndex(&mem_requirements, image_info->mem_properties);
        g_image_state.mem_indexes[image_index] = mem_index;

        // Push memory info to memory type array, initializing if necessary.
        Array<ImageMemoryInfo>* mem_info_array = mem_info_arrays + mem_index;
        if (mem_info_array->data == NULL)
        {
            InitArray(mem_info_array, &frame.allocator, g_image_state.image_count * g_image_state.frame_count);
        }
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            Push(mem_info_array,
                 {
                    .size      = mem_requirements.size,
                    .alignment = mem_requirements.alignment,
                    .offset    = 0, // Calculated during memory sizing below.
                    .hnd       = g_image_state.image_hnds[GetFrameOffset(frame_index) + image_index]
                 });
        }
    }

    // Calculate size of each image memory based on the required sizes and alignments of all images they will store;
    // image memory offsets are also calculated at this time. Then allocate device memory and bind images to their
    // respective offsets.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        Array<ImageMemoryInfo>* mem_infos = &mem_info_arrays[mem_index];
        if (mem_infos->count == 0)
        {
            continue;
        }

        // Sort memory information in descending order of alignment.
        InsertionSort(mem_infos, AlignmentDesc);
        ImageMemory* image_mem = &g_image_state.mems[mem_index];

        // Calculate image memory size and image offsets based on image size and alignment requirements.
        CTK_ITER(mem_info, mem_infos)
        {
            image_mem->size = MultipleOf(image_mem->size, mem_info->alignment);
            mem_info->offset = image_mem->size;
            image_mem->size += mem_info->size;
        }

        image_mem->device_mem = AllocateDeviceMemory(mem_index, image_mem->size, NULL);

        // Bind images to memory.
        CTK_ITER(mem_info, mem_infos)
        {
            res = vkBindImageMemory(device, mem_info->hnd, image_mem->device_mem, mem_info->offset);
            Validate(res, "vkBindImageMemory() failed");
        }
    }

    // Create default views now that images have been backed with memory.
    for (uint32 image_index = 0; image_index < g_image_state.image_count; ++image_index)
    {
        ImageViewInfo* default_view_info = &g_image_state.default_view_infos[image_index];
        uint32 frame_count = g_image_state.image_infos[image_index].per_frame ? g_image_state.frame_count : 1;
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            uint32 image_frame_index = GetFrameOffset(frame_index) + image_index;
            VkImageViewCreateInfo view_info =
            {
                .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext            = NULL,
                .flags            = default_view_info->flags,
                .image            = g_image_state.image_hnds[image_frame_index],
                .viewType         = default_view_info->type,
                .format           = default_view_info->format,
                .components       = default_view_info->components,
                .subresourceRange = default_view_info->subresource_range,
            };
            res = vkCreateImageView(device, &view_info, NULL, &g_image_state.default_view_hnds[image_frame_index]);
            Validate(res, "vkCreateImageView() failed");
        }
    }
}

static ImageInfo* GetImageInfo(ImageHnd hnd)
{
    CTK_ASSERT(hnd.index < g_image_state.image_count)
    return &g_image_state.image_infos[hnd.index];
}

static VkImage GetImage(ImageHnd hnd, uint32 frame_index)
{
    CTK_ASSERT(hnd.index < g_image_state.image_count)
    CTK_ASSERT(frame_index < g_image_state.frame_count)
    return g_image_state.image_hnds[GetFrameOffset(frame_index) + hnd.index];
}

static VkImageView GetDefaultView(ImageHnd hnd, uint32 frame_index)
{
    CTK_ASSERT(hnd.index < g_image_state.image_count)
    CTK_ASSERT(frame_index < g_image_state.frame_count)
    return g_image_state.default_view_hnds[GetFrameOffset(frame_index) + hnd.index];
}

static void LoadImageData(ImageData* image_data, const char* path)
{
    image_data->data = stbi_load(path, &image_data->width, &image_data->height, &image_data->channel_count, 0);
    if (image_data->data == NULL)
    {
        CTK_FATAL("failed to load image data from path '%s'", path);
    }

    image_data->size = image_data->width * image_data->height * image_data->channel_count;
}

static void DestroyImageData(ImageData* image_data)
{
    stbi_image_free(image_data->data);
    *image_data = {};
}

static void LoadImage(ImageHnd image_hnd, BufferHnd image_data_buffer_hnd, uint32 frame_index, const char* path)
{
    // Load image data and write to staging buffer.
    ImageData image_data = {};
    LoadImageData(&image_data, path);
    WriteHostBuffer(image_data_buffer_hnd, frame_index, image_data.data, (VkDeviceSize)image_data.size);
    DestroyImageData(&image_data);

    // Copy image data from buffer memory to image memory.
    VkImage image = GetImage(image_hnd, frame_index);
    BeginTempCommandBuffer();
        // Transition image layout for use in shader.
        VkImageMemoryBarrier pre_copy_mem_barrier =
        {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image,
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        vkCmdPipelineBarrier(GetTempCommandBuffer(),
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Source Stage Mask
                             VK_PIPELINE_STAGE_TRANSFER_BIT, // Destination Stage Mask
                             0, // Dependency Flags
                             0, // Memory Barrier Count
                             NULL, // Memory Barriers
                             0, // Buffer Memory Barrier Count
                             NULL, // Buffer Memory Barriers
                             1, // Image Memory Barrier Count
                             &pre_copy_mem_barrier); // Image Memory Barriers

        VkBufferImageCopy copy =
        {
            .bufferOffset      = GetOffset(image_data_buffer_hnd, frame_index),
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .imageOffset =
            {
                .x = 0,
                .y = 0,
                .z = 0,
            },
            .imageExtent = GetImageInfo(image_hnd)->extent,
        };
        vkCmdCopyBufferToImage(GetTempCommandBuffer(), GetBufferMemory(image_data_buffer_hnd)->hnd, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        // Transition image layout for use in shader.
        VkImageMemoryBarrier post_copy_mem_barrier =
        {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image,
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        vkCmdPipelineBarrier(GetTempCommandBuffer(),
                             VK_PIPELINE_STAGE_TRANSFER_BIT, // Source Stage Mask
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // Destination Stage Mask
                             0, // Dependency Flags
                             0, // Memory Barrier Count
                             NULL, // Memory Barriers
                             0, // Buffer Memory Barrier Count
                             NULL, // Buffer Memory Barriers
                             1, // Image Memory Barrier Count
                             &post_copy_mem_barrier); // Image Memory Barriers
    SubmitTempCommandBuffer();
}
