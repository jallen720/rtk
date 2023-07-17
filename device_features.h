/// Data
////////////////////////////////////////////////////////////
enum struct DeviceFeatures : uint32
{
    // Standard
    ROBUST_BUFFER_ACCESS,
    FULL_DRAW_INDEX_UINT32,
    IMAGE_CUBE_ARRAY,
    INDEPENDENT_BLEND,
    GEOMETRY_SHADER,
    TESSELLATION_SHADER,
    SAMPLE_RATE_SHADING,
    DUAL_SRC_BLEND,
    LOGIC_OP,
    MULTI_DRAW_INDIRECT,
    DRAW_INDIRECT_FIRST_INSTANCE,
    DEPTH_CLAMP,
    DEPTH_BIAS_CLAMP,
    FILL_MODE_NON_SOLID,
    DEPTH_BOUNDS,
    WIDE_LINES,
    LARGE_POINTS,
    ALPHA_TO_ONE,
    MULTI_VIEWPORT,
    SAMPLER_ANISOTROPY,
    TEXTURE_COMPRESSION_ETC2,
    TEXTURE_COMPRESSION_ASTC_LDR,
    TEXTURE_COMPRESSION_BC,
    OCCLUSION_QUERY_PRECISE,
    PIPELINE_STATISTICS_QUERY,
    VERTEX_PIPELINE_STORES_AND_ATOMICS,
    FRAGMENT_STORES_AND_ATOMICS,
    SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE,
    SHADER_IMAGE_GATHER_EXTENDED,
    SHADER_STORAGE_IMAGE_EXTENDED_FORMATS,
    SHADER_STORAGE_IMAGE_MULTISAMPLE,
    SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT,
    SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT,
    SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING,
    SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING,
    SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING,
    SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING,
    SHADER_CLIP_DISTANCE,
    SHADER_CULL_DISTANCE,
    SHADER_FLOAT64,
    SHADER_INT64,
    SHADER_INT16,
    SHADER_RESOURCE_RESIDENCY,
    SHADER_RESOURCE_MIN_LOD,
    SPARSE_BINDING,
    SPARSE_RESIDENCY_BUFFER,
    SPARSE_RESIDENCY_IMAGE2D,
    SPARSE_RESIDENCY_IMAGE3D,
    SPARSE_RESIDENCY_2_SAMPLES,
    SPARSE_RESIDENCY_4_SAMPLES,
    SPARSE_RESIDENCY_8_SAMPLES,
    SPARSE_RESIDENCY_16_SAMPLES,
    SPARSE_RESIDENCY_ALIASED,
    VARIABLE_MULTISAMPLE_RATE,
    INHERITED_QUERIES,

    // Scalar Block Layout
    SCALAR_BLOCK_LAYOUT,

    COUNT,
};

struct DeviceFeatureInfo
{
    VkPhysicalDeviceFeatures2                   standard;
    VkPhysicalDeviceScalarBlockLayoutFeatures   scalar_block_layout;
    FixedArray<VkBool32, DeviceFeatures::COUNT> array;
};

/// Interface
////////////////////////////////////////////////////////////
static void GetDeviceFeatureInfo(VkPhysicalDevice physical_device, DeviceFeatureInfo* info)
{
    info->standard =
    {
        .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext    = &info->scalar_block_layout,
        .features = {},
    };
    info->scalar_block_layout =
    {
        .sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
        .pNext             = NULL,
        .scalarBlockLayout = VK_FALSE,
    };
    vkGetPhysicalDeviceFeatures2(physical_device, &info->standard);

    // Query enabled standard features.
    auto standard_feature_array = (VkBool32*)&info->standard.features;
    for (uint32 i = 0; i < sizeof(info->standard.features) / sizeof(VkBool32); ++i)
    {
        Set(&info->array, i, standard_feature_array[i]);
    }

    // Query enabled extension features.
    Set(&info->array, DeviceFeatures::SCALAR_BLOCK_LAYOUT, info->scalar_block_layout.scalarBlockLayout);
}

