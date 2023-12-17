/// Data
////////////////////////////////////////////////////////////
static constexpr VkDeviceSize USE_MIN_OFFSET_ALIGNMENT = 0;

struct BufferMemoryHnd  { uint32 index; };
struct ImageMemoryHnd   { uint32 index; };
struct BufferHnd        { uint32 index; };
struct ImageHnd         { uint32 index; };
struct ResourceGroupHnd { uint32 index; };

// From https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#resources-association:
//
// The memoryTypeBits member is identical for all VkBuffer objects created with the same value for the flags and usage
// members in the VkBufferCreateInfo structure and the handleTypes member of the VkExternalMemoryBufferCreateInfo
// structure passed to vkCreateBuffer. Further, if usage1 and usage2 of type VkBufferUsageFlags are such that the bits
// set in usage2 are a subset of the bits set in usage1, and they have the same flags and
// VkExternalMemoryBufferCreateInfo::handleTypes, then the bits set in memoryTypeBits returned for usage1 must be a
// subset of the bits set in memoryTypeBits returned for usage2, for all values of flags.
struct BufferMemoryInfo
{
    VkDeviceSize          size;
    VkBufferCreateFlags   flags;
    VkBufferUsageFlags    usage;
    VkMemoryPropertyFlags properties;
};

struct BufferMemoryState
{
    VkBuffer     buffer;
    VkDeviceSize offset;
    VkDeviceSize index;
    uint32       res_mem_index;
};

struct BufferInfo
{
    VkDeviceSize size;
    VkDeviceSize alignment;
    bool         per_frame;
};

struct BufferState
{
    uint32 frame_stride;
    uint32 frame_count;
    uint32 buffer_mem_index;
};

struct BufferFrameState
{
    VkDeviceSize offset;
    VkDeviceSize index;
};

// From https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#resources-association:
//
// For images created with a color format, the memoryTypeBits member is identical for all VkImage objects created with
// the same combination of values for the tiling member, the VK_IMAGE_CREATE_SPARSE_BINDING_BIT bit of the flags member,
// the VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT bit of the flags member, the VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT
// bit of the usage member if the VkPhysicalDeviceHostImageCopyPropertiesEXT::identicalMemoryTypeRequirements property
// is VK_FALSE, handleTypes member of VkExternalMemoryImageCreateInfo, and the VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
// of the usage member in the VkImageCreateInfo structure passed to vkCreateImage.
//
// For images created with a depth/stencil format, the memoryTypeBits member is identical for all VkImage objects
// created with the same combination of values for the format member, the tiling member, the
// VK_IMAGE_CREATE_SPARSE_BINDING_BIT bit of the flags member, the VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT bit
// of the flags member, the VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT bit of the usage member if the
// VkPhysicalDeviceHostImageCopyPropertiesEXT::identicalMemoryTypeRequirements property is VK_FALSE, handleTypes member
// of VkExternalMemoryImageCreateInfo, and the VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT of the usage member in the
// VkImageCreateInfo structure passed to vkCreateImage.
struct ImageMemoryInfo
{
    VkDeviceSize          size;
    VkImageCreateFlags    flags;
    VkImageUsageFlags     usage;
    VkMemoryPropertyFlags properties;
    VkFormat              format;
    VkImageTiling         tiling;
};

struct ImageMemoryState
{
    VkDeviceSize offset;
    VkDeviceSize index;
    uint32       res_mem_index;
};

struct ImageInfo
{
    VkExtent3D            extent;
    bool                  per_frame;
    VkImageCreateFlags    flags;
    VkImageType           type;
    VkFormat              format;
    uint32                mip_levels;
    uint32                array_layers;
    VkSampleCountFlagBits samples;
    VkImageTiling         tiling;
    VkImageLayout         initial_layout;
    VkMemoryPropertyFlags mem_properties;
    VkImageUsageFlags     usage;
};

struct ImageState
{
    uint32       frame_stride;
    uint32       frame_count;
    uint32       image_mem_index;
    VkDeviceSize size;
    VkDeviceSize alignment;
};

struct ImageViewInfo
{
    VkImageViewCreateFlags  flags;
    VkImageViewType         type;
    VkFormat                format;
    VkComponentMapping      components;
    VkImageSubresourceRange subresource_range;
};

struct ImageFrameState
{
    VkImage      image;
    VkImageView  view;
    VkDeviceSize offset;
};

