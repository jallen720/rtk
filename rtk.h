#pragma once

#include <time.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

// Disable warnings when including stb_image.h.
#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include "rtk/stb/stb_image.h"
#pragma warning(pop)

#include "ctk3/ctk3.h"
#include "ctk3/allocator.h"
#include "ctk3/array.h"
#include "ctk3/array2d.h"
#include "ctk3/debug.h"
#include "ctk3/file.h"
#include "ctk3/free_list.h"
#include "ctk3/io.h"
#include "ctk3/optional.h"
#include "ctk3/pair.h"
#include "ctk3/ring_buffer.h"
#include "ctk3/stack.h"
#include "ctk3/string.h"
#include "ctk3/thread_pool.h"
#include "ctk3/window.h"

using namespace CTK;

namespace RTK
{

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
#include "rtk/mesh.h"
#include "rtk/shader.h"
#include "rtk/descriptor_set.h"
#include "rtk/render_target.h"
#include "rtk/pipeline.h"

// Misc.
#include "rtk/rendering.h"
#include "rtk/frame_metrics.h"

}
