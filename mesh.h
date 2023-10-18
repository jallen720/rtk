/// Data
////////////////////////////////////////////////////////////
struct MeshDataInfo
{
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
    sint32 vertex_offset;
    uint32 index_offset;
    uint32 index_count;
};

// /// Interface
// ////////////////////////////////////////////////////////////
static void InitMeshData(MeshData* mesh_data, MeshDataInfo* info)
{
    BufferInfo vertex_buffer_info =
    {
        .size             = info->vertex_buffer_size,
        .usage            = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame        = false,
        .mem_properties   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    mesh_data->vertex_buffer = CreateBuffer(&vertex_buffer_info);

    BufferInfo index_buffer_info =
    {
        .size             = info->index_buffer_size,
        .usage            = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame        = false,
        .mem_properties   = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    mesh_data->index_buffer = CreateBuffer(&index_buffer_info);
}

static MeshData* CreateMeshData(const Allocator* allocator, MeshDataInfo* info)
{
    auto mesh_data = Allocate<MeshData>(allocator, 1);
    InitMeshData(mesh_data, info);
    return mesh_data;
}

template<typename VertexType>
static void InitHostMesh(Mesh* mesh, MeshData* mesh_data, Array<VertexType> vertexes, Array<uint32> indexes)
{
    mesh->vertex_offset = (sint32)(GetIndex(mesh_data->vertex_buffer, 0) / sizeof(VertexType));
    mesh->index_offset  = (uint32)(GetIndex(mesh_data->index_buffer, 0) / sizeof(uint32));
    mesh->index_count   = indexes.count;

    AppendHostBuffer(mesh_data->vertex_buffer, 0, vertexes.data, ByteSize(&vertexes));
    AppendHostBuffer(mesh_data->index_buffer,  0, indexes.data, ByteSize(&indexes));
}

template<typename VertexType>
static Mesh* CreateHostMesh(const Allocator* allocator, MeshData* mesh_data, Array<VertexType> vertexes,
                            Array<uint32> indexes)
{
    auto mesh = Allocate<Mesh>(allocator, 1);
    InitHostMesh(mesh, mesh_data, vertexes, indexes);
    return mesh;
}

template<typename VertexType>
static void InitDeviceMesh(Mesh* mesh, MeshData* mesh_data, Array<VertexType> vertexes, Array<uint32> indexes,
                           BufferHnd staging_buffer)
{
    mesh->vertex_offset = (sint32)(GetIndex(mesh_data->vertex_buffer, 0) / sizeof(VertexType));
    mesh->index_offset  = (uint32)(GetIndex(mesh_data->index_buffer, 0) / sizeof(uint32));
    mesh->index_count   = indexes.count;

    Clear(staging_buffer);
    VkDeviceSize vertexes_size = ByteSize(&vertexes);
    VkDeviceSize indexes_size  = ByteSize(&indexes);
    AppendHostBuffer(staging_buffer, 0, vertexes.data, vertexes_size);
    AppendHostBuffer(staging_buffer, 0, indexes.data,  indexes_size);
    BeginTempCommandBuffer();
        AppendDeviceBufferCmd(mesh_data->vertex_buffer, 0, staging_buffer, 0, 0,             vertexes_size);
        AppendDeviceBufferCmd(mesh_data->index_buffer,  0, staging_buffer, 0, vertexes_size, indexes_size);
    SubmitTempCommandBuffer();
}

template<typename VertexType>
static Mesh* CreateDeviceMesh(const Allocator* allocator, MeshData* mesh_data, Array<VertexType> vertexes,
                              Array<uint32> indexes, BufferHnd staging_buffer)
{
    auto mesh = Allocate<Mesh>(allocator, 1);
    InitDeviceMesh(mesh, mesh_data, vertexes, indexes, staging_buffer);
    return mesh;
}
