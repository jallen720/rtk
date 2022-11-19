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
using MeshHnd   = PoolHnd;
using BufferHnd = PoolHnd;

struct Mesh
{
    uint32 vertex_offset;
    uint32 index_offset;
    uint32 index_count;
};

struct RTKStateInfo
{
    uint32 max_meshes;
    uint32 max_buffers;

    VkDeviceSize vertex_buffer_size;
    VkDeviceSize index_buffer_size;
};

struct RTKState
{
    // Pools
    RTKPool<Mesh>   mesh_pool;
    RTKPool<Buffer> buffer_pool;

    // Mesh State
    Buffer* vertex_buffer;
    Buffer* index_buffer;
};

/// Utils
////////////////////////////////////////////////////////////
static void InitMeshState(RTKState* state, RTKContext* rtk, RTKStateInfo* info)
{
    BufferInfo vertex_buffer_info =
    {
        .size               = info->vertex_buffer_size,
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    state->vertex_buffer = AllocatePtr(&state->buffer_pool);
    InitBuffer(state->vertex_buffer, rtk, &vertex_buffer_info);

    BufferInfo index_buffer_info =
    {
        .size               = info->index_buffer_size,
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    state->index_buffer = AllocatePtr(&state->buffer_pool);
    InitBuffer(state->index_buffer, rtk, &index_buffer_info);
}

/// Interface
////////////////////////////////////////////////////////////
static void InitRTKState(RTKState* state, Stack* mem, RTKContext* rtk, RTKStateInfo* info)
{
    InitRTKPool(&state->mesh_pool, mem, info->max_meshes);
    InitRTKPool(&state->buffer_pool, mem, info->max_buffers);
    InitMeshState(state, rtk, info);
}

template<typename VertexType>
static MeshHnd CreateMesh(RTKState* state, Array<VertexType> vertexes, Array<uint32> indexes)
{
    MeshHnd mesh_hnd = AllocateHnd(&state->mesh_pool);
    Mesh* mesh = GetDataPtr(&state->mesh_pool, mesh_hnd);

    mesh->vertex_offset = state->vertex_buffer->index / sizeof(VertexType);
    mesh->index_offset  = state->index_buffer->index / sizeof(uint32);
    mesh->index_count   = indexes.count;

    Write(state->vertex_buffer, vertexes.data, ByteSize(&vertexes));
    Write(state->index_buffer, indexes.data, ByteSize(&indexes));

    return mesh_hnd;
}

}
