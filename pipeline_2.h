/// Data
////////////////////////////////////////////////////////////
struct PipelineLayoutInfo2
{
    Array<VkDescriptorSetLayout> descriptor_set_layouts;
    Array<VkPushConstantRange>   push_constant_ranges;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitPipeline2(Pipeline* pipeline, Stack* temp_stack, FreeList* free_list, PipelineInfo* info,
                          PipelineLayoutInfo2* layout_info)
{
    Stack frame = CreateFrame(temp_stack);

    /// Configure Pipeline
    ////////////////////////////////////////////////////////////
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
