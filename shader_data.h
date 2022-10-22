#pragma once

#include "rtk/vulkan.h"
#include "rtk/rtk_context.h"
#include "rtk/memory.h"
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/containers.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
template<typename Layout>
struct ShaderData
{
    Array<Buffer> buffers;
};

/// Interface
////////////////////////////////////////////////////////////
template<typename Layout>
static void Init(ShaderData<Layout>* shader_data, Stack* mem, RTKContext* rtk)
{
    InitArrayFull(&shader_data->buffers, mem, rtk->frames.size);
    for (uint32 i = 0; i < rtk->frames.size; ++i)
    {
        InitBuffer(GetPtr(&shader_data->buffers, i), rtk,
        {
            .size               = sizeof(Layout),
            .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
            .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        });
    }
}

template<typename Layout>
static Layout* GetPtr(ShaderData<Layout>* shader_data, uint32 frame_index)
{
    return (Layout*)GetPtr(&shader_data->buffers, frame_index)->mapped_mem;
}

}
