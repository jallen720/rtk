/// Data
////////////////////////////////////////////////////////////
enum struct GLTFAccessorType
{
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4,
    COUNT,
};

static constexpr uint32 GLTF_COMPONENT_TYPE_START = 5120;
enum struct GLTFComponentType
{
    SIGNED_BYTE,
    UNSIGNED_BYTE,
    SIGNED_SHORT,
    UNSIGNED_SHORT,
    SIGNED_INT, // Signed 32-bit integer components are not supported; only here to keep enum value uniform.
    UNSIGNED_INT,
    FLOAT,
    COUNT,
};

static constexpr uint32 GLTF_TARGET_START = 34962;
enum struct GLTFTarget
{
    ARRAY_BUFFER,
    ELEMENT_ARRAY_BUFFER,
    COUNT,
};

enum struct GLTFAttributeType
{
    POSITION,
    NORMAL,
    TANGENT,
    TEXCOORD_0,
    TEXCOORD_1,
    TEXCOORD_2,
    TEXCOORD_3,
    COLOR_0,
    COLOR_1,
    COLOR_2,
    COLOR_3,
    JOINTS_0,
    JOINTS_1,
    JOINTS_2,
    JOINTS_3,
    WEIGHTS_0,
    WEIGHTS_1,
    WEIGHTS_2,
    WEIGHTS_3,
    COUNT,
};

struct GLTFAccessor
{
    GLTFAccessorType  type;
    GLTFComponentType component_type;
    uint32            buffer_view;
    uint32            count;
    uint32            offset;
};

struct GLTFBufferView
{
    GLTFTarget target;
    uint32     buffer;
    uint32     size;
    uint32     offset;
};

struct GLTFBuffer
{
    String uri;
    uint8* data;
    uint32 size;
};

struct GLTFAttribute
{
    GLTFAttributeType type;
    uint32            accessor;
};

struct GLTFPrimitive
{
    Array<GLTFAttribute> attributes;
    uint32               indexes_accessor;
};

struct GLTFMesh
{
    Array<GLTFPrimitive> primitives;
    String               name;
};

struct GLTF
{
    Array<GLTFAccessor>   accessors;
    Array<GLTFBufferView> buffer_views;
    Array<GLTFBuffer>     buffers;
    Array<GLTFMesh>       meshes;
};

static constexpr const char* GLTF_ACCESSOR_TYPE_NAMES[(uint32)GLTFAccessorType::COUNT] =
{
    "SCALAR",
    "VEC2",
    "VEC3",
    "VEC4",
    "MAT2",
    "MAT3",
    "MAT4",
};

static constexpr uint32 GLTF_ACCESSOR_TYPE_COMPONENT_COUNTS[(uint32)GLTFAccessorType::COUNT] =
{
    1,  // SCALAR
    2,  // VEC2
    3,  // VEC3
    4,  // VEC4
    4,  // MAT2
    9,  // MAT3
    16, // MAT4
};

static constexpr const char* GLTF_COMPONENT_TYPE_NAMES[(uint32)GLTFComponentType::COUNT] =
{
    "SIGNED_BYTE",
    "UNSIGNED_BYTE",
    "SIGNED_SHORT",
    "UNSIGNED_SHORT",
    "SIGNED_INT",
    "UNSIGNED_INT",
    "FLOAT",
};

static constexpr uint32 GLTF_COMPONENT_TYPE_SIZES[(uint32)GLTFComponentType::COUNT] =
{
    1, // SIGNED_BYTE
    1, // UNSIGNED_BYTE
    2, // SIGNED_SHORT
    2, // UNSIGNED_SHORT
    4, // SIGNED_INT
    4, // UNSIGNED_INT
    4, // FLOAT
};

static constexpr const char* GLTF_TARGET_NAMES[(uint32)GLTFTarget::COUNT] =
{
    "ARRAY_BUFFER",
    "ELEMENT_ARRAY_BUFFER",
};

static constexpr const char* GLTF_ATTRIBUTE_TYPE_NAMES[(uint32)GLTFAttributeType::COUNT] =
{
    "POSITION",
    "NORMAL",
    "TANGENT",
    "TEXCOORD_0",
    "TEXCOORD_1",
    "TEXCOORD_2",
    "TEXCOORD_3",
    "COLOR_0",
    "COLOR_1",
    "COLOR_2",
    "COLOR_3",
    "JOINTS_0",
    "JOINTS_1",
    "JOINTS_2",
    "JOINTS_3",
    "WEIGHTS_0",
    "WEIGHTS_1",
    "WEIGHTS_2",
    "WEIGHTS_3",
};

/// Utils
////////////////////////////////////////////////////////////
GLTFAccessorType GetGLTFAccessorType(String* accessor_type)
{
    for (uint32 i = 0; i < (uint32)GLTFAccessorType::COUNT; i += 1)
    {
        if (StringsMatch(accessor_type, GLTF_ACCESSOR_TYPE_NAMES[i]))
        {
            return (GLTFAccessorType)i;
        }
    }
    CTK_FATAL("unknown accessor type: %.*s", accessor_type->size, accessor_type->data);
}

