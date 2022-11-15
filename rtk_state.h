#pragma once

#include "ctk2/ctk.h"
#include "rtk/vulkan.h"
#include "rtk/rtk_pool.h"
#include "rtk/memory.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
using MeshHnd = uint32;

struct Mesh
{
    uint32 vertex_offset;
    uint32 index_offset;
    uint32 index_count;
};

struct RTKStateInfo
{
    struct
    {
        VkDeviceSize vertex_buffer_size;
        VkDeviceSize index_buffer_size;
        uint32       pool_size;
    }
    mesh;
};

struct RTKState
{
    struct
    {
        Buffer        vertex_buffer;
        Buffer        index_buffer;
        RTKPool<Mesh> pool;
    }
    mesh;
};

static void InitRTKState(RTKState* state, Stack* mem, RTKContext* rtk, RTKStateInfo* info)
{
    InitRTKPool(&state->mesh.pool, mem, info->mesh.pool_size);
    {
        BufferInfo vertex_buffer_info =
        {
            .size               = info->mesh.vertex_buffer_size,
            .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
            .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        InitBuffer(&state->mesh.vertex_buffer, rtk, &vertex_buffer_info);
    }
    {
        BufferInfo index_buffer_info =
        {
            .size               = info->mesh.index_buffer_size,
            .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
            .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };
        InitBuffer(&state->mesh.index_buffer, rtk, &index_buffer_info);
    }
}

template<typename VertexType>
static MeshHnd CreateMesh(RTKState* state, Array<VertexType> vertexes, Array<uint32> indexes)
{
    MeshHnd mesh_hnd = Allocate(&state->mesh.pool);

    Mesh* mesh = GetNodeData(&state->mesh.pool, mesh_hnd);
    mesh->vertex_offset = state->mesh.vertex_buffer.index / sizeof(VertexType);
    mesh->index_offset  = state->mesh.index_buffer.index / sizeof(uint32);
    mesh->index_count   = indexes.count;

    Write(&state->mesh.vertex_buffer, vertexes.data, ByteSize(&vertexes));
    Write(&state->mesh.index_buffer, indexes.data, ByteSize(&indexes));

    return mesh_hnd;
}

}
