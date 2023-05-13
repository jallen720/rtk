#pragma once

#include "ctk3/ctk3.h"
#include "rtk/vulkan.h"
#include "rtk/buffer.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct MeshDataInfo
{
    Buffer*      parent_buffer;
    VkDeviceSize vertex_buffer_size;
    VkDeviceSize index_buffer_size;
};

struct MeshData
{
    Buffer vertex_buffer;
    Buffer index_buffer;
};

struct Mesh
{
    uint32 vertex_offset;
    uint32 index_offset;
    uint32 index_count;
};

/// Interface
////////////////////////////////////////////////////////////
static MeshData* CreateMeshData(const Allocator* allocator, MeshDataInfo* info)
{
    auto mesh_data = Allocate<MeshData>(allocator, 1);

    InitBuffer(&mesh_data->vertex_buffer, info->parent_buffer, info->vertex_buffer_size);
    InitBuffer(&mesh_data->index_buffer, info->parent_buffer, info->index_buffer_size);

    return mesh_data;
}

template<typename VertexType>
static Mesh* CreateMesh(const Allocator* allocator, MeshData* mesh_data, Array<VertexType> vertexes,
                        Array<uint32> indexes)
{
    auto mesh = Allocate<Mesh>(allocator, 1);

    mesh->vertex_offset = mesh_data->vertex_buffer.index / sizeof(VertexType);
    mesh->index_offset  = mesh_data->index_buffer.index / sizeof(uint32);
    mesh->index_count   = indexes.count;
    WriteHostBuffer(&mesh_data->vertex_buffer, vertexes.data, ByteSize(&vertexes));
    WriteHostBuffer(&mesh_data->index_buffer, indexes.data, ByteSize(&indexes));

    return mesh;
}

}
