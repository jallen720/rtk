#include <windows.h>
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/multithreading.h"
#include "ctk2/math.h"
#include "ctk2/profile.h"
#include "stk2/stk.h"

// #define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

#include "rtk/tests/shared.h"
#include "rtk/tests/defs.h"

using namespace CTK;
using namespace STK;
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
    Mouse        mouse;
    View         view;
    EntityData   entity_data;
    FrameMetrics frame_metrics;
};

struct VSBuffer
{
    Matrix mvp_matrixes[MAX_ENTITIES];
};

struct UpdateMVPMatrixState
{
    BatchRange  batch_range;
    Matrix      view_projection_matrix;
    VSBuffer*   frame_vs_buffer;
    EntityData* entity_data;
};

struct MVPMatrixUpdate
{
    Array<BatchRange>           batch_ranges;
    Array<UpdateMVPMatrixState> states;
    Array<TaskHnd>              tasks;
};

struct RenderState
{
    VertexLayout    vertex_layout;
    VkSampler       sampler;
    MVPMatrixUpdate mvp_matrix_update;

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
        PipelineHnd main;
    }
    pipeline;
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

/// RTK
////////////////////////////////////////////////////////////
static void SelectPhysicalDevice(RTKContext* rtk)
{
    // Use first discrete device if any are available.
    for (uint32 i = 0; i < rtk->physical_devices.count; ++i)
    {
        if (GetPtr(&rtk->physical_devices, i)->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            UsePhysicalDevice(rtk, i);
            break;
        }
    }

    LogPhysicalDevice(rtk->physical_device, "selected physical device");
}

/// Game
////////////////////////////////////////////////////////////
static uint32 PushEntity(EntityData* entity_data)
{
    if (entity_data->count == MAX_ENTITIES)
        CTK_FATAL("can't push another entity: entity count (%u) at max (%u)", entity_data->count, MAX_ENTITIES);

    uint32 new_entity = entity_data->count;
    ++entity_data->count;
    return new_entity;
}

static void InitGame(Game* game)
{
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

    // FrameMetrics
    static constexpr float64 FPS_UPDATE_FREQUENCY = 0.0;
    InitFrameMetrics(&game->frame_metrics, FPS_UPDATE_FREQUENCY);
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
static void InitBuffers(RenderState* rs, RTKContext* rtk)
{
    BufferInfo host_buffer_info =
    {
        .size               = Megabyte(128),
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    rs->buffer.host = CreateBuffer(rtk, &host_buffer_info);

    BufferInfo device_buffer_info =
    {
        .size               = Megabyte(256),
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    rs->buffer.device = CreateBuffer(rtk, &device_buffer_info);

    rs->buffer.staging = CreateBuffer(rs->buffer.host, Megabyte(16));
}

static void InitRenderTargets(RenderState* rs, Stack* mem, Stack temp_mem, RTKContext* rtk)
{
    RenderTargetInfo info =
    {
        .depth_testing          = true,
        .color_attachment_count = 1,
    };
    rs->render_target.main = CreateRenderTarget(mem, temp_mem, rtk, &info);
    PushClearValue(rs->render_target.main, { 0.0f, 0.1f, 0.2f, 1.0f });
    PushClearValue(rs->render_target.main, { 1.0f });
}

static void InitVertexLayout(RenderState* rs, Stack* mem)
{
    VertexLayout* vertex_layout = &rs->vertex_layout;

    // Init pipeline vertex layout.
    InitArray(&vertex_layout->bindings, mem, 1);
    PushBinding(vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);

    InitArray(&vertex_layout->attributes, mem, 4);
    PushAttribute(vertex_layout, 3); // Position
    PushAttribute(vertex_layout, 2); // UV
}

static void InitSampler(RenderState* rs, RTKContext* rtk)
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
        .maxAnisotropy           = rtk->physical_device->properties.limits.maxSamplerAnisotropy,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .minLod                  = 0.0f,
        .maxLod                  = 0.0f,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    VkResult res = vkCreateSampler(rtk->device, &info, NULL, &rs->sampler);
    Validate(res, "vkCreateSampler() failed");
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

static ShaderDataHnd CreateTexture(cstring image_data_path, RTKContext* rtk, RenderState* rs, Stack* mem)
{
    ImageData image_data = {};
    LoadImageData(&image_data, image_data_path);

    ShaderDataInfo info = DefaultTextureInfo(rtk->swapchain.image_format, rs->sampler, &image_data);
    ShaderDataHnd shader_data_hnd = CreateShaderData(mem, rtk, &info);

    // Copy image data into staging buffer.
    Clear(rs->buffer.staging);
    Write(rs->buffer.staging, image_data.data, image_data.size);
    uint32 image_count = GetShaderData(shader_data_hnd)->image_hnds.count;
    for (uint32 i = 0; i < image_count; ++i)
        WriteToShaderDataImage(shader_data_hnd, i, rs->buffer.staging, rtk);

    DestroyImageData(&image_data);

    return shader_data_hnd;
}

static void InitShaderDatas(RenderState* rs, Stack* mem, RTKContext* rtk)
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
                .parent = rs->buffer.host,
                .size   = sizeof(VSBuffer),
            }
        };
        rs->data.vs_buffer = CreateShaderData(mem, rtk, &info);
    }

    rs->data.axis_cube_texture = CreateTexture("images/axis_cube.png", rtk, rs, mem);
    rs->data.dirt_block_texture = CreateTexture("images/dirt_block.png", rtk, rs, mem);
}

