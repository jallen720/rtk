/// Data
////////////////////////////////////////////////////////////
static constexpr const char* VULKAN_1_0_DEVICE_FEATURE_NAMES[] =
{
    "robustBufferAccess",
    "fullDrawIndexUint32",
    "imageCubeArray",
    "independentBlend",
    "geometryShader",
    "tessellationShader",
    "sampleRateShading",
    "dualSrcBlend",
    "logicOp",
    "multiDrawIndirect",
    "drawIndirectFirstInstance",
    "depthClamp",
    "depthBiasClamp",
    "fillModeNonSolid",
    "depthBounds",
    "wideLines",
    "largePoints",
    "alphaToOne",
    "multiViewport",
    "samplerAnisotropy",
    "textureCompressionETC2",
    "textureCompressionASTC_LDR",
    "textureCompressionBC",
    "occlusionQueryPrecise",
    "pipelineStatisticsQuery",
    "vertexPipelineStoresAndAtomics",
    "fragmentStoresAndAtomics",
    "shaderTessellationAndGeometryPointSize",
    "shaderImageGatherExtended",
    "shaderStorageImageExtendedFormats",
    "shaderStorageImageMultisample",
    "shaderStorageImageReadWithoutFormat",
    "shaderStorageImageWriteWithoutFormat",
    "shaderUniformBufferArrayDynamicIndexing",
    "shaderSampledImageArrayDynamicIndexing",
    "shaderStorageBufferArrayDynamicIndexing",
    "shaderStorageImageArrayDynamicIndexing",
    "shaderClipDistance",
    "shaderCullDistance",
    "shaderFloat64",
    "shaderInt64",
    "shaderInt16",
    "shaderResourceResidency",
    "shaderResourceMinLod",
    "sparseBinding",
    "sparseResidencyBuffer",
    "sparseResidencyImage2D",
    "sparseResidencyImage3D",
    "sparseResidency2Samples",
    "sparseResidency4Samples",
    "sparseResidency8Samples",
    "sparseResidency16Samples",
    "sparseResidencyAliased",
    "variableMultisampleRate",
    "inheritedQueries",
};
static constexpr uint32 VULKAN_1_0_DEVICE_FEATURE_COUNT = CTK_ARRAY_SIZE(VULKAN_1_0_DEVICE_FEATURE_NAMES);

static constexpr const char* VULKAN_1_1_DEVICE_FEATURE_NAMES[] =
{
    "storageBuffer16BitAccess",
    "uniformAndStorageBuffer16BitAccess",
    "storagePushConstant16",
    "storageInputOutput16",
    "multiview",
    "multiviewGeometryShader",
    "multiviewTessellationShader",
    "variablePointersStorageBuffer",
    "variablePointers",
    "protectedMemory",
    "samplerYcbcrConversion",
    "shaderDrawParameters",
};
static constexpr uint32 VULKAN_1_1_DEVICE_FEATURE_COUNT = CTK_ARRAY_SIZE(VULKAN_1_1_DEVICE_FEATURE_NAMES);

