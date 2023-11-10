/// Data
////////////////////////////////////////////////////////////
struct MeshHnd      { uint32 index; };
struct MeshGroupHnd { uint32 index; };

struct MeshInfo
{
    VkDeviceSize vertex_size;
    uint32       vertex_count;
    VkDeviceSize index_size;
    uint32       index_count;
};

struct Mesh
{
    VkDeviceSize vertex_size;
    uint32       vertex_offset;
    VkDeviceSize index_size;
    uint32       index_offset;
    uint32       index_count;
};

struct MeshGroupInfo
{
    uint32                max_meshes;
    VkMemoryPropertyFlags mem_properties;
};

struct MeshGroup
{
    Array<Mesh>           meshes;
    VkMemoryPropertyFlags mem_properties;

    VkDeviceSize          vertex_buffer_size;
    VkDeviceSize          index_buffer_size;
    BufferHnd             vertex_buffer;
    BufferHnd             index_buffer;
};

struct MeshModuleInfo
{
    uint32 max_mesh_groups;
};

static Array<MeshGroup> g_mesh_groups;

/// Utils
////////////////////////////////////////////////////////////
static uint32 GetMeshIndex(MeshHnd mesh_hnd)
{
    return 0x00FFFFFF & mesh_hnd.index;
}

static uint32 GetMeshGroupIndex(MeshHnd mesh_hnd)
{
    return (0xFF000000 & mesh_hnd.index) >> 24;
}

static uint32 CreateMeshHndIndex(MeshGroupHnd mesh_group_hnd, uint32 mesh_index)
{
    return (mesh_group_hnd.index << 24) | mesh_index;
}

static MeshGroup* GetMeshGroup(uint32 mesh_group_index)
{
    return &g_mesh_groups.data[mesh_group_index];
}

static Mesh* GetMesh(MeshGroup* mesh_group, uint32 mesh_index)
{
    return &mesh_group->meshes.data[mesh_index];
}

static void ValidateMeshGroupIndex(uint32 mesh_group_index, const char* action)
{
    if (mesh_group_index >= g_mesh_groups.count)
    {
        CTK_FATAL("%s: mesh group index %u exceeds mesh group count of %u",
                  action, mesh_group_index, g_mesh_groups.count);
    }
}

static void ValidateMeshIndex(MeshGroup* mesh_group, uint32 mesh_group_index, uint32 mesh_index, const char* action)
{
    if (mesh_index >= mesh_group->meshes.count)
    {
        CTK_FATAL("%s: mesh index %u exceeds mesh group %u's mesh count of %u",
                  action, mesh_index, mesh_group_index, mesh_group->meshes.count);
    }
}

