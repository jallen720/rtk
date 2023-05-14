#pragma once

#include "rtk/vulkan.h"
#include "rtk/context.h"
#include "rtk/stb_image.h"
#include "ctk3/ctk3.h"
#include "ctk3/allocator.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct ImageMemoryInfo
{
    VkImageCreateInfo     image_info;
    VkMemoryPropertyFlags mem_property_flags;
    uint32                max_image_count;
};

struct ImageMemory
{
    VkDeviceMemory       hnd;
    VkImageCreateInfo    image_info;
    VkMemoryRequirements requirements;
    uint32               index;
    uint32               size;
};

struct Image
{
    VkImage     hnd;
    VkImageView view;
    VkExtent3D  extent;
};

struct ImageData
{
    sint32 width;
    sint32 height;
    sint32 channel_count;
    sint32 size;
    uint8* data;
};

/// Interface
////////////////////////////////////////////////////////////
static ImageMemory* CreateImageMemory(const Allocator* allocator, ImageMemoryInfo* info,
                                      VkAllocationCallbacks* vk_allocators)
{
    VkDevice device = global_ctx.device;

    auto image_memory = Allocate<ImageMemory>(allocator, 1);

    // Create temp image with image info to get memory requiremnts, then destroy temp image.
    VkImage temp_image = VK_NULL_HANDLE;
    VkResult res = vkCreateImage(device, &info->image_info, NULL, &temp_image);
    Validate(res, "vkCreateImage() failed");
    vkGetImageMemoryRequirements(device, temp_image, &image_memory->requirements);
    vkDestroyImage(device, temp_image, NULL);
PrintResourceMemoryInfo("image memory", &image_memory->requirements, info->mem_property_flags);

    // Allocate image memory.
    image_memory->hnd = AllocateDeviceMemory(&image_memory->requirements, info->mem_property_flags, vk_allocators,
                                             info->max_image_count);

    image_memory->size = info->max_image_count;
    image_memory->image_info = info->image_info;

    return image_memory;
}

static void InitImage(Image* image, ImageMemory* image_memory, VkImageViewCreateInfo* view_info)
{
    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;

    // Create image.
    res = vkCreateImage(device, &image_memory->image_info, NULL, &image->hnd);
    Validate(res, "vkCreateImage() failed");

    // Bind memory and increment index.
    res = vkBindImageMemory(device, image->hnd, image_memory->hnd,
                            image_memory->index * image_memory->requirements.size);
    Validate(res, "vkBindImageMemory() failed");
    ++image_memory->index;

    // Create image view.
    view_info->image = image->hnd;
    res = vkCreateImageView(device, view_info, NULL, &image->view);
    Validate(res, "vkCreateImageView() failed");

    // Set image extent to image memory's extent.
    image->extent = image_memory->image_info.extent;
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

static VkSampler CreateSampler(VkSamplerCreateInfo* info)
{
    VkSampler sampler = VK_NULL_HANDLE;
    VkResult res = vkCreateSampler(global_ctx.device, info, NULL, &sampler);
    Validate(res, "vkCreateSampler() failed");
    return sampler;
}

}
