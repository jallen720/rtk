#include <windows.h>
#include "ctk3/ctk3.h"
#include "ctk3/memory.h"
#include "ctk3/free_list.h"
#include "ctk3/thread_pool.h"
#include "ctk3/math.h"
#include "ctk3/profile.h"
#include "ctk3/window.h"

// #define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

#include "rtk/tests/shared.h"
#include "rtk/tests/defs_3d.h"

using namespace CTK;
using namespace RTK;

/// Data
////////////////////////////////////////////////////////////
struct View
{
    Vec3<float32> position;
    Vec3<float32> rotation;
    float32       vertical_fov;
    float32       aspect;
    float32       z_near;
    float32       z_far;
    float32       max_x_angle;
};

struct Transform
{
    Vec3<float32> position;
    Vec3<float32> rotation;
};

struct EntityData
{
    Transform transforms[MAX_ENTITIES];
    uint32    count;
};

struct Game
{
    Mouse      mouse;
    View       view;
    EntityData entity_data;
};

struct VSBuffer
{
    Matrix mvp_matrixes[MAX_ENTITIES];
};

struct MVPMatrixState
{
    BatchRange  batch_range;
    Matrix      view_projection_matrix;
    VSBuffer*   frame_vs_buffer;
    EntityData* entity_data;
};

struct RenderState;

struct RenderCommandState
{
    RenderState*     render_state;
    uint32           thread_index;
    ShaderDataSetHnd texture;
    BatchRange       batch_range;
    MeshHnd          mesh;
};

template<typename StateType>
struct ThreadPoolJob
{
    Array<StateType> states;
    Array<TaskHnd>   tasks;
};

struct RenderState
{
    VertexLayout vertex_layout;
    VkSampler    sampler;

    struct
    {
        ThreadPoolJob<MVPMatrixState>     update_mvp_matrixes;
        ThreadPoolJob<RenderCommandState> record_render_commands;
    }
    thread_pool_job;

    // Resources
    struct
    {
        BufferHnd host;
        BufferHnd device;
        BufferHnd staging;
    }
    buffer;
    struct
    {
        RenderTargetHnd main;
    }
    render_target;
    struct
    {
        ShaderDataHnd vs_buffer;
        ShaderDataHnd axis_cube_texture;
        ShaderDataHnd dirt_block_texture;
    }
    data;
    struct
    {
        ShaderDataSetHnd entity_data;
        ShaderDataSetHnd axis_cube_texture;
        ShaderDataSetHnd dirt_block_texture;
    }
    data_set;
    struct
    {
        PipelineHnd main;
    }
    pipeline;
    struct
    {
        MeshDataHnd data;
        MeshHnd     cube;
        MeshHnd     quad;
    }
    mesh;
};

struct Vertex
{
    Vec3<float32> position;
    Vec2<float32> uv;
};

static constexpr uint32 RENDER_THREAD_COUNT = 4;

/// Game
////////////////////////////////////////////////////////////
static uint32 PushEntity(EntityData* entity_data)
{
    if (entity_data->count == MAX_ENTITIES)
    {
        CTK_FATAL("can't push another entity: entity count (%u) at max (%u)", entity_data->count, MAX_ENTITIES);
    }

    uint32 new_entity = entity_data->count;
    ++entity_data->count;
    return new_entity;
}

static Game* CreateGame(Stack* perm_stack)
{
    auto game = Allocate<Game>(perm_stack, 1);

    // View
    game->view =
    {
        .position     = { 0, 0, -1 },
        .rotation     = { 0, 0, 0 },
        .vertical_fov = 90.0f,
        .aspect       = 16.0f / 9.0f,
        .z_near       = 0.1f,
        .z_far        = 1000.0f,
        .max_x_angle  = 85.0f,
    };

    // Entities
    static constexpr uint32 CUBE_SIZE = 64;
    static constexpr uint32 CUBE_ENTITY_COUNT = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE;
    static_assert(CUBE_ENTITY_COUNT <= MAX_ENTITIES);
    for (uint32 x = 0; x < CUBE_SIZE; ++x)
    for (uint32 y = 0; y < CUBE_SIZE; ++y)
    for (uint32 z = 0; z < CUBE_SIZE; ++z)
    {
        uint32 entity = PushEntity(&game->entity_data);
        game->entity_data.transforms[entity] =
        {
            .position = { x * 1.5f, y * 1.5f, z * 1.5f },
            .rotation = { 0, 0, 0 },
        };
    }

    return game;
}

