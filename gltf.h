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
    uint32            count;
    uint32            buffer_view;
};

struct GLTFBufferView
{
    uint32     buffer;
    uint32     byte_length;
    uint32     byte_offset;
    GLTFTarget target;
};

struct GLTFBuffer
{
    uint32 byte_length;
    String uri;
};

struct GLTFAttribute
{
    GLTFAttributeType type;
    uint32            accessor;
};

struct GLTFPrimitive
{
    Array<GLTFAttribute> attributes;
    uint32               indices_accessor;
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
    for (uint32 i = 0; i < (uint32)GLTFAccessorType::COUNT; ++i)
    {
        if (StringsMatch(GLTF_ACCESSOR_TYPE_NAMES[i], accessor_type))
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
    for (uint32 i = 0; i < (uint32)GLTFAttributeType::COUNT; ++i)
    {
        if (StringsMatch(GLTF_ATTRIBUTE_TYPE_NAMES[i], attribute_type))
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
    PrintTabs(tabs); PrintLine("count:          %u", accessor->count);
    PrintTabs(tabs); PrintLine("buffer_view:    %u", accessor->buffer_view);
}

void PrintGLTFBufferView(GLTFBufferView* buffer_view, uint32 tabs = 0)
{
    PrintTabs(tabs); PrintLine("buffer:      %u", buffer_view->buffer);
    PrintTabs(tabs); PrintLine("byte_length: %u", buffer_view->byte_length);
    PrintTabs(tabs); PrintLine("byte_offset: %u", buffer_view->byte_offset);
    PrintTabs(tabs); PrintLine("target:      %s", GetGLTFTargetName(buffer_view->target));
}

void PrintGLTFBuffer(GLTFBuffer* buffer, uint32 tabs = 0)
{
    PrintTabs(tabs); PrintLine("byte_length: %u", buffer->byte_length);
    PrintTabs(tabs); PrintLine("uri:         \"%.*s\"", buffer->uri.size, buffer->uri.data);
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
        PrintTabs(tabs + 1); PrintLine("indices_accessor: %u", primitive->indices_accessor);
    }
}

void PrintGLTF(GLTF* gltf, uint32 tabs = 0)
{
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
    PrintTabs(tabs + 0);
    PrintLine("meshes:");
    CTK_ITER(mesh, &gltf->meshes)
    {
        PrintGLTFMesh(mesh, tabs + 1);
        PrintLine();
    }
}

/// Interface
////////////////////////////////////////////////////////////
void LoadGLTF(GLTF* gltf, const Allocator* allocator, const char* path)
{
    JSON json = {};
    LoadJSON(&json, allocator, path);

    JSONNode* json_accessors    = GetArray(&json, "accessors");
    JSONNode* json_buffer_views = GetArray(&json, "bufferViews");
    JSONNode* json_buffers      = GetArray(&json, "buffers");
    JSONNode* json_meshes       = GetArray(&json, "meshes");
    InitArray(&gltf->accessors,    allocator, json_accessors->list.size);
    InitArray(&gltf->buffer_views, allocator, json_buffer_views->list.size);
    InitArray(&gltf->buffers,      allocator, json_buffers->list.size);
    InitArray(&gltf->meshes,       allocator, json_meshes->list.size);
    for (uint32 i = 0; i < json_accessors->list.size; ++i)
    {
        GLTFAccessor* accessor = Push(&gltf->accessors);
        JSONNode* json_accessor = GetObject(&json, json_accessors, i);
        accessor->type           = GetGLTFAccessorType(GetString(&json, json_accessor, "type"));
        accessor->component_type = GetGLTFComponentType(GetUInt32(&json, json_accessor, "componentType"));
        accessor->count          = GetUInt32(&json, json_accessor, "count");
        accessor->buffer_view    = GetUInt32(&json, json_accessor, "bufferView");
    }
    for (uint32 i = 0; i < json_buffer_views->list.size; ++i)
    {
        GLTFBufferView* buffer_view = Push(&gltf->buffer_views);
        JSONNode* json_buffer_view = GetObject(&json, json_buffer_views, i);
        buffer_view->buffer      = GetUInt32(&json, json_buffer_view, "buffer");
        buffer_view->byte_length = GetUInt32(&json, json_buffer_view, "byteLength");
        buffer_view->byte_offset = GetUInt32(&json, json_buffer_view, "byteOffset");
        buffer_view->target      = GetGLTFTarget(GetUInt32(&json, json_buffer_view, "target"));
    }
    for (uint32 i = 0; i < json_buffers->list.size; ++i)
    {
        GLTFBuffer* buffer = Push(&gltf->buffers);
        JSONNode* json_buffer = GetObject(&json, json_buffers, i);
        buffer->byte_length = GetUInt32(&json, json_buffer, "byteLength");
        InitString(&buffer->uri, allocator, GetString(&json, json_buffer, "uri"));
    }
    for (uint32 i = 0; i < json_meshes->list.size; ++i)
    {
        GLTFMesh* mesh = Push(&gltf->meshes);
        JSONNode* json_mesh = GetObject(&json, json_meshes, i);
        InitString(&mesh->name, allocator, GetString(&json, json_mesh, "name"));

        // Primitives
        JSONNode* json_primitives = GetArray(&json, json_mesh, "primitives");
        InitArray(&mesh->primitives, allocator, json_primitives->list.size);
        for (uint32 primitive_index = 0; primitive_index < json_primitives->list.size; ++primitive_index)
        {
            GLTFPrimitive* primitive = Push(&mesh->primitives);
            JSONNode* json_primitive = GetObject(&json, json_primitives, primitive_index);
            primitive->indices_accessor = GetUInt32(&json, json_primitive, "indices");

            // Attributes
            JSONNode* json_attributes = GetObject(&json, json_primitive, "attributes");
            InitArray(&primitive->attributes, allocator, json_attributes->list.size);
            for (uint32 attribute_index = 0; attribute_index < json_attributes->list.size; ++attribute_index)
            {
                GLTFAttribute* attribute = Push(&primitive->attributes);
                JSONNode* json_attribute = GetNode(&json, json_attributes, attribute_index);

                attribute->type     = GetGLTFAttributeType(&json_attribute->key);
                attribute->accessor = json_attribute->num_uint32;
            }
        }
    }

    DeinitJSON(&json, allocator);
}