const char* GetGLTFAccessorTypeName(GLTFAccessorType accessor_type)
{
    CTK_ASSERT((uint32)accessor_type < (uint32)GLTFAccessorType::COUNT);
    return GLTF_ACCESSOR_TYPE_NAMES[(uint32)accessor_type];
}

GLTFComponentType GetGLTFComponentType(uint32 num)
{
    return (GLTFComponentType)(num - GLTF_COMPONENT_TYPE_START);
}

const char* GetGLTFComponentTypeName(GLTFComponentType component_type)
{
    CTK_ASSERT((uint32)component_type < (uint32)GLTFComponentType::COUNT);
    return GLTF_COMPONENT_TYPE_NAMES[(uint32)component_type];
}

GLTFTarget GetGLTFTarget(uint32 num)
{
    return (GLTFTarget)(num - GLTF_TARGET_START);
}

const char* GetGLTFTargetName(GLTFTarget target)
{
    CTK_ASSERT((uint32)target < (uint32)GLTFTarget::COUNT);
    return GLTF_TARGET_NAMES[(uint32)target];
}

GLTFAttributeType GetGLTFAttributeType(String* attribute_type)
{
    for (uint32 i = 0; i < (uint32)GLTFAttributeType::COUNT; i += 1)
    {
        if (StringsMatch(attribute_type, GLTF_ATTRIBUTE_TYPE_NAMES[i]))
        {
            return (GLTFAttributeType)i;
        }
    }
    CTK_FATAL("unknown attribute type: %.*s", attribute_type->size, attribute_type->data);
}

const char* GetGLTFAttributeTypeName(GLTFAttributeType attribute_type)
{
    CTK_ASSERT((uint32)attribute_type < (uint32)GLTFAttributeType::COUNT);
    return GLTF_ATTRIBUTE_TYPE_NAMES[(uint32)attribute_type];
}

/// Debug
////////////////////////////////////////////////////////////
void PrintGLTFAccessor(GLTFAccessor* accessor, uint32 tabs = 0)
{
    PrintTabs(tabs); PrintLine("type:           %s", GetGLTFAccessorTypeName(accessor->type));
    PrintTabs(tabs); PrintLine("component_type: %s", GetGLTFComponentTypeName(accessor->component_type));
    PrintTabs(tabs); PrintLine("buffer_view:    %u", accessor->buffer_view);
    PrintTabs(tabs); PrintLine("count:          %u", accessor->count);
    PrintTabs(tabs); PrintLine("offset:         %u", accessor->offset);
}

void PrintGLTFBufferView(GLTFBufferView* buffer_view, uint32 tabs = 0)
{
    PrintTabs(tabs); PrintLine("target: %s", GetGLTFTargetName(buffer_view->target));
    PrintTabs(tabs); PrintLine("buffer: %u", buffer_view->buffer);
    PrintTabs(tabs); PrintLine("size:   %u", buffer_view->size);
    PrintTabs(tabs); PrintLine("offset: %u", buffer_view->offset);
}

void PrintGLTFBuffer(GLTFBuffer* buffer, uint32 tabs = 0)
{
    PrintTabs(tabs); PrintLine("size: %u", buffer->size);
    PrintTabs(tabs); PrintLine("uri:  \"%.*s\"", buffer->uri.size, buffer->uri.data);
}

void PrintGLTFMesh(GLTFMesh* mesh, uint32 tabs = 0)
{
    PrintTabs(tabs + 0); PrintLine("name: %.*s", mesh->name.size, mesh->name.data);
    PrintTabs(tabs + 0); PrintLine("primitives:");
    CTK_ITER(primitive, &mesh->primitives)
    {
        PrintTabs(tabs + 1); PrintLine("attributes:");
        CTK_ITER(attribute, &primitive->attributes)
        {
            PrintTabs(tabs + 2); PrintLine("type: %s", GetGLTFAttributeTypeName(attribute->type));
            PrintTabs(tabs + 2); PrintLine("accessor: %u", attribute->accessor);
            PrintLine();
        }
        PrintTabs(tabs + 1); PrintLine("indexes_accessor: %u", primitive->indexes_accessor);
    }
}

void PrintGLTF(GLTF* gltf, uint32 tabs = 0)
{
    PrintTabs(tabs + 0);
    PrintLine("meshes:");
    CTK_ITER(mesh, &gltf->meshes)
    {
        PrintGLTFMesh(mesh, tabs + 1);
        PrintLine();
    }
    PrintTabs(tabs + 0);
    PrintLine("accessors:");
    CTK_ITER(accessor, &gltf->accessors)
    {
        PrintGLTFAccessor(accessor, tabs + 1);
        PrintLine();
    }
    PrintTabs(tabs + 0);
    PrintLine("buffer_views:");
    CTK_ITER(buffer_view, &gltf->buffer_views)
    {
        PrintGLTFBufferView(buffer_view, tabs + 1);
        PrintLine();
    }
    PrintTabs(tabs + 0);
    PrintLine("buffers:");
    CTK_ITER(buffer, &gltf->buffers)
    {
        PrintGLTFBuffer(buffer, tabs + 1);
        PrintLine();
    }
}