struct ResourceMemory
{
    VkDeviceSize          size;
    VkDeviceSize          index;
    VkDeviceMemory        device;
    uint8*                host;
    VkMemoryPropertyFlags properties;
};

struct ResourceGroupInfo
{
    uint32 max_buffer_mems;
    uint32 max_image_mems;
    uint32 max_buffers;
    uint32 max_images;
};

struct ResourceGroup
{
    uint32             frame_count;

    uint32             max_buffer_mems;
    uint32             buffer_mem_count;
    BufferMemoryInfo*  buffer_mem_infos;
    BufferMemoryState* buffer_mem_states;

    uint32             max_image_mems;
    uint32             image_mem_count;
    ImageMemoryInfo*   image_mem_infos;
    ImageMemoryState*  image_mem_states;

    uint32             max_buffers;
    uint32             buffer_count;
    BufferInfo*        buffer_infos;        // size: max_buffers
    BufferState*       buffer_states;       // size: max_buffers
    BufferFrameState*  buffer_frame_states; // size: max_buffers * frame_count

    uint32             max_images;
    uint32             image_count;
    ImageInfo*         image_infos;         // size: max_images
    ImageViewInfo*     image_view_infos;    // size: max_images
    ImageState*        image_states;        // size: max_images
    ImageFrameState*   image_frame_states;  // size: max_images * frame_count

    ResourceMemory     res_mems[VK_MAX_MEMORY_TYPES];
};

struct ResourceModuleInfo
{
    uint32 max_resource_groups;
};

static Array<ResourceGroup> g_res_groups;

/// Utils
////////////////////////////////////////////////////////////
static uint32 GetResourceIndex(uint32 res_hnd_index)
{
    return 0x00FFFFFF & res_hnd_index;
}

static uint32 GetResourceGroupIndex(uint32 res_hnd_index)
{
    return (0xFF000000 & res_hnd_index) >> 24;
}

static uint32 GetResourceHndIndex(ResourceGroupHnd res_group_hnd, uint32 res_index)
{
    return (res_group_hnd.index << 24) | res_index;
}

static ResourceGroup* GetResourceGroup(uint32 index)
{
    return &g_res_groups.data[index];
}

static ResourceGroup* GetResourceGroup(ResourceGroupHnd hnd)
{
    return GetResourceGroup(hnd.index);
}

static ResourceMemory* GetResourceMemory(ResourceGroup* res_group, uint32 res_mem_index)
{
    return &res_group->res_mems[res_mem_index];
}

static void ValidateResourceGroup(uint32 index, const char* action)
{
    if (index >= g_res_groups.count)
    {
        CTK_FATAL("%s: resource group index %u exceeds resource group count of %u", action, index, g_res_groups.count);
    }
}

static void ValidateResourceGroup(ResourceGroupHnd hnd, const char* action)
{
    ValidateResourceGroup(hnd.index, action);
}

static uint32 GetCapableMemoryTypeIndex(VkMemoryRequirements* mem_requirements, VkMemoryPropertyFlags mem_properties)
{
    // Reference: https://registry.khronos.org/vulkan/specs/1.3/html/vkspec.html#memory-device
    VkPhysicalDeviceMemoryProperties* device_mem_properties = &GetPhysicalDevice()->mem_properties;
    for (uint32 mem_type_index = 0; mem_type_index < device_mem_properties->memoryTypeCount; ++mem_type_index)
    {
        // Memory type at index must be supported for resource.
        if ((mem_requirements->memoryTypeBits & (1 << mem_type_index)) == 0)
        {
            continue;
        }

        // Memory type at index must support all resource memory properties.
        if ((device_mem_properties->memoryTypes[mem_type_index].propertyFlags & mem_properties) != mem_properties)
        {
            continue;
        }

        return mem_type_index;
    }

    CTK_FATAL("failed to find memory type that satisfies requested memory requirements & properties");
}

static VkDeviceMemory AllocateDeviceMemory(uint32 mem_type_index, VkDeviceSize size, VkAllocationCallbacks* allocators)
{
    // From https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateMemory.html:
    // Allocations returned by vkAllocateMemory are guaranteed to meet any alignment requirement of the implementation.
    // For example, if an implementation requires 128 byte alignment for images and 64 byte alignment for buffers, the
    // device memory returned through this mechanism would be 128-byte aligned. This ensures that applications can
    // correctly suballocate objects of different types (with potentially different alignment requirements) in the same
    // memory object.

    // Allocate memory using selected memory type index.
    VkMemoryAllocateInfo info =
    {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = size,
        .memoryTypeIndex = mem_type_index,
    };
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkResult res = vkAllocateMemory(GetDevice(), &info, allocators, &mem);
    Validate(res, "vkAllocateMemory() failed");

    return mem;
}

