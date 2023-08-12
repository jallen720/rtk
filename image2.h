/// Data
////////////////////////////////////////////////////////////
struct ImageMemoryType
{
    VkMemory     mem;
    VkDeviceSize size;
};

struct Image2
{
    VkImage     hnd;
    VkImageView view;
    VkExtent3D  extent;
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
    Array<Image2>   images;
    Array<uint32>   image_mem_indexes;
    ImageMemoryType image_mem_types[VK_MAX_MEMORY_TYPES];
}
global_image_state;

/// Interface
////////////////////////////////////////////////////////////
static void InitImageState(Stack* perm_stack, uint32 max_images)
{
    InitArray(&global_image_state.images, perm_stack, max_images);
}

static ImageHnd PushImage(VkImageCreateInfo info)
{

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