static constexpr const char* VULKAN_1_2_DEVICE_FEATURE_NAMES[] =
{
    "samplerMirrorClampToEdge",
    "drawIndirectCount",
    "storageBuffer8BitAccess",
    "uniformAndStorageBuffer8BitAccess",
    "storagePushConstant8",
    "shaderBufferInt64Atomics",
    "shaderSharedInt64Atomics",
    "shaderFloat16",
    "shaderInt8",
    "descriptorIndexing",
    "shaderInputAttachmentArrayDynamicIndexing",
    "shaderUniformTexelBufferArrayDynamicIndexing",
    "shaderStorageTexelBufferArrayDynamicIndexing",
    "shaderUniformBufferArrayNonUniformIndexing",
    "shaderSampledImageArrayNonUniformIndexing",
    "shaderStorageBufferArrayNonUniformIndexing",
    "shaderStorageImageArrayNonUniformIndexing",
    "shaderInputAttachmentArrayNonUniformIndexing",
    "shaderUniformTexelBufferArrayNonUniformIndexing",
    "shaderStorageTexelBufferArrayNonUniformIndexing",
    "descriptorBindingUniformBufferUpdateAfterBind",
    "descriptorBindingSampledImageUpdateAfterBind",
    "descriptorBindingStorageImageUpdateAfterBind",
    "descriptorBindingStorageBufferUpdateAfterBind",
    "descriptorBindingUniformTexelBufferUpdateAfterBind",
    "descriptorBindingStorageTexelBufferUpdateAfterBind",
    "descriptorBindingUpdateUnusedWhilePending",
    "descriptorBindingPartiallyBound",
    "descriptorBindingVariableDescriptorCount",
    "runtimeDescriptorArray",
    "samplerFilterMinmax",
    "scalarBlockLayout",
    "imagelessFramebuffer",
    "uniformBufferStandardLayout",
    "shaderSubgroupExtendedTypes",
    "separateDepthStencilLayouts",
    "hostQueryReset",
    "timelineSemaphore",
    "bufferDeviceAddress",
    "bufferDeviceAddressCaptureReplay",
    "bufferDeviceAddressMultiDevice",
    "vulkanMemoryModel",
    "vulkanMemoryModelDeviceScope",
    "vulkanMemoryModelAvailabilityVisibilityChains",
    "shaderOutputViewportIndex",
    "shaderOutputLayer",
    "subgroupBroadcastDynamicId",
};
static constexpr uint32 VULKAN_1_2_DEVICE_FEATURE_COUNT = CTK_ARRAY_SIZE(VULKAN_1_2_DEVICE_FEATURE_NAMES);

static constexpr const char* VULKAN_1_3_DEVICE_FEATURE_NAMES[] =
{
    "robustImageAccess",
    "inlineUniformBlock",
    "descriptorBindingInlineUniformBlockUpdateAfterBind",
    "pipelineCreationCacheControl",
    "privateData",
    "shaderDemoteToHelperInvocation",
    "shaderTerminateInvocation",
    "subgroupSizeControl",
    "computeFullSubgroups",
    "synchronization2",
    "textureCompressionASTC_HDR",
    "shaderZeroInitializeWorkgroupMemory",
    "dynamicRendering",
    "shaderIntegerDotProduct",
    "maintenance4",
};
static constexpr uint32 VULKAN_1_3_DEVICE_FEATURE_COUNT = CTK_ARRAY_SIZE(VULKAN_1_3_DEVICE_FEATURE_NAMES);

struct DeviceFeatures
{
    union
    {
        VkPhysicalDeviceFeatures2 list;
        struct
        {

            VkStructureType          sType;
            void*                    pNext;
            VkPhysicalDeviceFeatures vulkan_1_0;
        };
    };

    VkPhysicalDeviceVulkan11Features vulkan_1_1;
    VkPhysicalDeviceVulkan12Features vulkan_1_2;
    VkPhysicalDeviceVulkan13Features vulkan_1_3;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitDeviceFeatures(DeviceFeatures* device_features)
{
    device_features->list =
    {
        .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext    = &device_features->vulkan_1_1,
        .features = {},
    };
    device_features->vulkan_1_1 =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &device_features->vulkan_1_2,
    };
    device_features->vulkan_1_2 =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &device_features->vulkan_1_3,
    };
    device_features->vulkan_1_3 =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = NULL,
    };
}

static const char* GetDeviceFeatureName_Vulkan_1_0(uint32 feature_index)
{
    CTK_ASSERT(feature_index < VULKAN_1_0_DEVICE_FEATURE_COUNT);
    return VULKAN_1_0_DEVICE_FEATURE_NAMES[feature_index];
}

static const char* GetDeviceFeatureName_Vulkan_1_1(uint32 feature_index)
{
    CTK_ASSERT(feature_index < VULKAN_1_1_DEVICE_FEATURE_COUNT);
    return VULKAN_1_1_DEVICE_FEATURE_NAMES[feature_index];
}

static const char* GetDeviceFeatureName_Vulkan_1_2(uint32 feature_index)
{
    CTK_ASSERT(feature_index < VULKAN_1_2_DEVICE_FEATURE_COUNT);
    return VULKAN_1_2_DEVICE_FEATURE_NAMES[feature_index];
}

static const char* GetDeviceFeatureName_Vulkan_1_3(uint32 feature_index)
{
    CTK_ASSERT(feature_index < VULKAN_1_3_DEVICE_FEATURE_COUNT);
    return VULKAN_1_3_DEVICE_FEATURE_NAMES[feature_index];
}