static void InitShaderDataSets(RenderState* rs, Stack* mem, Stack temp_mem, RTKContext* rtk)
{
    // data_set.vs_buffer
    {
        ShaderDataHnd datas[] =
        {
            rs->data.vs_buffer,
        };
        rs->data_set.entity_data = CreateShaderDataSet(mem, temp_mem, rtk, WRAP_ARRAY(datas));
    }

    // data_set.axis_cube_texture
    {
        ShaderDataHnd datas[] =
        {
            rs->data.axis_cube_texture,
        };
        rs->data_set.axis_cube_texture = CreateShaderDataSet(mem, temp_mem, rtk, WRAP_ARRAY(datas));
    }

    // data_set.dirt_block_texture
    {
        ShaderDataHnd datas[] =
        {
            rs->data.dirt_block_texture,
        };
        rs->data_set.dirt_block_texture = CreateShaderDataSet(mem, temp_mem, rtk, WRAP_ARRAY(datas));
    }
}

static void InitPipelines(RenderState* rs, Stack temp_mem, RTKContext* rtk)
{
    // Pipeline info arrays.
    Shader shaders[] =
    {
        {
            .module = LoadShaderModule(rtk, temp_mem, "shaders/bin/3d.vert.spv"),
            .stage  = VK_SHADER_STAGE_VERTEX_BIT
        },
        {
            .module = LoadShaderModule(rtk, temp_mem, "shaders/bin/3d.frag.spv"),
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT
        },
    };
    ShaderDataSetHnd shader_data_sets[] =
    {
        rs->data_set.entity_data,
        rs->data_set.axis_cube_texture,
    };
    VkPushConstantRange push_constant_ranges[] =
    {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset     = 0,
            .size       = sizeof(uint32)
        },
    };

    PipelineInfo pipeline_info =
    {
        .vertex_layout        = &rs->vertex_layout,
        .shaders              = WRAP_ARRAY(shaders),
        .shader_data_sets     = WRAP_ARRAY(shader_data_sets),
        .push_constant_ranges = WRAP_ARRAY(push_constant_ranges),
    };
    rs->pipeline.main = CreatePipeline(temp_mem, rs->render_target.main, rtk, &pipeline_info);
}

static void InitMeshes(RenderState* rs)
{
    MeshDataInfo mesh_data_info =
    {
        .parent_buffer      = rs->buffer.host,
        .vertex_buffer_size = Megabyte(1),
        .index_buffer_size  = Megabyte(1),
    };
    rs->mesh.data = CreateMeshData(&mesh_data_info);

    {
        #include "rtk/meshes/cube.h"
        rs->mesh.cube = CreateMesh(rs->mesh.data, WRAP_ARRAY(vertexes), WRAP_ARRAY(indexes));
    }

    {
        #include "rtk/meshes/quad.h"
        rs->mesh.quad = CreateMesh(rs->mesh.data, WRAP_ARRAY(vertexes), WRAP_ARRAY(indexes));
    }
}

static void InitMVPMatrixUpdate(RenderState* rs, Stack* mem, ThreadPool* thread_pool)
{
    uint32 thread_count = thread_pool->size;
    MVPMatrixUpdate* mvp_matrix_update = &rs->mvp_matrix_update;
    InitArrayFull(&mvp_matrix_update->batch_ranges, mem, thread_count);
    InitArrayFull(&mvp_matrix_update->states, mem, thread_count);
    InitArrayFull(&mvp_matrix_update->tasks, mem, thread_count);
}

