/// Data
////////////////////////////////////////////////////////////
template<typename StateType>
struct Job
{
    Array<StateType> states;
    Array<TaskHnd>   tasks;
};

struct Transform
{
    Vec3<float32> position;
    Vec3<float32> rotation;
};

struct View
{
    Vec3<float32> position;
    Vec3<float32> rotation;
    float32       vertical_fov;
    float32       z_near;
    float32       z_far;
    float32       max_x_angle;
};

struct EntityBuffer
{
    Matrix mvp_matrixes   [MAX_ENTITIES];
    uint32 texture_indexes[MAX_ENTITIES];
    uint32 sampler_indexes[MAX_ENTITIES];
};

struct MVPMatrixState
{
    BatchRange    batch_range;
    Matrix        view_projection_matrix;
    EntityBuffer* frame_entity_buffer;
    Transform*    transforms;
};

struct RenderCommandState
{
    BatchRange batch_range;
    uint32     thread_index;
    MeshHnd    mesh;
    Stack*     temp_stack;
};

struct RenderState
{
    // Render Job
    Job<RenderCommandState> render_command_job;
    Job<MVPMatrixState>     mvp_matrix_job;
    Array<Stack>            render_thread_temp_stacks;

    // Resources
    ResourceGroupHnd res_group;
    BufferHnd        host_buffer;
    BufferHnd        device_buffer;
    ImageMemoryHnd   image_mem;
    BufferHnd        staging_buffer;
    BufferHnd        entity_buffer;
    Array<ImageHnd>  textures;
    Array<VkSampler> samplers;
    MeshGroupHnd     mesh_group;
    Array<MeshHnd>   meshes;

    // Assets
    DescriptorSetHnd entity_descriptor_set;
    DescriptorSetHnd textures_descriptor_set;
    RenderTarget     render_target;
    VkShaderModule   vert_shader;
    VkShaderModule   frag_shader;
    VertexLayout     vertex_layout;
    Pipeline         pipeline;
};

static constexpr const char* TEXTURE_IMAGE_PATHS[] =
{
    "images/axis_cube.png",
    "images/axis_cube_inv.png",
    "images/icosphere_triangle.png",
    "images/dirt_block.png",
    "images/stone_tiles.png",
    "images/icosphere_triangle.png",
};
static constexpr uint32 TEXTURE_COUNT = CTK_ARRAY_SIZE(TEXTURE_IMAGE_PATHS);
static_assert(TEXTURE_COUNT == MAX_TEXTURES);

/// Instance
////////////////////////////////////////////////////////////
static RenderState g_render_state;

/// Utils
////////////////////////////////////////////////////////////
template<typename StateType>
static void InitThreadPoolJob(Job<StateType>* job, const Allocator* allocator, uint32 thread_count)
{
    InitArrayFull(&job->states, allocator, thread_count);
    InitArrayFull(&job->tasks,  allocator, thread_count);
}

