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

/// Debug
////////////////////////////////////////////////////////////
static void LogImages()
{
    PrintLine("images:");
    for (uint32 image_index = 0; image_index < GetResourceGroup()->image_count; ++image_index)
    {
        ImageInfo* image_info = GetImageInfo(image_index);
        ResourceState* image_state = GetImageState(image_index);
        PrintLine("   [%2u] extent:       { w: %u, h: %u, d: %u }",
                  image_index,
                  image_info->extent.width,
                  image_info->extent.height,
                  image_info->extent.depth);
        PrintLine("        per_frame:    %s", image_info->per_frame ? "true" : "false");

        PrintLine("        flags:");
        PrintImageCreateFlags(image_info->flags, 3);
        PrintLine("        type:         %s", VkImageTypeName(image_info->type));
        PrintLine("        format:       %s", VkFormatName(image_info->format));
        PrintLine("        mip_levels:   %u", image_info->mip_levels);
        PrintLine("        array_layers: %u", image_info->array_layers);
        PrintLine("        samples:      %s", VkSampleCountName(image_info->samples));
        PrintLine("        tiling:       %s", VkImageTilingName(image_info->tiling));
        PrintLine("        mem_properties:");
        PrintMemoryPropertyFlags(image_info->mem_properties, 3);
        PrintLine("        usage:");
        PrintImageUsageFlags(image_info->usage, 3);

        PrintLine("        frame_stride: %u", image_state->frame_stride);
        PrintLine("        frame_count:  %u", image_state->frame_count);
        PrintLine("        mem_index:    %u", image_state->mem_index);
        PrintLine("        images:");
        for (uint32 frame_index = 0; frame_index < image_state->frame_count; ++frame_index)
        {
            PrintLine("            [%u] %p", frame_index, GetImage(image_index, frame_index));
        }
        PrintLine();
    }
}

static void LogDefaultViews()
{
    PrintLine("default views:");
    for (uint32 image_index = 0; image_index < GetResourceGroup()->image_count; ++image_index)
    {
        ImageViewInfo* default_view_info = GetDefaultViewInfo(image_index);
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

        PrintLine("        default_views:");
        for (uint32 frame_index = 0; frame_index < GetImageState(image_index)->frame_count; ++frame_index)
        {
            PrintLine("            [%u] %p", frame_index, GetDefaultView(image_index, frame_index));
        }
        PrintLine();
    }
}

/// Interface
////////////////////////////////////////////////////////////
static ImageHnd CreateImage(ImageInfo* image_info, ImageViewInfo* default_view_info)
{
    ResourceGroup* res_group = GetResourceGroup();
    if (res_group->image_count >= res_group->max_images)
    {
        CTK_FATAL("can't create image: already at max of %u", res_group->max_images);
    }

    // Copy info.
    ImageHnd hnd = { .index = res_group->image_count };
    ++res_group->image_count;
    *GetImageInfo(hnd.index) = *image_info;
    res_group->default_view_infos[hnd.index] = *default_view_info;
    ResourceState* image_state = GetImageState(hnd.index);
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

    return hnd;
}

