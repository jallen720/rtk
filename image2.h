/// Data
////////////////////////////////////////////////////////////
struct ImageMemoryType
{
    VkDeviceMemory mem;
    VkDeviceSize   size;
};

struct Image2
{
    VkImage              hnd;
    VkMemoryRequirements mem_requirements;
    VkImageType          type;
    VkFormat             format;
    VkExtent3D           extent;
    VkImageView          default_view;
};

struct ImageData2
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
    Image2*         data;
    uint32*         mem_type_indexes;
    uint32          count;
    uint32          max;
    ImageMemoryType mem_types[VK_MAX_MEMORY_TYPES];
}
global_image_state;

/// Utils
////////////////////////////////////////////////////////////
static bool ImageMemAlignmentDesc(Image2* a, Image2* b)
{
    return b->mem_requirements.alignment > a->mem_requirements.alignment;
}

/// Interface
////////////////////////////////////////////////////////////
static void InitImageState(Allocator* allocator, uint32 max)
{
    global_image_state.data             = Allocate<Image2>(allocator, max);
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
    Image2* image = global_image_state.data + image_hnd;
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
        Image2* image = global_image_state.data + i;
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
        Image2* image = global_image_state.data + i;
        uint32 mem_type_index = global_image_state.mem_type_indexes[i];
        res = vkBindImageMemory(device, image->hnd, global_image_state.mem_types[mem_type_index].mem,
                                mem_type_offsets[mem_type_index]);
        Validate(res, "vkBindImageMemory() failed");
        mem_type_offsets[mem_type_index] += image->mem_requirements.size;
    }

    // Create default views now that images have been backed with memory.
    CTK_ITERATE_PTR(image, global_image_state.data, global_image_state.count)
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

static void LogImageState()
{
    PrintLine("images:");
    for (uint32 i = 0; i < global_image_state.count; ++i)
    {
        Image2* image = global_image_state.data + i;
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

// static void InitImageMemory(ImageMemory* image_memory, const Allocator* allocator, uint32 max_image_types)
// {
//     image_memory->types.data             = Allocate<ImageType>(allocator, max_image_types);
//     image_memory->types.mem_type_indexes = Allocate<uint32>(allocator, max_image_types);
//     image_memory->types.size             = max_image_types;
//     image_memory->types.count            = 0;
// }

// static ImageTypeHnd PushImageType(ImageMemory* image_memory, ImageType* image_type, VkDeviceSize size)
// {
//     ImageTypes* image_types = &image_memory->types;
//     if (image_types->count == image_types->size)
//     {
//         CTK_FATAL("can't push image-type to image-memory; image-memory already has max image-types (%u)",
//                   image_types->size);
//     }

//     VkDevice device = global_ctx.device;

//     // Copy image-type data.
//     ImageTypeHnd image_type_hnd = image_types->count;
//     ++image_types->count;
//     image_types->data[image_type_hnd] = *image_type;

//     // Create temp image with image-type data to get image-type's mem-type-index from its memory requirements.
//     VkImageCreateInfo image_create_info =
//     {
//         .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
//         .pNext                 = NULL,
//         .flags                 = image_type->flags,
//         .imageType             = VK_IMAGE_TYPE_2D,
//         .format                = image_type->format,
//         .extent                = { 1, 1, 1 },
//         .mipLevels             = 1,
//         .arrayLayers           = 1,
//         .samples               = VK_SAMPLE_COUNT_1_BIT,
//         .tiling                = image_type->tiling,
//         .usage                 = image_type->usage
//         .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
//         .queueFamilyIndexCount = 0,
//         .pQueueFamilyIndices   = NULL,
//         .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
//     };
//     VkImage image = VK_NULL_HANDLE;
//     VkResult res = vkCreateImage(device, &image_create_info, NULL, &image);
//     Validate(res, "vkCreateImage() failed");

//     VkMemoryRequirements mem_requirements = {};
//     vkGetImageMemoryRequirements(device, image, &mem_requirements);

//     vkDestroyImage(device, image, NULL);

//     image_types->mem_type_indexes[image_type_hnd] =
//         GetCapableMemoryTypeIndex(mem_requirements.memoryTypeBits, &image_type->mem_properties);
// }

// static void InitImageStack(ImageMemory* image_memory, ImageTypeHnd image_type, VkDeviceSize size)
// {
//     ImageTypes* image_types = &image_memory->types;
//     if (image_type >= image_types->count)
//     {
//         CTK_FATAL("image_type index %u is out of range (current max is %u)", image_type, image_types->count);
//     }

//     ImageStack* image_stack = image_memory->stacks + image_types->mem_type_indexes[image_type];
//     if (image_stack->size != 0)
//     {
//         CTK_FATAL("can't initialize image-stack for image-type %u: image-stack is already intialized", image_type);
//     }

//     image_stack->mem   = AllocateDeviceMemory(size, image_type, NULL);
//     image_stack->size  = size;
//     image_stack->count = 0;
// }

// static void InitImage2(Image* image, ImageMemory* image_memory, ImageTypeHnd image_type)
// {
//     // VkDeviceSize remaining_space = image_stack->size - image_stack->count;
//     // if (remaining_space < size)
//     // {
//     //     CTK_FATAL("can't allocate %u bytes of image-type memory: image-stack for that type only has %u bytes remaining",
//     //               size, remaining_space);
//     // }

// }
