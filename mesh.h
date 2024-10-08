/// Data
////////////////////////////////////////////////////////////
struct MeshHnd      { uint32 group_index : 8; uint32 index : 24; };
struct MeshGroupHnd { uint32 index; };

union Swizzle
{
    struct { uint8 x, y, z, w; };
    uint8 array[4];
};

union AttributeSwizzles
{
    struct
    {
        Swizzle* POSITION;
        Swizzle* NORMAL;
        Swizzle* TANGENT;
        Swizzle* TEXCOORD_0;
        Swizzle* TEXCOORD_1;
        Swizzle* TEXCOORD_2;
        Swizzle* TEXCOORD_3;
        Swizzle* COLOR_0;
        Swizzle* COLOR_1;
        Swizzle* COLOR_2;
        Swizzle* COLOR_3;
        Swizzle* JOINTS_0;
        Swizzle* JOINTS_1;
        Swizzle* JOINTS_2;
        Swizzle* JOINTS_3;
        Swizzle* WEIGHTS_0;
        Swizzle* WEIGHTS_1;
        Swizzle* WEIGHTS_2;
        Swizzle* WEIGHTS_3;
    };
    Swizzle* array[(uint32)GLTFAttributeType::COUNT];
};

struct MeshInfo
{
    uint32 vertex_size;
    uint32 vertex_count;
    uint32 index_size;
    uint32 index_count;
};

struct MeshData
{
    uint8*   vertex_buffer;
    uint8*   index_buffer;
    MeshInfo info;
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
    uint32 max_meshes;
    uint32 vertex_buffer_size;
    uint32 index_buffer_size;
};

struct MeshGroup
{
    Array<Mesh> meshes;

    BufferHnd   vertex_buffer;
    uint32      vertex_buffer_size;
    uint32      vertex_count;

    BufferHnd   index_buffer;
    uint32      index_buffer_size;
    uint32      index_count;
};

struct MeshModuleInfo
{
    uint32 max_mesh_groups;
};

static Array<MeshGroup> g_mesh_groups;

/// Utils
////////////////////////////////////////////////////////////
static MeshGroup* GetMeshGroup(uint32 mesh_group_index)
{
    return GetPtr(&g_mesh_groups, mesh_group_index);
}

