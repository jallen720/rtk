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
    VkPhysicalDeviceFeatures2                 standard;
    VkPhysicalDeviceScalarBlockLayoutFeatures scalar_block_layout;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitDeviceFeatureInfo(DeviceFeatureInfo* info)
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
}

static VkBool32* GetStandardFeatureArray(DeviceFeatureInfo* info)
{
    return (VkBool32*)&info->standard.features;
}

static const char* GetDeviceFeatureName(DeviceFeatures feature)
{
    switch (feature)
    {
        case DeviceFeatures::ROBUST_BUFFER_ACCESS:                         return "DeviceFeatures::ROBUST_BUFFER_ACCESS";
        case DeviceFeatures::FULL_DRAW_INDEX_UINT32:                       return "DeviceFeatures::FULL_DRAW_INDEX_UINT32";
        case DeviceFeatures::IMAGE_CUBE_ARRAY:                             return "DeviceFeatures::IMAGE_CUBE_ARRAY";
        case DeviceFeatures::INDEPENDENT_BLEND:                            return "DeviceFeatures::INDEPENDENT_BLEND";
        case DeviceFeatures::GEOMETRY_SHADER:                              return "DeviceFeatures::GEOMETRY_SHADER";
        case DeviceFeatures::TESSELLATION_SHADER:                          return "DeviceFeatures::TESSELLATION_SHADER";
        case DeviceFeatures::SAMPLE_RATE_SHADING:                          return "DeviceFeatures::SAMPLE_RATE_SHADING";
        case DeviceFeatures::DUAL_SRC_BLEND:                               return "DeviceFeatures::DUAL_SRC_BLEND";
        case DeviceFeatures::LOGIC_OP:                                     return "DeviceFeatures::LOGIC_OP";
        case DeviceFeatures::MULTI_DRAW_INDIRECT:                          return "DeviceFeatures::MULTI_DRAW_INDIRECT";
        case DeviceFeatures::DRAW_INDIRECT_FIRST_INSTANCE:                 return "DeviceFeatures::DRAW_INDIRECT_FIRST_INSTANCE";
        case DeviceFeatures::DEPTH_CLAMP:                                  return "DeviceFeatures::DEPTH_CLAMP";
        case DeviceFeatures::DEPTH_BIAS_CLAMP:                             return "DeviceFeatures::DEPTH_BIAS_CLAMP";
        case DeviceFeatures::FILL_MODE_NON_SOLID:                          return "DeviceFeatures::FILL_MODE_NON_SOLID";
        case DeviceFeatures::DEPTH_BOUNDS:                                 return "DeviceFeatures::DEPTH_BOUNDS";
        case DeviceFeatures::WIDE_LINES:                                   return "DeviceFeatures::WIDE_LINES";
        case DeviceFeatures::LARGE_POINTS:                                 return "DeviceFeatures::LARGE_POINTS";
        case DeviceFeatures::ALPHA_TO_ONE:                                 return "DeviceFeatures::ALPHA_TO_ONE";
        case DeviceFeatures::MULTI_VIEWPORT:                               return "DeviceFeatures::MULTI_VIEWPORT";
        case DeviceFeatures::SAMPLER_ANISOTROPY:                           return "DeviceFeatures::SAMPLER_ANISOTROPY";
        case DeviceFeatures::TEXTURE_COMPRESSION_ETC2:                     return "DeviceFeatures::TEXTURE_COMPRESSION_ETC2";
        case DeviceFeatures::TEXTURE_COMPRESSION_ASTC_LDR:                 return "DeviceFeatures::TEXTURE_COMPRESSION_ASTC_LDR";
        case DeviceFeatures::TEXTURE_COMPRESSION_BC:                       return "DeviceFeatures::TEXTURE_COMPRESSION_BC";
        case DeviceFeatures::OCCLUSION_QUERY_PRECISE:                      return "DeviceFeatures::OCCLUSION_QUERY_PRECISE";
        case DeviceFeatures::PIPELINE_STATISTICS_QUERY:                    return "DeviceFeatures::PIPELINE_STATISTICS_QUERY";
        case DeviceFeatures::VERTEX_PIPELINE_STORES_AND_ATOMICS:           return "DeviceFeatures::VERTEX_PIPELINE_STORES_AND_ATOMICS";
        case DeviceFeatures::FRAGMENT_STORES_AND_ATOMICS:                  return "DeviceFeatures::FRAGMENT_STORES_AND_ATOMICS";
        case DeviceFeatures::SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE:  return "DeviceFeatures::SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE";
        case DeviceFeatures::SHADER_IMAGE_GATHER_EXTENDED:                 return "DeviceFeatures::SHADER_IMAGE_GATHER_EXTENDED";
        case DeviceFeatures::SHADER_STORAGE_IMAGE_EXTENDED_FORMATS:        return "DeviceFeatures::SHADER_STORAGE_IMAGE_EXTENDED_FORMATS";
        case DeviceFeatures::SHADER_STORAGE_IMAGE_MULTISAMPLE:             return "DeviceFeatures::SHADER_STORAGE_IMAGE_MULTISAMPLE";
        case DeviceFeatures::SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT:     return "DeviceFeatures::SHADER_STORAGE_IMAGE_READ_WITHOUT_FORMAT";
        case DeviceFeatures::SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT:    return "DeviceFeatures::SHADER_STORAGE_IMAGE_WRITE_WITHOUT_FORMAT";
        case DeviceFeatures::SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING: return "DeviceFeatures::SHADER_UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING";
        case DeviceFeatures::SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING:  return "DeviceFeatures::SHADER_SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING";
        case DeviceFeatures::SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING: return "DeviceFeatures::SHADER_STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING";
        case DeviceFeatures::SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING:  return "DeviceFeatures::SHADER_STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING";
        case DeviceFeatures::SHADER_CLIP_DISTANCE:                         return "DeviceFeatures::SHADER_CLIP_DISTANCE";
        case DeviceFeatures::SHADER_CULL_DISTANCE:                         return "DeviceFeatures::SHADER_CULL_DISTANCE";
        case DeviceFeatures::SHADER_FLOAT64:                               return "DeviceFeatures::SHADER_FLOAT64";
        case DeviceFeatures::SHADER_INT64:                                 return "DeviceFeatures::SHADER_INT64";
        case DeviceFeatures::SHADER_INT16:                                 return "DeviceFeatures::SHADER_INT16";
        case DeviceFeatures::SHADER_RESOURCE_RESIDENCY:                    return "DeviceFeatures::SHADER_RESOURCE_RESIDENCY";
        case DeviceFeatures::SHADER_RESOURCE_MIN_LOD:                      return "DeviceFeatures::SHADER_RESOURCE_MIN_LOD";
        case DeviceFeatures::SPARSE_BINDING:                               return "DeviceFeatures::SPARSE_BINDING";
        case DeviceFeatures::SPARSE_RESIDENCY_BUFFER:                      return "DeviceFeatures::SPARSE_RESIDENCY_BUFFER";
        case DeviceFeatures::SPARSE_RESIDENCY_IMAGE2D:                     return "DeviceFeatures::SPARSE_RESIDENCY_IMAGE2D";
        case DeviceFeatures::SPARSE_RESIDENCY_IMAGE3D:                     return "DeviceFeatures::SPARSE_RESIDENCY_IMAGE3D";
        case DeviceFeatures::SPARSE_RESIDENCY_2_SAMPLES:                   return "DeviceFeatures::SPARSE_RESIDENCY_2_SAMPLES";
        case DeviceFeatures::SPARSE_RESIDENCY_4_SAMPLES:                   return "DeviceFeatures::SPARSE_RESIDENCY_4_SAMPLES";
        case DeviceFeatures::SPARSE_RESIDENCY_8_SAMPLES:                   return "DeviceFeatures::SPARSE_RESIDENCY_8_SAMPLES";
        case DeviceFeatures::SPARSE_RESIDENCY_16_SAMPLES:                  return "DeviceFeatures::SPARSE_RESIDENCY_16_SAMPLES";
        case DeviceFeatures::SPARSE_RESIDENCY_ALIASED:                     return "DeviceFeatures::SPARSE_RESIDENCY_ALIASED";
        case DeviceFeatures::VARIABLE_MULTISAMPLE_RATE:                    return "DeviceFeatures::VARIABLE_MULTISAMPLE_RATE";
        case DeviceFeatures::INHERITED_QUERIES:                            return "DeviceFeatures::INHERITED_QUERIES";
        case DeviceFeatures::SCALAR_BLOCK_LAYOUT:                          return "DeviceFeatures::SCALAR_BLOCK_LAYOUT";
        default: CTK_FATAL("unhandled device feature: %u", feature);
    }
}
