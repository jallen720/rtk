#pragma once

#include <time.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

// Disable warnings when including stb_image.h.
#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include "rtk/stb/stb_image.h"
#pragma warning(pop)

#include "ctk3/ctk.h"
using namespace CTK;

namespace RTK
{
#ifdef USE_WIP
    // Utils
    #include "rtk/debug.h"
    #include "rtk/vk_array.h"
    #include "rtk/device_features.h"

    // Context
    #include "rtk/context.h"

    // Resources
    #include "rtk/resource_2.h"
    #include "rtk/buffer_2.h"
    #include "rtk/image_2.h"

    // Assets
    #include "rtk/gltf.h"
    #include "rtk/mesh_2.h"
    #include "rtk/descriptor_set.h"
    #include "rtk/render_target_2.h"
    #include "rtk/pipeline.h"

    // Misc.
    #include "rtk/rendering.h"
    #include "rtk/frame_metrics.h"
#else
    // Utils
    #include "rtk/debug.h"
    #include "rtk/vk_array.h"
    #include "rtk/device_features.h"

    // Context
    #include "rtk/context.h"

    // Resources
    #include "rtk/resource.h"
    #include "rtk/buffer.h"
    #include "rtk/image.h"

    // Assets
    #include "rtk/gltf.h"
    #include "rtk/mesh.h"
    #include "rtk/descriptor_set.h"
    #include "rtk/render_target.h"
    #include "rtk/pipeline.h"

    // Misc.
    #include "rtk/rendering.h"
    #include "rtk/frame_metrics.h"
#endif

}
