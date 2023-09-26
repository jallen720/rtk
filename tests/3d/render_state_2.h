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
};

struct RenderState
{
    Job<RenderCommandState> render_command_job;
    uint32 entity_count;

    // Device Memory
    BufferStack device_stack;
    BufferStack host_stack;
    Buffer      staging_buffer;

    // Rendering State
    RenderTarget render_target;
    Shader       vert_shader;
    Shader       frag_shader;
    MeshData     mesh_data;
    Mesh         cube_mesh;
    Mesh         quad_mesh;

    // Descriptor Datas
    Buffer          entity_buffer;
    Array<ImageHnd> textures;
    VkSampler       texture_sampler;

    // Descriptor Sets/Pipelines
    DescriptorSet entity_descriptor_set;
    DescriptorSet textures_descriptor_set;
    VertexLayout  vertex_layout;
    Pipeline      pipeline;
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
static RenderState render_state;

/// Utils
////////////////////////////////////////////////////////////
static void InitDeviceMemory()
{
    BufferStackInfo host_stack_info =
    {
        .size               = Megabyte32<64>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    InitBufferStack(&render_state.host_stack, &host_stack_info);
    BufferStackInfo device_stack_info =
    {
        .size               = Megabyte32<64>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    InitBufferStack(&render_state.device_stack, &device_stack_info);
    BufferInfo staging_buffer_info =
    {
        .type             = BufferType::BUFFER,
        .size             = Megabyte32<4>(),
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .instance_count   = 1,
    };
    InitBuffer(&render_state.staging_buffer, &render_state.host_stack, &staging_buffer_info);
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
    InitRenderTarget(&render_state.render_target, temp_stack, free_list, &info);
}

static void InitShaders(Stack* temp_stack)
{
    InitShader(&render_state.vert_shader, temp_stack, "shaders/bin/3d_2.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    InitShader(&render_state.frag_shader, temp_stack, "shaders/bin/3d_2.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

static void InitMeshes()
{
    MeshDataInfo mesh_data_info =
    {
        .vertex_buffer_size = Megabyte32<1>(),
        .index_buffer_size  = Megabyte32<1>(),
    };
    InitMeshData(&render_state.mesh_data, &render_state.device_stack, &mesh_data_info);

    {
        #include "rtk/meshes/cube.h"
        InitDeviceMesh(&render_state.cube_mesh, &render_state.mesh_data,
                       CTK_WRAP_ARRAY(vertexes), CTK_WRAP_ARRAY(indexes),
                       &render_state.staging_buffer);
    }

    {
        #include "rtk/meshes/quad.h"
        InitDeviceMesh(&render_state.quad_mesh, &render_state.mesh_data,
                       CTK_WRAP_ARRAY(vertexes), CTK_WRAP_ARRAY(indexes),
                       &render_state.staging_buffer);
    }
}

static void InitDescriptorDatas(Stack* perm_stack)
{
    // Entity Buffer
    BufferInfo entity_buffer_info =
    {
        .type             = BufferType::UNIFORM,
        .size             = sizeof(EntityBuffer),
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .instance_count   = GetFrameCount(),
    };
    InitBuffer(&render_state.entity_buffer, &render_state.host_stack, &entity_buffer_info);

    // Textures
    InitImageState(&perm_stack->allocator, 16);
    InitArray(&render_state.textures, &perm_stack->allocator, TEXTURE_COUNT);
    VkImageCreateInfo texture_create_info =
    {
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext     = NULL,
        .flags     = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format    = GetSwapchain()->surface_format.format,
        .extent =
        {
            .width  = 64,
            .height = 32,
            .depth  = 1
        },
        .mipLevels             = 1,
        .arrayLayers           = 1,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                 VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = NULL,
        .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    for (uint32 i = 0; i < TEXTURE_COUNT; ++i)
    {
        Push(&render_state.textures, CreateImage(&texture_create_info));
    }
    BackImagesWithMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    for (uint32 i = 0; i < TEXTURE_COUNT; ++i)
    {
        LoadImage(Get(&render_state.textures, i), &render_state.staging_buffer, 0, TEXTURE_IMAGE_PATHS[i]);
    }

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
    render_state.texture_sampler = CreateSampler(&texture_sampler_info);
}

static void InitDescriptorSets(Stack* perm_stack, Stack* temp_stack)
{
    // Entity
    {
        DescriptorData datas[] =
        {
            {
                .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stages     = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .count      = 1,
                .buffers    = &render_state.entity_buffer,
            },
        };
        InitDescriptorSet(&render_state.entity_descriptor_set, perm_stack, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    // Textures
    {
        DescriptorData datas[] =
        {
            {
                .type       = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .stages     = VK_SHADER_STAGE_FRAGMENT_BIT,
                .count      = render_state.textures.count,
                .image_hnds = render_state.textures.data,
            },
            {
                .type       = VK_DESCRIPTOR_TYPE_SAMPLER,
                .stages     = VK_SHADER_STAGE_FRAGMENT_BIT,
                .count      = 1,
                .samplers   = &render_state.texture_sampler,
            },
        };
        InitDescriptorSet(&render_state.textures_descriptor_set, perm_stack, temp_stack, CTK_WRAP_ARRAY(datas));
    }
}

static void InitVertexLayout(Stack* perm_stack)
{
    VertexLayout* vertex_layout = &render_state.vertex_layout;

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
        render_state.entity_descriptor_set.layout,
        render_state.textures_descriptor_set.layout,
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
        &render_state.vert_shader,
        &render_state.frag_shader,
    };
    PipelineInfo pipeline_info =
    {
        .shaders       = CTK_WRAP_ARRAY(shaders),
        .viewports     = CTK_WRAP_ARRAY(viewports),
        .vertex_layout = &render_state.vertex_layout,
        .depth_testing = true,
        .render_target = &render_state.render_target,
    };

    InitPipeline(&render_state.pipeline, temp_stack, free_list, &pipeline_info, &pipeline_layout_info);
}

static void RecordRenderCommandsThread(void* data)
{
    auto state = (RenderCommandState*)data;
    VkCommandBuffer command_buffer = BeginRenderCommands(&render_state.render_target, state->thread_index);
        Pipeline* pipeline = &render_state.pipeline;
        BindPipeline(command_buffer, pipeline);
        DescriptorSet* descriptor_sets[] =
        {
            &render_state.entity_descriptor_set,
            &render_state.textures_descriptor_set,
        };
        BindDescriptorSets(command_buffer, pipeline, CTK_WRAP_ARRAY(descriptor_sets), 0);
        BindMeshData(command_buffer, &render_state.mesh_data);
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
            .maxDepth = 1
        },
    };
    UpdatePipelineViewports(&render_state.pipeline, free_list, CTK_WRAP_ARRAY(viewports));
}

static void UpdateAllRenderTargetAttachments(Stack* temp_stack, FreeList* free_list)
{
    UpdateRenderTargetAttachments(&render_state.render_target, temp_stack, free_list);
}

/// Interface
////////////////////////////////////////////////////////////
static void InitRenderState(Stack* perm_stack, Stack* temp_stack, FreeList* free_list, uint32 job_task_count)
{
    InitThreadPoolJob(&render_state.render_command_job, perm_stack, job_task_count);

    InitDeviceMemory();

    InitRenderTargets(temp_stack, free_list);
    InitShaders(temp_stack);
    InitMeshes();

    InitDescriptorDatas(perm_stack);
    InitDescriptorSets(perm_stack, temp_stack);
    InitVertexLayout(perm_stack);
    InitPipelines(temp_stack, free_list);
}

static void RecordRenderCommands(ThreadPool* thread_pool)
{
    Job<RenderCommandState>* job = &render_state.render_command_job;

    static Mesh* meshes[] =
    {
        &render_state.quad_mesh,
        &render_state.cube_mesh,
    };
    static constexpr uint32 TEXTURE_COUNT = 3;
    static constexpr uint32 MESH_COUNT    = CTK_ARRAY_SIZE(meshes);
    static constexpr uint32 THREAD_COUNT  = TEXTURE_COUNT * MESH_COUNT;
    CTK_ASSERT(THREAD_COUNT <= JOB_TASK_COUNT);

    for (uint32 texture_index = 0; texture_index < TEXTURE_COUNT; ++texture_index)
    {
        for (uint32 mesh_index = 0; mesh_index < MESH_COUNT; ++mesh_index)
        {
            uint32 thread_index = (texture_index * MESH_COUNT) + mesh_index;
            RenderCommandState* state = GetPtr(&job->states, thread_index);
            state->thread_index = thread_index;
            state->mesh         = meshes[mesh_index];
            state->batch_range  = GetBatchRange(thread_index, THREAD_COUNT, render_state.entity_count);

            Set(&job->tasks, thread_index, SubmitTask(thread_pool, state, RecordRenderCommandsThread));
        }
    }

    // Wait for tasks to complete.
    for (uint32 i = 0; i < THREAD_COUNT; ++i)
    {
        Wait(thread_pool, Get(&job->tasks, i));
    }
}

static RenderTarget* GetRenderTarget()
{
    return &render_state.render_target;
}

static Buffer* GetEntityBuffer()
{
    return &render_state.entity_buffer;
}

static void SetEntityCount(uint32 entity_count)
{
    CTK_ASSERT(entity_count <= MAX_ENTITIES);
    render_state.entity_count = entity_count;
}