static void InitRenderCommandJob(Stack* perm_stack)
{
    uint32 render_thread_count = GetRenderThreadCount();
    InitThreadPoolJob(&g_render_state.render_command_job, &perm_stack->allocator, render_thread_count);

    // Initialize temp stacks for render_command_job threads.
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
        .max_buffers    = 8,
        .max_image_mems = 4,
        .max_images     = 8,
    };
    g_render_state.res_group = CreateResourceGroup(&perm_stack->allocator, &res_group_info);

    BufferInfo host_buffer_info =
    {
        .size       = Megabyte32<16>(),
        .alignment  = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame  = false,
        .flags      = 0,
        .usage      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    g_render_state.host_buffer = DefineBuffer(g_render_state.res_group, &host_buffer_info);
    BufferInfo device_buffer_info =
    {
        .size       = Megabyte32<1>(),
        .alignment  = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame  = false,
        .flags      = 0,
        .usage      = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    g_render_state.device_buffer = DefineBuffer(g_render_state.res_group, &device_buffer_info);
    ImageMemoryInfo image_mem_info =
    {
        .size       = Megabyte32<8>(),
        .flags      = 0,
        .usage      = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .format     = GetSwapchain()->surface_format.format,
        .tiling     = VK_IMAGE_TILING_OPTIMAL,
    };
    g_render_state.image_mem = DefineImageMemory(g_render_state.res_group, &image_mem_info);

    AllocateResourceGroup(g_render_state.res_group);

    // Staging Buffer
    BufferInfo staging_buffer_info =
    {
        .size      = Megabyte32<8>(),
        .alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame = false,
    };
    g_render_state.staging_buffer = CreateBuffer(g_render_state.host_buffer, &staging_buffer_info);

    // Entity Buffer
    BufferInfo entity_buffer_info =
    {
        .size      = sizeof(EntityBuffer),
        .alignment = USE_MIN_OFFSET_ALIGNMENT,
        .per_frame = true,
    };
    g_render_state.entity_buffer = CreateBuffer(g_render_state.host_buffer, &entity_buffer_info);

    // Textures
    InitArray(&g_render_state.textures, &perm_stack->allocator, TEXTURE_COUNT);
    for (uint32 i = 0; i < TEXTURE_COUNT; ++i)
    {
        ImageData texture_data = {};
        LoadImageData(&texture_data, TEXTURE_IMAGE_PATHS[i]);
        ImageInfo texture_info =
        {
            .extent =
            {
                .width  = (uint32)texture_data.width,
                .height = (uint32)texture_data.height,
                .depth  = 1
            },
            .type           = VK_IMAGE_TYPE_2D,
            .mip_levels     = 1,
            .array_layers   = 1,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .per_frame      = false,
        };
        ImageViewInfo texture_view_info =
        {
            .flags      = 0,
            .type       = VK_IMAGE_VIEW_TYPE_2D,
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
        ImageHnd texture = CreateImage(g_render_state.image_mem, &texture_info, &texture_view_info);
        Push(&g_render_state.textures, texture);
        LoadImage(texture, g_render_state.staging_buffer, 0, &texture_data);
        DestroyImageData(&texture_data);
    }

    // Meshes
    static constexpr const char* MESH_PATHS[] =
    {
        "blender/cube.gltf",
        "blender/quad.gltf",
        "blender/icosphere.gltf",
    };
    static constexpr uint32 MESH_COUNT = CTK_ARRAY_SIZE(MESH_PATHS);

    InitMeshModule(&perm_stack->allocator, { .max_mesh_groups = 1 });

    MeshGroupInfo mesh_group_info =
    {
        .max_meshes         = MESH_COUNT,
        .vertex_buffer_size = Kilobyte32<8>(),
        .index_buffer_size  = Kilobyte32<8>(),
    };
    g_render_state.mesh_group = CreateMeshGroup(&perm_stack->allocator, g_render_state.device_buffer,
                                                &mesh_group_info);

    Swizzle position_swizzle = { 0, 2, 1 };
    AttributeSwizzles attribute_swizzles = { .POSITION = &position_swizzle };
    InitArray(&g_render_state.meshes, &perm_stack->allocator, MESH_COUNT);
    CTK_ITER_PTR(mesh_path, MESH_PATHS, MESH_COUNT)
    {
        MeshData mesh_data = {};
        LoadMeshData(&mesh_data, &free_list->allocator, *mesh_path, &attribute_swizzles);
        MeshHnd mesh = CreateMesh(g_render_state.mesh_group, &mesh_data.info);
        Push(&g_render_state.meshes, mesh);
        LoadDeviceMesh(mesh, g_render_state.staging_buffer, &mesh_data);
        DestroyMeshData(&mesh_data, &free_list->allocator);
    }
}

static void CreateSamplers(Stack* perm_stack)
{
    VkResult res = VK_SUCCESS;
    VkDevice device = GetDevice();
    VkSamplerCreateInfo sampler_infos[MAX_SAMPLERS] =
    {
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
        },
        {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext                   = NULL,
            .flags                   = 0,
            .magFilter               = VK_FILTER_LINEAR,
            .minFilter               = VK_FILTER_LINEAR,
            .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
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
        },
    };
    InitArray(&g_render_state.samplers, &perm_stack->allocator, MAX_SAMPLERS);
    for (uint32 i = 0; i < MAX_SAMPLERS; ++i)
    {
        res = vkCreateSampler(device, &sampler_infos[i], NULL, Push(&g_render_state.samplers));
        Validate(res, "vkCreateSampler() failed");
    }
}

static void CreateDescriptorSets(Stack* perm_stack, Stack* temp_stack)
{
    InitDescriptorSetModule(&perm_stack->allocator, { .max_descriptor_sets = 16 });

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
                .count    = g_render_state.samplers.count,
                .samplers = g_render_state.samplers.data,
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
    g_render_state.vert_shader = LoadShaderModule(temp_stack, "shaders/bin/3d.vert.spv");
    g_render_state.frag_shader = LoadShaderModule(temp_stack, "shaders/bin/3d.frag.spv");
}

static void InitVertexLayout(Stack* perm_stack)
{
    AttributeInfo attribute_infos[] =
    {
        { .component_count = 3, .type = AttributeType::FLOAT32 }, // Position
        { .component_count = 2, .type = AttributeType::FLOAT32 }, // UV
    };
    BindingInfo binding_infos[]
    {
        {
            .input_rate      = VK_VERTEX_INPUT_RATE_VERTEX,
            .attribute_infos = CTK_WRAP_ARRAY(attribute_infos),
        },
    };
    InitVertexLayout(&g_render_state.vertex_layout, &perm_stack->allocator, CTK_WRAP_ARRAY(binding_infos));
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
            .maxDepth = 1,
        },
    };
    Shader shaders[] =
    {
        {
            .module = g_render_state.vert_shader,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        },
        {
            .module = g_render_state.frag_shader,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
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

static Matrix GetViewProjectionMatrix(View* view)
{
    // View Matrix
    Matrix view_model_matrix = ID_MATRIX;
    view_model_matrix = RotateX(view_model_matrix, view->rotation.x);
    view_model_matrix = RotateY(view_model_matrix, view->rotation.y);
    view_model_matrix = RotateZ(view_model_matrix, view->rotation.z);
    Vec3<float32> forward =
    {
        .x = Get(&view_model_matrix, 0, 2),
        .y = Get(&view_model_matrix, 1, 2),
        .z = Get(&view_model_matrix, 2, 2),
    };
    static constexpr Vec3<float32> UP = { 0.0f, -1.0f, 0.0f }; // Vulkan has -Y as up.
    Matrix view_matrix = LookAt(view->position, view->position + forward, UP);

    // Projection Matrix
    VkExtent2D swapchain_extent = GetSwapchain()->surface_extent;
    float32 swapchain_aspect_ratio = (float32)swapchain_extent.width / swapchain_extent.height;
    Matrix projection_matrix = GetPerspectiveMatrix(view->vertical_fov, swapchain_aspect_ratio,
                                                    view->z_near, view->z_far);

    return projection_matrix * view_matrix;
}

static void UpdateMVPMatrixesThread(void* data)
{
    auto state = (MVPMatrixState*)data;
    BatchRange batch_range = state->batch_range;

    // Update entity MVP matrixes.
    for (uint32 i = batch_range.start; i < batch_range.start + batch_range.size; ++i)
    {
        Transform* entity_transform = &state->transforms[i];
        Matrix model_matrix = ID_MATRIX;
        model_matrix = Translate(model_matrix, entity_transform->position);
        model_matrix = RotateX  (model_matrix, entity_transform->rotation.x);
        model_matrix = RotateY  (model_matrix, entity_transform->rotation.y);
        model_matrix = RotateZ  (model_matrix, entity_transform->rotation.z);
        // model_matrix = Scale    (model_matrix, entity_transform->scale);

        state->frame_entity_buffer->mvp_matrixes[i] = state->view_projection_matrix * model_matrix;
    }
}

static void RecreateSwapchain(Stack* temp_stack, FreeList* free_list)
{
    WaitIdle();

    UpdateSwapchainSurfaceExtent(temp_stack, free_list);

    VkExtent2D swapchain_extent = GetSwapchain()->surface_extent;
    VkViewport viewport =
    {
        .x        = 0,
        .y        = 0,
        .width    = (float32)swapchain_extent.width,
        .height   = (float32)swapchain_extent.height,
        .minDepth = 0,
        .maxDepth = 1,
    };
    UpdatePipelineViewports(&g_render_state.pipeline, free_list, CTK_WRAP_ARRAY_1(&viewport));

    UpdateRenderTargetAttachments(&g_render_state.render_target, temp_stack, free_list);
}

/// Interface
////////////////////////////////////////////////////////////
static void InitRenderState(Stack* perm_stack, Stack* temp_stack, FreeList* free_list,
                            uint32 mvp_matrix_job_thread_count)
{
    InitRenderCommandJob(perm_stack);
    InitThreadPoolJob(&g_render_state.mvp_matrix_job, &perm_stack->allocator, mvp_matrix_job_thread_count);

    // Resources
    CreateResources(perm_stack, temp_stack, free_list);

    // Assets
    CreateSamplers(perm_stack);
    CreateDescriptorSets(perm_stack, temp_stack);
    InitRenderTargets(perm_stack, temp_stack, free_list);
    InitShaders(temp_stack);
    InitVertexLayout(perm_stack);
    InitPipelines(temp_stack, free_list);
}

static void SetTextureIndexes(uint32* texture_indexes, uint32 entity_count, uint32 frame_index)
{
    CTK_ASSERT(entity_count <= MAX_ENTITIES);
    CTK_ASSERT(frame_index < GetFrameCount());
    EntityBuffer* frame_entity_buffer = GetMappedMemory<EntityBuffer>(g_render_state.entity_buffer, frame_index);
    memcpy(frame_entity_buffer->texture_indexes, texture_indexes, sizeof(uint32) * entity_count);
}

static void SetSamplerIndexes(uint32* sampler_indexes, uint32 entity_count, uint32 frame_index)
{
    CTK_ASSERT(entity_count <= MAX_ENTITIES);
    CTK_ASSERT(frame_index < GetFrameCount());
    EntityBuffer* frame_entity_buffer = GetMappedMemory<EntityBuffer>(g_render_state.entity_buffer, frame_index);
    memcpy(frame_entity_buffer->sampler_indexes, sampler_indexes, sizeof(uint32) * entity_count);
}

static void UpdateMVPMatrixes(ThreadPool* thread_pool, View* view, Transform* transforms, uint32 entity_count)
{
    Job<MVPMatrixState>* job = &g_render_state.mvp_matrix_job;
    Matrix view_projection_matrix = GetViewProjectionMatrix(view);
    auto frame_entity_buffer = GetMappedMemory<EntityBuffer>(g_render_state.entity_buffer, GetFrameIndex());

    // Initialize thread states and submit tasks.
    uint32 thread_count = job->states.count;
    for (uint32 thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        MVPMatrixState* state = GetPtr(&job->states, thread_index);
        state->batch_range            = GetBatchRange(thread_index, thread_count, entity_count);
        state->view_projection_matrix = view_projection_matrix;
        state->frame_entity_buffer    = frame_entity_buffer;
        state->transforms             = transforms;

        UpdateMVPMatrixesThread((void*)state);
        // Set(&job->tasks, thread_index, SubmitTask(thread_pool, state, UpdateMVPMatrixesThread));
    }

    // // Wait for tasks to complete.
    // CTK_ITER(task, &job->tasks)
    // {
    //     Wait(thread_pool, *task);
    // }
}

static void RecordRenderCommands(ThreadPool* thread_pool, uint32 entity_count)
{
    Job<RenderCommandState>* job = &g_render_state.render_command_job;

    uint32 thread_count = job->states.count;
    for (uint32 thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        RenderCommandState* state = GetPtr(&job->states, thread_index);
        state->batch_range  = GetBatchRange(thread_index, thread_count, entity_count);
        state->thread_index = thread_index;
        state->mesh         = Get(&g_render_state.meshes, thread_index % g_render_state.meshes.count);
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
