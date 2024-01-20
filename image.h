/// Data
////////////////////////////////////////////////////////////
static constexpr VkComponentMapping RGBA_COMPONENT_SWIZZLE_IDENTITY =
{
    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
};

struct ImageData
{
    sint32 width;
    sint32 height;
    sint32 channel_count;
    sint32 size;
    uint8* data;
};

/// Utils
////////////////////////////////////////////////////////////
static void ValidateImage(ResourceGroup* res_group, uint32 image_index, const char* action)
{
    if (image_index >= res_group->image_count)
    {
        CTK_FATAL("%s: image index %u exceeds image count of %u", action, image_index, res_group->image_count);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void LoadImageData(ImageData* image_data, const char* path)
{
    image_data->data = stbi_load(path, &image_data->width, &image_data->height, &image_data->channel_count, 0);
    if (image_data->data == NULL)
    {
        CTK_FATAL("failed to load image data from path '%s'", path);
    }
    if (image_data->channel_count != 4)
    {
        CTK_FATAL("image data from path '%s' has %u channels but 4 are required", path, image_data->channel_count);
    }

    image_data->size = image_data->width * image_data->height * image_data->channel_count;
}

static uint32 MipLevels(ImageData* image_data)
{
    return ((uint32)Log2((float32)Max(image_data->width, image_data->height))) + 1;
}

static void DestroyImageData(ImageData* image_data)
{
    stbi_image_free(image_data->data);
    *image_data = {};
}

static void LoadImage(ImageHnd image_hnd, BufferHnd staging_buffer_hnd, uint32 frame_index,
                      VkDeviceSize size, uint8* data, VkDeviceSize offset)
{
    ResourceGroup* res_group = GetResourceGroup(image_hnd.group_index);
    ValidateImage(res_group, image_hnd.index, "can't load image");
    ValidateBuffer(res_group, staging_buffer_hnd.index, "can't load image from buffer");

    HostBufferWrite image_data_write =
    {
        .size       = size,
        .src_data   = data,
        .src_offset = offset,
        .dst_hnd    = staging_buffer_hnd,
    };
    WriteHostBuffer(&image_data_write, frame_index);

    // Copy image data from buffer memory to image memory.
    VkImage image = GetImageFrameState(res_group, image_hnd.index, frame_index)->image;
    BeginTempCommandBuffer();
        // Transition image layout for use in shader.
        VkImageMemoryBarrier pre_copy_mem_barrier =
        {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_NONE,
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
                             VK_PIPELINE_STAGE_TRANSFER_BIT,    // Destination Stage Mask
                             0,                                 // Dependency Flags
                             0,                                 // Memory Barrier Count
                             NULL,                              // Memory Barriers
                             0,                                 // Buffer Memory Barrier Count
                             NULL,                              // Buffer Memory Barriers
                             1,                                 // Image Memory Barrier Count
                             &pre_copy_mem_barrier);            // Image Memory Barriers

        VkBufferImageCopy copy =
        {
            .bufferOffset      = GetBufferFrameState(staging_buffer_hnd, frame_index)->res_mem_offset,
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
            .imageExtent = GetImageInfo(res_group, image_hnd.index)->extent,
        };
        vkCmdCopyBufferToImage(GetTempCommandBuffer(), GetBuffer(res_group, staging_buffer_hnd.index), image,
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
                             VK_PIPELINE_STAGE_TRANSFER_BIT,        // Source Stage Mask
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // Destination Stage Mask
                             0,                                     // Dependency Flags
                             0,                                     // Memory Barrier Count
                             NULL,                                  // Memory Barriers
                             0,                                     // Buffer Memory Barrier Count
                             NULL,                                  // Buffer Memory Barriers
                             1,                                     // Image Memory Barrier Count
                             &post_copy_mem_barrier);               // Image Memory Barriers
    SubmitTempCommandBuffer();
}

static void LoadImage(ImageHnd image_hnd, BufferHnd staging_buffer_hnd, uint32 frame_index, ImageData* image_data)
{
    LoadImage(image_hnd, staging_buffer_hnd, frame_index, (VkDeviceSize)image_data->size, image_data->data, (VkDeviceSize)0);
}

static VkImageView GetImageView(ImageHnd image_hnd, uint32 frame_index)
{
    ResourceGroup* res_group = GetResourceGroup(image_hnd.group_index);
    ValidateImage(res_group, image_hnd.index, "can't get image view");

    CTK_ASSERT(frame_index < res_group->frame_count)

    return GetImageFrameState(res_group, image_hnd.index, frame_index)->view;
}
