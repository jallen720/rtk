/// Data
////////////////////////////////////////////////////////////
struct ImageHnd { uint32 index; };

struct Image
{
    VkExtent3D        extent;
    VkImageUsageFlags usage;
    bool              per_frame;

    // Properties used to select resource.
    VkMemoryPropertyFlags mem_properties;
    VkImageCreateFlags    flags;
    VkImageType           type;
    VkFormat              format;
    uint32                mip_levels;
    uint32                array_layers;
    VkSampleCountFlagBits samples;
    VkImageTiling         tiling;
};
using ImageInfo = Image; // Stored information for Image is the same information used to create it.

struct ImageMemory
{
    VkDeviceSize   size;
    VkDeviceMemory device_mem;
    uint8*         host_mem;

    // Unique combination of properties for this resource.
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
    uint32       max_images;
    uint32       image_count;
    uint32       frame_count;

    Image*       images;      // size: max_images
    uint32*      mem_indexes; // size: max_images
    VkImage*     hnds;        // size: max_images * frame_count
    VkImageView* views;       // size: max_images * frame_count

    ImageMemory  mems[VK_MAX_MEMORY_TYPES];
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
        Image* image = g_image_state.images + image_index;
        uint32 frame_count = image->per_frame ? GetFrameCount() : 1;

        PrintLine("   [%2u] extent:       { w: %u, h: %u }", image_index, image->extent.width, image->extent.height);
        PrintLine("        per_frame:    %s", image->per_frame ? "true" : "false");

        PrintLine("        type:         %s", VkImageTypeName(image->type));
        PrintLine("        format:       %s", VkFormatName(image->format));
        PrintLine("        mip_levels:   %u", image->mip_levels);
        PrintLine("        array_layers: %u", image->array_layers);
        PrintLine("        samples:      %s", VkSampleCountName(image->samples));
        PrintLine("        tiling:       %s", VkImageTilingName(image->tiling));
        PrintLine("        mem_index:    %u", g_image_state.mem_indexes[image_index]);

        PrintLine("        mem_properties:");
        PrintMemoryPropertyFlags(image->mem_properties, 3);
        PrintLine("        flags:");
        PrintImageCreateFlags(image->flags, 3);
        PrintLine("        usage:");
        PrintImageUsageFlags(image->usage, 3);
        PrintLine("        hnds:");
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            uint32 frame_offset = frame_index * g_image_state.max_images;
            PrintLine("            [%u] %p", frame_index, g_image_state.hnds[frame_offset + image_index]);
        }
        PrintLine("        views:");
        for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            uint32 frame_offset = frame_index * g_image_state.max_images;
            PrintLine("            [%u] %p", frame_index, g_image_state.views[frame_offset + image_index]);
        }
        PrintLine();
    }
}