/// Buffer Memory Utils
////////////////////////////////////////////////////////////
static uint32 GetBufferMemoryIndex(BufferMemoryHnd hnd)
{
    return GetResourceIndex(hnd.index);
}

static ResourceGroup* GetResourceGroup(BufferMemoryHnd hnd)
{
    return GetResourceGroup(GetResourceGroupIndex(hnd.index));
}

static BufferMemoryInfo* GetBufferMemoryInfo(ResourceGroup* res_group, uint32 buffer_mem_index)
{
    return &res_group->buffer_mem_infos[buffer_mem_index];
}

static BufferMemoryState* GetBufferMemoryState(ResourceGroup* res_group, uint32 buffer_mem_index)
{
    return &res_group->buffer_mem_states[buffer_mem_index];
}

static void ValidateBufferMemory(BufferMemoryHnd hnd, const char* action)
{
    uint32 res_group_index = GetResourceGroupIndex(hnd.index);
    ValidateResourceGroup(res_group_index, action);

    ResourceGroup* res_group = GetResourceGroup(res_group_index);
    uint32 buffer_mem_index = GetBufferMemoryIndex(hnd);
    if (buffer_mem_index >= res_group->buffer_mem_count)
    {
        CTK_FATAL("%s: buffer memory index %u exceeds buffer memory count of %u",
                  action, buffer_mem_index, res_group->buffer_mem_count);
    }
}

/// Buffer Utils
////////////////////////////////////////////////////////////
static uint32 GetBufferIndex(BufferHnd hnd)
{
    return GetResourceIndex(hnd.index);
}

static ResourceGroup* GetResourceGroup(BufferHnd hnd)
{
    return GetResourceGroup(GetResourceGroupIndex(hnd.index));
}

static BufferInfo* GetBufferInfo(ResourceGroup* res_group, uint32 buffer_index)
{
    return &res_group->buffer_infos[buffer_index];
}

static BufferState* GetBufferState(ResourceGroup* res_group, uint32 buffer_index)
{
    return &res_group->buffer_states[buffer_index];
}

static BufferFrameState* GetBufferFrameState(ResourceGroup* res_group, uint32 buffer_index, uint32 frame_index)
{
    uint32 frame_offset = GetBufferState(res_group, buffer_index)->frame_stride * frame_index;
    return &res_group->buffer_frame_states[frame_offset + buffer_index];
}

static VkBuffer GetBuffer(ResourceGroup* res_group, uint32 buffer_index)
{
    return res_group->buffer_mem_states[GetBufferState(res_group, buffer_index)->buffer_mem_index].buffer;
}

static ResourceMemory* GetBufferResourceMemory(ResourceGroup* res_group, uint32 buffer_index)
{
    uint32 buffer_mem_index = GetBufferState(res_group, buffer_index)->buffer_mem_index;
    return GetResourceMemory(res_group, GetBufferMemoryState(res_group, buffer_mem_index)->res_mem_index);
}

/// Image Memory Utils
////////////////////////////////////////////////////////////
static uint32 GetImageMemoryIndex(ImageMemoryHnd hnd)
{
    return GetResourceIndex(hnd.index);
}

static ResourceGroup* GetResourceGroup(ImageMemoryHnd hnd)
{
    return GetResourceGroup(GetResourceGroupIndex(hnd.index));
}

static ImageMemoryInfo* GetImageMemoryInfo(ResourceGroup* res_group, uint32 image_mem_index)
{
    return &res_group->image_mem_infos[image_mem_index];
}

static ImageMemoryState* GetImageMemoryState(ResourceGroup* res_group, uint32 image_mem_index)
{
    return &res_group->image_mem_states[image_mem_index];
}

static void ValidateImageMemory(ImageMemoryHnd hnd, const char* action)
{
    uint32 res_group_index = GetResourceGroupIndex(hnd.index);
    ValidateResourceGroup(res_group_index, action);

    ResourceGroup* res_group = GetResourceGroup(res_group_index);
    uint32 image_mem_index = GetImageMemoryIndex(hnd);
    if (image_mem_index >= res_group->image_mem_count)
    {
        CTK_FATAL("%s: image memory index %u exceeds image memory count of %u",
                  action, image_mem_index, res_group->image_mem_count);
    }
}

