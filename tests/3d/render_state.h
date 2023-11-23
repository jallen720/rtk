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
    MeshHnd    mesh;
    Stack*     temp_stack;
};

struct RenderState
{
    // Render Job
    Job<RenderCommandState> render_command_job;
    Array<Stack>            render_thread_temp_stacks;

    // Resources
    ResourceGroupHnd res_group;
    BufferHnd        staging_buffer;
    BufferHnd        entity_buffer;
    Array<ImageHnd>  textures;
    VkSampler        texture_sampler;
    MeshGroupHnd     mesh_group;
    Array<MeshHnd>   meshes;

    // Assets
    DescriptorSetHnd entity_descriptor_set;
    DescriptorSetHnd textures_descriptor_set;
    RenderTarget     render_target;
    Shader           vert_shader;
    Shader           frag_shader;
    VertexLayout     vertex_layout;
    Pipeline         pipeline;
};

struct Vertex
{
    Vec3<float32> position;
    Vec2<float32> uv;
};

static constexpr const char* TEXTURE_IMAGE_PATHS[] =
{
    "images/axis_cube.png",
    "images/dirt_block.png",
    "images/stone_tiles.png",
    "images/axis_cube_inv.png",
    "images/axis_cube.png",
    "images/stone_tiles.png",
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

static void CreateResources(Stack* perm_stack, Stack* temp_stack, FreeList* free_list)
{
    InitResourceModule(&perm_stack->allocator, { .max_resource_groups = 4 });

    ResourceGroupInfo res_group_info =
    {
        .max_buffers = 32,
        .max_images  = 32,
    };
    g_render_state.res_group = CreateResourceGroup(&perm_stack->allocator, &res_group_info);

    // Staging Buffer
    BufferInfo staging_buffer_info =
    {
        .size             = Megabyte32<4>(),
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame        = false,
        .mem_properties   = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };
    g_render_state.staging_buffer = CreateBuffer(g_render_state.res_group, &staging_buffer_info);

    // Entity Buffer
    BufferInfo entity_buffer_info =
    {
        .size             = sizeof(EntityBuffer),
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame        = true,
        .mem_properties   = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .usage            = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    };
    g_render_state.entity_buffer = CreateBuffer(g_render_state.res_group, &entity_buffer_info);

    // Textures
    ImageData texture_datas[TEXTURE_COUNT] = {};
    InitArray(&g_render_state.textures, &perm_stack->allocator, TEXTURE_COUNT);
    for (uint32 i = 0; i < TEXTURE_COUNT; ++i)
    {
        ImageData* texture_data = &texture_datas[i];
        LoadImageData(texture_data, TEXTURE_IMAGE_PATHS[i]);
        ImageInfo texture_info =
        {
            .extent =
            {
                .width  = (uint32)texture_data->width,
                .height = (uint32)texture_data->height,
                .depth  = 1
            },
            .per_frame      = false,
            .flags          = 0,
            .type           = VK_IMAGE_TYPE_2D,
            .format         = GetSwapchain()->surface_format.format,
            .mip_levels     = 1,
            .array_layers   = 1,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .tiling         = VK_IMAGE_TILING_OPTIMAL,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .mem_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .usage          = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        };
        ImageViewInfo texture_view_info =
        {
            .flags      = 0,
            .type       = VK_IMAGE_VIEW_TYPE_2D,
            .format     = texture_info.format,
            .components = RGBA_COMPONENT_SWIZZLE_IDENTITY,
            .subresource_range =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        Push(&g_render_state.textures, CreateImage(g_render_state.res_group, &texture_info, &texture_view_info));
    }

    // Meshes
    #include "meshes/quad.h"
    #include "meshes/cube.h"
    #include "meshes/cube_repeating.h"
    static constexpr uint32 MESH_COUNT = 3;
    MeshData mesh_datas[MESH_COUNT] = {};
    mesh_datas[0].vertex_buffer = (uint8*)quad_vertexes;
    mesh_datas[0].vertex_size   = sizeof(Vertex);
    mesh_datas[0].vertex_count  = CTK_ARRAY_SIZE(quad_vertexes);
    mesh_datas[0].index_buffer  = (uint8*)quad_indexes;
    mesh_datas[0].index_size    = sizeof(uint32);
    mesh_datas[0].index_count   = CTK_ARRAY_SIZE(quad_indexes);

    mesh_datas[1].vertex_buffer = (uint8*)cube_vertexes;
    mesh_datas[1].vertex_size   = sizeof(Vertex);
    mesh_datas[1].vertex_count  = CTK_ARRAY_SIZE(cube_vertexes);
    mesh_datas[1].index_buffer  = (uint8*)cube_indexes;
    mesh_datas[1].index_size    = sizeof(uint32);
    mesh_datas[1].index_count   = CTK_ARRAY_SIZE(cube_indexes);

    // mesh_datas[2].vertex_buffer.data  = (uint8*)cube_repeating_vertexes;
    // mesh_datas[2].vertex_buffer.size  = sizeof(cube_repeating_vertexes);
    // mesh_datas[2].vertex_buffer.count = mesh_datas[2].vertex_buffer.size;
    // mesh_datas[2].index_buffer        = CTK_WRAP_ARRAY(cube_repeating_indexes);
LoadMeshData(&mesh_datas[2], &free_list->allocator, "meshes/cube.gltf");

    InitArray(&g_render_state.meshes, &perm_stack->allocator, CTK_ARRAY_SIZE(mesh_datas));
    InitMeshModule(&perm_stack->allocator, { .max_mesh_groups = 1 });
    MeshGroupInfo mesh_group_info =
    {
        .max_meshes     = 16,
        .mem_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    g_render_state.mesh_group = CreateMeshGroup(&perm_stack->allocator, &mesh_group_info);
    CTK_ITER_PTR(mesh_data, mesh_datas, CTK_ARRAY_SIZE(mesh_datas))
    {
        MeshInfo mesh_info =
        {
            .vertex_size  = mesh_data->vertex_size,
            .vertex_count = mesh_data->vertex_count,
            .index_size   = mesh_data->index_size,
            .index_count  = mesh_data->index_count,
        };
        Push(&g_render_state.meshes, CreateMesh(g_render_state.mesh_group, &mesh_info));
    }
    InitMeshGroup(g_render_state.mesh_group, g_render_state.res_group);

    // Load data into assets resources.
    InitResourceGroup(g_render_state.res_group, temp_stack);

    // Textures
    for (uint32 i = 0; i < g_render_state.textures.count; ++i)
    {
        ImageData* texture_data = &texture_datas[i];
        LoadImage(Get(&g_render_state.textures, i), g_render_state.staging_buffer, 0, texture_data);
        DestroyImageData(texture_data);
    }

    // Meshes
    for (uint32 i = 0; i < g_render_state.meshes.count; ++i)
    {
        LoadDeviceMesh(Get(&g_render_state.meshes, i), g_render_state.staging_buffer, &mesh_datas[i]);
    }
}

static void CreateSampler()
{
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

static void CreateDescriptorSets(Stack* perm_stack, Stack* temp_stack)
{
    InitDescriptorSetModule(&perm_stack->allocator, 16);

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
                .type     = VK_DESCRIPTOR_TYPE_SAMPLER,
                .stages   = VK_SHADER_STAGE_FRAGMENT_BIT,
                .count    = 1,
                .samplers = &g_render_state.texture_sampler,
            },
        };
        g_render_state.textures_descriptor_set =
            CreateDescriptorSet(&perm_stack->allocator, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    InitDescriptorSets(temp_stack);
}

static void InitRenderTargets(Stack* perm_stack, Stack* temp_stack, FreeList* free_list)
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
    InitRenderTarget(&g_render_state.render_target, perm_stack, temp_stack, free_list, &info);
}

static void InitShaders(Stack* temp_stack)
{
    InitShader(&g_render_state.vert_shader, temp_stack, "shaders/bin/3d.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    InitShader(&g_render_state.frag_shader, temp_stack, "shaders/bin/3d.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
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

static void RecordRenderCommandsThread(void* data)
{
    auto state = (RenderCommandState*)data;
    VkCommandBuffer command_buffer = BeginRenderCommands(&g_render_state.render_target, state->thread_index);
        Pipeline* pipeline = &g_render_state.pipeline;
        DescriptorSetHnd descriptor_sets[] =
        {
            g_render_state.entity_descriptor_set,
            g_render_state.textures_descriptor_set,
        };
        BindDescriptorSets(command_buffer, pipeline, state->temp_stack, CTK_WRAP_ARRAY(descriptor_sets), 0);
        BindPipeline(command_buffer, pipeline);
        BindMeshGroup(command_buffer, g_render_state.mesh_group);
#if 1
        DrawMesh(command_buffer, state->mesh, state->batch_range.start, state->batch_range.size);
#else
        for (uint32 i = state->batch_range.start; i < state->batch_range.start + state->batch_range.size; ++i)
        {
            DrawMesh(command_buffer, state->mesh, i, 1);
        }
#endif
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
    InitRenderJob(perm_stack);

    // Resources
    CreateResources(perm_stack, temp_stack, free_list);

    // Assets
    CreateSampler();
    CreateDescriptorSets(perm_stack, temp_stack);
    InitRenderTargets(perm_stack, temp_stack, free_list);
    InitShaders(temp_stack);
    InitVertexLayout(perm_stack);
    InitPipelines(temp_stack, free_list);

LogResourceGroups();
}

static void RecordRenderCommands(ThreadPool* thread_pool, uint32 entity_count)
{
    Job<RenderCommandState>* job = &g_render_state.render_command_job;

    uint32 render_thread_count = GetRenderThreadCount();
    for (uint32 thread_index = 0; thread_index < render_thread_count; ++thread_index)
    {
        RenderCommandState* state = GetPtr(&job->states, thread_index);
        state->thread_index = thread_index;
        state->mesh         = Get(&g_render_state.meshes, thread_index % g_render_state.meshes.count);
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