static void ValidateMeshLoad(MeshGroup* mesh_group, uint32 mesh_group_index, VkDeviceSize vertexes_byte_size,
                             VkDeviceSize indexes_byte_size)
{
    if (vertexes_byte_size >= mesh_group->vertex_buffer_size)
    {
        CTK_FATAL("can't load mesh data: vertex data size of %u exceeds mesh group %u's vertex buffer size of %u",
                  vertexes_byte_size, mesh_group_index, mesh_group->vertex_buffer_size);

    }
    if (indexes_byte_size >= mesh_group->index_buffer_size)
    {
        CTK_FATAL("can't load mesh data: index data size of %u exceeds mesh group %u's index buffer size of %u",
                  indexes_byte_size, mesh_group_index, mesh_group->index_buffer_size);

    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitMeshModule(const Allocator* allocator, MeshModuleInfo info)
{
    InitArray(&g_mesh_groups, allocator, info.max_mesh_groups);
};

static MeshGroupHnd CreateMeshGroup(const Allocator* allocator, MeshGroupInfo* info)
{
    if (g_mesh_groups.count >= g_mesh_groups.size)
    {
        CTK_FATAL("can't create mesh group: already at max of %u", g_mesh_groups.size);
    }

    MeshGroupHnd hnd = { .index = g_mesh_groups.count };
    MeshGroup* mesh_group = Push(&g_mesh_groups);
    InitArray(&mesh_group->meshes, allocator, info->max_meshes);
    mesh_group->mem_properties = info->mem_properties;

    return hnd;
}

static MeshHnd CreateMesh(MeshGroupHnd mesh_group_hnd, MeshInfo* info)
{
    ValidateMeshGroupIndex(mesh_group_hnd.index, "can't create mesh");
    MeshGroup* mesh_group = GetMeshGroup(mesh_group_hnd.index);

    if (mesh_group->meshes.count >= mesh_group->meshes.size)
    {
        CTK_FATAL("can't create mesh: already at max of %u", mesh_group->meshes.size);
    }

    MeshHnd mesh_hnd = { .index = CreateMeshHndIndex(mesh_group_hnd, mesh_group->meshes.count) };
    Mesh* mesh = Push(&mesh_group->meshes);
    mesh->vertex_size   = info->vertex_size;
    mesh->vertex_offset = mesh_group->vertex_buffer_size;
    mesh->index_size    = info->index_size;
    mesh->index_offset  = mesh_group->index_buffer_size;
    mesh->index_count   = info->index_count;

    mesh_group->vertex_buffer_size += info->vertex_count * info->vertex_size;
    mesh_group->index_buffer_size  += info->index_count  * info->index_size;

    return mesh_hnd;
}

static void InitMeshGroup(MeshGroupHnd mesh_group_hnd, ResourceGroupHnd res_group_hnd)
{
    ValidateMeshGroupIndex(mesh_group_hnd.index, "can't initialize mesh group");
    MeshGroup* mesh_group = GetMeshGroup(mesh_group_hnd.index);

    BufferInfo vertex_buffer_info =
    {
        .size             = mesh_group->vertex_buffer_size,
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame        = false,
        .mem_properties   = mesh_group->mem_properties,
        .usage            = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    };
    mesh_group->vertex_buffer = CreateBuffer(res_group_hnd, &vertex_buffer_info);

    BufferInfo index_buffer_info =
    {
        .size             = mesh_group->index_buffer_size,
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame        = false,
        .mem_properties   = mesh_group->mem_properties,
        .usage            = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    };
    mesh_group->index_buffer = CreateBuffer(res_group_hnd, &index_buffer_info);
}

template<typename VertexType>
static void LoadHostMesh(MeshHnd mesh_hnd, Array<VertexType> vertexes, Array<uint32> indexes)
{
    uint32 mesh_group_index = GetMeshGroupIndex(mesh_hnd);
    ValidateMeshGroupIndex(mesh_group_index, "can't load mesh");
    MeshGroup* mesh_group = GetMeshGroup(mesh_group_index);

    uint32 mesh_index = GetMeshIndex(mesh_hnd);
    ValidateMeshIndex(mesh_group, mesh_group_index, mesh_index, "can't load mesh");
    Mesh* mesh = GetMesh(mesh_group, mesh_index);

    VkDeviceSize vertexes_byte_size = ByteSize(&vertexes);
    VkDeviceSize indexes_byte_size  = ByteSize(&indexes);
    ValidateMeshLoad(mesh_group, mesh_group_index, vertexes_byte_size, indexes_byte_size);

    static constexpr uint32 FRAME_INDEX = 0;
    HostBufferWrite vertex_buffer_write =
    {
        .size       = vertexes_byte_size,
        .src_data   = vertexes.data,
        .src_offset = 0,
        .dst_hnd    = mesh_group->vertex_buffer,
        .dst_offset = mesh->vertex_offset,
    };
    WriteHostBuffer(&vertex_buffer_write, FRAME_INDEX);
    HostBufferWrite index_buffer_write =
    {
        .size       = indexes_byte_size,
        .src_data   = indexes.data,
        .src_offset = 0,
        .dst_hnd    = mesh_group->index_buffer,
        .dst_offset = mesh->index_offset,
    };
    WriteHostBuffer(&index_buffer_write, FRAME_INDEX);
}

template<typename VertexType>
static void
LoadDeviceMesh(MeshHnd mesh_hnd, BufferHnd staging_buffer, Array<VertexType> vertexes, Array<uint32> indexes)
{
    uint32 mesh_group_index = GetMeshGroupIndex(mesh_hnd);
    ValidateMeshGroupIndex(mesh_group_index, "can't load mesh");
    MeshGroup* mesh_group = GetMeshGroup(mesh_group_index);

    uint32 mesh_index = GetMeshIndex(mesh_hnd);
    ValidateMeshIndex(mesh_group, mesh_group_index, mesh_index, "can't load mesh");
    Mesh* mesh = GetMesh(mesh_group, mesh_index);

    VkDeviceSize vertexes_byte_size = ByteSize(&vertexes);
    VkDeviceSize indexes_byte_size  = ByteSize(&indexes);
    ValidateMeshLoad(mesh_group, mesh_group_index, vertexes_byte_size, indexes_byte_size);

    static constexpr uint32 FRAME_INDEX = 0;
    Clear(staging_buffer);

    HostBufferAppend vertex_staging =
    {
        .size       = vertexes_byte_size,
        .src_data   = vertexes.data,
        .src_offset = 0,
        .dst_hnd    = staging_buffer,
    };
    AppendHostBuffer(&vertex_staging, FRAME_INDEX);
    HostBufferAppend index_staging =
    {
        .size       = indexes_byte_size,
        .src_data   = indexes.data,
        .src_offset = 0,
        .dst_hnd    = staging_buffer,
    };
    AppendHostBuffer(&index_staging, FRAME_INDEX);

    BeginTempCommandBuffer();
        DeviceBufferWrite vertex_buffer_write =
        {
            .size       = vertexes_byte_size,
            .src_hnd    = staging_buffer,
            .src_offset = 0,
            .dst_hnd    = mesh_group->vertex_buffer,
            .dst_offset = mesh->vertex_offset,
        };
        WriteDeviceBufferCmd(&vertex_buffer_write, FRAME_INDEX);
        DeviceBufferWrite index_buffer_write =
        {
            .size       = indexes_byte_size,
            .src_hnd    = staging_buffer,
            .src_offset = vertexes_byte_size,
            .dst_hnd    = mesh_group->index_buffer,
            .dst_offset = mesh->index_offset,
        };
        WriteDeviceBufferCmd(&index_buffer_write, FRAME_INDEX);
    SubmitTempCommandBuffer();
}

static MeshGroup* GetMeshGroup(MeshGroupHnd mesh_group_hnd)
{
    ValidateMeshGroupIndex(mesh_group_hnd.index, "can't get mesh");
    return GetMeshGroup(mesh_group_hnd.index);
}

static Mesh* GetMesh(MeshHnd mesh_hnd)
{
    uint32 mesh_group_index = GetMeshGroupIndex(mesh_hnd);
    ValidateMeshGroupIndex(mesh_group_index, "can't get mesh");
    MeshGroup* mesh_group = GetMeshGroup(mesh_group_index);

    uint32 mesh_index = GetMeshIndex(mesh_hnd);
    ValidateMeshIndex(mesh_group, mesh_group_index, mesh_index, "can't get mesh");
    return GetMesh(mesh_group, mesh_index);
}
