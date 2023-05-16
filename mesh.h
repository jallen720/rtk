/// Data
////////////////////////////////////////////////////////////
struct MeshDataInfo
{
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
    sint32 vertex_offset;
    uint32 index_offset;
    uint32 index_count;
};

/// Interface
////////////////////////////////////////////////////////////
static MeshData* CreateMeshData(const Allocator* allocator, DeviceStack* device_stack, MeshDataInfo* info)
{
    auto mesh_data = Allocate<MeshData>(allocator, 1);

    InitBuffer(&mesh_data->vertex_buffer, device_stack, info->vertex_buffer_size);
    InitBuffer(&mesh_data->index_buffer, device_stack, info->index_buffer_size);

    return mesh_data;
}

template<typename VertexType>
static Mesh* CreateHostMesh(const Allocator* allocator, MeshData* mesh_data, Array<VertexType> vertexes,
                            Array<uint32> indexes)
{
    auto mesh = Allocate<Mesh>(allocator, 1);

    mesh->vertex_offset = (sint32)(mesh_data->vertex_buffer.index / sizeof(VertexType));
    mesh->index_offset  = (uint32)(mesh_data->index_buffer.index / sizeof(uint32));
    mesh->index_count   = indexes.count;

    AppendHostBuffer(&mesh_data->vertex_buffer, vertexes.data, ByteSize(&vertexes));
    AppendHostBuffer(&mesh_data->index_buffer, indexes.data, ByteSize(&indexes));

    return mesh;
}

template<typename VertexType>
static Mesh* CreateDeviceMesh(const Allocator* allocator, MeshData* mesh_data, Array<VertexType> vertexes,
                              Array<uint32> indexes, Buffer* staging_buffer)
{
    auto mesh = Allocate<Mesh>(allocator, 1);

    mesh->vertex_offset = (sint32)(mesh_data->vertex_buffer.index / sizeof(VertexType));
    mesh->index_offset  = (uint32)(mesh_data->index_buffer.index / sizeof(uint32));
    mesh->index_count   = indexes.count;

    Clear(staging_buffer);
    VkDeviceSize vertexes_size = ByteSize(&vertexes);
    VkDeviceSize indexes_size  = ByteSize(&indexes);
    AppendHostBuffer(staging_buffer, vertexes.data, vertexes_size);
    AppendHostBuffer(staging_buffer, indexes.data,  indexes_size);
    BeginTempCommandBuffer();
        AppendDeviceBufferCmd(&mesh_data->vertex_buffer, staging_buffer, 0,             vertexes_size);
        AppendDeviceBufferCmd(&mesh_data->index_buffer,  staging_buffer, vertexes_size, indexes_size);
    SubmitTempCommandBuffer();

    return mesh;
}