static Mesh* GetMesh(MeshGroup* mesh_group, uint32 mesh_index)
{
    return GetPtr(&mesh_group->meshes, mesh_index);
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
static void InitMeshModule(Allocator* allocator, MeshModuleInfo info)
{
    g_mesh_groups = CreateArray<MeshGroup>(allocator, info.max_mesh_groups);
};

static MeshGroupHnd CreateMeshGroup(Allocator* allocator, BufferHnd parent_buffer, MeshGroupInfo* info)
{
    if (g_mesh_groups.count >= g_mesh_groups.size)
    {
        CTK_FATAL("can't create mesh group: already at max of %u", g_mesh_groups.size);
    }

    BufferInfo vertex_buffer_info =
    {
        .size      = info->vertex_buffer_size,
        .alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame = false,
    };
    BufferInfo index_buffer_info =
    {
        .size      = info->index_buffer_size,
        .alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame = false,
    };

    MeshGroupHnd hnd = { .index = g_mesh_groups.count };
    MeshGroup* mesh_group = Push(&g_mesh_groups);
    mesh_group->meshes             = CreateArray<Mesh>(allocator, info->max_meshes);
    mesh_group->vertex_buffer_size = info->vertex_buffer_size;
    mesh_group->index_buffer_size  = info->index_buffer_size;
    mesh_group->vertex_buffer      = CreateBuffer(parent_buffer, &vertex_buffer_info);
    mesh_group->index_buffer       = CreateBuffer(parent_buffer, &index_buffer_info);

    return hnd;
}

static MeshHnd CreateMesh(MeshGroupHnd mesh_group_hnd, MeshInfo* info)
{
    MeshGroup* mesh_group = GetMeshGroup(mesh_group_hnd.index);
    if (mesh_group->meshes.count >= mesh_group->meshes.size)
    {
        CTK_FATAL("can't create mesh: already at max of %u", mesh_group->meshes.size);
    }

    // Create mesh handle.
    MeshHnd mesh_hnd = { .group_index = mesh_group_hnd.index, .index = mesh_group->meshes.count };

    // Init mesh.
    BufferFrameState* vertex_buffer_frame_state = GetBufferFrameState(mesh_group->vertex_buffer, 0);
    BufferFrameState* index_buffer_frame_state  = GetBufferFrameState(mesh_group->index_buffer,  0);
    Mesh* mesh = Push(&mesh_group->meshes);
    mesh->vertex_buffer_offset       = vertex_buffer_frame_state->index;
    mesh->index_buffer_offset        = index_buffer_frame_state->index;
    mesh->vertex_buffer_index_offset = mesh_group->vertex_count;
    mesh->index_buffer_index_offset  = mesh_group->index_count;
    mesh->index_count                = info->index_count;

    vertex_buffer_frame_state->index = mesh->vertex_buffer_offset + (info->vertex_count * info->vertex_size);
    index_buffer_frame_state->index  = mesh->index_buffer_offset  + (info->index_count  * info->index_size);
    mesh_group->vertex_count += info->vertex_count;
    mesh_group->index_count  += info->index_count;

    return mesh_hnd;
}

static void LoadHostMesh(MeshHnd mesh_hnd, MeshData* mesh_data)
{
    MeshGroup* mesh_group = GetMeshGroup(mesh_hnd.group_index);

    uint32 vertex_buffer_size = mesh_data->info.vertex_size * mesh_data->info.vertex_count;
    uint32 index_buffer_size  = mesh_data->info.index_size  * mesh_data->info.index_count;
    ValidateMeshDataLoad(mesh_group, mesh_hnd.group_index, vertex_buffer_size, index_buffer_size);

    Mesh* mesh = GetMesh(mesh_group, mesh_hnd.index);

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
    MeshGroup* mesh_group = GetMeshGroup(mesh_hnd.group_index);

    uint32 vertex_buffer_size = mesh_data->info.vertex_size * mesh_data->info.vertex_count;
    uint32 index_buffer_size  = mesh_data->info.index_size  * mesh_data->info.index_count;
    ValidateMeshDataLoad(mesh_group, mesh_hnd.group_index, vertex_buffer_size, index_buffer_size);

    Mesh* mesh = GetMesh(mesh_group, mesh_hnd.index);

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

static void LoadMeshData(MeshData* mesh_data, Allocator* allocator, const char* path,
                         AttributeSwizzles* attribute_swizzles = NULL)
{
    GLTF gltf = {};
    LoadGLTF(&gltf, allocator, path);

    CTK_ASSERT(gltf.meshes.count == 1);
    GLTFMesh* mesh = GetPtr(&gltf.meshes, 0);

    CTK_ASSERT(mesh->primitives.count == 1);
    GLTFPrimitive* primitive = GetPtr(&mesh->primitives, 0);
    CTK_ASSERT(primitive->attributes.count > 0);

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview:
    // All attribute accessors for a given primitive MUST have the same count.
    // So get mesh_data->info.vertex_count from first accessor's count.
    GLTFAccessor* indexes_accessor = GetPtr(&gltf.accessors, primitive->indexes_accessor);
    mesh_data->info.vertex_count = GetPtr(&gltf.accessors, GetPtr(&primitive->attributes, 0)->accessor)->count;
    mesh_data->info.index_count  = indexes_accessor->count;

    // Get vertex size from attribute accessors, and index size from index accessor.
    CTK_ITER(attribute, &primitive->attributes)
    {
        GLTFAccessor* accessor = GetPtr(&gltf.accessors, attribute->accessor);
        mesh_data->info.vertex_size += GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)accessor->type] *
                                  GLTF_COMPONENT_TYPE_SIZES[(uint32)accessor->component_type];
    }
    // Force index size to be 4.
    // mesh_data->info.index_size = GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)indexes_accessor->type] *
    //                         GLTF_COMPONENT_TYPE_SIZES[(uint32)indexes_accessor->component_type];
    mesh_data->info.index_size = 4;

    mesh_data->vertex_buffer = Allocate<uint8>(allocator, mesh_data->info.vertex_size * mesh_data->info.vertex_count);
    mesh_data->index_buffer  = Allocate<uint8>(allocator, mesh_data->info.index_size  * mesh_data->info.index_count);

    // Write attributes from buffer to vertex buffer interleaved.
    uint32 attribute_offset = 0;
    CTK_ITER(attribute, &primitive->attributes)
    {
        GLTFAccessor*   accessor    = GetPtr(&gltf.accessors,    attribute->accessor);
        GLTFBufferView* buffer_view = GetPtr(&gltf.buffer_views, accessor->buffer_view);
        GLTFBuffer*     buffer      = GetPtr(&gltf.buffers,      buffer_view->buffer);

        uint32 buffer_offset        = buffer_view->offset + accessor->offset;
        uint32 buffer_view_offset   = 0;
        uint32 vertex_buffer_offset = attribute_offset;
        uint32 component_count      = GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)accessor->type];
        uint32 component_size       = GLTF_COMPONENT_TYPE_SIZES[(uint32)accessor->component_type];
        uint32 attribute_size       = component_count * component_size;

        Swizzle* swizzle = attribute_swizzles != NULL
                         ? attribute_swizzles->array[(uint32)attribute->type]
                         : NULL;
        if (swizzle != NULL)
        {
            while (buffer_view_offset < buffer_view->size)
            {
                uint8* src = &buffer->data[buffer_offset + buffer_view_offset];
                uint8* dst = &mesh_data->vertex_buffer[vertex_buffer_offset];
                for (uint32 component_index = 0; component_index < component_count; ++component_index)
                {
                    memcpy(&dst[swizzle->array[component_index] * component_size],
                           &src[component_index * component_size],
                           component_size);
                }
                buffer_view_offset   += attribute_size;
                vertex_buffer_offset += mesh_data->info.vertex_size;
            }
        }
        else
        {
            while (buffer_view_offset < buffer_view->size)
            {
                memcpy(&mesh_data->vertex_buffer[vertex_buffer_offset],
                       &buffer->data[buffer_offset + buffer_view_offset],
                       attribute_size);
                buffer_view_offset   += attribute_size;
                vertex_buffer_offset += mesh_data->info.vertex_size;
            }
        }

        attribute_offset += attribute_size;
    }

    // Write indexes from buffer to index buffer. This is done with offsets because index size is 4. If indexes were
    // stored in the same size as they are in the GLTF buffer, we could just memcpy() the entire buffer_view for indexes
    // to the index buffer.
    GLTFBufferView* buffer_view = GetPtr(&gltf.buffer_views, indexes_accessor->buffer_view);
    GLTFBuffer*     buffer      = GetPtr(&gltf.buffers,      buffer_view->buffer);

    uint32 buffer_offset       = buffer_view->offset + indexes_accessor->offset;
    uint32 buffer_view_offset  = 0;
    uint32 index_buffer_offset = 0;
    uint32 index_size          = GLTF_COMPONENT_TYPE_SIZES[(uint32)indexes_accessor->component_type];
    while (buffer_view_offset < buffer_view->size)
    {
        memcpy(&mesh_data->index_buffer[index_buffer_offset],
               &buffer->data[buffer_offset + buffer_view_offset],
               index_size);
        buffer_view_offset  += index_size;
        index_buffer_offset += mesh_data->info.index_size;
    }
}

