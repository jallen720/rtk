#pragma once

#include "rtk/vulkan.h"
#include "rtk/rtk_context.h"
#include "rtk/shader.h"
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/containers.h"

using namespace CTK;

namespace RTK {

/// Data
////////////////////////////////////////////////////////////
static constexpr VkColorComponentFlags COLOR_COMPONENT_RGBA = VK_COLOR_COMPONENT_R_BIT |
                                                              VK_COLOR_COMPONENT_G_BIT |
                                                              VK_COLOR_COMPONENT_B_BIT |
                                                              VK_COLOR_COMPONENT_A_BIT;

static constexpr VkPipelineVertexInputStateCreateInfo DEFAULT_VERTEX_INPUT_STATE = {
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

struct VertexLayout
{
    Array<VkVertexInputBindingDescription>   bindings;
    Array<VkVertexInputAttributeDescription> attributes;
    uint32                                   attribute_location;
};

struct PipelineInfo
{
    VertexLayout*                vertex_layout;
    Array<Shader>                shaders;
    Array<VkDescriptorSetLayout> descriptor_set_layouts;
    Array<VkPushConstantRange>   push_constant_ranges;
};

struct Pipeline
{
    VkPipeline hnd;
    VkPipelineLayout layout;
};

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

static void PushAttribute(VertexLayout* layout, uint32 field_count)
{
    static constexpr VkFormat FORMATS[] =
    {
        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
    };

    CTK_ASSERT(layout->bindings.count > 0);
    CTK_ASSERT(field_count >= 1 && field_count <= 4);

    uint32 current_binding_index = layout->bindings.count - 1;
    VkVertexInputBindingDescription* current_binding = GetPtr(&layout->bindings, current_binding_index);

    Push(&layout->attributes,
    {
        .location = layout->attribute_location,
        .binding  = current_binding_index,
        .format   = FORMATS[field_count - 1],
        .offset   = current_binding->stride,
    });

    // Update binding state for future attributes.
    current_binding->stride += field_count * 4;
    layout->attribute_location++;
}

static void InitPipeline(Pipeline* pipeline, Stack temp_mem, RTKContext* rtk, PipelineInfo* info)
{
    VkDevice device = rtk->device;
    VkExtent2D surface_extent = rtk->surface.capabilities.currentExtent;
    VkResult res = VK_SUCCESS;

    VkPipelineVertexInputStateCreateInfo vertex_input_state = DEFAULT_VERTEX_INPUT_STATE;
    vertex_input_state.vertexBindingDescriptionCount   = info->vertex_layout->bindings.count;
    vertex_input_state.pVertexBindingDescriptions      = info->vertex_layout->bindings.data;
    vertex_input_state.vertexAttributeDescriptionCount = info->vertex_layout->attributes.count;
    vertex_input_state.pVertexAttributeDescriptions    = info->vertex_layout->attributes.data;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = DEFAULT_INPUT_ASSEMBLY_STATE;

    // Viewport
    VkViewport viewport =
    {
        .x        = 0,
        .y        = 0,
        .width    = (float32)surface_extent.width,
        .height   = (float32)surface_extent.height,
        .minDepth = 0,
        .maxDepth = 1
    };
    VkRect2D scissor =
    {
        .offset = { 0, 0 },
        .extent = surface_extent,
    };
    VkPipelineViewportStateCreateInfo viewport_state = DEFAULT_VIEWPORT_STATE;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = &viewport;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state = DEFAULT_RASTERIZATION_STATE;
    VkPipelineMultisampleStateCreateInfo multisample_state = DEFAULT_MULTISAMPLE_STATE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = DEFAULT_DEPTH_STENCIL_STATE;
    // depth_stencil_state.depthTestEnable = VK_TRUE;
    // depth_stencil_state.depthWriteEnable = VK_TRUE;
    // depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    // Color Blend
    VkPipelineColorBlendAttachmentState swapchain_image_color_blend = DEFAULT_COLOR_BLEND_ATTACHMENT_STATE;
    VkPipelineColorBlendStateCreateInfo color_blend_state = DEFAULT_COLOR_BLEND_STATE;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments    = &swapchain_image_color_blend;

    VkPipelineDynamicStateCreateInfo dynamic_state = DEFAULT_DYNAMIC_STATE;

    // Pipeline Layout
    VkPipelineLayoutCreateInfo layout_create_info = DEFAULT_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount         = info->descriptor_set_layouts.count;
    layout_create_info.pSetLayouts            = info->descriptor_set_layouts.data;
    layout_create_info.pushConstantRangeCount = info->push_constant_ranges.count;
    layout_create_info.pPushConstantRanges    = info->push_constant_ranges.data;
    res = vkCreatePipelineLayout(device, &layout_create_info, NULL, &pipeline->layout);
    Validate(res, "failed to create graphics pipeline layout");

    // Pipeline
    auto shader_stages = CreateArray<VkPipelineShaderStageCreateInfo>(&temp_mem, info->shaders.count);
    for (uint32 i = 0; i < info->shaders.count; ++i)
        Push(shader_stages, DefaultShaderStageCreateInfo(GetPtr(&info->shaders, i)));

    VkGraphicsPipelineCreateInfo create_info =
    {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = NULL,
        .flags               = 0,
        .stageCount          = shader_stages->count,
        .pStages             = shader_stages->data,
        .pVertexInputState   = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState  = NULL,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState   = &multisample_state,
        .pDepthStencilState  = &depth_stencil_state,
        .pColorBlendState    = &color_blend_state,
        .pDynamicState       = &dynamic_state,
        .layout              = pipeline->layout,
        .renderPass          = rtk->render_pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };
    res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline->hnd);
    Validate(res, "failed to create graphics pipeline");
}

}