static void LogImageMems()
{
    PrintLine("image mems:");
    for (uint32 i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
    {
        // ImageMemory* image_mem = GetPtr(&g_image_state.mems, i);
        // PrintLine("   [%2u] hnd:        0x%p", i, image_mem->hnd);
        // PrintLine("        size:       %llu", image_mem->size);
        // PrintLine("        device_mem: 0x%p", image_mem->device_mem);
        // PrintLine("        host_mem:   0x%p", image_mem->host_mem);
        // PrintLine("        mem_properties:");
        // PrintMemoryPropertyFlags(image_mem->mem_properties, 3);
        // PrintLine("        usage:");
        // PrintImageUsageFlags(image_mem->usage, 3);
        // PrintLine();
    }
}

// /// Utils
// ////////////////////////////////////////////////////////////
// static bool ImageMemAlignmentDesc(Image* a, Image* b)
// {
//     return a->mem_requirements.alignment > b->mem_requirements.alignment;
// }

/// Interface
////////////////////////////////////////////////////////////
static void InitImageModule(const Allocator* allocator, uint32 max_images)
{
    uint32 frame_count = GetFrameCount();

    g_image_state.max_images  = max_images;
    g_image_state.image_count = 0;
    g_image_state.frame_count = frame_count;

    g_image_state.images      = Allocate<Image>      (allocator, max_images);
    g_image_state.mem_indexes = Allocate<uint32>     (allocator, max_images);
    g_image_state.hnds        = Allocate<VkImage>    (allocator, max_images * frame_count);
    g_image_state.views       = Allocate<VkImageView>(allocator, max_images * frame_count);

}

static ImageHnd CreateImage(ImageInfo* info)
{
    if (g_image_state.image_count >= g_image_state.max_images)
    {
        CTK_FATAL("can't create image: already at max of %u images", g_image_state.max_images);
    }

    ImageHnd hnd = { .index = g_image_state.image_count };
    ++g_image_state.image_count;
    Image* image = g_image_state.images + hnd.index;
    *image = *info;

    // Validate image info against physical device limits.
    VkPhysicalDeviceLimits* physical_device_limits = &GetPhysicalDevice()->properties.limits;
    if (info->type == VK_IMAGE_TYPE_3D)
    {
        if (info->extent.depth > physical_device_limits->maxImageDimension3D)
        {
            CTK_FATAL("can't create image: image depth of %u exceeds max 3D image dimension of %u",
                      info->extent.depth, physical_device_limits->maxImageDimension3D);
        }
        if (info->extent.height > physical_device_limits->maxImageDimension3D)
        {
            CTK_FATAL("can't create image: image height of %u exceeds max 3D image dimension of %u",
                      info->extent.height, physical_device_limits->maxImageDimension3D);
        }
        if (info->extent.width > physical_device_limits->maxImageDimension3D)
        {
            CTK_FATAL("can't create image: image width of %u exceeds max 3D image dimension of %u",
                      info->extent.width, physical_device_limits->maxImageDimension3D);
        }
    }
    else if (info->type == VK_IMAGE_TYPE_2D)
    {
        if (info->extent.height > physical_device_limits->maxImageDimension2D)
        {
            CTK_FATAL("can't create image: image height of %u exceeds max 2D image dimension of %u",
                      info->extent.height, physical_device_limits->maxImageDimension2D);
        }
        if (info->extent.width > physical_device_limits->maxImageDimension2D)
        {
            CTK_FATAL("can't create image: image width of %u exceeds max 2D image dimension of %u",
                      info->extent.width, physical_device_limits->maxImageDimension2D);
        }
    }
    else
    {
        if (info->extent.width > physical_device_limits->maxImageDimension1D)
        {
            CTK_FATAL("can't create image: image width of %u exceeds max 1D image dimension of %u",
                      info->extent.width, physical_device_limits->maxImageDimension1D);
        }
    }

    if (info->array_layers > physical_device_limits->maxImageArrayLayers)
    {
        CTK_FATAL("can't create image: image width of %u exceeds max 1D image dimension of %u",
                  info->array_layers, physical_device_limits->maxImageDimension1D);
    }

    return hnd;
}

// {
//     VkDevice device = global_ctx.device;
//     VkResult res = VK_SUCCESS;

//     // Create image.
//     VkImageCreateInfo create_info = {};
//     create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//     create_info.pNext         = NULL;
//     create_info.flags         = 0;
//     create_info.imageType     = info->type;
//     create_info.format        = info->format;
//     create_info.extent        = info->extent;
//     create_info.mipLevels     = info->mip_levels;
//     create_info.arrayLayers   = info->array_layers;
//     create_info.samples       = info->samples;
//     create_info.tiling        = info->tiling;
//     create_info.usage         = info->usage;
//     create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

//     // Check if sharing mode needs to be concurrent due to separate graphics & present queue families.
//     if (queue_families->graphics != queue_families->present)
//     {
//         create_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
//         create_info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(uint32);
//         create_info.pQueueFamilyIndices   = (uint32*)queue_families;
//     }
//     else
//     {
//         create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
//         create_info.queueFamilyIndexCount = 0;
//         create_info.pQueueFamilyIndices   = NULL;
//     }

//     res = vkCreateImage(device, info, NULL, &image->hnd);
//     Validate(res, "vkCreateImage() failed");
//     vkGetImageMemoryRequirements(device, image->hnd, &image->mem_requirements);

//     // Cache image info (used for view creation).
//     image->type   = info->imageType;
//     image->format = info->format;
//     image->extent = info->extent;

// }

static Image* GetImage(ImageHnd hnd)
{
    if (hnd.index >= g_image_state.image_count)
    {
        CTK_FATAL("can't get image for handle: image index %u exceeds count of %u",
                  hnd.index, g_image_state.image_count);
    }
    return g_image_state.images + hnd.index;
}

static void AllocateImages()
{
    // VkResult res = VK_SUCCESS;
    // VkDevice device = global_ctx.device;

    // // Ensure images are sorted in descending order of alignment.
    // InsertionSort(g_image_state.images, g_image_state.image_count, ImageMemAlignmentDesc);

    // // Set memory type sizes based on size of images that will be backed with that memory type.
    // CTK_ITER_PTR(image, g_image_state.images, g_image_state.image_count)
    // {
    //     image->mem_index = GetCapableMemoryTypeIndex(&image->mem_requirements, mem_properties);
    //     GetPtr(&g_image_state.mems, image->mem_index)->size += image->mem_requirements.size;
    // }

    // // Allocate memory for each memory type.
    // for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    // {
    //     ImageMemory* mem = g_image_state.mems + mem_index;
    //     if (mem->size > 0)
    //     {
    //         mem->hnd = AllocateDeviceMemory(mem_index, mem->size, NULL);
    //     }
    // }

    // // Bind images to memory.
    // VkDeviceSize mem_offsets[VK_MAX_MEMORY_TYPES] = {};
    // for (uint32 i = 0; i < g_image_state.image_count; ++i)
    // {
    //     Image* image = g_image_state.images + i;
    //     res = vkBindImageMemory(device, image->hnd, g_image_state.mems[image->mem_index].hnd,
    //                             mem_offsets[image->mem_index]);
    //     Validate(res, "vkBindImageMemory() failed");
    //     mem_offsets[image->mem_index] += image->mem_requirements.size;
    // }

    // // Create default views now that images have been backed with memory.
    // CTK_ITER_PTR(image, g_image_state.images, g_image_state.image_count)
    // {
    //     // Create default view.
    //     VkImageViewCreateInfo view_info =
    //     {
    //         .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    //         .pNext      = NULL,
    //         .flags      = 0,
    //         .image      = image->hnd,
    //         .viewType   = image->type == VK_IMAGE_TYPE_1D ? VK_IMAGE_VIEW_TYPE_1D :
    //                       image->type == VK_IMAGE_TYPE_2D ? VK_IMAGE_VIEW_TYPE_2D :
    //                       image->type == VK_IMAGE_TYPE_3D ? VK_IMAGE_VIEW_TYPE_3D :
    //                       VK_IMAGE_VIEW_TYPE_2D,
    //         .format     = image->format,
    //         .components =
    //         {
    //             .r = VK_COMPONENT_SWIZZLE_IDENTITY,
    //             .g = VK_COMPONENT_SWIZZLE_IDENTITY,
    //             .b = VK_COMPONENT_SWIZZLE_IDENTITY,
    //             .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    //         },
    //         .subresourceRange =
    //         {
    //             .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
    //             .baseMipLevel   = 0,
    //             .levelCount     = VK_REMAINING_MIP_LEVELS,
    //             .baseArrayLayer = 0,
    //             .layerCount     = VK_REMAINING_ARRAY_LAYERS,
    //         },
    //     };
    //     res = vkCreateImageView(device, &view_info, NULL, &image->default_view);
    //     Validate(res, "vkCreateImageView() failed");
    // }
}

// static void LoadImageData(ImageData* image_data, const char* path)
// {
//     image_data->data = stbi_load(path, &image_data->width, &image_data->height, &image_data->channel_count, 0);
//     if (image_data->data == NULL)
//     {
//         CTK_FATAL("failed to load image data from path '%s'", path);
//     }

//     image_data->size = image_data->width * image_data->height * image_data->channel_count;
// }

// static void DestroyImageData(ImageData* image_data)
// {
//     stbi_image_free(image_data->data);
//     *image_data = {};
// }

// static void LoadImage(ImageHnd image_hnd, BufferHnd image_data_buffer_hnd, uint32 frame_index, const char* path)
// {
//     Image* image = GetImage(image_hnd);

//     // Load image data and write to staging buffer.
//     ImageData image_data = {};
//     LoadImageData(&image_data, path);
//     WriteHostBuffer(image_data_buffer_hnd, frame_index, image_data.data, (VkDeviceSize)image_data.size);
//     DestroyImageData(&image_data);

//     // Copy image data from buffer memory to image memory.
//     BeginTempCommandBuffer();
//         // Transition image layout for use in shader.
//         VkImageMemoryBarrier pre_copy_mem_barrier =
//         {
//             .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//             .srcAccessMask       = 0,
//             .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
//             .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
//             .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//             .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//             .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//             .image               = image->hnd,
//             .subresourceRange =
//             {
//                 .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
//                 .baseMipLevel   = 0,
//                 .levelCount     = 1,
//                 .baseArrayLayer = 0,
//                 .layerCount     = 1,
//             },
//         };
//         vkCmdPipelineBarrier(global_ctx.temp_command_buffer,
//                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Source Stage Mask
//                              VK_PIPELINE_STAGE_TRANSFER_BIT, // Destination Stage Mask
//                              0, // Dependency Flags
//                              0, // Memory Barrier Count
//                              NULL, // Memory Barriers
//                              0, // Buffer Memory Barrier Count
//                              NULL, // Buffer Memory Barriers
//                              1, // Image Memory Barrier Count
//                              &pre_copy_mem_barrier); // Image Memory Barriers

//         VkBufferImageCopy copy =
//         {
//             .bufferOffset      = GetOffset(image_data_buffer_hnd, frame_index),
//             .bufferRowLength   = 0,
//             .bufferImageHeight = 0,
//             .imageSubresource =
//             {
//                 .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
//                 .mipLevel       = 0,
//                 .baseArrayLayer = 0,
//                 .layerCount     = 1,
//             },
//             .imageOffset =
//             {
//                 .x = 0,
//                 .y = 0,
//                 .z = 0,
//             },
//             .imageExtent = image->extent,
//         };
//         vkCmdCopyBufferToImage(global_ctx.temp_command_buffer, GetBufferMemory(image_data_buffer_hnd)->hnd, image->hnd,
//                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

//         // Transition image layout for use in shader.
//         VkImageMemoryBarrier post_copy_mem_barrier =
//         {
//             .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//             .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
//             .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
//             .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//             .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//             .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//             .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//             .image               = image->hnd,
//             .subresourceRange =
//             {
//                 .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
//                 .baseMipLevel   = 0,
//                 .levelCount     = 1,
//                 .baseArrayLayer = 0,
//                 .layerCount     = 1,
//             },
//         };
//         vkCmdPipelineBarrier(global_ctx.temp_command_buffer,
//                              VK_PIPELINE_STAGE_TRANSFER_BIT, // Source Stage Mask
//                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // Destination Stage Mask
//                              0, // Dependency Flags
//                              0, // Memory Barrier Count
//                              NULL, // Memory Barriers
//                              0, // Buffer Memory Barrier Count
//                              NULL, // Buffer Memory Barriers
//                              1, // Image Memory Barrier Count
//                              &post_copy_mem_barrier); // Image Memory Barriers
//     SubmitTempCommandBuffer();
// }
