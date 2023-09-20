/// Data
////////////////////////////////////////////////////////////
struct MeshDataInfo2
{
    VkDeviceSize vertex_buffer_size;
    VkDeviceSize index_buffer_size;
};

struct MeshData2
{
    Buffer2 vertex_buffer;
    Buffer2 index_buffer;
};

struct Mesh2
{
    sint32 vertex_offset;
    uint32 index_offset;
    uint32 index_count;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitMeshData2(MeshData2* mesh_data, BufferStack2* buffer_stack, MeshDataInfo2* info)
{
    InitBuffer2(&mesh_data->vertex_buffer, buffer_stack, info->vertex_buffer_size);
    InitBuffer2(&mesh_data->index_buffer, buffer_stack, info->index_buffer_size);
}

template<typename VertexType>
static void InitHostMesh2(Mesh2* mesh, MeshData2* mesh_data, Array<VertexType> vertexes, Array<uint32> indexes)
{
    mesh->vertex_offset = (sint32)(mesh_data->vertex_buffer.index / sizeof(VertexType));
    mesh->index_offset  = (uint32)(mesh_data->index_buffer.index / sizeof(uint32));
    mesh->index_count   = indexes.count;

    AppendHostBuffer2(&mesh_data->vertex_buffer, vertexes.data, ByteSize(&vertexes));
    AppendHostBuffer2(&mesh_data->index_buffer, indexes.data, ByteSize(&indexes));
}

template<typename VertexType>
static void InitDeviceMesh2(Mesh2* mesh, MeshData2* mesh_data, Array<VertexType> vertexes, Array<uint32> indexes,
                            Buffer2* staging_buffer)
{
    mesh->vertex_offset = (sint32)(mesh_data->vertex_buffer.index / sizeof(VertexType));
    mesh->index_offset  = (uint32)(mesh_data->index_buffer.index / sizeof(uint32));
    mesh->index_count   = indexes.count;

    Clear2(staging_buffer);
    VkDeviceSize vertexes_size = ByteSize(&vertexes);
    VkDeviceSize indexes_size  = ByteSize(&indexes);
    AppendHostBuffer2(staging_buffer, vertexes.data, vertexes_size);
    AppendHostBuffer2(staging_buffer, indexes.data,  indexes_size);
    BeginTempCommandBuffer();
        AppendDeviceBufferCmd2(&mesh_data->vertex_buffer, staging_buffer, 0,             vertexes_size);
        AppendDeviceBufferCmd2(&mesh_data->index_buffer,  staging_buffer, vertexes_size, indexes_size);
    SubmitTempCommandBuffer();
}