static void DestroyMeshData(MeshData* mesh_data, Allocator* allocator)
{
    Deallocate(allocator, mesh_data->vertex_buffer);
    Deallocate(allocator, mesh_data->index_buffer);
}

static MeshGroup* GetMeshGroup(MeshGroupHnd mesh_group_hnd)
{
    return GetMeshGroup(mesh_group_hnd.index);
}

static Mesh* GetMesh(MeshHnd mesh_hnd)
{
    return GetMesh(GetMeshGroup(mesh_hnd.group_index), mesh_hnd.index);
}

/// Debug
////////////////////////////////////////////////////////////
static void PrintAccessorValues(GLTF* gltf, GLTFAccessor* accessor, const char* name)
{
    GLTFBufferView* buffer_view = GetPtr(&gltf->buffer_views, accessor->buffer_view);
    GLTFBuffer*     buffer      = GetPtr(&gltf->buffers,      buffer_view->buffer);

    uint32 buffer_offset      = buffer_view->offset + accessor->offset;
    uint32 buffer_view_offset = 0;
    uint32 component_count    = GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)accessor->type];
    uint32 attribute_size     = component_count * GLTF_COMPONENT_TYPE_SIZES[(uint32)accessor->component_type];
    uint32 attribute_index    = 0;
    PrintLine("%s: ", name);
    while (buffer_view_offset < buffer_view->size)
    {
        Print("    [%4u] ", attribute_index);
        if (accessor->component_type == GLTFComponentType::FLOAT)
        {
            auto vector = (float32*)&buffer->data[buffer_offset + buffer_view_offset];
            for (uint32 i = 0; i < component_count; ++i)
            {
                Print("%8.3f,", vector[i]);
            }
            PrintLine();
        }
        else if (accessor->component_type == GLTFComponentType::SIGNED_BYTE)
        {
            auto vector = (sint8*)&buffer->data[buffer_offset + buffer_view_offset];
            for (uint32 i = 0; i < component_count; ++i)
            {
                Print("%8i,", vector[i]);
            }
            PrintLine();
        }
        else if (accessor->component_type == GLTFComponentType::SIGNED_SHORT)
        {
            auto vector = (sint16*)&buffer->data[buffer_offset + buffer_view_offset];
            for (uint32 i = 0; i < component_count; ++i)
            {
                Print("%8i,", vector[i]);
            }
            PrintLine();
        }
        else if (accessor->component_type == GLTFComponentType::SIGNED_INT)
        {
            auto vector = (sint32*)&buffer->data[buffer_offset + buffer_view_offset];
            for (uint32 i = 0; i < component_count; ++i)
            {
                Print("%8i,", vector[i]);
            }
            PrintLine();
        }
        else if (accessor->component_type == GLTFComponentType::UNSIGNED_BYTE)
        {
            auto vector = (uint8*)&buffer->data[buffer_offset + buffer_view_offset];
            for (uint32 i = 0; i < component_count; ++i)
            {
                Print("%8u,", vector[i]);
            }
            PrintLine();
        }
        else if (accessor->component_type == GLTFComponentType::UNSIGNED_SHORT)
        {
            auto vector = (uint16*)&buffer->data[buffer_offset + buffer_view_offset];
            for (uint32 i = 0; i < component_count; ++i)
            {
                Print("%8u,", vector[i]);
            }
            PrintLine();
        }
        else if (accessor->component_type == GLTFComponentType::UNSIGNED_INT)
        {
            auto vector = (uint32*)&buffer->data[buffer_offset + buffer_view_offset];
            for (uint32 i = 0; i < component_count; ++i)
            {
                Print("%8u,", vector[i]);
            }
            PrintLine();
        }
        else
        {
            CTK_FATAL("unhandle GLTFComponentType: %u", (uint32)accessor->component_type);
        }
        ++attribute_index;

        buffer_view_offset += attribute_size;
    }
}