/// Image Utils
////////////////////////////////////////////////////////////
static uint32 GetImageIndex(ImageHnd hnd)
{
    return GetResourceIndex(hnd.index);
}

static ResourceGroup* GetResourceGroup(ImageHnd hnd)
{
    return GetResourceGroup(GetResourceGroupIndex(hnd.index));
}

static ImageInfo* GetImageInfo(ResourceGroup* res_group, uint32 image_index)
{
    return &res_group->image_infos[image_index];
}

static ImageViewInfo* GetImageViewInfo(ResourceGroup* res_group, uint32 image_index)
{
    return &res_group->image_view_infos[image_index];
}

static ImageState* GetImageState(ResourceGroup* res_group, uint32 image_index)
{
    return &res_group->image_states[image_index];
}

static ImageFrameState* GetImageFrameState(ResourceGroup* res_group, uint32 image_index, uint32 frame_index)
{
    uint32 frame_offset = GetImageState(res_group, image_index)->frame_stride * frame_index;
    return &res_group->image_frame_states[frame_offset + image_index];
}

/// Interface
////////////////////////////////////////////////////////////
static void InitResourceModule(const Allocator* allocator, ResourceModuleInfo info)
{
    InitArray(&g_res_groups, allocator, info.max_resource_groups);
}

static ResourceGroupHnd CreateResourceGroup(const Allocator* allocator, ResourceGroupInfo* info)
{
    ResourceGroupHnd hnd = { .index = g_res_groups.count };
    ResourceGroup* res_group = Push(&g_res_groups);

    uint32 frame_count = GetFrameCount();
    res_group->frame_count = frame_count;

    // Resource Memory Data
    res_group->max_buffer_mems  = info->max_buffer_mems;
    res_group->buffer_mem_count = 0;
    if (res_group->max_buffer_mems > 0)
    {
        res_group->buffer_mem_infos  = Allocate<BufferMemoryInfo> (allocator, info->max_buffer_mems);
        res_group->buffer_mem_states = Allocate<BufferMemoryState>(allocator, info->max_buffer_mems);
    }

    res_group->max_image_mems  = info->max_image_mems;
    res_group->image_mem_count = 0;
    if (res_group->max_image_mems > 0)
    {
        res_group->image_mem_infos  = Allocate<ImageMemoryInfo> (allocator, info->max_image_mems);
        res_group->image_mem_states = Allocate<ImageMemoryState>(allocator, info->max_image_mems);
    }

    // Resource Data
    res_group->max_buffers  = info->max_buffers;
    res_group->buffer_count = 0;
    if (res_group->max_buffers > 0)
    {
        res_group->buffer_infos        = Allocate<BufferInfo>      (allocator, info->max_buffers);
        res_group->buffer_states       = Allocate<BufferState>     (allocator, info->max_buffers);
        res_group->buffer_frame_states = Allocate<BufferFrameState>(allocator, info->max_buffers * frame_count);
    }

    res_group->max_images  = info->max_images;
    res_group->image_count = 0;
    if (res_group->max_images > 0)
    {
        res_group->image_infos        = Allocate<ImageInfo>      (allocator, info->max_images);
        res_group->image_view_infos   = Allocate<ImageViewInfo>  (allocator, info->max_images);
        res_group->image_states       = Allocate<ImageState>     (allocator, info->max_images);
        res_group->image_frame_states = Allocate<ImageFrameState>(allocator, info->max_images * frame_count);
    }

    return hnd;
}

