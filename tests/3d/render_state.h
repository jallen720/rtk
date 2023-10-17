/// Data
////////////////////////////////////////////////////////////
struct EntityBuffer
{
    Matrix mvp_matrixes[MAX_ENTITIES];
    uint32 texture_indexes[MAX_ENTITIES];
};

struct RenderCommandState
{
    uint32     thread_index;
    BatchRange batch_range;
    Mesh*      mesh;
    Stack*     temp_stack;
};

struct RenderState
{
    // Render Job
    Job<RenderCommandState> render_command_job;
    Array<Stack>            render_thread_temp_stacks;

    // Device Memory
    BufferHnd staging_buffer;

    // Rendering State
    RenderTarget render_target;
    Shader       vert_shader;
    Shader       frag_shader;
    MeshData     mesh_data;
    Mesh         cube_mesh;
    Mesh         quad_mesh;

    // Descriptor Datas
    BufferHnd       entity_buffer;
    // ImageGroupHnd   textures_group;
    Array<ImageHnd> textures;
    VkSampler       texture_sampler;

    // Descriptor Sets/Pipelines
    DescriptorSetHnd entity_descriptor_set;
    DescriptorSetHnd textures_descriptor_set;
    VertexLayout     vertex_layout;
    Pipeline         pipeline;
};

static constexpr const char* TEXTURE_IMAGE_PATHS[] =
{
    "images/axis_cube.png",
    "images/axis_cube_inv.png",
    "images/dirt_block.png",
};
static constexpr uint32 TEXTURE_COUNT = CTK_ARRAY_SIZE(TEXTURE_IMAGE_PATHS);
static_assert(TEXTURE_COUNT == MAX_TEXTURES);

/// Instance
////////////////////////////////////////////////////////////
static RenderState g_render_state;

/// Utils
////////////////////////////////////////////////////////////
static void InitRenderJob(Stack* perm_stack)
{
    uint32 render_thread_count = GetRenderThreadCount();

    InitThreadPoolJob(&g_render_state.render_command_job, perm_stack, render_thread_count);

    InitArray(&g_render_state.render_thread_temp_stacks, &perm_stack->allocator, render_thread_count);
    for (uint32 i = 0; i < render_thread_count; ++i)
    {
        InitStack(Push(&g_render_state.render_thread_temp_stacks), &perm_stack->allocator, Kilobyte32<1>());
    }
}

