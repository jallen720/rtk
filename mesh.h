/// Data
////////////////////////////////////////////////////////////
struct MeshHnd      { uint32 index; };
struct MeshGroupHnd { uint32 index; };

struct MeshData
{
    uint8* vertex_buffer;
    uint32 vertex_size;
    uint32 vertex_count;
    uint8* index_buffer;
    uint32 index_size;
    uint32 index_count;
};

struct MeshInfo
{
    uint32 vertex_size;
    uint32 vertex_count;
    uint32 index_size;
    uint32 index_count;
};

struct Mesh
{
    uint32 vertex_buffer_offset;
    uint32 vertex_buffer_index_offset;
    uint32 index_buffer_offset;
    uint32 index_buffer_index_offset;
    uint32 index_count;
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

    BufferHnd vertex_buffer;
    uint32    vertex_buffer_size;
    uint32    vertex_count;

    BufferHnd index_buffer;
    uint32    index_buffer_size;
    uint32    index_count;
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

static void ValidateMeshDataLoad(MeshGroup* mesh_group, uint32 mesh_group_index, uint32 vertex_buffer_size,
                                 uint32 index_buffer_size)
{
    if (vertex_buffer_size >= mesh_group->vertex_buffer_size)
    {
        CTK_FATAL("can't load mesh data: vertex buffer size of %u exceeds mesh group %u's vertex buffer size of %u",
                  vertex_buffer_size, mesh_group_index, mesh_group->vertex_buffer_size);

    }
    if (index_buffer_size >= mesh_group->index_buffer_size)
    {
        CTK_FATAL("can't load mesh data: index buffer size of %u exceeds mesh group %u's index buffer size of %u",
                  index_buffer_size, mesh_group_index, mesh_group->index_buffer_size);

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
    mesh->vertex_buffer_offset       = mesh_group->vertex_buffer_size;
    mesh->vertex_buffer_index_offset = mesh_group->vertex_count;
    mesh->index_buffer_offset        = mesh_group->index_buffer_size;
    mesh->index_buffer_index_offset  = mesh_group->index_count;
    mesh->index_count                = info->index_count;

    mesh_group->vertex_buffer_size += info->vertex_count * info->vertex_size;
    mesh_group->vertex_count       += info->vertex_count;
    mesh_group->index_buffer_size  += info->index_count  * info->index_size;
    mesh_group->index_count        += info->index_count;

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

static void LoadHostMesh(MeshHnd mesh_hnd, MeshData* mesh_data)
{
    uint32 mesh_group_index = GetMeshGroupIndex(mesh_hnd);
    ValidateMeshGroupIndex(mesh_group_index, "can't load mesh");
    MeshGroup* mesh_group = GetMeshGroup(mesh_group_index);

    uint32 vertex_buffer_size = mesh_data->vertex_size * mesh_data->vertex_count;
    uint32 index_buffer_size  = mesh_data->index_size  * mesh_data->index_count;
    ValidateMeshDataLoad(mesh_group, mesh_group_index, vertex_buffer_size, index_buffer_size);

    uint32 mesh_index = GetMeshIndex(mesh_hnd);
    ValidateMeshIndex(mesh_group, mesh_group_index, mesh_index, "can't load mesh");
    Mesh* mesh = GetMesh(mesh_group, mesh_index);

    static constexpr uint32 FRAME_INDEX = 0;
    HostBufferWrite vertex_buffer_write =
    {
        .size       = vertex_buffer_size,
        .src_data   = mesh_data->vertex_buffer,
        .src_offset = 0,
        .dst_hnd    = mesh_group->vertex_buffer,
        .dst_offset = mesh->vertex_buffer_offset,
    };
    WriteHostBuffer(&vertex_buffer_write, FRAME_INDEX);
    HostBufferWrite index_buffer_write =
    {
        .size       = index_buffer_size,
        .src_data   = mesh_data->index_buffer,
        .src_offset = 0,
        .dst_hnd    = mesh_group->index_buffer,
        .dst_offset = mesh->index_buffer_offset,
    };
    WriteHostBuffer(&index_buffer_write, FRAME_INDEX);
}

static void LoadDeviceMesh(MeshHnd mesh_hnd, BufferHnd staging_buffer_hnd, MeshData* mesh_data)
{
    ValidateBuffer(staging_buffer_hnd, "can't load mesh from buffer");

    uint32 mesh_group_index = GetMeshGroupIndex(mesh_hnd);
    ValidateMeshGroupIndex(mesh_group_index, "can't load mesh");
    MeshGroup* mesh_group = GetMeshGroup(mesh_group_index);

    uint32 vertex_buffer_size = mesh_data->vertex_size * mesh_data->vertex_count;
    uint32 index_buffer_size  = mesh_data->index_size * mesh_data->index_count;
    ValidateMeshDataLoad(mesh_group, mesh_group_index, vertex_buffer_size, index_buffer_size);

    uint32 mesh_index = GetMeshIndex(mesh_hnd);
    ValidateMeshIndex(mesh_group, mesh_group_index, mesh_index, "can't load mesh");
    Mesh* mesh = GetMesh(mesh_group, mesh_index);

    static constexpr uint32 FRAME_INDEX = 0;
    Clear(staging_buffer_hnd);

    HostBufferAppend vertex_staging =
    {
        .size       = vertex_buffer_size,
        .src_data   = mesh_data->vertex_buffer,
        .src_offset = 0,
        .dst_hnd    = staging_buffer_hnd,
    };
    AppendHostBuffer(&vertex_staging, FRAME_INDEX);
    HostBufferAppend index_staging =
    {
        .size       = index_buffer_size,
        .src_data   = mesh_data->index_buffer,
        .src_offset = 0,
        .dst_hnd    = staging_buffer_hnd,
    };
    AppendHostBuffer(&index_staging, FRAME_INDEX);

    BeginTempCommandBuffer();
        DeviceBufferWrite vertex_buffer_write =
        {
            .size       = vertex_buffer_size,
            .src_hnd    = staging_buffer_hnd,
            .src_offset = 0,
            .dst_hnd    = mesh_group->vertex_buffer,
            .dst_offset = mesh->vertex_buffer_offset,
        };
        WriteDeviceBufferCmd(&vertex_buffer_write, FRAME_INDEX);
        DeviceBufferWrite index_buffer_write =
        {
            .size       = index_buffer_size,
            .src_hnd    = staging_buffer_hnd,
            .src_offset = vertex_buffer_size,
            .dst_hnd    = mesh_group->index_buffer,
            .dst_offset = mesh->index_buffer_offset,
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

static void LoadMeshData(MeshData* mesh_data, const Allocator* allocator, const char* path)
{
    GLTF gltf = {};
    LoadGLTF(&gltf, allocator, path);

    PrintLine("%s:", path);
    PrintGLTF(&gltf, 1);

    CTK_ASSERT(gltf.meshes.count == 1);
    GLTFMesh* mesh = GetPtr(&gltf.meshes, 0);

    CTK_ASSERT(mesh->primitives.count == 1);
    GLTFPrimitive* primitive = GetPtr(&mesh->primitives, 0);
    CTK_ASSERT(primitive->attributes.count > 0);

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview:
    // All attribute accessors for a given primitive MUST have the same count.
    // So get mesh_data->vertex_count from first accessor's count.
    GLTFAccessor* indexes_accessor = GetPtr(&gltf.accessors, primitive->indexes_accessor);
    mesh_data->vertex_count = GetPtr(&gltf.accessors, GetPtr(&primitive->attributes, 0)->accessor)->count;
    mesh_data->index_count  = indexes_accessor->count;

    // Get vertex size from attribute accessors, and index size from index accessor.
    CTK_ITER(attribute, &primitive->attributes)
    {
        GLTFAccessor* accessor = GetPtr(&gltf.accessors, attribute->accessor);
        mesh_data->vertex_size += GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)accessor->type] *
                                  GLTF_COMPONENT_TYPE_SIZES[(uint32)accessor->component_type];
    }
    // Force index size to be 4.
    // mesh_data->index_size = GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)indexes_accessor->type] *
    //                         GLTF_COMPONENT_TYPE_SIZES[(uint32)indexes_accessor->component_type];
    mesh_data->index_size = 4;

    PrintLine("vertex size:  %u", mesh_data->vertex_size);
    PrintLine("vertex count: %u", mesh_data->vertex_count);
    PrintLine("index  size:  %u", mesh_data->index_size);
    PrintLine("index  count: %u", mesh_data->index_count);

    mesh_data->vertex_buffer = Allocate<uint8>(allocator, mesh_data->vertex_size * mesh_data->vertex_count);
    mesh_data->index_buffer  = Allocate<uint8>(allocator, mesh_data->index_size  * mesh_data->index_count);

    // Write attributes from buffer to vertex buffer interleaved.
    uint32 prev_attribute_size = 0;
    CTK_ITER(attribute, &primitive->attributes)
    {
        GLTFAccessor*   accessor    = GetPtr(&gltf.accessors,    attribute->accessor);
        GLTFBufferView* buffer_view = GetPtr(&gltf.buffer_views, accessor->buffer_view);
        GLTFBuffer*     buffer      = GetPtr(&gltf.buffers,      buffer_view->buffer);

        uint32 buffer_offset        = buffer_view->offset + accessor->offset;
        uint32 buffer_view_offset   = 0;
        uint32 vertex_buffer_offset = prev_attribute_size;
        uint32 attribute_size       = GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)accessor->type] *
                                      GLTF_COMPONENT_TYPE_SIZES[(uint32)accessor->component_type];
        while (buffer_view_offset < buffer_view->size)
        {
            memcpy(&mesh_data->vertex_buffer[vertex_buffer_offset],
                   &buffer->data[buffer_offset + buffer_view_offset],
                   attribute_size);
            buffer_view_offset   += attribute_size;
            vertex_buffer_offset += mesh_data->vertex_size;
        }

        prev_attribute_size = attribute_size;
    }

    // Write indexes from buffer to index buffer. Done this way as we're force index size to 4. If we stored indexes in
    // the same size as they are in the GLTF buffer, we could just memcpy() the entire buffer_view for indexes to the
    // index buffer.
    GLTFBufferView* buffer_view = GetPtr(&gltf.buffer_views, indexes_accessor->buffer_view);
    GLTFBuffer*     buffer      = GetPtr(&gltf.buffers,      buffer_view->buffer);

    uint32 buffer_offset       = buffer_view->offset + indexes_accessor->offset;
    uint32 buffer_view_offset  = 0;
    uint32 index_buffer_offset = 0;
    uint32 index_size          = GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)indexes_accessor->type] *
                                 GLTF_COMPONENT_TYPE_SIZES[(uint32)indexes_accessor->component_type];
    while (buffer_view_offset < buffer_view->size)
    {
        memcpy(&mesh_data->index_buffer[index_buffer_offset],
               &buffer->data[buffer_offset + buffer_view_offset],
               index_size);
        buffer_view_offset  += index_size;
        index_buffer_offset += mesh_data->index_size;
    }
}