/// Interface
////////////////////////////////////////////////////////////
void LoadGLTF(GLTF* gltf, Allocator* allocator, const char* path)
{
    JSON json = LoadJSON(allocator, path);

    JSONNode* json_accessors    = GetArray(&json, "accessors");
    JSONNode* json_buffer_views = GetArray(&json, "bufferViews");
    JSONNode* json_buffers      = GetArray(&json, "buffers");
    JSONNode* json_meshes       = GetArray(&json, "meshes");
    gltf->accessors    = CreateArray<GLTFAccessor>  (allocator, json_accessors   ->list.size);
    gltf->buffer_views = CreateArray<GLTFBufferView>(allocator, json_buffer_views->list.size);
    gltf->buffers      = CreateArray<GLTFBuffer>    (allocator, json_buffers     ->list.size);
    gltf->meshes       = CreateArray<GLTFMesh>      (allocator, json_meshes      ->list.size);
    for (uint32 i = 0; i < json_accessors->list.size; i += 1)
    {
        JSONNode* json_accessor = GetObject(&json, json_accessors, i);
        JSONNode* json_offset   = FindNode(&json, json_accessor, "offset");
        GLTFAccessor* accessor   = Push(&gltf->accessors);
        accessor->buffer_view    = GetUInt32(&json, json_accessor, "bufferView");
        accessor->count          = GetUInt32(&json, json_accessor, "count");
        accessor->offset         = json_offset == NULL ? 0 : json_offset->num_uint32;
        accessor->type           = GetGLTFAccessorType(GetString(&json, json_accessor, "type"));
        accessor->component_type = GetGLTFComponentType(GetUInt32(&json, json_accessor, "componentType"));
    }
    for (uint32 i = 0; i < json_buffer_views->list.size; i += 1)
    {
        JSONNode* json_buffer_view = GetObject(&json, json_buffer_views, i);
        GLTFBufferView* buffer_view = Push(&gltf->buffer_views);
        buffer_view->buffer = GetUInt32(&json, json_buffer_view, "buffer");
        buffer_view->size   = GetUInt32(&json, json_buffer_view, "byteLength");
        buffer_view->offset = GetUInt32(&json, json_buffer_view, "byteOffset");
        buffer_view->target = GetGLTFTarget(GetUInt32(&json, json_buffer_view, "target"));
    }
    for (uint32 i = 0; i < json_buffers->list.size; i += 1)
    {
        JSONNode* json_buffer = GetObject(&json, json_buffers, i);
        String* json_uri = GetString(&json, json_buffer, "uri");
        uint32 path_dir_size = GetPathDirSize(path);

        // Append buffer uri in GLTF file to GLTF file directory.
        GLTFBuffer* buffer = Push(&gltf->buffers);
        buffer->uri = CreateString(allocator, path_dir_size + json_uri->size + 1);
        Write(&buffer->uri, "%.*s%.*s", path_dir_size, path, json_uri->size, json_uri->data);
        ReadFile(&buffer->data, &buffer->size, allocator, buffer->uri.data);
        CTK_ASSERT(buffer->size == GetUInt32(&json, json_buffer, "byteLength"));
    }
    for (uint32 i = 0; i < json_meshes->list.size; i += 1)
    {
        JSONNode* json_mesh = GetObject(&json, json_meshes, i);
        GLTFMesh* mesh = Push(&gltf->meshes);
        mesh->name = CreateString(allocator, GetString(&json, json_mesh, "name"));

        // Primitives
        JSONNode* json_primitives = GetArray(&json, json_mesh, "primitives");
        mesh->primitives = CreateArray<GLTFPrimitive>(allocator, json_primitives->list.size);
        for (uint32 primitive_index = 0; primitive_index < json_primitives->list.size; primitive_index += 1)
        {
            JSONNode* json_primitive = GetObject(&json, json_primitives, primitive_index);
            GLTFPrimitive* primitive = Push(&mesh->primitives);
            primitive->indexes_accessor = GetUInt32(&json, json_primitive, "indices");

            // Attributes
            JSONNode* json_attributes = GetObject(&json, json_primitive, "attributes");
            primitive->attributes = CreateArray<GLTFAttribute>(allocator, json_attributes->list.size);
            for (uint32 attribute_index = 0; attribute_index < json_attributes->list.size; attribute_index += 1)
            {
                GLTFAttribute* attribute = Push(&primitive->attributes);
                JSONNode* json_attribute = GetNode(&json, json_attributes, attribute_index);

                attribute->type     = GetGLTFAttributeType(&json_attribute->key);
                attribute->accessor = json_attribute->num_uint32;
            }
        }
    }

    DestroyJSON(&json);
}
