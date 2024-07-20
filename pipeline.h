/// Data
////////////////////////////////////////////////////////////
static constexpr VkColorComponentFlags COLOR_COMPONENT_RGBA = VK_COLOR_COMPONENT_R_BIT |
                                                              VK_COLOR_COMPONENT_G_BIT |
                                                              VK_COLOR_COMPONENT_B_BIT |
                                                              VK_COLOR_COMPONENT_A_BIT;

struct Shader
{
    VkShaderModule        module;
    VkShaderStageFlagBits stage;
};

#include "rtk/pipeline_defaults.h"

enum struct AttributeType
{
    UINT32,
    SINT32,
    FLOAT32,
    COUNT,
};

struct AttributeInfo
{
    uint32        component_count;
    AttributeType type;
};

struct BindingInfo
{
    VkVertexInputRate    input_rate;
    Array<AttributeInfo> attribute_infos;
};

struct VertexLayout
{
    Array<VkVertexInputBindingDescription>   bindings;
    Array<VkVertexInputAttributeDescription> attributes;
    uint32                                   attribute_location;
};

struct PipelineInfo
{
    Array<Shader>     shaders;
    Array<VkViewport> viewports;
    VertexLayout*     vertex_layout;
    bool              depth_testing;
    RenderTarget*     render_target;
};

struct PipelineLayoutInfo
{
    Array<VkDescriptorSetLayout> descriptor_set_layouts;
    Array<VkPushConstantRange>   push_constant_ranges;
};

struct Pipeline
{
    // Configuration
    Array<VkPipelineShaderStageCreateInfo> shader_stage_create_infos;
    Array<VkViewport>                      viewports;
    Array<VkRect2D>                        scissors;
    VertexLayout*                          vertex_layout;
    bool                                   depth_testing;
    VkRenderPass                           render_pass;

    // State
    VkPipelineLayout layout;
    VkPipeline       hnd;
};

/// Utils
////////////////////////////////////////////////////////////
static void InitScissors(Pipeline* pipeline)
{
    // Make all scissors cover their corresponding viewports.
    for (uint32 i = 0; i < pipeline->viewports.count; ++i)
    {
        VkViewport* viewport = GetPtr(&pipeline->viewports, i);
        VkRect2D* scissor = Push(&pipeline->scissors);
        scissor->offset =
        {
            .x = (sint32)viewport->x,
            .y = (sint32)viewport->y,
        };
        scissor->extent =
        {
            .width  = (uint32)viewport->width,
            .height = (uint32)viewport->height,
        };
    }
}

static void CreatePipeline(Pipeline* pipeline)
{
    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertex_input_state = DEFAULT_VERTEX_INPUT_STATE;
    vertex_input_state.vertexBindingDescriptionCount   = pipeline->vertex_layout->bindings.count;
    vertex_input_state.pVertexBindingDescriptions      = pipeline->vertex_layout->bindings.data;
    vertex_input_state.vertexAttributeDescriptionCount = pipeline->vertex_layout->attributes.count;
    vertex_input_state.pVertexAttributeDescriptions    = pipeline->vertex_layout->attributes.data;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = DEFAULT_INPUT_ASSEMBLY_STATE;

    // Viewport/Scissors
    VkPipelineViewportStateCreateInfo viewport_state = DEFAULT_VIEWPORT_STATE;
    viewport_state.viewportCount = pipeline->viewports.count;
    viewport_state.pViewports    = pipeline->viewports.data;
    viewport_state.scissorCount  = pipeline->scissors.count;
    viewport_state.pScissors     = pipeline->scissors.data;

    VkPipelineRasterizationStateCreateInfo rasterization_state = DEFAULT_RASTERIZATION_STATE;
    VkPipelineMultisampleStateCreateInfo multisample_state = DEFAULT_MULTISAMPLE_STATE;

    // Depth/Stencil
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = DEFAULT_DEPTH_STENCIL_STATE;
    if (pipeline->depth_testing)
    {
        depth_stencil_state.depthTestEnable  = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    // Color Blend
    static constexpr VkPipelineColorBlendStateCreateInfo COLOR_BLEND_STATE =
        DefaultColorBlendStateCreateInfo(&DEFAULT_COLOR_BLEND_ATTACHMENT_STATE, 1);

    VkPipelineDynamicStateCreateInfo dynamic_state = DEFAULT_DYNAMIC_STATE;

    // Pipeline
    VkGraphicsPipelineCreateInfo create_info =
    {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = NULL,
        .flags               = 0,
        .stageCount          = pipeline->shader_stage_create_infos.count,
        .pStages             = pipeline->shader_stage_create_infos.data,
        .pVertexInputState   = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState  = NULL,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState   = &multisample_state,
        .pDepthStencilState  = &depth_stencil_state,
        .pColorBlendState    = &COLOR_BLEND_STATE,
        .pDynamicState       = &dynamic_state,
        .layout              = pipeline->layout,
        .renderPass          = pipeline->render_pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };
    VkResult res = vkCreateGraphicsPipelines(GetDevice(), VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline->hnd);
    Validate(res, "vkCreateGraphicsPipelines() failed");
}

/// Interface
////////////////////////////////////////////////////////////
static VkShaderModule LoadShaderModule(Stack* temp_stack, const char* path)
{
    Stack frame = CreateFrame(temp_stack);

    Array<uint32> bytecode = ReadFile<uint32>(&frame.allocator, path);
    VkShaderModuleCreateInfo info =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .codeSize = ByteSize(&bytecode),
        .pCode    = bytecode.data,
    };
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(GetDevice(), &info, NULL, &shader_module);
    Validate(res, "vkCreateShaderModule() failed");

    return shader_module;
}