static BufferMemoryHnd CreateBufferMemory(ResourceGroupHnd res_group_hnd, BufferMemoryInfo* create_info)
{
    VkDevice device = GetDevice();
    QueueFamilies* queue_families = &GetPhysicalDevice()->queue_families;
    VkResult res = VK_SUCCESS;

    ValidateResourceGroup(res_group_hnd, "can't create buffer memory type");
    ResourceGroup* res_group = GetResourceGroup(res_group_hnd);
    if (res_group->buffer_mem_count >= res_group->max_buffer_mems)
    {
        CTK_FATAL("can't create buffer memory type: already at max of %u", res_group->max_buffer_mems);
    }

    uint32 buffer_mem_index = res_group->buffer_mem_count;
    BufferMemoryHnd hnd = { .index = buffer_mem_index };
    res_group->buffer_mem_count += 1;

    BufferMemoryInfo* buffer_mem_info = GetBufferMemoryInfo(res_group, buffer_mem_index);
    *buffer_mem_info = *create_info;

    // Create buffer to get memory requirements and for usage by buffer memory.
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = NULL;
    buffer_create_info.flags = 0;
    buffer_create_info.size  = create_info->size;
    buffer_create_info.usage = create_info->usage;
    if (queue_families->graphics != queue_families->present)
    {
        buffer_create_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        buffer_create_info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(uint32);
        buffer_create_info.pQueueFamilyIndices   = (uint32*)queue_families;
    }
    else
    {
        buffer_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices   = NULL;
    }
    VkBuffer mem_buffer = VK_NULL_HANDLE;
    res = vkCreateBuffer(device, &buffer_create_info, NULL, &mem_buffer);
    Validate(res, "vkCreateBuffer() failed");
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(device, mem_buffer, &mem_requirements);
    buffer_mem_info->size = mem_requirements.size;

    // Store buffer, offset into resource memory, and resource memory index in buffer memory state.
    uint32 res_mem_index = GetCapableMemoryTypeIndex(&mem_requirements, create_info->properties);
    ResourceMemory* res_mem = GetResourceMemory(res_group, res_mem_index);
    BufferMemoryState* buffer_mem_state = GetBufferMemoryState(res_group, buffer_mem_index);
    buffer_mem_state->buffer        = mem_buffer;
    buffer_mem_state->offset        = res_mem->size;
    buffer_mem_state->res_mem_index = res_mem_index;

    // Increase resource memory size by buffer memory size.
    res_mem->size += buffer_mem_info->size;

    return hnd;
}

static ImageMemoryHnd CreateImageMemory(ResourceGroupHnd res_group_hnd, ImageMemoryInfo* create_info)
{
    VkDevice device = GetDevice();
    QueueFamilies* queue_families = &GetPhysicalDevice()->queue_families;
    VkResult res = VK_SUCCESS;

    ValidateResourceGroup(res_group_hnd, "can't create image memory type");
    ResourceGroup* res_group = GetResourceGroup(res_group_hnd);
    if (res_group->image_mem_count >= res_group->max_image_mems)
    {
        CTK_FATAL("can't create image memory type: already at max of %u", res_group->max_image_mems);
    }

    uint32 image_mem_index = res_group->image_mem_count;
    ImageMemoryHnd hnd = { .index = image_mem_index };
    res_group->image_mem_count += 1;

    ImageMemoryInfo* image_mem_info = GetImageMemoryInfo(res_group, image_mem_index);
    *image_mem_info = *create_info;

    // Create dummy image to get memory requirements.
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext         = NULL;
    image_create_info.flags         = create_info->flags;
    image_create_info.imageType     = VK_IMAGE_TYPE_1D;
    image_create_info.format        = create_info->format;
    image_create_info.extent        = { 1, 1, 1 };
    image_create_info.mipLevels     = 1;
    image_create_info.arrayLayers   = 1;
    image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling        = create_info->tiling;
    image_create_info.usage         = create_info->usage;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (queue_families->graphics != queue_families->present)
    {
        image_create_info.sharingMode           = VK_SHARING_MODE_CONCURRENT;
        image_create_info.queueFamilyIndexCount = sizeof(QueueFamilies) / sizeof(uint32);
        image_create_info.pQueueFamilyIndices   = (uint32*)queue_families;
    }
    else
    {
        image_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.queueFamilyIndexCount = 0;
        image_create_info.pQueueFamilyIndices   = NULL;
    }
    VkImage temp = VK_NULL_HANDLE;
    res = vkCreateImage(device, &image_create_info, NULL, &temp);
    Validate(res, "vkCreateImage() failed");
    VkMemoryRequirements mem_requirements = {};
    vkGetImageMemoryRequirements(device, temp, &mem_requirements);
    vkDestroyImage(device, temp, NULL);

    // Store offset into resource memory and resource memory index in image memory state.
    uint32 res_mem_index = GetCapableMemoryTypeIndex(&mem_requirements, create_info->properties);
    ResourceMemory* res_mem = GetResourceMemory(res_group, res_mem_index);
    ImageMemoryState* image_mem_state = GetImageMemoryState(res_group, image_mem_index);
    image_mem_state->offset        = res_mem->size;
    image_mem_state->res_mem_index = res_mem_index;

    // Increase resource memory size by image memory size.
    res_mem->size += image_mem_info->size;

    return hnd;
}

