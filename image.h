/// Data
////////////////////////////////////////////////////////////
struct ImageHnd { uint32 index; };
struct ImageGroupHnd { uint32 index; };

struct Image
{
    VkExtent3D        extent;
    VkImageUsageFlags usage;
    VkDeviceSize      offset_alignment;
    bool              per_frame;

    // Properties used to select memory allocation.
    VkMemoryPropertyFlags mem_properties;
    VkImageCreateFlags    flags;
    VkImageType           type;
    VkFormat              format;
    uint32                mip_levels;
    uint32                array_layers;
    VkSampleCountFlagBits samples;
    VkImageTiling         tiling;
};

struct ImageMemory
{
    VkImage           hnd;
    VkDeviceSize      size;
    VkImageUsageFlags usage;
    VkDeviceMemory    device_mem;
    uint8*            host_mem;

    // Unique combination of properties for this memory allocation.
    VkMemoryPropertyFlags mem_properties;
    VkImageCreateFlags    flags;
    VkImageType           type;
    VkFormat              format;
    uint32                mip_levels;
    uint32                array_layers;
    VkSampleCountFlagBits samples;
    VkImageTiling         tiling;
};

static struct ImageState
{
    Image*        images;      // size: max_images
    uint32*       mem_indexes; // size: max_images
    VkImageView   views;       // size: max_images * frame_count
    VkDeviceSize* offsets;     // size: max_images * frame_count
    uint32        max_images;
    uint32        image_count;
    uint32        frame_count;

    FArray<ImageMemory, VK_MAX_MEMORY_TYPES> mems;
}
g_image_state;

struct ImageData
{
    sint32 width;
    sint32 height;
    sint32 channel_count;
    sint32 size;
    uint8* data;
};

/// Debug
////////////////////////////////////////////////////////////
static void LogImages()
{
    PrintLine("images:");
    for (uint32 image_index = 0; image_index < g_image_state.image_count; ++image_index)
    {
        Image* image = g_image_state.image + image_index;
        PrintLine("   [%2u] extent:           { w: %u, h: %u }", image_index, image->extent.widht, image->extent.height);
        PrintLine("        usage:");
        PrintBufferUsage(image->usage, 3);
        PrintLine("        offset_alignment: %llu", image->offset_alignment);
        PrintLine("        per_frame:        %s", image->per_frame ? "true" : "false");

        PrintLine("        mem_properties:");
        PrintMemoryProperties(image->mem_properties, 3);
        PrintLine("        flags:            %s", VkImageCreateName(image->flags));
        PrintLine("        type:             %s", VkImageTypeName(image->type));
        PrintLine("        format:           %s", VkFormatName(image->format));
        PrintLine("        mip_levels:       %u", image->mip_levels);
        PrintLine("        array_layers:     %u", image->array_layers);
        PrintLine("        samples:          %s", VkSampleCountName(image->samples));
        PrintLine("        tiling:           %s", VkImageTilingName(image->tiling));

        PrintLine("        mem_index:        %u", g_image_state.mem_indexes[image_index]);
        PrintLine("        offsets:");
        uint32 frame_count = image->per_frame ? GetFrameCount() : 1;
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            uint32 frame_offset = frame_index * g_image_state.max_buffers;
            PrintLine("            [%u] %llu", frame_index, g_image_state.offsets[frame_offset + image_index]);
        }
        PrintLine("        views:");
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            uint32 frame_offset = frame_index * g_image_state.max_buffers;
            PrintLine("            [%u] %p", frame_index, g_image_state.views[frame_offset + image_index]);
        }
        PrintLine();
    }
}

static void LogImageMems()
{
    PrintLine("image mems:");
    for (uint32 i = 0; i < g_image_state.mems.count; ++i)
    {
        BufferMemory* buffer_mem = GetPtr(&g_image_state.mems, i);
        PrintLine("   [%2u] hnd:        0x%p", i, buffer_mem->hnd);
        PrintLine("        size:       %llu", buffer_mem->size);
        PrintLine("        device_mem: 0x%p", buffer_mem->device_mem);
        PrintLine("        host_mem:   0x%p", buffer_mem->host_mem);
        PrintLine("        mem_properties:");
        PrintMemoryProperties(buffer_mem->mem_properties, 3);
        PrintLine("        usage:");
        PrintBufferUsage(buffer_mem->usage, 3);
        PrintLine();
    }
}

/// Utils
////////////////////////////////////////////////////////////
static bool ImageMemAlignmentDesc(Image* a, Image* b)
{
    return a->mem_requirements.alignment > b->mem_requirements.alignment;
}

static uint32 GetImageGroupIndex(ImageHnd image_hnd)
{
    return 0xFFFF & image_hnd.index;
}

static uint32 GetImageIndex(ImageHnd image_hnd)
{
    return image_hnd.index >> 16;
}

/// Interface
////////////////////////////////////////////////////////////
static void InitImageModule(const Allocator* allocator, uint32 max_image_groups)
{
    CTK_ASSERT(max_image_groups <= 0xFFFF);
    InitArray(&global_image_groups, allocator, max_image_groups);
}

static ImageGroupHnd CreateImageGroup(const Allocator* allocator, uint32 size)
{
    if (global_image_groups.count >= global_image_groups.size)
    {
        CTK_FATAL("can't create image group; already at max of %u", global_image_groups.size);
    }

    ImageGroupHnd image_group_hnd = { .index = global_image_groups.count };

    ImageGroup* image_group = Push(&global_image_groups);
    image_group->images = Allocate<Image>(allocator, size);
    image_group->count  = 0;
    image_group->size   = size;

    return image_group_hnd;
}

