/// Data
////////////////////////////////////////////////////////////
static constexpr VkColorComponentFlags COLOR_COMPONENT_RGBA = VK_COLOR_COMPONENT_R_BIT |
                                                              VK_COLOR_COMPONENT_G_BIT |
                                                              VK_COLOR_COMPONENT_B_BIT |
                                                              VK_COLOR_COMPONENT_A_BIT;

#include "rtk/pipeline_defaults.h"

enum AttributeType
{
    ATTRIBUTE_TYPE_UINT32,
    ATTRIBUTE_TYPE_SINT32,
    ATTRIBUTE_TYPE_FLOAT32,
    ATTRIBUTE_TYPE_COUNT,
};

struct VertexLayout
{
    Array<VkVertexInputBindingDescription>   bindings;
    Array<VkVertexInputAttributeDescription> attributes;
    uint32                                   attribute_location;
};

struct PipelineInfo
{
    Array<Shader*>    shaders;
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
        Push(&pipeline->scissors,
        {
            .offset =
            {
                .x = (sint32)viewport->x,
                .y = (sint32)viewport->y,
            },
            .extent =
            {
                .width  = (uint32)viewport->width,
                .height = (uint32)viewport->height,
            }
        });
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
    static constexpr VkPipelineColorBlendAttachmentState SWAPCHAIN_IMAGE_COLOR_BLEND =
        DEFAULT_COLOR_BLEND_ATTACHMENT_STATE;
    static constexpr VkPipelineColorBlendStateCreateInfo COLOR_BLEND_STATE =
        DefaultColorBlendStateCreateInfo(&SWAPCHAIN_IMAGE_COLOR_BLEND, 1);

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
    VkResult res = vkCreateGraphicsPipelines(global_ctx.device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline->hnd);
    Validate(res, "vkCreateGraphicsPipelines() failed");
}

/// Interface
////////////////////////////////////////////////////////////
static void PushBinding(VertexLayout* layout, VkVertexInputRate input_rate)
{
    Push(&layout->bindings,
    {
        .binding   = layout->bindings.count,
        .stride    = 0,
        .inputRate = input_rate,
    });

    layout->attribute_location = 0; // Reset attribute location for new binding.
}

static void PushAttribute(VertexLayout* layout, uint32 field_count, AttributeType attribute_type)
{
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

    CTK_ASSERT(layout->bindings.count > 0);
    CTK_ASSERT(field_count >= 1 && field_count <= 4);
    CTK_ASSERT(attribute_type < ATTRIBUTE_TYPE_COUNT);

    uint32 current_binding_index = layout->bindings.count - 1;
    VkVertexInputBindingDescription* current_binding = GetPtr(&layout->bindings, current_binding_index);

    uint32 format_index = ((field_count - 1) * (uint32)ATTRIBUTE_TYPE_COUNT) + (uint32)attribute_type;
    Push(&layout->attributes,
    {
        .location = layout->attribute_location,
        .binding  = current_binding_index,
        .format   = FORMATS[format_index],
        .offset   = current_binding->stride,
    });

    // Update binding state for future attributes.
    current_binding->stride += field_count * 4;
    ++layout->attribute_location;
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

    VkResult res = vkCreatePipelineLayout(global_ctx.device, &layout_create_info, NULL, &pipeline->layout);
    Validate(res, "vkCreatePipelineLayout() failed");

    // Shader Stages
    InitArray(&pipeline->shader_stage_create_infos, &free_list->allocator, info->shaders.count);
    for (uint32 i = 0; i < info->shaders.count; ++i)
    {
        Push(&pipeline->shader_stage_create_infos, DefaultShaderStageCreateInfo(Get(&info->shaders, i)));
    }

    // Viewports/Scissors
    InitArray(&pipeline->viewports, &free_list->allocator, &info->viewports);
    InitArray(&pipeline->scissors, &free_list->allocator, pipeline->viewports.size);
    InitScissors(pipeline);

    pipeline->vertex_layout = info->vertex_layout;
    pipeline->depth_testing = info->depth_testing;
    pipeline->render_pass   = info->render_target->render_pass;

    /// Create Pipeline
    ////////////////////////////////////////////////////////////
    CreatePipeline(pipeline);
}

static Pipeline* CreatePipeline(const Allocator* allocator, Stack* temp_stack, FreeList* free_list, PipelineInfo* info,
                                PipelineLayoutInfo* layout_info)
{
    auto pipeline = Allocate<Pipeline>(allocator, 1);
    InitPipeline(pipeline, temp_stack, free_list, info, layout_info);
    return pipeline;
}

static void UpdatePipelineViewports(Pipeline* pipeline, FreeList* free_list, Array<VkViewport> viewports)
{
    // Update viewports and scissors arrays.
    Clear(&pipeline->viewports);
    Clear(&pipeline->scissors);
    if (!CanPush(&pipeline->viewports, viewports.count))
    {
        Resize(&pipeline->viewports, &free_list->allocator, viewports.count);
        Resize(&pipeline->scissors, &free_list->allocator, viewports.count);
    }
    PushRange(&pipeline->viewports, &viewports);
    InitScissors(pipeline);

    // Recreate pipeline.
    vkDestroyPipeline(global_ctx.device, pipeline->hnd, NULL);
    CreatePipeline(pipeline);
}