static void AllocateResourceMemory(ResourceGroupHnd res_group_hnd)
{
    VkDevice device = GetDevice();
    PhysicalDevice* physical_device = GetPhysicalDevice();
    QueueFamilies* queue_families = &physical_device->queue_families;
    VkResult res = VK_SUCCESS;

    ValidateResourceGroup(res_group_hnd, "can't allocate resource memory");
    ResourceGroup* res_group = GetResourceGroup(res_group_hnd);

    // Allocate resource memory and create/bind buffer for sub-allocating other buffers.
    for (uint32 res_mem_index = 0; res_mem_index < VK_MAX_MEMORY_TYPES; ++res_mem_index)
    {
        ResourceMemory* res_mem = GetResourceMemory(res_group, res_mem_index);
        if (res_mem->size == 0)
        {
            continue;
        }

        // Allocate memory.
        res_mem->device     = AllocateDeviceMemory(res_mem_index, res_mem->size, NULL);
        res_mem->properties = physical_device->mem_properties.memoryTypes[res_mem_index].propertyFlags;

        // Map host visible memory.
        if (res_mem->properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            vkMapMemory(device, res_mem->device, 0, res_mem->size, 0, (void**)&res_mem->host);
        }
    }
}

static BufferHnd CreateBuffer(ResourceGroupHnd res_group_hnd, BufferMemoryHnd buffer_mem_hnd, BufferInfo* info)
{
    ValidateResourceGroup(res_group_hnd, "can't create buffer");
    ValidateBufferMemory(buffer_mem_hnd, "can't create buffer");

    ResourceGroup* res_group = GetResourceGroup(res_group_hnd);
    if (res_group->buffer_count >= res_group->max_buffers)
    {
        CTK_FATAL("can't create buffer: already at max of %u", res_group->max_buffers);
    }

    BufferMemoryInfo* buffer_mem_info = GetBufferMemoryInfo(res_group, buffer_mem_hnd.index);

    // Figure out minimum offset alignment if requested.
    // Spec: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VkMemoryRequirements
    if (info->alignment == USE_MIN_OFFSET_ALIGNMENT)
    {
        VkPhysicalDeviceLimits* physical_device_limits = &GetPhysicalDevice()->properties.limits;
        info->alignment = 4;

        // Uniform
        if ((buffer_mem_info->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) &&
            info->alignment < physical_device_limits->minUniformBufferOffsetAlignment)
        {
            info->alignment = physical_device_limits->minUniformBufferOffsetAlignment;
        }

        // Storage
        if ((buffer_mem_info->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) &&
            info->alignment < physical_device_limits->minStorageBufferOffsetAlignment)
        {
            info->alignment = physical_device_limits->minStorageBufferOffsetAlignment;
        }

        // Texel
        VkBufferUsageFlags texel_usage_flags = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                                               VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        if ((buffer_mem_info->usage & texel_usage_flags) &&
            info->alignment < physical_device_limits->minTexelBufferOffsetAlignment)
        {
            info->alignment = physical_device_limits->minTexelBufferOffsetAlignment;
        }
    }

    uint32 buffer_index = res_group->buffer_count;
    ++res_group->buffer_count;
    BufferHnd hnd = { .index = GetResourceHndIndex(res_group_hnd, buffer_index) };
    *GetBufferInfo(res_group, buffer_index) = *info;

    return hnd;
}

static ImageHnd CreateImage(ResourceGroupHnd res_group_hnd,
                            ImageMemoryHnd   image_mem_hnd,
                            ImageInfo*       image_info,
                            ImageViewInfo*   view_info)
{
    ValidateResourceGroup(res_group_hnd, "can't create image");
    ValidateImageMemory(image_mem_hnd, "can't create image");

    ResourceGroup* res_group = GetResourceGroup(res_group_hnd);
    if (res_group->image_count >= res_group->max_images)
    {
        CTK_FATAL("can't create image: already at max of %u", res_group->max_images);
    }

    // Copy info.
    uint32 image_index = res_group->image_count;
    ++res_group->image_count;
    ImageHnd hnd = { .index = GetResourceHndIndex(res_group_hnd, image_index) };
    *GetImageInfo(res_group, image_index) = *image_info;
    *GetImageViewInfo(res_group, image_index) = *view_info;

    return hnd;
}

