#pragma once

#include "ctk3/ctk3.h"
#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/context.h"
#include "rtk/resources.h"

#define STB_IMAGE_IMPLEMENTATION
#include "rtk/stb/stb_image.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct ImageData
{
    sint32 width;
    sint32 height;
    sint32 channel_count;
    sint32 size;
    uint8* data;
};

struct ImageInfo
{
    VkImageCreateInfo     image;
    VkImageViewCreateInfo view;
    VkMemoryPropertyFlags mem_property_flags;
};

struct Image
{
    VkImage        hnd;
    VkImageView    view;
    VkDeviceMemory mem;
    VkExtent3D     extent;
};

/// Interface
////////////////////////////////////////////////////////////
static void LoadImageData(ImageData* image_data, const char* path)
{
    image_data->data = stbi_load(path, &image_data->width, &image_data->height, &image_data->channel_count, 0);
    if (image_data->data == NULL)
    {
        CTK_FATAL("failed to open image file from path '%s'", path);
    }

    image_data->size = image_data->width * image_data->height * image_data->channel_count;
}

static void DestroyImageData(ImageData* image_data)
{
    stbi_image_free(image_data->data);
    *image_data = {};
}

static void InitImage(Image* image, ImageInfo* info)
{
    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;

    res = vkCreateImage(device, &info->image, NULL, &image->hnd);
    Validate(res, "vkCreateImage() failed");

    // Allocate/bind image memory.
    VkMemoryRequirements mem_requirements = {};
    vkGetImageMemoryRequirements(device, image->hnd, &mem_requirements);
PrintResourceMemoryInfo("image", &mem_requirements, info->mem_property_flags);

    image->mem = AllocateDeviceMemory(mem_requirements, info->mem_property_flags, NULL);
    res = vkBindImageMemory(device, image->hnd, image->mem, 0);
    Validate(res, "vkBindImageMemory() failed");

    image->extent = info->image.extent;

    info->view.image = image->hnd;
    res = vkCreateImageView(device, &info->view, NULL, &image->view);
    Validate(res, "vkCreateImageView() failed");
}

static void DeinitImage(Image* image)
{
    VkDevice device = global_ctx.device;
    vkDestroyImageView(device, image->view, NULL);
    vkDestroyImage(device, image->hnd, NULL);
    vkFreeMemory(device, image->mem, NULL);
}

static ImageHnd CreateImage(ImageInfo* info)
{
    ImageHnd image_hnd = AllocateImage();
    InitImage(GetImage(image_hnd), info);
    return image_hnd;
}

static VkSampler CreateSampler(VkSamplerCreateInfo* info)
{
    VkSampler sampler = VK_NULL_HANDLE;
    VkResult res = vkCreateSampler(global_ctx.device, info, NULL, &sampler);
    Validate(res, "vkCreateSampler() failed");
    return sampler;
}

}
