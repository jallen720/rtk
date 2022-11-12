#pragma once

#include "rtk/vulkan.h"
#include "rtk/rtk_context.h"
#include "rtk/shader.h"
#include "rtk/shader_data.h"
#include "rtk/render_target.h"
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

#include "rtk/pipeline_defaults.h"

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
    VkPipeline       hnd;
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
    ++layout->attribute_location;
}

static void InitPipeline(Pipeline* pipeline, Stack temp_mem, RenderTarget* render_target, RTKContext* rtk,
                         PipelineInfo* info)
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
    depth_stencil_state.depthTestEnable  = VK_TRUE;
    depth_stencil_state.depthWriteEnable = VK_TRUE;
    depth_stencil_state.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

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

    VkPipelineLayout layout = VK_NULL_HANDLE;
    res = vkCreatePipelineLayout(rtk->device, &layout_create_info, NULL, &pipeline->layout);
    Validate(res, "vkCreatePipelineLayout() failed");

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
        .renderPass          = render_target->render_pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };
    res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline->hnd);
    Validate(res, "vkCreateGraphicsPipelines() failed");
}

static void BindShaderDataSets(Array<VkDescriptorSet> sets, Pipeline* pipeline, VkCommandBuffer command_buffer,
                               uint32 start)
{
    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline->layout,
                            start, // First set
                            sets.count, sets.data, // Descriptor set count + pointer
                            0, NULL); // Dynamic offset count + pointer
}

}
