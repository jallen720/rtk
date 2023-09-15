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
static constexpr uint32 VULKAN_1_0_LONGEST_DEVICE_FEATURE_NAME_SIZE = 39u;

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
static constexpr uint32 VULKAN_1_1_LONGEST_DEVICE_FEATURE_NAME_SIZE = 34u;

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
static constexpr uint32 VULKAN_1_2_LONGEST_DEVICE_FEATURE_NAME_SIZE = 50u;

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
static constexpr uint32 VULKAN_1_3_LONGEST_DEVICE_FEATURE_NAME_SIZE = 50u;

struct VkPhysicalDeviceVulkan10Features
{
    VkStructureType sType;
    void*           pNext;
    VkBool32        robustBufferAccess;
    VkBool32        fullDrawIndexUint32;
    VkBool32        imageCubeArray;
    VkBool32        independentBlend;
    VkBool32        geometryShader;
    VkBool32        tessellationShader;
    VkBool32        sampleRateShading;
    VkBool32        dualSrcBlend;
    VkBool32        logicOp;
    VkBool32        multiDrawIndirect;
    VkBool32        drawIndirectFirstInstance;
    VkBool32        depthClamp;
    VkBool32        depthBiasClamp;
    VkBool32        fillModeNonSolid;
    VkBool32        depthBounds;
    VkBool32        wideLines;
    VkBool32        largePoints;
    VkBool32        alphaToOne;
    VkBool32        multiViewport;
    VkBool32        samplerAnisotropy;
    VkBool32        textureCompressionETC2;
    VkBool32        textureCompressionASTC_LDR;
    VkBool32        textureCompressionBC;
    VkBool32        occlusionQueryPrecise;
    VkBool32        pipelineStatisticsQuery;
    VkBool32        vertexPipelineStoresAndAtomics;
    VkBool32        fragmentStoresAndAtomics;
    VkBool32        shaderTessellationAndGeometryPointSize;
    VkBool32        shaderImageGatherExtended;
    VkBool32        shaderStorageImageExtendedFormats;
    VkBool32        shaderStorageImageMultisample;
    VkBool32        shaderStorageImageReadWithoutFormat;
    VkBool32        shaderStorageImageWriteWithoutFormat;
    VkBool32        shaderUniformBufferArrayDynamicIndexing;
    VkBool32        shaderSampledImageArrayDynamicIndexing;
    VkBool32        shaderStorageBufferArrayDynamicIndexing;
    VkBool32        shaderStorageImageArrayDynamicIndexing;
    VkBool32        shaderClipDistance;
    VkBool32        shaderCullDistance;
    VkBool32        shaderFloat64;
    VkBool32        shaderInt64;
    VkBool32        shaderInt16;
    VkBool32        shaderResourceResidency;
    VkBool32        shaderResourceMinLod;
    VkBool32        sparseBinding;
    VkBool32        sparseResidencyBuffer;
    VkBool32        sparseResidencyImage2D;
    VkBool32        sparseResidencyImage3D;
    VkBool32        sparseResidency2Samples;
    VkBool32        sparseResidency4Samples;
    VkBool32        sparseResidency8Samples;
    VkBool32        sparseResidency16Samples;
    VkBool32        sparseResidencyAliased;
    VkBool32        variableMultisampleRate;
    VkBool32        inheritedQueries;
};
static constexpr VkStructureType VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_0_FEATURES = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

struct DeviceFeatures
{
    VkPhysicalDeviceVulkan10Features vulkan_1_0;
    VkPhysicalDeviceVulkan11Features vulkan_1_1;
    VkPhysicalDeviceVulkan12Features vulkan_1_2;
    VkPhysicalDeviceVulkan13Features vulkan_1_3;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitDeviceFeatures(DeviceFeatures* device_features)
{
    device_features->vulkan_1_0 =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_0_FEATURES,
        .pNext = &device_features->vulkan_1_1,
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

static const char* GetDeviceFeatureName_1_0(uint32 feature_index)
{
    CTK_ASSERT(feature_index < VULKAN_1_0_DEVICE_FEATURE_COUNT);
    return VULKAN_1_0_DEVICE_FEATURE_NAMES[feature_index];
}

static const char* GetDeviceFeatureName_1_1(uint32 feature_index)
{
    CTK_ASSERT(feature_index < VULKAN_1_1_DEVICE_FEATURE_COUNT);
    return VULKAN_1_1_DEVICE_FEATURE_NAMES[feature_index];
}

static const char* GetDeviceFeatureName_1_2(uint32 feature_index)
{
    CTK_ASSERT(feature_index < VULKAN_1_2_DEVICE_FEATURE_COUNT);
    return VULKAN_1_2_DEVICE_FEATURE_NAMES[feature_index];
}

static const char* GetDeviceFeatureName_1_3(uint32 feature_index)
{
    CTK_ASSERT(feature_index < VULKAN_1_3_DEVICE_FEATURE_COUNT);
    return VULKAN_1_3_DEVICE_FEATURE_NAMES[feature_index];
}

static VkBool32* GetDeviceFeaturesArray_1_0(VkPhysicalDeviceVulkan10Features* vulkan_1_0)
{
    return (VkBool32*)&vulkan_1_0->robustBufferAccess;
}

static VkBool32* GetDeviceFeaturesArray_1_1(VkPhysicalDeviceVulkan11Features* vulkan_1_1)
{
    return (VkBool32*)&vulkan_1_1->storageBuffer16BitAccess;
}

static VkBool32* GetDeviceFeaturesArray_1_2(VkPhysicalDeviceVulkan12Features* vulkan_1_2)
{
    return (VkBool32*)&vulkan_1_2->samplerMirrorClampToEdge;
}

static VkBool32* GetDeviceFeaturesArray_1_3(VkPhysicalDeviceVulkan13Features* vulkan_1_3)
{
    return (VkBool32*)&vulkan_1_3->robustImageAccess;
}

static void GetDeviceFeatures(VkPhysicalDevice physical_device, DeviceFeatures* device_features)
{
    vkGetPhysicalDeviceFeatures2(physical_device, (VkPhysicalDeviceFeatures2*)&device_features->vulkan_1_0);
}