static const char* GetDeviceFeatureName(DeviceFeatures feature)
{
    switch (feature)
    {
        case ROBUST_BUFFER_ACCESS:                         return "ROBUST_BUFFER_ACCESS",
        case FULL_DRAW_INDEX_UINT32:                       return "FULL_DRAW_INDEX_UINT32",
        case IMAGE_CUBE_ARRAY:                             return "IMAGE_CUBE_ARRAY",
        case INDEPENDENT_BLEND:                            return "INDEPENDENT_BLEND",
        case GEOMETRY_SHADER:                              return "GEOMETRY_SHADER",
        case TESSELLATION_SHADER:                          return "TESSELLATION_SHADER",
        case SAMPLE_RATE_SHADING:                          return "SAMPLE_RATE_SHADING",
        case DUAL_SRC_BLEND:                               return "DUAL_SRC_BLEND",
        case LOGIC_OP:                                     return "LOGIC_OP",
        case MULTI_DRAW_INDIRECT:                          return "MULTI_DRAW_INDIRECT",
        case DRAW_INDIRECT_FIRST_INSTANCE:                 return "DRAW_INDIRECT_FIRST_INSTANCE",
        case DEPTH_CLAMP:                                  return "DEPTH_CLAMP",
        case DEPTH_BIAS_CLAMP:                             return "DEPTH_BIAS_CLAMP",
        case FILL_MODE_NON_SOLID:                          return "FILL_MODE_NON_SOLID",
        case DEPTH_BOUNDS:                                 return "DEPTH_BOUNDS",
        case WIDE_LINES:                                   return "WIDE_LINES",
        case LARGE_POINTS:                                 return "LARGE_POINTS",
        case ALPHA_TO_ONE:                                 return "ALPHA_TO_ONE",
        case MULTI_VIEWPORT:                               return "MULTI_VIEWPORT",
        case SAMPLER_ANISOTROPY:                           return "SAMPLER_ANISOTROPY",
        case TEXTURE_COMPRESSION_ETC2:                     return "TEXTURE_COMPRESSION_ETC2",
        case TEXTURE_COMPRESSION_ASTC_LDR:                 return "TEXTURE_COMPRESSION_ASTC_LDR",
        case TEXTURE_COMPRESSION_BC:                       return "TEXTURE_COMPRESSION_BC",
        case OCCLUSION_QUERY_PRECISE:                      return "OCCLUSION_QUERY_PRECISE",
        case PIPELINE_STATISTICS_QUERY:                    return "PIPELINE_STATISTICS_QUERY",
        case VERTEX_PIPELINE_STORES_AND_ATOMICS:           return "VERTEX_PIPELINE_STORES_AND_ATOMICS",
        case FRAGMENT_STORES_AND_ATOMICS:                  return "FRAGMENT_STORES_AND_ATOMICS",
        case SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE:  return "SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE",
        case SHADER_IMAGE_GATHER_EXTENDED:                 return "SHADER_IMAGE_GATHER_EXTENDED",
        case SHADER_STORAGE_IMAGE_EXTENDED_FORMATS:        return "SHADER_STORAGE_IMAGE_EXTENDED_FORMATS",
        case SHADER_STORAGE_IMAGE_MULTISAMPLE:             return "SHADER_STORAGE_IMAGE_MULTISAMPLE",
        case SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT:     return "SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT",
        case SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT:    return "SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT",
        case SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING: return "SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING",
        case SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING:  return "SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING",
        case SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING: return "SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING",
        case SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING:  return "SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING",
        case SHADER_CLIP_DISTANCE:                         return "SHADER_CLIP_DISTANCE",
        case SHADER_CULL_DISTANCE:                         return "SHADER_CULL_DISTANCE",
        case SHADER_FLOAT64:                               return "SHADER_FLOAT64",
        case SHADER_INT64:                                 return "SHADER_INT64",
        case SHADER_INT16:                                 return "SHADER_INT16",
        case SHADER_RESOURCE_RESIDENCY:                    return "SHADER_RESOURCE_RESIDENCY",
        case SHADER_RESOURCE_MIN_LOD:                      return "SHADER_RESOURCE_MIN_LOD",
        case SPARSE_BINDING:                               return "SPARSE_BINDING",
        case SPARSE_RESIDENCY_BUFFER:                      return "SPARSE_RESIDENCY_BUFFER",
        case SPARSE_RESIDENCY_IMAGE2D:                     return "SPARSE_RESIDENCY_IMAGE2D",
        case SPARSE_RESIDENCY_IMAGE3D:                     return "SPARSE_RESIDENCY_IMAGE3D",
        case SPARSE_RESIDENCY_2_SAMPLES:                   return "SPARSE_RESIDENCY_2_SAMPLES",
        case SPARSE_RESIDENCY_4_SAMPLES:                   return "SPARSE_RESIDENCY_4_SAMPLES",
        case SPARSE_RESIDENCY_8_SAMPLES:                   return "SPARSE_RESIDENCY_8_SAMPLES",
        case SPARSE_RESIDENCY_16_SAMPLES:                  return "SPARSE_RESIDENCY_16_SAMPLES",
        case SPARSE_RESIDENCY_ALIASED:                     return "SPARSE_RESIDENCY_ALIASED",
        case VARIABLE_MULTISAMPLE_RATE:                    return "VARIABLE_MULTISAMPLE_RATE",
        case INHERITED_QUERIES:                            return "INHERITED_QUERIES",
        case SCALAR_BLOCK_LAYOUT:                          return "SCALAR_BLOCK_LAYOUT",
        default: CTK_FATAL("unhandled device feature: %u", feature);
    }
}