static void InitDeviceMemory()
{
    BufferInfo staging_buffer_info =
    {
        .size             = Megabyte32<4>(),
        .usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame        = false,
        .mem_properties   = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    g_render_state.staging_buffer = CreateBuffer(&staging_buffer_info);
}

static void InitRenderTargets(Stack* temp_stack, FreeList* free_list)
{
    VkClearValue attachment_clear_values[] =
    {
        { .color        = { 0.0f, 0.1f, 0.2f, 1.0f } },
        { .depthStencil = { 1.0f, 0u }               },
    };
    RenderTargetInfo info =
    {
        .depth_testing           = true,
        .attachment_clear_values = CTK_WRAP_ARRAY(attachment_clear_values),
    };
    InitRenderTarget(&g_render_state.render_target, temp_stack, free_list, &info);
}

static void InitShaders(Stack* temp_stack)
{
    InitShader(&g_render_state.vert_shader, temp_stack, "shaders/bin/3d.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    InitShader(&g_render_state.frag_shader, temp_stack, "shaders/bin/3d.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

static void InitMeshes()
{
    MeshDataInfo mesh_data_info =
    {
        .vertex_buffer_size = Megabyte32<1>(),
        .index_buffer_size  = Megabyte32<1>(),
    };
    InitMeshData(&g_render_state.mesh_data, &mesh_data_info);
}

static void InitDescriptorDatas(Stack* perm_stack)
{
    // Entity Buffer
    BufferInfo entity_buffer_info =
    {
        .size             = sizeof(EntityBuffer),
        .usage            = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame        = true,
        .mem_properties   = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    g_render_state.entity_buffer = CreateBuffer(&entity_buffer_info);

CTK_TODO("uncomment");
    // // Textures
    // InitArray(&g_render_state.textures, &perm_stack->allocator, TEXTURE_COUNT);
    // VkImageInfo texture_create_info =
    // {
    //     .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    //     .pNext     = NULL,
    //     .flags     = 0,
    //     .imageType = VK_IMAGE_TYPE_2D,
    //     .format    = GetSwapchain()->surface_format.format,
    //     .extent =
    //     {
    //         .width  = 64,
    //         .height = 32,
    //         .depth  = 1
    //     },
    //     .mipLevels             = 1,
    //     .arrayLayers           = 1,
    //     .samples               = VK_SAMPLE_COUNT_1_BIT,
    //     .tiling                = VK_IMAGE_TILING_OPTIMAL,
    //     .usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    //                              VK_IMAGE_USAGE_SAMPLED_BIT,
    //     .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
    //     .queueFamilyIndexCount = 0,
    //     .pQueueFamilyIndices   = NULL,
    //     .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    // };
    // for (uint32 i = 0; i < TEXTURE_COUNT; ++i)
    // {
    //     Push(&g_render_state.textures, CreateImage(g_render_state.textures_group, &texture_create_info));
    // }

    // Texture Sampler
    VkSamplerCreateInfo texture_sampler_info =
    {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext                   = NULL,
        .flags                   = 0,
        .magFilter               = VK_FILTER_NEAREST,
        .minFilter               = VK_FILTER_NEAREST,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias              = 0.0f,
        .anisotropyEnable        = VK_FALSE,
        .maxAnisotropy           = GetPhysicalDevice()->properties.limits.maxSamplerAnisotropy,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .minLod                  = 0.0f,
        .maxLod                  = 0.0f,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    VkResult res = vkCreateSampler(GetDevice(), &texture_sampler_info, NULL, &g_render_state.texture_sampler);
    Validate(res, "vkCreateSampler() failed");
}

static void InitDescriptorSets(Stack* perm_stack, Stack* temp_stack)
{
    // Entity
    {
        DescriptorData datas[] =
        {
            {
                .type        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stages      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .count       = 1,
                .buffer_hnds = &g_render_state.entity_buffer,
            },
        };
        g_render_state.entity_descriptor_set =
            CreateDescriptorSet(&perm_stack->allocator, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    // Textures
    {
        DescriptorData datas[] =
        {
            {
                .type       = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .stages     = VK_SHADER_STAGE_FRAGMENT_BIT,
                .count      = g_render_state.textures.count,
                .image_hnds = g_render_state.textures.data,
            },
            {
                .type       = VK_DESCRIPTOR_TYPE_SAMPLER,
                .stages     = VK_SHADER_STAGE_FRAGMENT_BIT,
                .count      = 1,
                .samplers   = &g_render_state.texture_sampler,
            },
        };
        g_render_state.textures_descriptor_set =
            CreateDescriptorSet(&perm_stack->allocator, temp_stack, CTK_WRAP_ARRAY(datas));
    }
}

static void InitVertexLayout(Stack* perm_stack)
{
    VertexLayout* vertex_layout = &g_render_state.vertex_layout;

    // Init pipeline vertex layout.
    InitArray(&vertex_layout->bindings, &perm_stack->allocator, 1);
    PushBinding(vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);

    InitArray(&vertex_layout->attributes, &perm_stack->allocator, 4);
    PushAttribute(vertex_layout, 3, ATTRIBUTE_TYPE_FLOAT32); // Position
    PushAttribute(vertex_layout, 2, ATTRIBUTE_TYPE_FLOAT32); // UV
}

static void InitPipelines(Stack* temp_stack, FreeList* free_list)
{
    // Pipeline Layout
    VkDescriptorSetLayout descriptor_set_layouts[] =
    {
        GetLayout(g_render_state.entity_descriptor_set),
        GetLayout(g_render_state.textures_descriptor_set),
    };
    // VkPushConstantRange push_constant_ranges[] =
    // {
    //     {
    //         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    //         .offset     = 0,
    //         .size       = sizeof(uint32)
    //     },
    // };
    PipelineLayoutInfo pipeline_layout_info =
    {
        .descriptor_set_layouts = CTK_WRAP_ARRAY(descriptor_set_layouts),
        // .push_constant_ranges   = CTK_WRAP_ARRAY(push_constant_ranges),
    };

    // Pipeline Info
    VkExtent2D swapchain_extent = GetSwapchain()->surface_extent;
    VkViewport viewports[] =
    {
        {
            .x        = 0,
            .y        = 0,
            .width    = (float32)swapchain_extent.width,
            .height   = (float32)swapchain_extent.height,
            .minDepth = 0,
            .maxDepth = 1
        },
    };
    Shader* shaders[] =
    {
        &g_render_state.vert_shader,
        &g_render_state.frag_shader,
    };
    PipelineInfo pipeline_info =
    {
        .shaders       = CTK_WRAP_ARRAY(shaders),
        .viewports     = CTK_WRAP_ARRAY(viewports),
        .vertex_layout = &g_render_state.vertex_layout,
        .depth_testing = true,
        .render_target = &g_render_state.render_target,
    };

    InitPipeline(&g_render_state.pipeline, temp_stack, free_list, &pipeline_info, &pipeline_layout_info);
}

static void AllocateResources(Stack* temp_stack)
{
    AllocateBuffers();
    AllocateImages(temp_stack);
    CTK_TODO("uncomment")
    // AllocateDescriptorSets(temp_stack);
}

static void WriteResources()
{
    CTK_TODO("uncomment")
    // // Images
    // for (uint32 i = 0; i < TEXTURE_COUNT; ++i)
    // {
    //     LoadImage(Get(&g_render_state.textures, i), g_render_state.staging_buffer, 0, TEXTURE_IMAGE_PATHS[i]);
    // }

    // Meshes
    {
        #include "rtk/meshes/cube.h"
        InitDeviceMesh(&g_render_state.cube_mesh, &g_render_state.mesh_data,
                       CTK_WRAP_ARRAY(vertexes), CTK_WRAP_ARRAY(indexes),
                       g_render_state.staging_buffer);
    }
    {
        #include "rtk/meshes/quad.h"
        InitDeviceMesh(&g_render_state.quad_mesh, &g_render_state.mesh_data,
                       CTK_WRAP_ARRAY(vertexes), CTK_WRAP_ARRAY(indexes),
                       g_render_state.staging_buffer);
    }
}

static void RecordRenderCommandsThread(void* data)
{
    auto state = (RenderCommandState*)data;
    VkCommandBuffer command_buffer = BeginRenderCommands(&g_render_state.render_target, state->thread_index);
        Pipeline* pipeline = &g_render_state.pipeline;
        BindPipeline(command_buffer, pipeline);
        DescriptorSetHnd descriptor_sets[] =
        {
            g_render_state.entity_descriptor_set,
            g_render_state.textures_descriptor_set,
        };
        BindDescriptorSets(command_buffer, pipeline, state->temp_stack, CTK_WRAP_ARRAY(descriptor_sets), 0);
        BindMeshData(command_buffer, &g_render_state.mesh_data);
        DrawMesh(command_buffer, state->mesh, state->batch_range.start, state->batch_range.size);
    EndRenderCommands(command_buffer);
}

static void UpdateAllPipelineViewports(FreeList* free_list)
{
    VkExtent2D swapchain_extent = GetSwapchain()->surface_extent;
    VkViewport viewports[] =
    {
        {
            .x        = 0,
            .y        = 0,
            .width    = (float32)swapchain_extent.width,
            .height   = (float32)swapchain_extent.height,
            .minDepth = 0,
            .maxDepth = 1,
        },
    };
    UpdatePipelineViewports(&g_render_state.pipeline, free_list, CTK_WRAP_ARRAY(viewports));
}

static void UpdateAllRenderTargetAttachments(Stack* temp_stack, FreeList* free_list)
{
    UpdateRenderTargetAttachments(&g_render_state.render_target, temp_stack, free_list);
}

static void RecreateSwapchain(Stack* temp_stack, FreeList* free_list)
{
    WaitIdle();
    UpdateSwapchainSurfaceExtent(temp_stack, free_list);
    UpdateAllPipelineViewports(free_list);
    UpdateAllRenderTargetAttachments(temp_stack, free_list);
}

/// Interface
////////////////////////////////////////////////////////////
static void InitRenderState(Stack* perm_stack, Stack* temp_stack, FreeList* free_list)
{
    InitBufferModule(&perm_stack->allocator, 32);
    InitImageModule(&perm_stack->allocator, 32);
    InitDescriptorSetModule(&perm_stack->allocator, 16);

    // InitRenderJob(perm_stack);
    // InitDeviceMemory();

    // InitRenderTargets(temp_stack, free_list);
    // InitShaders(temp_stack);
    // InitMeshes();

    // InitDescriptorDatas(perm_stack);
    // InitDescriptorSets(perm_stack, temp_stack);
    // InitVertexLayout(perm_stack);
    // InitPipelines(temp_stack, free_list);

ImageInfo image_infos[] =
{
    {
        .extent =
        {
            .width  = 16,
            .height = 16,
            .depth  = 1
        },
        .usage          = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .per_frame      = true,
        .mem_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .flags          = 0,
        .type           = VK_IMAGE_TYPE_2D,
        .format         = GetSwapchain()->surface_format.format,
        .mip_levels     = 1,
        .array_layers   = 1,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .tiling         = VK_IMAGE_TILING_OPTIMAL,
    },
    {
        .extent =
        {
            .width  = 64,
            .height = 32,
            .depth  = 1
        },
        .usage          = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .per_frame      = false,
        .mem_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .flags          = 0,
        .type           = VK_IMAGE_TYPE_2D,
        .format         = GetPhysicalDevice()->depth_image_format,
        .mip_levels     = 1,
        .array_layers   = 1,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .tiling         = VK_IMAGE_TILING_OPTIMAL,
    },
    {
        .extent =
        {
            .width  = 32,
            .height = 32,
            .depth  = 32
        },
        .usage          = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .per_frame      = false,
        .mem_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .flags          = 0,
        .type           = VK_IMAGE_TYPE_3D,
        .format         = GetSwapchain()->surface_format.format,
        .mip_levels     = 1,
        .array_layers   = 1,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .tiling         = VK_IMAGE_TILING_OPTIMAL,
    },
};
ImageViewInfo default_view_infos[] =
{
    {
        .flags      = 0,
        .type       = VK_IMAGE_VIEW_TYPE_2D,
        .format     = image_infos[0].format,
        .components = RGBA_COMPONENT_SWIZZLE_IDENTITY,
        .subresource_range =
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount     = VK_REMAINING_ARRAY_LAYERS,
        },
    },
    {
        .flags      = 0,
        .type       = VK_IMAGE_VIEW_TYPE_2D,
        .format     = image_infos[1].format,
        .components = RGBA_COMPONENT_SWIZZLE_IDENTITY,
        .subresource_range =
        {
            .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel   = 0,
            .levelCount     = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount     = VK_REMAINING_ARRAY_LAYERS,
        },
    },
    {
        .flags      = 0,
        .type       = VK_IMAGE_VIEW_TYPE_3D,
        .format     = image_infos[2].format,
        .components = RGBA_COMPONENT_SWIZZLE_IDENTITY,
        .subresource_range =
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount     = VK_REMAINING_ARRAY_LAYERS,
        },
    },
};
ImageHnd images[] =
{
    CreateImage(&image_infos[0], &default_view_infos[0]),
    CreateImage(&image_infos[1], &default_view_infos[1]),
    CreateImage(&image_infos[2], &default_view_infos[2]),
};
    AllocateResources(temp_stack);
LogImages();
LogDefaultViews();
LogImageMemory();
exit(0);
    // WriteResources();
}

static void RecordRenderCommands(ThreadPool* thread_pool, uint32 entity_count)
{
    Job<RenderCommandState>* job = &g_render_state.render_command_job;

    static Mesh* meshes[] =
    {
        &g_render_state.quad_mesh,
        &g_render_state.cube_mesh,
    };
    static constexpr uint32 MESH_COUNT = CTK_ARRAY_SIZE(meshes);

    uint32 render_thread_count = GetRenderThreadCount();
    for (uint32 thread_index = 0; thread_index < render_thread_count; ++thread_index)
    {
        RenderCommandState* state = GetPtr(&job->states, thread_index);
        state->thread_index = thread_index;
        state->mesh         = meshes[thread_index % MESH_COUNT];
        state->batch_range  = GetBatchRange(thread_index, render_thread_count, entity_count);
        state->temp_stack   = GetPtr(&g_render_state.render_thread_temp_stacks, thread_index);

        Set(&job->tasks, thread_index, SubmitTask(thread_pool, state, RecordRenderCommandsThread));
    }

    // Wait for tasks to complete.
    CTK_ITER(task, &job->tasks)
    {
        Wait(thread_pool, *task);
    }
}

static RenderTarget* GetRenderTarget()
{
    return &g_render_state.render_target;
}

static BufferHnd GetEntityBuffer()
{
    return g_render_state.entity_buffer;
}