static void InitRenderState(RenderState* rs, Stack* mem, Stack temp_mem, RTKContext* rtk,
                            ThreadPool* thread_pool)
{
    InitBuffers(rs, rtk);
    InitRenderTargets(rs, mem, temp_mem, rtk);
    InitVertexLayout(rs, mem);
    InitSampler(rs, rtk);
    InitShaderDatas(rs, mem, rtk);
    InitShaderDataSets(rs, mem, temp_mem, rtk);
    InitPipelines(rs, temp_mem, rtk);
    InitMeshes(rs);
    InitMVPMatrixUpdate(rs, mem, thread_pool);
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

static void ViewControls(Game* game, Window* window)
{
    View* view = &game->view;

    // Translation
    static constexpr float32 BASE_TRANSLATION_SPEED = 0.01f;
    float32 mod = KeyDown(window, Key::SHIFT) ? 8.0f : 1.0f;
    float32 translation_speed = BASE_TRANSLATION_SPEED * mod;
    Vec3<float32> translation = {};

    if (KeyDown(window, Key::W)) translation.z += translation_speed;
    if (KeyDown(window, Key::S)) translation.z -= translation_speed;
    if (KeyDown(window, Key::D)) translation.x += translation_speed;
    if (KeyDown(window, Key::A)) translation.x -= translation_speed;
    if (KeyDown(window, Key::E)) translation.y += translation_speed;
    if (KeyDown(window, Key::Q)) translation.y -= translation_speed;

    LocalTranslate(view, translation);

    // Rotation
    if (MouseButtonDown(window, 1))
    {
        static constexpr float32 ROTATION_SPEED = 0.2f;
        view->rotation.x -= game->mouse.delta.y * ROTATION_SPEED;
        view->rotation.y -= game->mouse.delta.x * ROTATION_SPEED;
        view->rotation.x = Clamp(view->rotation.x, -view->max_x_angle, view->max_x_angle);
    }
}

static void Controls(Game* game, Window* window)
{
    if (KeyDown(window, Key::ESCAPE))
    {
        window->open = false;
        return;
    }

    ViewControls(game, window);
}

static void UpdateGame(Game* game, Window* window)
{
    UpdateMouse(&game->mouse, window);

    Controls(game, window);
    if (!window->open)
        return; // Controls closed window.
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
    auto state = (UpdateMVPMatrixState*)data;
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

static void UpdateMVPMatrixes(RenderState* rs, Game* game, RTKContext* rtk, ThreadPool* thread_pool)
{
    MVPMatrixUpdate* mvp_matrix_update = &rs->mvp_matrix_update;
    Matrix view_projection_matrix = CreateViewProjectionMatrix(&game->view);
    auto frame_vs_buffer = GetBufferMem<VSBuffer>(rs->data.vs_buffer, rtk->frames.index);
    uint32 thread_count = thread_pool->size;

    // Initialize thread states.
    CalculateBatchRanges(&mvp_matrix_update->batch_ranges, game->entity_data.count);
    for (uint32 i = 0; i < thread_count; ++i)
    {
        UpdateMVPMatrixState* state = GetPtr(&mvp_matrix_update->states, i);
        state->batch_range            = Get(&mvp_matrix_update->batch_ranges, i);
        state->view_projection_matrix = view_projection_matrix;
        state->frame_vs_buffer        = frame_vs_buffer;
        state->entity_data            = &game->entity_data;
    }

    // Submit tasks.
    for (uint32 i = 0; i < thread_count; ++i)
    {
        UpdateMVPMatrixState* state = GetPtr(&mvp_matrix_update->states, i);
        TaskHnd task = SubmitTask(thread_pool, state, UpdateMVPMatrixesThread);
        Set(&mvp_matrix_update->tasks, i, task);
    }

    // Wait for tasks to complete.
    for (uint32 i = 0; i < thread_count; ++i)
        Wait(thread_pool, Get(&mvp_matrix_update->tasks, i));
}

struct RecordRenderCommandState
{

};

static void RecordRenderCommandsThread(void* data)
{
    CTK_UNUSED(data)
}

static void RecordRenderCommands(Game* game, RenderState* rs, RTKContext* rtk)
{
    uint32 entity_region_count = game->entity_data.count / 4;
    uint32 entity_region_start = 0;

    static constexpr uint32 THREAD_INDEX = 0;
    VkCommandBuffer command_buffer = BeginRecordingRenderCommands(rtk, rs->render_target.main, THREAD_INDEX);
        BindPipeline(command_buffer, rs->pipeline.main);

        // Bind per-pipeline shader data.
        BindShaderDataSet(command_buffer, rs->data_set.entity_data, rs->pipeline.main, rtk, 0);

        // Bind mesh data buffers.
        BindMeshData(command_buffer, rs->mesh.data);

        // Axis Cube Texture
        {
            BindShaderDataSet(command_buffer, rs->data_set.axis_cube_texture, rs->pipeline.main, rtk, 1);

            // Cube Mesh
            DrawMesh(command_buffer, rs->mesh.cube, entity_region_start, entity_region_count);
            entity_region_start += entity_region_count;

            // Quad Mesh
            DrawMesh(command_buffer, rs->mesh.quad, entity_region_start, entity_region_count);
            entity_region_start += entity_region_count;
        }

        // Dirt Block Texture
        {
            BindShaderDataSet(command_buffer, rs->data_set.dirt_block_texture, rs->pipeline.main, rtk, 1);

            // Cube Mesh
            DrawMesh(command_buffer, rs->mesh.cube, entity_region_start, entity_region_count);
            entity_region_start += entity_region_count;

            // Quad Mesh
            DrawMesh(command_buffer, rs->mesh.quad, entity_region_start, entity_region_count);
            entity_region_start += entity_region_count;
        }
    EndRecordingRenderCommands(command_buffer);
}

void TestMain()
{
    Stack* mem = CreateStack(Megabyte(8));
    Stack* temp_mem = CreateStack(mem, Megabyte(1));

    // Init Win32 + Window
    InitWin32();

    WindowInfo window_info =
    {
        .surface =
        {
            .x      = 0,
            .y      = 90, // Taskbar height.
            .width  = 1080,
            .height = 720,
        },
        .title    = L"3D Test",
        .callback = DefaultWindowCallback,
    };
    auto window = Allocate<Window>(mem, 1);
    InitWindow(window, &window_info);

    // Init Threadpool
    auto thread_pool = Allocate<ThreadPool>(mem, 1);
    InitThreadPool(thread_pool, mem, 8);

    // Init RTK Context + State
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
    RTKInfo rtk_info =
    {
        .application_name = "RTK Test",
#ifdef RTK_ENABLE_VALIDATION
        .debug_callback         = DefaultDebugCallback,
        .debug_message_severity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .debug_message_type     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
#endif
        .required_features =
        {
            .as_struct =
            {
                .geometryShader    = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
            },
        },
        .max_physical_devices  = 8,
        .render_thread_count   = 1,
        .descriptor_pool_sizes = WRAP_ARRAY(descriptor_pool_sizes),
    };
    auto rtk = Allocate<RTKContext>(mem, 1);
    InitRTKContext(rtk, mem, *temp_mem, window, &rtk_info);

    RTKStateInfo state_info =
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
    InitRTKState(mem, &state_info);

    // Init Game
    auto game = Allocate<Game>(mem, 1);
    InitGame(game);

    auto rs = Allocate<RenderState>(mem, 1);
    InitRenderState(rs, mem, *temp_mem, rtk, thread_pool);

    auto prof_mgr = Allocate<ProfileManager>(mem, 1);
    InitProfileManager(prof_mgr, mem, 64);

    // Run game.
    while (1)
    {
        StartProfile(prof_mgr, "Frame");

        ProcessEvents(window);
        if (!window->open)
            break; // Quit event closed window.

        if (IsActive(window))
        {
            StartProfile(prof_mgr, "NextFrame()");
            NextFrame(rtk);
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "UpdateGame()");
            UpdateGame(game, window);
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "UpdateMVPMatrixes()");
            UpdateMVPMatrixes(rs, game, rtk, thread_pool);
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "RecordRenderCommands()");
            RecordRenderCommands(game, rs, rtk);
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "SubmitRenderCommands()");
            SubmitRenderCommands(rtk, rs->render_target.main);
            EndProfile(prof_mgr);
        }
        else
        {
            Sleep(1);
        }

        EndProfile(prof_mgr);
        PrintProfiles(prof_mgr);
        ClearProfiles(prof_mgr);
        if (Tick(&game->frame_metrics))
            PrintLine("FPS: %.2f", game->frame_metrics.fps);
    }
}
