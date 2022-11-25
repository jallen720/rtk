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
static void InitMeshData(MeshData* mesh_data, MeshDataInfo* info)
{
    mesh_data->vertex_buffer = CreateBuffer(info->parent_buffer, info->vertex_buffer_size);
    mesh_data->index_buffer = CreateBuffer(info->parent_buffer, info->index_buffer_size);
}

template<typename VertexType>
static void InitMesh(Mesh* mesh, MeshData* mesh_data, Array<VertexType> vertexes, Array<uint32> indexes)
{
    mesh->vertex_offset = GetBuffer(mesh_data->vertex_buffer)->index / sizeof(VertexType);
    mesh->index_offset  = GetBuffer(mesh_data->index_buffer)->index / sizeof(uint32);
    mesh->index_count   = indexes.count;

    Write(mesh_data->vertex_buffer, vertexes.data, ByteSize(&vertexes));
    Write(mesh_data->index_buffer, indexes.data, ByteSize(&indexes));
}

}
