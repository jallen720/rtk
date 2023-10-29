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
static void ValidateImageHnd(ImageHnd hnd, const char* action)
{
    ResourceGroup* res_group = GetResourceGroup();
    if (hnd.index >= res_group->image_count)
    {
        CTK_FATAL("%s: image handle index %u exceeds max image count of %u",
                  action, hnd.index, res_group->image_count);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static ImageHnd CreateImage(ImageInfo* info, ImageViewInfo* view_info)
{
    ResourceGroup* res_group = GetResourceGroup();
    if (res_group->image_count >= res_group->max_images)
    {
        CTK_FATAL("can't create image: already at max of %u", res_group->max_images);
    }

    // Copy info.
    ImageHnd hnd = { .index = res_group->image_count };
    ++res_group->image_count;
    *GetImageInfo(hnd.index) = *info;
    *GetImageViewInfo(hnd.index) = *view_info;

    return hnd;
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
    VkImage image = GetImageFrameState(image_hnd.index, frame_index)->image;
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
            .imageExtent = GetImageInfo(image_hnd.index)->extent,
        };
        vkCmdCopyBufferToImage(GetTempCommandBuffer(), GetBuffer(image_data_buffer_hnd.index), image,
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

static ImageInfo* GetInfo(ImageHnd hnd)
{
    ValidateImageHnd(hnd, "can't get image info");
    return GetImageInfo(hnd.index);
}

static ImageViewInfo* GetImageViewInfo(ImageHnd hnd)
{
    ValidateImageHnd(hnd, "can't get image view info");
    return GetImageViewInfo(hnd.index);
}

static VkImageView GetImageView(ImageHnd hnd, uint32 frame_index)
{
    ValidateImageHnd(hnd, "can't get image");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count)
    return GetImageFrameState(hnd.index, frame_index)->view;
}
