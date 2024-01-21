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

static uint32 GetMipLevels(ImageData* image_data)
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

    // Validate image's memory's format support linear filtering for mipmap generation.
    uint32 image_mem_index = GetImageState(res_group, image_hnd.index)->image_mem_index;
    VkFormat image_format = GetImageMemoryInfo(res_group, image_mem_index)->format;
    VkFormatProperties format_properties = {};
    vkGetPhysicalDeviceFormatProperties(GetPhysicalDevice()->hnd, image_format, &format_properties);
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        CTK_FATAL("can't load image: image's memory's format properties does not support "
                  "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT required for mipmap generation.");
    }

    HostBufferWrite image_data_write =
    {
        .size       = size,
        .src_data   = data,
        .src_offset = offset,
        .dst_hnd    = staging_buffer_hnd,
    };
    WriteHostBuffer(&image_data_write, frame_index);

    // Copy image data from buffer memory to image memory.
    VkBuffer staging_buffer = GetBuffer(res_group, staging_buffer_hnd.index);
    VkImage image = GetImageFrameState(res_group, image_hnd.index, frame_index)->image;
    ImageInfo* image_info = GetImageInfo(res_group, image_hnd.index);
    VkCommandBuffer temp_command_buffer = GetTempCommandBuffer();
    BeginTempCommandBuffer();
        // Transition all mip levels to transfer_write & transfer_dst.
        VkImageMemoryBarrier transition_transfer_dst =
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
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        vkCmdPipelineBarrier(temp_command_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Source Stage Mask
                             VK_PIPELINE_STAGE_TRANSFER_BIT,    // Destination Stage Mask
                             0,                                 // Dependency Flags
                             0, NULL,                           // Memory Barriers
                             0, NULL,                           // Buffer Memory Barriers
                             1, &transition_transfer_dst);      // Image Memory Barriers

        // Copy buffer data to image.
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
            .imageExtent = image_info->extent,
        };
        vkCmdCopyBufferToImage(temp_command_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &copy);

        // Blit mip images.
        sint32 mip_width  = image_info->extent.width;
        sint32 mip_height = image_info->extent.height;
        for (uint32 i = 1; i < image_info->mip_levels; i += 1)
        {
            // Transition previous mip level to transfer_read & transfer_src.
            VkImageMemoryBarrier transition_prev_mip_level =
            {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = image,
                .subresourceRange =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = i - 1,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
            vkCmdPipelineBarrier(temp_command_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, // Source Stage Mask
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, // Destination Stage Mask
                                 0,                              // Dependency Flags
                                 0, NULL,                        // Memory Barriers
                                 0, NULL,                        // Buffer Memory Barriers
                                 1, &transition_prev_mip_level); // Image Memory Barriers

            sint32 half_mip_width  = Max(1, mip_width  / 2);
            sint32 half_mip_height = Max(1, mip_height / 2);
            VkImageBlit image_blit =
            {
                .srcSubresource =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel       = i - 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
                .srcOffsets =
                {
                    { 0,         0,          0 },
                    { mip_width, mip_height, 1 },
                },
                .dstSubresource =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel       = i,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
                .dstOffsets =
                {
                    { 0,              0,               0 },
                    { half_mip_width, half_mip_height, 1 },
                },
            };
            vkCmdBlitImage(temp_command_buffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &image_blit,
                           VK_FILTER_LINEAR);

            mip_width  = half_mip_width;
            mip_height = half_mip_height;
        }

        // Transition mip levels 0 -> level_count - 1 to shader_read & shader_read_only_optimal.
        if (image_info->mip_levels > 1)
        {
            VkImageMemoryBarrier transition_shader_read_only_optimal =
            {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
                .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = image,
                .subresourceRange =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = image_info->mip_levels - 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
            vkCmdPipelineBarrier(temp_command_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,           // Source Stage Mask
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,    // Destination Stage Mask
                                 0,                                        // Dependency Flags
                                 0, NULL,                                  // Memory Barriers
                                 0, NULL,                                  // Buffer Memory Barriers
                                 1, &transition_shader_read_only_optimal); // Image Memory Barriers
        }

        // Transition last mip level to shader_read & shader_read_only_optimal.
        {
            VkImageMemoryBarrier transition_shader_read_only_optimal =
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
                    .baseMipLevel   = image_info->mip_levels - 1,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
            vkCmdPipelineBarrier(temp_command_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,           // Source Stage Mask
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,    // Destination Stage Mask
                                 0,                                        // Dependency Flags
                                 0, NULL,                                  // Memory Barriers
                                 0, NULL,                                  // Buffer Memory Barriers
                                 1, &transition_shader_read_only_optimal); // Image Memory Barriers
        }
    SubmitTempCommandBuffer();
}

static void LoadImage(ImageHnd image_hnd, BufferHnd staging_buffer_hnd, uint32 frame_index, ImageData* image_data)
{
    LoadImage(image_hnd, staging_buffer_hnd, frame_index, (VkDeviceSize)image_data->size, image_data->data,
              (VkDeviceSize)0);
}

static VkImageView GetImageView(ImageHnd image_hnd, uint32 frame_index)
{
    ResourceGroup* res_group = GetResourceGroup(image_hnd.group_index);
    ValidateImage(res_group, image_hnd.index, "can't get image view");

    CTK_ASSERT(frame_index < res_group->frame_count)

    return GetImageFrameState(res_group, image_hnd.index, frame_index)->view;
}
