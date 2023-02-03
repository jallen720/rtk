#pragma once

#include "ctk3/ctk3.h"
#include "rtk/vulkan.h"
#include "rtk/resources.h"
#include "rtk/buffer.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct MeshDataInfo
{
    BufferHnd    parent_buffer_hnd;
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
static MeshDataHnd CreateMeshData(MeshDataInfo* info)
{
    MeshDataHnd mesh_data_hnd = AllocateMeshData();
    MeshData* mesh_data = GetMeshData(mesh_data_hnd);
    Buffer* parent_buffer = GetBuffer(info->parent_buffer_hnd);
    InitBuffer(&mesh_data->vertex_buffer, parent_buffer, info->vertex_buffer_size);
    InitBuffer(&mesh_data->index_buffer, parent_buffer, info->index_buffer_size);
    return mesh_data_hnd;
}

template<typename VertexType>
static MeshHnd CreateMesh(MeshDataHnd mesh_data_hnd, Array<VertexType> vertexes, Array<uint32> indexes)
{
    MeshData* mesh_data = GetMeshData(mesh_data_hnd);

    MeshHnd mesh_hnd = AllocateMesh();
    Mesh* mesh = GetMesh(mesh_hnd);
    mesh->vertex_offset = mesh_data->vertex_buffer.index / sizeof(VertexType);
    mesh->index_offset  = mesh_data->index_buffer.index / sizeof(uint32);
    mesh->index_count   = indexes.count;

    Write(&mesh_data->vertex_buffer, vertexes.data, ByteSize(&vertexes));
    Write(&mesh_data->index_buffer, indexes.data, ByteSize(&indexes));

    return mesh_hnd;
}

}
