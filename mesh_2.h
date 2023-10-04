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

// /// Interface
// ////////////////////////////////////////////////////////////
// static void InitMeshData(MeshData* mesh_data, BufferStack* buffer_stack, MeshDataInfo* info)
// {
//     BufferInfo vertex_buffer_info =
//     {
//         .type             = BufferType::BUFFER,
//         .size             = info->vertex_buffer_size,
//         .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
//         .instance_count   = 1,
//     };
//     InitBuffer(&mesh_data->vertex_buffer, buffer_stack, &vertex_buffer_info);

//     BufferInfo index_buffer_info =
//     {
//         .type             = BufferType::BUFFER,
//         .size             = info->index_buffer_size,
//         .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
//         .instance_count   = 1,
//     };
//     InitBuffer(&mesh_data->index_buffer, buffer_stack, &index_buffer_info);
// }

// static MeshData* CreateMeshData(const Allocator* allocator, BufferStack* buffer_stack, MeshDataInfo* info)
// {
//     auto mesh_data = Allocate<MeshData>(allocator, 1);
//     InitMeshData(mesh_data, buffer_stack, info);
//     return mesh_data;
// }

// template<typename VertexType>
// static void InitHostMesh(Mesh* mesh, MeshData* mesh_data, Array<VertexType> vertexes, Array<uint32> indexes)
// {
//     mesh->vertex_offset = (sint32)(mesh_data->vertex_buffer.indexes[0] / sizeof(VertexType));
//     mesh->index_offset  = (uint32)(mesh_data->index_buffer.indexes[0] / sizeof(uint32));
//     mesh->index_count   = indexes.count;

//     AppendHostBuffer(&mesh_data->vertex_buffer, 0, vertexes.data, ByteSize(&vertexes));
//     AppendHostBuffer(&mesh_data->index_buffer,  0, indexes.data, ByteSize(&indexes));
// }

// template<typename VertexType>
// static Mesh* CreateHostMesh(const Allocator* allocator, MeshData* mesh_data, Array<VertexType> vertexes,
//                             Array<uint32> indexes)
// {
//     auto mesh = Allocate<Mesh>(allocator, 1);
//     InitHostMesh(mesh, mesh_data, vertexes, indexes);
//     return mesh;
// }

// template<typename VertexType>
// static void InitDeviceMesh(Mesh* mesh, MeshData* mesh_data, Array<VertexType> vertexes, Array<uint32> indexes,
//                            Buffer* staging_buffer)
// {
//     mesh->vertex_offset = (sint32)(mesh_data->vertex_buffer.indexes[0] / sizeof(VertexType));
//     mesh->index_offset  = (uint32)(mesh_data->index_buffer.indexes[0] / sizeof(uint32));
//     mesh->index_count   = indexes.count;

//     Clear(staging_buffer);
//     VkDeviceSize vertexes_size = ByteSize(&vertexes);
//     VkDeviceSize indexes_size  = ByteSize(&indexes);
//     AppendHostBuffer(staging_buffer, 0, vertexes.data, vertexes_size);
//     AppendHostBuffer(staging_buffer, 0, indexes.data,  indexes_size);
//     BeginTempCommandBuffer();
//         AppendDeviceBufferCmd(&mesh_data->vertex_buffer, 0, staging_buffer, 0, 0,             vertexes_size);
//         AppendDeviceBufferCmd(&mesh_data->index_buffer,  0, staging_buffer, 0, vertexes_size, indexes_size);
//     SubmitTempCommandBuffer();
// }

// template<typename VertexType>
// static Mesh* CreateDeviceMesh(const Allocator* allocator, MeshData* mesh_data, Array<VertexType> vertexes,
//                               Array<uint32> indexes, Buffer* staging_buffer)
// {
//     auto mesh = Allocate<Mesh>(allocator, 1);
//     InitDeviceMesh(mesh, mesh_data, vertexes, indexes, staging_buffer);
//     return mesh;
// }
