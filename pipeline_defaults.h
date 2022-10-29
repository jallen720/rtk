static constexpr VkPipelineVertexInputStateCreateInfo DEFAULT_VERTEX_INPUT_STATE =
{
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext                           = NULL,
    .flags                           = 0,
    .vertexBindingDescriptionCount   = 0,
    .pVertexBindingDescriptions      = NULL,
    .vertexAttributeDescriptionCount = 0,
    .pVertexAttributeDescriptions    = NULL,
};

static constexpr VkPipelineInputAssemblyStateCreateInfo DEFAULT_INPUT_ASSEMBLY_STATE =
{
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext                  = NULL,
    .flags                  = 0,
    .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
};

static constexpr VkPipelineViewportStateCreateInfo DEFAULT_VIEWPORT_STATE =
{
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext         = NULL,
    .flags         = 0,
    .viewportCount = 0,
    .pViewports    = NULL,
    .scissorCount  = 0,
    .pScissors     = NULL,
};

static constexpr VkPipelineRasterizationStateCreateInfo DEFAULT_RASTERIZATION_STATE =
{
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext                   = NULL,
    .flags                   = 0,
    .depthClampEnable        = VK_FALSE, // Don't clamp fragments within depth range.
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode             = VK_POLYGON_MODE_FILL, // Only available mode on AMD gpus?
    .cullMode                = VK_CULL_MODE_BACK_BIT,
    .frontFace               = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable         = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp          = 0.0f,
    .depthBiasSlopeFactor    = 0.0f,
    .lineWidth               = 1.0f,
};

static constexpr VkPipelineMultisampleStateCreateInfo DEFAULT_MULTISAMPLE_STATE =
{
    .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext                 = NULL,
    .flags                 = 0,
    .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable   = VK_FALSE,
    .minSampleShading      = 1.0f,
    .pSampleMask           = NULL,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable      = VK_FALSE,
};

static constexpr VkPipelineDepthStencilStateCreateInfo DEFAULT_DEPTH_STENCIL_STATE =
{
    .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext                 = NULL,
    .flags                 = 0,
    .depthTestEnable       = VK_FALSE,
    .depthWriteEnable      = VK_FALSE,
    .depthCompareOp        = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable     = VK_FALSE,
    .front =
    {
        .failOp      = VK_STENCIL_OP_KEEP,
        .passOp      = VK_STENCIL_OP_KEEP,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp   = VK_COMPARE_OP_NEVER,
        .compareMask = 0xFF,
        .writeMask   = 0xFF,
        .reference   = 1,
    },
    .back =
    {
        .failOp      = VK_STENCIL_OP_KEEP,
        .passOp      = VK_STENCIL_OP_KEEP,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp   = VK_COMPARE_OP_NEVER,
        .compareMask = 0xFF,
        .writeMask   = 0xFF,
        .reference   = 1,
    },
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f,
};

static constexpr VkPipelineColorBlendAttachmentState DEFAULT_COLOR_BLEND_ATTACHMENT_STATE =
{
    .blendEnable         = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp        = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp        = VK_BLEND_OP_ADD,
    .colorWriteMask      = COLOR_COMPONENT_RGBA,
};

static constexpr VkPipelineColorBlendStateCreateInfo DEFAULT_COLOR_BLEND_STATE =
{
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext           = NULL,
    .flags           = 0,
    .logicOpEnable   = VK_FALSE,
    .logicOp         = VK_LOGIC_OP_COPY,
    .attachmentCount = 0,
    .pAttachments    = NULL,
    .blendConstants  = { 1.0f, 1.0f, 1.0f, 1.0f },
};

static constexpr VkPipelineDynamicStateCreateInfo DEFAULT_DYNAMIC_STATE =
{
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .pNext             = NULL,
    .flags             = 0,
    .dynamicStateCount = 0,
    .pDynamicStates    = NULL,
};

static constexpr VkPipelineLayoutCreateInfo DEFAULT_LAYOUT_CREATE_INFO =
{
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext                  = NULL,
    .flags                  = 0,
    .setLayoutCount         = 0,
    .pSetLayouts            = NULL,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges    = NULL,
};

static VkPipelineShaderStageCreateInfo DefaultShaderStageCreateInfo(Shader* shader)
{
    return
    {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = NULL,
        .flags               = 0,
        .stage               = shader->stage,
        .module              = shader->hnd,
        .pName               = "main",
        .pSpecializationInfo = NULL,
    };
}
