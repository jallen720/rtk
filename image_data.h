#pragma once

#include "ctk2/ctk.h"

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

/// Interface
////////////////////////////////////////////////////////////
static void LoadImageData(ImageData* image_data, cstring path)
{
    image_data->data = stbi_load(path, &image_data->width, &image_data->height, &image_data->channel_count, 0);
    if (image_data->data == NULL)
        CTK_FATAL("failed to open image file from path '%s'", path);

    image_data->size = image_data->width * image_data->height * image_data->channel_count;
}

static void DestroyImageData(ImageData* image_data)
{
    stbi_image_free(image_data->data);
    *image_data = {};
}

}
