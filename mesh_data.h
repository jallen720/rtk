#pragma once

#include "ctk2/ctk.h"
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
    BufferHnd    parent_buffer;
    VkDeviceSize vertex_buffer_size;
    VkDeviceSize index_buffer_size;
};

struct MeshData
{
    BufferHnd vertex_buffer;
    BufferHnd index_buffer;
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
    mesh_data->vertex_buffer = CreateBuffer(info->parent_buffer, info->vertex_buffer_size);
    mesh_data->index_buffer  = CreateBuffer(info->parent_buffer, info->index_buffer_size);
    return mesh_data_hnd;
}

template<typename VertexType>
static MeshHnd CreateMesh(MeshDataHnd mesh_data_hnd, Array<VertexType> vertexes, Array<uint32> indexes)
{
    MeshData* mesh_data = GetMeshData(mesh_data_hnd);

    MeshHnd mesh_hnd = AllocateMesh();
    Mesh* mesh = GetMesh(mesh_hnd);
    mesh->vertex_offset = GetBuffer(mesh_data->vertex_buffer)->index / sizeof(VertexType);
    mesh->index_offset  = GetBuffer(mesh_data->index_buffer)->index / sizeof(uint32);
    mesh->index_count   = indexes.count;

    Write(mesh_data->vertex_buffer, vertexes.data, ByteSize(&vertexes));
    Write(mesh_data->index_buffer, indexes.data, ByteSize(&indexes));

    return mesh_hnd;
}

}