static void ValidateIndex(EntityData* entity_data, uint32 index)
{
    if (index >= entity_data->count)
    {
        CTK_FATAL("can't access entity data at index %u: index exceeds highest index %u", index,
                  entity_data->count - 1);
    }
}

static Transform* GetTransformPtr(EntityData* entity_data, uint32 index)
{
    ValidateIndex(entity_data, index);
    return entity_data->transforms + index;
}

/// Render State
////////////////////////////////////////////////////////////
static void InitBuffers(RenderState* render_state)
{
    BufferInfo host_buffer_info =
    {
        .size               = Megabyte32<128>(),
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    render_state->buffer.host = CreateBuffer(&host_buffer_info);

    BufferInfo device_buffer_info =
    {
        .size               = Megabyte32<256>(),
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    render_state->buffer.device = CreateBuffer(&device_buffer_info);

    render_state->buffer.staging = CreateBuffer(render_state->buffer.host, Megabyte32<16>());
}

static void InitRenderTargets(RenderState* render_state, Stack* perm_stack, Stack temp_stack)
{
    RenderTargetInfo info =
    {
        .depth_testing          = true,
        .color_attachment_count = 1,
    };
    render_state->render_target.main = CreateRenderTarget(perm_stack, temp_stack, &info);
    PushClearValue(render_state->render_target.main, { 0.0f, 0.1f, 0.2f, 1.0f });
    PushClearValue(render_state->render_target.main, { 1.0f });
}

static void InitVertexLayout(RenderState* render_state, Stack* perm_stack)
{
    VertexLayout* vertex_layout = &render_state->vertex_layout;

    // Init pipeline vertex layout.
    InitArray(&vertex_layout->bindings, perm_stack, 1);
    PushBinding(vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);

    InitArray(&vertex_layout->attributes, perm_stack, 4);
    PushAttribute(vertex_layout, 3, ATTRIBUTE_TYPE_FLOAT32); // Position
    PushAttribute(vertex_layout, 2, ATTRIBUTE_TYPE_FLOAT32); // UV
}

static void InitSampler(RenderState* render_state)
{
    VkSamplerCreateInfo info =
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
    render_state->sampler = CreateSampler(&info);
}

static ShaderDataInfo DefaultTextureInfo(VkFormat format, VkSampler sampler, ImageData* image_data)
{
    return
    {
        .stages    = VK_SHADER_STAGE_FRAGMENT_BIT,
        .type      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .per_frame = false,
        .image =
        {
            .info =
            {
                .image =
                {
                    .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    .pNext     = NULL,
                    .flags     = 0,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format    = format,
                    .extent =
                    {
                        .width  = (uint32)image_data->width,
                        .height = (uint32)image_data->height,
                        .depth  = 1
                    },
                    .mipLevels             = 1,
                    .arrayLayers           = 1,
                    .samples               = VK_SAMPLE_COUNT_1_BIT,
                    .tiling                = VK_IMAGE_TILING_OPTIMAL,
                    .usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = 0,
                    .pQueueFamilyIndices   = NULL,
                    .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
                },
                .view =
                {
                    .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext    = NULL,
                    .flags    = 0,
                    .image    = VK_NULL_HANDLE,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format   = format,
                    .components =
                    {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    .subresourceRange =
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
                .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            },
            .sampler = sampler,
        },
    };
}

static ShaderDataHnd CreateTexture(const char* image_path, RenderState* render_state, Stack* perm_stack)
{
    ImageData image_data = {};
    LoadImageData(&image_data, image_path);

    ShaderDataInfo info = DefaultTextureInfo(GetSwapchain()->image_format, render_state->sampler, &image_data);
    ShaderDataHnd shader_data_hnd = CreateShaderData(perm_stack, &info);

    // Copy image data into staging buffer.
    Clear(render_state->buffer.staging);
    Write(render_state->buffer.staging, image_data.data, image_data.size);
    uint32 image_count = GetShaderData(shader_data_hnd)->image_hnds.count;
    for (uint32 i = 0; i < image_count; ++i)
    {
        WriteToShaderDataImage(shader_data_hnd, i, render_state->buffer.staging);
    }

    DestroyImageData(&image_data);

    return shader_data_hnd;
}

static void InitShaderDatas(RenderState* render_state, Stack* perm_stack)
{
    // vs_buffer
    {
        ShaderDataInfo info =
        {
            .stages    = VK_SHADER_STAGE_VERTEX_BIT,
            .type      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .per_frame = true,
            .buffer =
            {
                .parent = render_state->buffer.host,
                .size   = sizeof(VSBuffer),
            }
        };
        render_state->data.vs_buffer = CreateShaderData(perm_stack, &info);
    }

    render_state->data.axis_cube_texture  = CreateTexture("images/axis_cube.png", render_state, perm_stack);
    render_state->data.dirt_block_texture = CreateTexture("images/dirt_block.png", render_state, perm_stack);
}

static void InitShaderDataSets(RenderState* render_state, Stack* perm_stack, Stack temp_stack)
{
    // data_set.vs_buffer
    {
        ShaderDataHnd datas[] =
        {
            render_state->data.vs_buffer,
        };
        render_state->data_set.entity_data = CreateShaderDataSet(perm_stack, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    // data_set.axis_cube_texture
    {
        ShaderDataHnd datas[] =
        {
            render_state->data.axis_cube_texture,
        };
        render_state->data_set.axis_cube_texture = CreateShaderDataSet(perm_stack, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    // data_set.dirt_block_texture
    {
        ShaderDataHnd datas[] =
        {
            render_state->data.dirt_block_texture,
        };
        render_state->data_set.dirt_block_texture = CreateShaderDataSet(perm_stack, temp_stack, CTK_WRAP_ARRAY(datas));
    }
}

static void InitPipelines(RenderState* render_state, Stack temp_stack)
{
    // Pipeline info arrays.
    Shader shaders[] =
    {
        {
            .module = LoadShaderModule(temp_stack, "shaders/bin/3d.vert.spv"),
            .stage  = VK_SHADER_STAGE_VERTEX_BIT
        },
        {
            .module = LoadShaderModule(temp_stack, "shaders/bin/3d.frag.spv"),
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT
        },
    };
    ShaderDataSetHnd shader_data_sets[] =
    {
        render_state->data_set.entity_data,
        render_state->data_set.axis_cube_texture,
    };
    // VkPushConstantRange push_constant_ranges[] =
    // {
    //     {
    //         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    //         .offset     = 0,
    //         .size       = sizeof(uint32)
    //     },
    // };

    PipelineInfo pipeline_info =
    {
        .vertex_layout        = &render_state->vertex_layout,
        .shaders              = CTK_WRAP_ARRAY(shaders),
        .shader_data_sets     = CTK_WRAP_ARRAY(shader_data_sets),
        // .push_constant_ranges = CTK_WRAP_ARRAY(push_constant_ranges),
    };
    render_state->pipeline.main = CreatePipeline(temp_stack, render_state->render_target.main, &pipeline_info);
}

static void InitMeshes(RenderState* render_state)
{
    MeshDataInfo mesh_data_info =
    {
        .parent_buffer      = render_state->buffer.host,
        .vertex_buffer_size = Megabyte32<1>(),
        .index_buffer_size  = Megabyte32<1>(),
    };
    render_state->mesh.data = CreateMeshData(&mesh_data_info);

    {
        #include "rtk/meshes/cube.h"
        render_state->mesh.cube = CreateMesh(render_state->mesh.data,
                                             CTK_WRAP_ARRAY(vertexes),
                                             CTK_WRAP_ARRAY(indexes));
    }

    {
        #include "rtk/meshes/quad_3d.h"
        render_state->mesh.quad = CreateMesh(render_state->mesh.data,
                                             CTK_WRAP_ARRAY(vertexes),
                                             CTK_WRAP_ARRAY(indexes));
    }
}

template<typename StateType>
static void InitThreadPoolJob(ThreadPoolJob<StateType>* job, Stack* perm_stack, uint32 thread_count)
{
    InitArrayFull(&job->states, perm_stack, thread_count);
    InitArrayFull(&job->tasks, perm_stack, thread_count);
}

static void InitThreadPoolJobs(RenderState* render_state, Stack* perm_stack, ThreadPool* thread_pool)
{
    InitThreadPoolJob(&render_state->thread_pool_job.update_mvp_matrixes, perm_stack, thread_pool->size);
    InitThreadPoolJob(&render_state->thread_pool_job.record_render_commands, perm_stack, RENDER_THREAD_COUNT);
}

static RenderState* CreateRenderState(Stack* perm_stack, Stack temp_stack, ThreadPool* thread_pool)
{
    RenderState* render_state = Allocate<RenderState>(perm_stack, 1);
    InitBuffers(render_state);
    InitRenderTargets(render_state, perm_stack, temp_stack);
    InitVertexLayout(render_state, perm_stack);
    InitSampler(render_state);
    InitShaderDatas(render_state, perm_stack);
    InitShaderDataSets(render_state, perm_stack, temp_stack);
    InitPipelines(render_state, temp_stack);
    InitMeshes(render_state);
    InitThreadPoolJobs(render_state, perm_stack, thread_pool);
    return render_state;
}

/// Game Update
////////////////////////////////////////////////////////////
static void LocalTranslate(View* view, Vec3<float32> translation)
{
    Matrix matrix = ID_MATRIX;
    matrix = RotateX(matrix, view->rotation.x);
    matrix = RotateY(matrix, view->rotation.y);
    matrix = RotateZ(matrix, view->rotation.z);
    matrix = Translate(matrix, translation);

    Vec3<float32> forward =
    {
        .x = Get(&matrix, 0, 2),
        .y = Get(&matrix, 1, 2),
        .z = Get(&matrix, 2, 2),
    };
    Vec3<float32> right =
    {
        .x = Get(&matrix, 0, 0),
        .y = Get(&matrix, 1, 0),
        .z = Get(&matrix, 2, 0),
    };
    view->position = view->position + (forward * translation.z);
    view->position = view->position + (right * translation.x);
    view->position.y += translation.y;
}

static void ViewControls(Game* game)
{
    View* view = &game->view;

    // Translation
    static constexpr float32 BASE_TRANSLATION_SPEED = 0.01f;
    float32 mod = KeyDown(KEY_SHIFT) ? 8.0f : 1.0f;
    float32 translation_speed = BASE_TRANSLATION_SPEED * mod;
    Vec3<float32> translation = {};

    if (KeyDown(KEY_W)) { translation.z += translation_speed; }
    if (KeyDown(KEY_S)) { translation.z -= translation_speed; }
    if (KeyDown(KEY_D)) { translation.x += translation_speed; }
    if (KeyDown(KEY_A)) { translation.x -= translation_speed; }
    if (KeyDown(KEY_E)) { translation.y += translation_speed; }
    if (KeyDown(KEY_Q)) { translation.y -= translation_speed; }

    LocalTranslate(view, translation);

    // Rotation
    if (MouseButtonDown(1))
    {
        static constexpr float32 ROTATION_SPEED = 0.2f;
        view->rotation.x -= game->mouse.delta.y * ROTATION_SPEED;
        view->rotation.y -= game->mouse.delta.x * ROTATION_SPEED;
        view->rotation.x = Clamp(view->rotation.x, -view->max_x_angle, view->max_x_angle);
    }
}

static void Controls(Game* game)
{
    if (KeyDown(KEY_ESCAPE))
    {
        CloseWindow();
        return;
    }

    ViewControls(game);
}

static void UpdateGame(Game* game)
{
    UpdateMouse(&game->mouse);

    Controls(game);
    if (!WindowIsOpen())
    {
        return; // Controls closed window.
    }
}

/// RenderState Update
////////////////////////////////////////////////////////////
static Matrix CreateViewProjectionMatrix(View* view)
{
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
    Matrix view_matrix = LookAt(view->position, view->position + forward, { 0.0f, -1.0f, 0.0f });

    // Projection Matrix
    Matrix projection_matrix = GetPerspectiveMatrix(view->vertical_fov, view->aspect, view->z_near, view->z_far);

    return projection_matrix * view_matrix;
}

static void UpdateMVPMatrixesThread(void* data)
{
    auto state = (MVPMatrixState*)data;
    BatchRange  batch_range            = state->batch_range;
    Matrix      view_projection_matrix = state->view_projection_matrix;
    VSBuffer*   frame_vs_buffer        = state->frame_vs_buffer;
    EntityData* entity_data            = state->entity_data;

    // Update entity MVP matrixes.
    for (uint32 i = batch_range.start; i < batch_range.start + batch_range.size; ++i)
    {
        Transform* entity_transform = GetTransformPtr(entity_data, i);
        Matrix model_matrix = ID_MATRIX;
        model_matrix = Translate(model_matrix, entity_transform->position);
        model_matrix = RotateX(model_matrix, entity_transform->rotation.x);
        model_matrix = RotateY(model_matrix, entity_transform->rotation.y);
        model_matrix = RotateZ(model_matrix, entity_transform->rotation.z);
        // model_matrix = Scale(model_matrix, entity_transform->scale);

        frame_vs_buffer->mvp_matrixes[i] = view_projection_matrix * model_matrix;
    }
}

static void UpdateMVPMatrixes(RenderState* render_state, Game* game, ThreadPool* thread_pool)
{
    ThreadPoolJob<MVPMatrixState>* job = &render_state->thread_pool_job.update_mvp_matrixes;
    Matrix view_projection_matrix = CreateViewProjectionMatrix(&game->view);
    auto frame_vs_buffer = GetBufferMem<VSBuffer>(render_state->data.vs_buffer, RTK::global_ctx.frames.index);
    uint32 thread_count = thread_pool->size;

    // Initialize thread states and submit tasks.
    for (uint32 thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        MVPMatrixState* state = GetPtr(&job->states, thread_index);
        state->batch_range            = GetBatchRange(thread_index, thread_count, game->entity_data.count);
        state->view_projection_matrix = view_projection_matrix;
        state->frame_vs_buffer        = frame_vs_buffer;
        state->entity_data            = &game->entity_data;

        Set(&job->tasks, thread_index, SubmitTask(thread_pool, state, UpdateMVPMatrixesThread));
    }

    // Wait for tasks to complete.
    for (uint32 i = 0; i < thread_count; ++i)
    {
        Wait(thread_pool, Get(&job->tasks, i));
    }
}

static void RecordRenderCommandsThread(void* data)
{
    auto state = (RenderCommandState*)data;
    RenderState* render_state = state->render_state;

    VkCommandBuffer command_buffer = BeginRecordingRenderCommands(render_state->render_target.main,
                                                                  state->thread_index);
        BindPipeline(command_buffer, render_state->pipeline.main);
        BindMeshData(command_buffer, render_state->mesh.data);
        BindShaderDataSet(command_buffer, render_state->data_set.entity_data, render_state->pipeline.main, 0);
        BindShaderDataSet(command_buffer, state->texture, render_state->pipeline.main, 1);
        DrawMesh(command_buffer, state->mesh, state->batch_range.start, state->batch_range.size);
    EndRecordingRenderCommands(command_buffer);
}

static void RecordRenderCommands(Game* game, RenderState* render_state, ThreadPool* thread_pool)
{
    ThreadPoolJob<RenderCommandState>* job = &render_state->thread_pool_job.record_render_commands;

    // Initialize thread states and submit tasks.
    static ShaderDataSetHnd textures[] =
    {
        render_state->data_set.axis_cube_texture,
        render_state->data_set.dirt_block_texture,
    };
    static MeshHnd meshes[] =
    {
        render_state->mesh.cube,
        render_state->mesh.quad,
    };
    static constexpr uint32 TEXTURE_COUNT = CTK_ARRAY_SIZE(textures);
    static constexpr uint32 MESH_COUNT = CTK_ARRAY_SIZE(meshes);
    static constexpr uint32 THREAD_COUNT = TEXTURE_COUNT * MESH_COUNT;
    static_assert(THREAD_COUNT <= RENDER_THREAD_COUNT);
    uint32 entity_count = game->entity_data.count;

    for (uint32 texture_index = 0; texture_index < TEXTURE_COUNT; ++texture_index)
    {
        for (uint32 mesh_index = 0; mesh_index < MESH_COUNT; ++mesh_index)
        {
            uint32 thread_index = (texture_index * MESH_COUNT) + mesh_index;
            RenderCommandState* state = GetPtr(&job->states, thread_index);
            state->render_state = render_state;
            state->thread_index = thread_index;
            state->texture      = textures[texture_index];
            state->mesh         = meshes[mesh_index];
            state->batch_range  = GetBatchRange(thread_index, THREAD_COUNT, entity_count);

            Set(&job->tasks, thread_index, SubmitTask(thread_pool, state, RecordRenderCommandsThread));
        }
    }

    // Wait for tasks to complete.
    for (uint32 i = 0; i < THREAD_COUNT; ++i)
    {
        Wait(thread_pool, Get(&job->tasks, i));
    }
}

static void TestMain()
{
    Stack* perm_stack = CreateStack(Megabyte32<8>());
    Stack* temp_stack = CreateStack(perm_stack, Megabyte32<1>());
    FreeList* free_list = CreateFreeList(Megabyte32<1>());

    // Make win32 process DPI aware so windows scale properly.
    SetProcessDPIAware();

    WindowInfo window_info =
    {
        .x        = 0,
        .y        = 90, // Taskbar height @ 1.5x zoom (laptop).
        .width    = 1080,
        .height   = 720,
        .title    = "3D Test",
        .callback = DefaultWindowCallback,
    };
    OpenWindow(&window_info);

    ThreadPool* thread_pool = CreateThreadPool(perm_stack, 8);

    // Init RTK Context + Resources
    VkDescriptorPoolSize descriptor_pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                1024 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1024 },
    };
    ContextInfo context_info = {};
    context_info.instance_info.application_name = "RTK 3D Test",
    context_info.instance_info.extensions       = {};
#ifdef RTK_ENABLE_VALIDATION
    context_info.instance_info.debug_callback         = DefaultDebugCallback,
    context_info.instance_info.debug_message_severity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    context_info.instance_info.debug_message_type     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
#endif
    context_info.required_features.as_struct =
    {
        .geometryShader    = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };
    context_info.render_thread_count   = RENDER_THREAD_COUNT,
    context_info.descriptor_pool_sizes = CTK_WRAP_ARRAY(descriptor_pool_sizes),
    InitContext(perm_stack, *temp_stack, &context_info);

    ResourcesInfo resources_info =
    {
        .max_buffers          = 16,
        .max_images           = 8,
        .max_shader_datas     = 8,
        .max_shader_data_sets = 8,
        .max_mesh_datas       = 1,
        .max_meshes           = 8,
        .max_render_targets   = 1,
        .max_pipelines        = 1,
    };
    InitResources(perm_stack, &resources_info);

    Game* game = CreateGame(perm_stack);
    RenderState* render_state = CreateRenderState(perm_stack, *temp_stack, thread_pool);
    // ProfileTree* prof_tree = CreateProfileTree(perm_stack, 64);

    // Run game.
    for (;;)
    {
        // StartProfile(prof_tree, "Frame");

        ProcessWindowEvents();
        if (!WindowIsOpen())
        {
            break; // Quit event closed window.
        }

        if (WindowIsActive())
        {
            // StartProfile(prof_tree, "NextFrame()");
            NextFrame();
            // EndProfile(prof_tree);

            // StartProfile(prof_tree, "UpdateGame()");
            UpdateGame(game);
            // EndProfile(prof_tree);

            // StartProfile(prof_tree, "UpdateMVPMatrixes()");
            UpdateMVPMatrixes(render_state, game, thread_pool);
            // EndProfile(prof_tree);

            // StartProfile(prof_tree, "RecordRenderCommands()");
            RecordRenderCommands(game, render_state, thread_pool);
            // EndProfile(prof_tree);

            // StartProfile(prof_tree, "SubmitRenderCommands()");
            SubmitRenderCommands(render_state->render_target.main);
            // EndProfile(prof_tree);
        }
        else
        {
            Sleep(1);
        }

        // EndProfile(prof_tree);
        // PrintProfileTree(prof_tree);
        // ClearProfileTree(prof_tree);
    }
}