static void InitVertexLayout(VertexLayout* layout, Allocator* allocator, Array<BindingInfo> binding_infos)
{
    CTK_ITER(binding_info, &binding_infos)
    {
        layout->attributes.size += binding_info->attribute_infos.count;
    }

    layout->bindings   = CreateArray<VkVertexInputBindingDescription>  (allocator, binding_infos.size);
    layout->attributes = CreateArray<VkVertexInputAttributeDescription>(allocator, layout->attributes.size);

    CTK_ITER(binding_info, &binding_infos)
    {
        uint32 binding_index = layout->bindings.count;
        VkVertexInputBindingDescription* binding = Push(&layout->bindings);
        binding->binding   = binding_index;
        binding->stride    = 0; // Set by accumulating attribute sizes.
        binding->inputRate = binding_info->input_rate;

        static constexpr VkFormat FORMATS[] =
        {
            VK_FORMAT_R32_UINT,
            VK_FORMAT_R32_SINT,
            VK_FORMAT_R32_SFLOAT,
            VK_FORMAT_R32G32_UINT,
            VK_FORMAT_R32G32_SINT,
            VK_FORMAT_R32G32_SFLOAT,
            VK_FORMAT_R32G32B32_UINT,
            VK_FORMAT_R32G32B32_SINT,
            VK_FORMAT_R32G32B32_SFLOAT,
            VK_FORMAT_R32G32B32A32_UINT,
            VK_FORMAT_R32G32B32A32_SINT,
            VK_FORMAT_R32G32B32A32_SFLOAT,
        };
        static constexpr uint32 COMPONENT_BYTE_SIZE = 4; // All components in above formats have 32 bits.

        uint32 attribute_index = 0;
        CTK_ITER(attribute_info, &binding_info->attribute_infos)
        {
            CTK_ASSERT(attribute_info->component_count >= 1 && attribute_info->component_count <= 4);

            uint32 format_index = ((attribute_info->component_count - 1) * (uint32)AttributeType::COUNT) +
                                  (uint32)attribute_info->type;

            VkVertexInputAttributeDescription* attribute = Push(&layout->attributes);
            attribute->location = attribute_index;
            attribute->binding  = binding_index;
            attribute->format   = FORMATS[format_index];
            attribute->offset   = binding->stride;

            // Update binding state for future attributes.
            binding->stride += attribute_info->component_count * COMPONENT_BYTE_SIZE;
            attribute_index += 1;
        }
    }
}

static void InitPipeline(Pipeline* pipeline, Stack* temp_stack, FreeList* free_list, PipelineInfo* info,
                         PipelineLayoutInfo* layout_info)
{
    Stack frame = CreateFrame(temp_stack);

    /// Configure Pipeline
    ////////////////////////////////////////////////////////////

    // Layout
    VkPipelineLayoutCreateInfo layout_create_info = DEFAULT_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount         = layout_info->descriptor_set_layouts.count;
    layout_create_info.pSetLayouts            = layout_info->descriptor_set_layouts.data;
    layout_create_info.pushConstantRangeCount = layout_info->push_constant_ranges.count;
    layout_create_info.pPushConstantRanges    = layout_info->push_constant_ranges.data;

    VkResult res = vkCreatePipelineLayout(GetDevice(), &layout_create_info, NULL, &pipeline->layout);
    Validate(res, "vkCreatePipelineLayout() failed");

    // Shader Stages
    pipeline->shader_stage_create_infos =
        CreateArray<VkPipelineShaderStageCreateInfo>(&free_list->allocator, info->shaders.count);
    for (uint32 i = 0; i < info->shaders.count; ++i)
    {
        Push(&pipeline->shader_stage_create_infos, DefaultShaderStageCreateInfo(GetPtr(&info->shaders, i)));
    }

    // Viewports/Scissors
    pipeline->viewports = CreateArray<VkViewport>(&free_list->allocator, &info->viewports);
    pipeline->scissors  = CreateArray<VkRect2D>  (&free_list->allocator, pipeline->viewports.size);
    InitScissors(pipeline);

    pipeline->vertex_layout = info->vertex_layout;
    pipeline->depth_testing = info->depth_testing;
    pipeline->render_pass   = info->render_target->render_pass;

    /// Create Pipeline
    ////////////////////////////////////////////////////////////
    CreatePipeline(pipeline);
}

static Pipeline* CreatePipeline(Allocator* allocator, Stack* temp_stack, FreeList* free_list, PipelineInfo* info,
                                PipelineLayoutInfo* layout_info)
{
    auto pipeline = Allocate<Pipeline>(allocator, 1);
    InitPipeline(pipeline, temp_stack, free_list, info, layout_info);
    return pipeline;
}

static void UpdatePipelineViewports(Pipeline* pipeline, Array<VkViewport> viewports)
{
    // Update viewports and scissors arrays.
    Clear(&pipeline->viewports);
    Clear(&pipeline->scissors);
    if (!CanPush(&pipeline->viewports, viewports.count))
    {
        Resize(&pipeline->viewports, viewports.count);
        Resize(&pipeline->scissors,  viewports.count);
    }
    PushRange(&pipeline->viewports, &viewports);
    InitScissors(pipeline);

    // Recreate pipeline.
    vkDestroyPipeline(GetDevice(), pipeline->hnd, NULL);
    CreatePipeline(pipeline);
}