/// Debug
////////////////////////////////////////////////////////////
static void LogResourceGroups()
{
    for (uint32 res_group_index = 0; res_group_index < g_res_groups.count; ++res_group_index)
    {
        ResourceGroup* res_group = GetResourceGroup(res_group_index);
        PrintLine("resource group %u:", res_group_index);

        PrintLine("    memory:");
        for (uint32 buffer_mem_index = 0; buffer_mem_index < res_group->buffer_mem_count;
             buffer_mem_index += 1)
        {
            BufferMemoryInfo*  buffer_mem_info  = GetBufferMemoryInfo (res_group, buffer_mem_index);
            BufferMemoryState* buffer_mem_state = GetBufferMemoryState(res_group, buffer_mem_index);
            PrintLine("        buffer memory %u:", buffer_mem_index);
            PrintLine("            info:");
            PrintLine("                size: %llu", buffer_mem_info->size);
            PrintLine("                flags:");
            PrintBufferCreateFlags(buffer_mem_info->flags, 5);
            PrintLine("                usage:");
            PrintBufferUsageFlags(buffer_mem_info->usage, 5);
            PrintLine("                properties:");
            PrintMemoryPropertyFlags(buffer_mem_info->properties, 5);
            PrintLine("            state:");
            PrintLine("                buffer:        0x%p", buffer_mem_state->buffer);
            PrintLine("                offset:        %u", buffer_mem_state->offset);
            PrintLine("                index:         %u", buffer_mem_state->index);
            PrintLine("                res_mem_index: %u", buffer_mem_state->res_mem_index);
        }
        for (uint32 image_mem_index = 0; image_mem_index < res_group->image_mem_count;
             image_mem_index += 1)
        {
            ImageMemoryInfo*  image_mem_info  = GetImageMemoryInfo (res_group, image_mem_index);
            ImageMemoryState* image_mem_state = GetImageMemoryState(res_group, image_mem_index);
            PrintLine("        image memory %u:", image_mem_index);
            PrintLine("            info:");
            PrintLine("                size:   %llu", image_mem_info->size);
            PrintLine("                flags:");
            PrintImageCreateFlags(image_mem_info->flags, 5);
            PrintLine("                usage:");
            PrintImageUsageFlags(image_mem_info->usage, 5);
            PrintLine("                properties:");
            PrintMemoryPropertyFlags(image_mem_info->properties, 5);
            PrintLine("                format: %s", VkFormatName(image_mem_info->format));
            PrintLine("                tiling: %s", VkImageTilingName(image_mem_info->tiling));
            PrintLine("            state:");
            PrintLine("                offset:        %u", image_mem_state->offset);
            PrintLine("                index:         %u", image_mem_state->index);
            PrintLine("                res_mem_index: %u", image_mem_state->res_mem_index);
        }
        for (uint32 res_mem_index = 0; res_mem_index < VK_MAX_MEMORY_TYPES; ++res_mem_index)
        {
            ResourceMemory* res_mem = GetResourceMemory(res_group, res_mem_index);
            if (res_mem->size == 0)
            {
                continue;
            }
            PrintLine("        resource memory %u:", res_mem_index);
            PrintLine("             size:   %llu", res_mem->size);
            PrintLine("             device: 0x%p", res_mem->device);
            PrintLine("             host:   0x%p", res_mem->host);
            PrintLine("             properties: ");
            PrintMemoryPropertyFlags(res_mem->properties, 4);
        }

        PrintLine("    resources:");
        for (uint32 buffer_index = 0; buffer_index < res_group->buffer_count; buffer_index += 1)
        {
            BufferInfo* info = GetBufferInfo(res_group, buffer_index);
            BufferState* state = GetBufferState(res_group, buffer_index);
            PrintLine("        buffer %u:", buffer_index);
            PrintLine("            info:");
            PrintLine("                size:      %llu", info->size);
            PrintLine("                alignment: %llu", info->alignment);
            PrintLine("                per_frame: %s", info->per_frame ? "true" : "false");
            PrintLine("            state:");
            PrintLine("                frame_stride:     %u", state->frame_stride);
            PrintLine("                frame_count:      %u", state->frame_count);
            PrintLine("                buffer_mem_index: %u", state->buffer_mem_index);
            PrintLine("            frame_states:");
            for (uint32 frame_index = 0; frame_index < state->frame_count; ++frame_index)
            {
                BufferFrameState* frame_state = GetBufferFrameState(res_group, buffer_index, frame_index);
                PrintLine("                frame %u:", frame_index);
                PrintLine("                    offset: %llu", frame_state->offset);
                PrintLine("                    index:  %llu", frame_state->index);
            }
            PrintLine();
        }
        for (uint32 image_index = 0; image_index < res_group->image_count; image_index += 1)
        {
            ImageInfo* info = GetImageInfo(res_group, image_index);
            ImageViewInfo* view_info = GetImageViewInfo(res_group, image_index);
            ImageState* state = GetImageState(res_group, image_index);
            PrintLine("        image %u:", image_index);
            PrintLine("            info:");
            PrintLine("                extent:       { w: %u, h: %u, d: %u }",
                      info->extent.width,
                      info->extent.height,
                      info->extent.depth);
            PrintLine("                per_frame:    %s", info->per_frame ? "true" : "false");

            PrintLine("                flags:");
            PrintImageCreateFlags(info->flags, 5);
            PrintLine("                type:         %s", VkImageTypeName(info->type));
            PrintLine("                format:       %s", VkFormatName(info->format));
            PrintLine("                mip_levels:   %u", info->mip_levels);
            PrintLine("                array_layers: %u", info->array_layers);
            PrintLine("                samples:      %s", VkSampleCountName(info->samples));
            PrintLine("                tiling:       %s", VkImageTilingName(info->tiling));
            PrintLine("                mem_properties:");
            PrintMemoryPropertyFlags(info->mem_properties, 5);
            PrintLine("                usage:");
            PrintImageUsageFlags(info->usage, 5);
            PrintLine("            view_info:");
            PrintLine("                flags:");
            PrintImageViewCreateFlags(view_info->flags, 5);
            PrintLine("                type:   %s", VkImageViewTypeName(view_info->type));
            PrintLine("                format: %s", VkFormatName(view_info->format));
            PrintLine("                components:");
            PrintLine("                    r: %s", VkComponentSwizzleName(view_info->components.r));
            PrintLine("                    g: %s", VkComponentSwizzleName(view_info->components.g));
            PrintLine("                    b: %s", VkComponentSwizzleName(view_info->components.b));
            PrintLine("                    a: %s", VkComponentSwizzleName(view_info->components.a));
            PrintLine("                subresource_range:");
            PrintLine("                    aspectMask:");
            PrintImageAspectFlags(view_info->subresource_range.aspectMask, 6);
            PrintLine("                    baseMipLevel:   %u", view_info->subresource_range.baseMipLevel);

            uint32 level_count = view_info->subresource_range.levelCount;
            if (level_count == VK_REMAINING_MIP_LEVELS)
            {
                PrintLine("                    levelCount:     VK_REMAINING_MIP_LEVELS");
            }
            else
            {
                PrintLine("                    levelCount:     %u", level_count);
            }
            PrintLine("                    baseArrayLayer: %u", view_info->subresource_range.baseArrayLayer);

            uint32 layer_count = view_info->subresource_range.layerCount;
            if (layer_count == VK_REMAINING_ARRAY_LAYERS)
            {
                PrintLine("                    layerCount:     VK_REMAINING_ARRAY_LAYERS");
            }
            else
            {
                PrintLine("                    layerCount:     %u", layer_count);
            }

            PrintLine("                state:");
            PrintLine("                    frame_stride:    %u", state->frame_stride);
            PrintLine("                    frame_count:     %u", state->frame_count);
            PrintLine("                    image_mem_index: %u", state->image_mem_index);
            PrintLine("                    size:            %llu", state->size);
            PrintLine("                    alignment:       %llu", state->alignment);
            PrintLine("                frame_states:");
            for (uint32 frame_index = 0; frame_index < state->frame_count; ++frame_index)
            {
                ImageFrameState* frame_state = GetImageFrameState(res_group, image_index, frame_index);
                PrintLine("                    frame %u:", frame_index);
                PrintLine("                        image:  %p", frame_state->image);
                PrintLine("                        view:   %p", frame_state->view);
                PrintLine("                        offset: %llu", frame_state->offset);
            }
            PrintLine();
        }

        PrintLine();
    }
}
