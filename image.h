/// Data
////////////////////////////////////////////////////////////
struct ImageMemoryType
{
    VkDeviceMemory mem;
    VkDeviceSize   size;
};

struct Image
{
    VkImage              hnd;
    VkMemoryRequirements mem_requirements;
    VkImageType          type;
    VkFormat             format;
    VkExtent3D           extent;
    VkImageView          default_view;
};

struct ImageData
{
    sint32 width;
    sint32 height;
    sint32 channel_count;
    sint32 size;
    uint8* data;
};

using ImageHnd = uint32;

static struct ImageState
{
    Image*          data;
    uint32*         mem_type_indexes;
    uint32          count;
    uint32          max;
    ImageMemoryType mem_types[VK_MAX_MEMORY_TYPES];
}
global_image_state;

/// Debug
////////////////////////////////////////////////////////////
static void LogImageState()
{
    PrintLine("images:");
    for (uint32 i = 0; i < global_image_state.count; ++i)
    {
        Image* image = global_image_state.data + i;
        PrintLine("    [%3u] hnd:       %llu", i, image->hnd);
        PrintLine("          view:      %llu", image->default_view);
        PrintLine("          extent:    { w: %u, h: %u, d: %u }",
                  image->extent.width,
                  image->extent.height,
                  image->extent.depth);
        PrintLine("          size:      %llu", image->mem_requirements.size);
        PrintLine("          alignment: %llu", image->mem_requirements.alignment);
        PrintLine();
    }

    PrintLine("memory types:");
    for (uint32 i = 0; i < global_image_state.count; ++i)
    {
        ImageMemoryType* mem_type = global_image_state.mem_types + i;
        PrintLine("    [%3u] hnd:    %llu", i, mem_type->mem);
        PrintLine("          size:   %u", mem_type->size);
        PrintLine();
    }
}

/// Utils
////////////////////////////////////////////////////////////
static bool ImageMemAlignmentDesc(Image* a, Image* b)
{
    return b->mem_requirements.alignment > a->mem_requirements.alignment;
}

/// Interface
////////////////////////////////////////////////////////////
static void InitImageState(Allocator* allocator, uint32 max)
{
    global_image_state.data             = Allocate<Image>(allocator, max);
    global_image_state.mem_type_indexes = Allocate<uint32>(allocator, max);
    global_image_state.count            = 0;
    global_image_state.max              = max;
}

static ImageHnd CreateImage(VkImageCreateInfo* info)
{
    if (global_image_state.count >= global_image_state.max)
    {
        CTK_FATAL("can't create image: already at max image count of %u", global_image_state.max);
    }

    ImageHnd image_hnd = global_image_state.count;
    Image* image = global_image_state.data + image_hnd;
    ++global_image_state.count;

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

    return image_hnd;
}

static void BackImagesWithMemory(VkMemoryPropertyFlags mem_properties)
{
    VkResult res = VK_SUCCESS;
    VkDevice device = global_ctx.device;

    // Ensure images are sorted in descending order of alignment.
    InsertionSort(global_image_state.data, global_image_state.count, ImageMemAlignmentDesc);

    // Set memory type sizes based on size of images that will be backed with that memory type.
    for (uint32 i = 0; i < global_image_state.count; ++i)
    {
        Image* image = global_image_state.data + i;
        uint32 mem_type_index = GetCapableMemoryTypeIndex(image->mem_requirements.memoryTypeBits, mem_properties);
        global_image_state.mem_type_indexes[i] = mem_type_index;
        global_image_state.mem_types[mem_type_index].size += image->mem_requirements.size;
    }

    // Allocate memory for each memory type.
    for (uint32 mem_type_index = 0; mem_type_index < VK_MAX_MEMORY_TYPES; ++mem_type_index)
    {
        ImageMemoryType* mem_type = global_image_state.mem_types + mem_type_index;
        if (mem_type->size > 0)
        {
            mem_type->mem = AllocateDeviceMemory(mem_type_index, mem_type->size, NULL);
        }
    }

    // Bind images to memory.
    VkDeviceSize mem_type_offsets[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 i = 0; i < global_image_state.count; ++i)
    {
        Image* image = global_image_state.data + i;
        uint32 mem_type_index = global_image_state.mem_type_indexes[i];
        res = vkBindImageMemory(device, image->hnd, global_image_state.mem_types[mem_type_index].mem,
                                mem_type_offsets[mem_type_index]);
        Validate(res, "vkBindImageMemory() failed");
        mem_type_offsets[mem_type_index] += image->mem_requirements.size;
    }

    // Create default views now that images have been backed with memory.
    CTK_ITER_PTR(image, global_image_state.data, global_image_state.count)
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

static void ValidateImageHnd(ImageHnd hnd)
{
    if (hnd >= global_image_state.count)
    {
        CTK_FATAL("can't access image for handle %u: handle exceeds image count of %u", hnd, global_image_state.count);
    }
}

static VkImage GetImage(ImageHnd hnd)
{
    ValidateImageHnd(hnd);
    return global_image_state.data[hnd].hnd;
}

static VkExtent3D GetImageExtent(ImageHnd hnd)
{
    ValidateImageHnd(hnd);
    return global_image_state.data[hnd].extent;
}

static VkImageView GetImageView(ImageHnd hnd)
{
    ValidateImageHnd(hnd);
    return global_image_state.data[hnd].default_view;
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

static void LoadImageData2(ImageHnd image_hnd, Buffer2* image_data_buffer, const char* path)
{
    VkImage image = GetImage(image_hnd);

    // Load image data and write to staging buffer.
    ImageData image_data = {};
    image_data.data = stbi_load(path, &image_data.width, &image_data.height, &image_data.channel_count, 0);
    if (image_data.data == NULL)
    {
        CTK_FATAL("failed to load image data from path '%s'", path);
    }
    image_data.size = image_data.width * image_data.height * image_data.channel_count;
    WriteHostBuffer2(image_data_buffer, image_data.data, (VkDeviceSize)image_data.size);
    stbi_image_free(image_data.data);

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
            .bufferOffset      = image_data_buffer->offset,
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
            .imageExtent = GetImageExtent(image_hnd),
        };
        vkCmdCopyBufferToImage(global_ctx.temp_command_buffer, image_data_buffer->hnd, image,
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

static VkSampler CreateSampler(VkSamplerCreateInfo* info)
{
    VkSampler sampler = VK_NULL_HANDLE;
    VkResult res = vkCreateSampler(global_ctx.device, info, NULL, &sampler);
    Validate(res, "vkCreateSampler() failed");
    return sampler;
}
