#pragma once

#include "ctk2/ctk.h"
#include "rtk/vulkan.h"
#include "rtk/memory.h"

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
static void InitMeshData(MeshData* mesh_data, MeshDataInfo* info)
{
    InitBuffer(&mesh_data->vertex_buffer, info->parent_buffer, info->vertex_buffer_size);
    InitBuffer(&mesh_data->index_buffer, info->parent_buffer, info->index_buffer_size);
}

template<typename VertexType>
static void InitMesh(Mesh* mesh, MeshData* mesh_data, Array<VertexType> vertexes, Array<uint32> indexes)
{
    mesh->vertex_offset = mesh_data->vertex_buffer.index / sizeof(VertexType);
    mesh->index_offset  = mesh_data->index_buffer.index / sizeof(uint32);
    mesh->index_count   = indexes.count;

    Write(&mesh_data->vertex_buffer, vertexes.data, ByteSize(&vertexes));
    Write(&mesh_data->index_buffer, indexes.data, ByteSize(&indexes));
}

}