static ImageGroup* GetImageGroup(ImageGroupHnd image_group_hnd)
{
    if (image_group_hnd.index >= global_image_groups.count)
    {
        CTK_FATAL("can't get image group for image group handle: image group index %u exceeds count %u",
                  image_group_hnd.index, global_image_groups.count);
    }
    return GetPtr(&global_image_groups, image_group_hnd.index);
}

static ImageGroup* GetImageGroup(ImageHnd image_hnd)
{
    uint32 image_group_index = GetImageGroupIndex(image_hnd);
    if (image_group_index >= global_image_groups.count)
    {
        CTK_FATAL("can't get image group for image handle: image group index %u exceeds count %u",
                  image_group_index, global_image_groups.count);
    }
    return GetPtr(&global_image_groups, image_group_index);
}

static ImageHnd CreateImage(ImageGroupHnd image_group_hnd, VkImageCreateInfo* info)
{
    ImageGroup* image_group = GetImageGroup(image_group_hnd);
    if (image_group->count >= image_group->size)
    {
        CTK_FATAL("can't create image: already at max of %u in image group at index %u",
                  image_group->size, image_group_hnd.index);
    }

    uint32 image_index = image_group->count;
    Image* image = image_group->images + image_index;
    ++image_group->count;

    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;

    // Create image.
    res = vkCreateImage(device, info, NULL, &image->hnd);
    Validate(res, "vkCreateImage() failed");
    vkGetImageMemoryRequirements(device, image->hnd, &image->mem_requirements);

    // Cache image info (used for view creation).
    image->type   = info->imageType;
    image->format = info->format;
    image->extent = info->extent;

    return { .index = (image_index << 16) | image_group_hnd.index };
}

static Image* GetImage(ImageHnd image_hnd)
{
    ImageGroup* image_group = GetImageGroup(image_hnd);
    uint32 image_index = GetImageIndex(image_hnd);
    if (image_index >= image_group->count)
    {
        CTK_FATAL("can't get image for image handle: image index %u exceeds count of %u for image group at index %u",
                  image_index, image_group->count, image_group - global_image_groups.data);
    }
    return image_group->images + image_index;
}

static void AllocateImageGroup(ImageGroupHnd image_group_hnd)
{
    VkResult res = VK_SUCCESS;
    VkDevice device = global_ctx.device;
    ImageGroup* image_group = GetImageGroup(image_group_hnd);

    // Ensure images are sorted in descending order of alignment.
    InsertionSort(image_group->images, image_group->count, ImageMemAlignmentDesc);

    // Set memory type sizes based on size of images that will be backed with that memory type.
    CTK_ITER_PTR(image, image_group->images, image_group->count)
    {
        image->mem_index = GetCapableMemoryTypeIndex(&image->mem_requirements, mem_properties);
        image_group->mem[image->mem_index].size += image->mem_requirements.size;
    }

    // Allocate memory for each memory type.
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        ImageMemory* mem = image_group->mem + mem_index;
        if (mem->size > 0)
        {
            mem->hnd = AllocateDeviceMemory(mem_index, mem->size, NULL);
        }
    }

    // Bind images to memory.
    VkDeviceSize mem_offsets[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 i = 0; i < image_group->count; ++i)
    {
        Image* image = image_group->images + i;
        res = vkBindImageMemory(device, image->hnd, image_group->mem[image->mem_index].hnd,
                                mem_offsets[image->mem_index]);
        Validate(res, "vkBindImageMemory() failed");
        mem_offsets[image->mem_index] += image->mem_requirements.size;
    }

    // Create default views now that images have been backed with memory.
    CTK_ITER_PTR(image, image_group->images, image_group->count)
    {
        // Create default view.
        VkImageViewCreateInfo view_info =
        {
            .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext      = NULL,
            .flags      = 0,
            .image      = image->hnd,
            .viewType   = image->type == VK_IMAGE_TYPE_1D ? VK_IMAGE_VIEW_TYPE_1D :
                          image->type == VK_IMAGE_TYPE_2D ? VK_IMAGE_VIEW_TYPE_2D :
                          image->type == VK_IMAGE_TYPE_3D ? VK_IMAGE_VIEW_TYPE_3D :
                          VK_IMAGE_VIEW_TYPE_2D,
            .format     = image->format,
            .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        res = vkCreateImageView(device, &view_info, NULL, &image->default_view);
        Validate(res, "vkCreateImageView() failed");
    }
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
    Image* image = GetImage(image_hnd);

    // Load image data and write to staging buffer.
    ImageData image_data = {};
    LoadImageData(&image_data, path);
    WriteHostBuffer(image_data_buffer_hnd, frame_index, image_data.data, (VkDeviceSize)image_data.size);
    DestroyImageData(&image_data);

    // Copy image data from buffer memory to image memory.
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
            .image               = image->hnd,
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        vkCmdPipelineBarrier(global_ctx.temp_command_buffer,
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
            .imageExtent = image->extent,
        };
        vkCmdCopyBufferToImage(global_ctx.temp_command_buffer, GetBufferMemory(image_data_buffer_hnd)->hnd, image->hnd,
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
            .image               = image->hnd,
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        vkCmdPipelineBarrier(global_ctx.temp_command_buffer,
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