static void AllocateImages(Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);

    VkDevice device = GetDevice();
    ResourceGroup* res_group = GetResourceGroup();
    VkResult res = VK_SUCCESS;

    // Create images and get their memory information.
    QueueFamilies* queue_families = &GetPhysicalDevice()->queue_families;
    auto mem_infos = CreateArray<ImageMemoryInfo>(&frame.allocator, res_group->image_count);
    uint32 mem_info_counts[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 image_index = 0; image_index < res_group->image_count; ++image_index)
    {
        ImageInfo* image_info = GetImageInfo(image_index);
        ResourceState* image_state = GetImageState(image_index);

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
            res = vkCreateImage(device, &info, NULL, GetImagePtr(image_index, frame_index));
            Validate(res, "vkCreateImage() failed");
        }

        // Get image memory info.
        VkMemoryRequirements mem_requirements = {};
        vkGetImageMemoryRequirements(device, GetImage(image_index, 0), &mem_requirements);

        uint32 mem_index = GetCapableMemoryTypeIndex(&mem_requirements, image_info->mem_properties);
        image_state->mem_index = mem_index;
        ++mem_info_counts[mem_index];

        ImageMemoryInfo* mem_info = Push(mem_infos);
        mem_info->size        = mem_requirements.size;
        mem_info->alignment   = mem_requirements.alignment;
        mem_info->stride      = Align(mem_requirements.size, mem_requirements.alignment);
        mem_info->image_index = image_index;
    }

    // Associate mem_infos with their memory types in descending order of alignment.
    InsertionSort(mem_infos, AlignmentDesc);
    Array<uint32> mem_info_index_arrays[VK_MAX_MEMORY_TYPES] = {};
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        uint32 mem_info_count = mem_info_counts[mem_index];
        if (mem_info_count == 0) { continue; }
        InitArray(&mem_info_index_arrays[mem_index], &frame.allocator, mem_info_count);
    }
    for (uint32 mem_info_index = 0; mem_info_index < mem_infos->count; ++mem_info_index)
    {
        uint32 mem_index = GetImageState(GetPtr(mem_infos, mem_info_index)->image_index)->mem_index;
        Push(&mem_info_index_arrays[mem_index], mem_info_index);
    }

    // Calculate size of each image memory based on the required sizes and alignments of all images they will store;
    // image memory offsets are also calculated at this time. Then allocate device memory and bind images to their
    // respective offsets.
    auto base_offsets = CreateArray<VkDeviceSize>(&frame.allocator, res_group->image_count);
    for (uint32 mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index)
    {
        Array<uint32>* mem_info_indexes = &mem_info_index_arrays[mem_index];
        if (mem_info_indexes->count == 0) { continue; }

        ResourceMemory* image_mem = GetImageMemory(mem_index);

        // Calculate image memory size and image offsets based on image size and alignment requirements.
        Clear(base_offsets);
        CTK_ITER(mem_info_index, mem_info_indexes)
        {
            ImageMemoryInfo* mem_info = GetPtr(mem_infos, *mem_info_index);
            image_mem->size = Align(image_mem->size, mem_info->alignment);
            Push(base_offsets, image_mem->size);
            image_mem->size += TotalSize(mem_info);
        }

        image_mem->device = AllocateDeviceMemory(mem_index, image_mem->size, NULL);

        // Bind images to memory.
        for (uint32 i = 0; i < mem_info_indexes->count; ++i)
        {
            ImageMemoryInfo* mem_info = GetPtr(mem_infos, Get(mem_info_indexes, i));
            VkDeviceSize base_offset = Get(base_offsets, i);
            for (uint32 frame_index = 0; frame_index < GetImageState(mem_info->image_index)->frame_count; ++frame_index)
            {
                VkImage image = GetImage(mem_info->image_index, frame_index);
                VkDeviceSize offset = base_offset + (mem_info->stride * frame_index);
                res = vkBindImageMemory(device, image, image_mem->device, offset);
                Validate(res, "vkBindImageMemory() failed");
            }
        }
    }

    // Create default views now that images have been backed with memory.
    for (uint32 image_index = 0; image_index < res_group->image_count; ++image_index)
    {
        ImageViewInfo* default_view_info = GetDefaultViewInfo(image_index);
        for (uint32 frame_index = 0; frame_index < GetImageState(image_index)->frame_count; ++frame_index)
        {
            VkImageViewCreateInfo view_info =
            {
                .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext            = NULL,
                .flags            = default_view_info->flags,
                .image            = GetImage(image_index, frame_index),
                .viewType         = default_view_info->type,
                .format           = default_view_info->format,
                .components       = default_view_info->components,
                .subresourceRange = default_view_info->subresource_range,
            };
            res = vkCreateImageView(device, &view_info, NULL, GetDefaultViewPtr(image_index, frame_index));
            Validate(res, "vkCreateImageView() failed");
        }
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
    // Load image data and write to staging buffer.
    ImageData image_data = {};
    LoadImageData(&image_data, path);
    WriteHostBuffer(image_data_buffer_hnd, frame_index, image_data.data, (VkDeviceSize)image_data.size);
    DestroyImageData(&image_data);

    // Copy image data from buffer memory to image memory.
    VkImage image = GetImage(image_hnd.index, frame_index);
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

static VkImage GetImage(ImageHnd hnd, uint32 frame_index)
{
    ValidateImageHnd(hnd, "can't get image");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count)
    return GetImage(hnd.index, frame_index);
}

static ImageViewInfo* GetDefaultViewInfo(ImageHnd hnd)
{
    ValidateImageHnd(hnd, "can't get image default view info");
    return GetDefaultViewInfo(hnd.index);
}

static VkImageView GetDefaultView(ImageHnd hnd, uint32 frame_index)
{
    ValidateImageHnd(hnd, "can't get image default view");
    CTK_ASSERT(frame_index < GetResourceGroup()->frame_count)
    return GetDefaultView(hnd.index, frame_index);
}
