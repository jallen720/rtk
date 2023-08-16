#pragma once

#include "ctk3/ctk3.h"
#include "ctk3/debug.h"
#include "ctk3/math.h"
#include "ctk3/stack.h"
#include "ctk3/free_list.h"
#include "ctk3/array.h"
#include "ctk3/thread_pool.h"
#include "rtk/rtk.h"
#include "rtk/tests/3d/defs.h"
#include "rtk/tests/3d/game_state.h"

using namespace CTK;
using namespace RTK;

namespace Test3D
{

/// Data
////////////////////////////////////////////////////////////
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

struct RenderCommandState
{
    uint32         thread_index;
    ShaderDataSet* texture;
    BatchRange     batch_range;
    Mesh*          mesh;
};

template<typename StateType>
struct Job
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
        Job<MVPMatrixState>     update_mvp_matrixes;
        Job<RenderCommandState> record_render_commands;
    }
    job;

    // Memory
    DeviceStack device_stack;
    DeviceStack host_stack;
    Buffer      staging_buffer;

    // Resources
    RenderTarget* render_target;
    struct
    {
        ShaderData* vs_buffer;
        ShaderData* axis_cube_texture;
        ShaderData* dirt_block_texture;
    }
    data;
    struct
    {
        ShaderDataSet* entity_data;
        ShaderDataSet* axis_cube_texture;
        ShaderDataSet* dirt_block_texture;
    }
    data_set;
    struct
    {
        Shader* vert;
        Shader* frag;
    }
    shader;
    Pipeline* pipeline;
    struct
    {
        MeshData* data;
        Mesh*     cube;
        Mesh*     quad;
    }
    mesh;
};

static constexpr uint32 RENDER_THREAD_COUNT = 4;

/// Instance
////////////////////////////////////////////////////////////
static RenderState global_rs;

/// Utils
////////////////////////////////////////////////////////////
static void InitDeviceMemory()
{
    DeviceStackInfo host_stack_info =
    {
        .size               = Megabyte32<64>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    InitDeviceStack(&global_rs.host_stack, &host_stack_info);

    DeviceStackInfo device_stack_info =
    {
        .size               = Megabyte32<64>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    InitDeviceStack(&global_rs.device_stack, &device_stack_info);

    InitBuffer(&global_rs.staging_buffer, &global_rs.host_stack, Megabyte32<4>());
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
    global_rs.render_target = CreateRenderTarget(&perm_stack->allocator, temp_stack, free_list, &info);
}

static void InitVertexLayout(Stack* perm_stack)
{
    VertexLayout* vertex_layout = &global_rs.vertex_layout;

    // Init pipeline vertex layout.
    InitArray(&vertex_layout->bindings, &perm_stack->allocator, 1);
    PushBinding(vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);

    InitArray(&vertex_layout->attributes, &perm_stack->allocator, 4);
    PushAttribute(vertex_layout, 3, ATTRIBUTE_TYPE_FLOAT32); // Position
    PushAttribute(vertex_layout, 2, ATTRIBUTE_TYPE_FLOAT32); // UV
}

static void InitSampler()
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
    global_rs.sampler = CreateSampler(&info);
}

static void InitShaderDatas(Stack* perm_stack)
{
    // Buffers
    {
        ShaderDataInfo info =
        {
            .stages      = VK_SHADER_STAGE_VERTEX_BIT,
            .type        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .per_frame   = true,
            .count       = 1,
            .buffer =
            {
                .stack = &global_rs.host_stack,
                .size  = sizeof(VSBuffer),
            },
        };
        global_rs.data.vs_buffer = CreateShaderData(&perm_stack->allocator, &info);
    }

    // Textures
    {
        ShaderDataInfo info =
        {
            .stages    = VK_SHADER_STAGE_FRAGMENT_BIT,
            .type      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .per_frame = false,
            .count     = 1,
            .image =
            {
                .image =
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
                },
                .sampler = global_rs.sampler,
            },
        };
        global_rs.data.axis_cube_texture  = CreateShaderData(&perm_stack->allocator, &info);
        global_rs.data.dirt_block_texture = CreateShaderData(&perm_stack->allocator, &info);
    }
}

static void InitShaderDataSets(Stack* perm_stack, Stack* temp_stack)
{
    // data_set.vs_buffer
    {
        ShaderData* datas[] =
        {
            global_rs.data.vs_buffer,
        };
        global_rs.data_set.entity_data =
            CreateShaderDataSet(&perm_stack->allocator, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    // data_set.axis_cube_texture
    {
        ShaderData* datas[] =
        {
            global_rs.data.axis_cube_texture,
        };
        global_rs.data_set.axis_cube_texture =
            CreateShaderDataSet(&perm_stack->allocator, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    // data_set.dirt_block_texture
    {
        ShaderData* datas[] =
        {
            global_rs.data.dirt_block_texture,
        };
        global_rs.data_set.dirt_block_texture =
            CreateShaderDataSet(&perm_stack->allocator, temp_stack, CTK_WRAP_ARRAY(datas));
    }
}

static void InitShaders(Stack* perm_stack, Stack* temp_stack)
{
    global_rs.shader.vert =
        CreateShader(&perm_stack->allocator, temp_stack, "shaders/bin/3d.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    global_rs.shader.frag =
        CreateShader(&perm_stack->allocator, temp_stack, "shaders/bin/3d.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

static void InitPipelines(Stack* perm_stack, Stack* temp_stack, FreeList* free_list)
{
    // Pipeline Layout
    ShaderDataSet* shader_data_sets[] =
    {
        global_rs.data_set.entity_data,
        global_rs.data_set.axis_cube_texture,
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
        .shader_data_sets     = CTK_WRAP_ARRAY(shader_data_sets),
        // .push_constant_ranges = CTK_WRAP_ARRAY(push_constant_ranges),
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
        global_rs.shader.vert,
        global_rs.shader.frag,
    };
    PipelineInfo pipeline_info =
    {
        .shaders       = CTK_WRAP_ARRAY(shaders),
        .viewports     = CTK_WRAP_ARRAY(viewports),
        .vertex_layout = &global_rs.vertex_layout,
        .depth_testing = true,
        .render_target = global_rs.render_target,
    };

    global_rs.pipeline = CreatePipeline(&perm_stack->allocator, temp_stack, free_list, &pipeline_info,
                                        &pipeline_layout_info);
}

static void InitMeshes(Stack* perm_stack)
{
    MeshDataInfo mesh_data_info =
    {
        .vertex_buffer_size = Megabyte32<1>(),
        .index_buffer_size  = Megabyte32<1>(),
    };
    global_rs.mesh.data = CreateMeshData(&perm_stack->allocator, &global_rs.device_stack, &mesh_data_info);

    {
        #include "rtk/meshes/cube.h"
        global_rs.mesh.cube = CreateDeviceMesh(&perm_stack->allocator, global_rs.mesh.data,
                                               CTK_WRAP_ARRAY(vertexes), CTK_WRAP_ARRAY(indexes),
                                               &global_rs.staging_buffer);
    }

    {
        #include "rtk/meshes/quad.h"
        global_rs.mesh.quad = CreateDeviceMesh(&perm_stack->allocator, global_rs.mesh.data,
                                               CTK_WRAP_ARRAY(vertexes), CTK_WRAP_ARRAY(indexes),
                                               &global_rs.staging_buffer);
    }
}

template<typename StateType>
static void InitThreadPoolJob(Job<StateType>* job, Stack* perm_stack, uint32 thread_count)
{
    InitArrayFull(&job->states, &perm_stack->allocator, thread_count);
    InitArrayFull(&job->tasks, &perm_stack->allocator, thread_count);
}

static void InitThreadPoolJobs(Stack* perm_stack, ThreadPool* thread_pool)
{
    InitThreadPoolJob(&global_rs.job.update_mvp_matrixes, perm_stack, thread_pool->size);
    InitThreadPoolJob(&global_rs.job.record_render_commands, perm_stack, RENDER_THREAD_COUNT);
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
    Matrix view_matrix = LookAt(view->position, view->position + forward, { 0.0f, -1.0f, 0.0f });

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
    Matrix view_projection_matrix = state->view_projection_matrix;
    VSBuffer* frame_vs_buffer = state->frame_vs_buffer;
    EntityData* entity_data = state->entity_data;

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

static void RecordRenderCommandsThread(void* data)
{
    auto state = (RenderCommandState*)data;
    VkCommandBuffer command_buffer = BeginRenderCommands(global_rs.render_target, state->thread_index);
        Pipeline* pipeline = global_rs.pipeline;
        BindPipeline(command_buffer, pipeline);
        BindShaderDataSet(command_buffer, global_rs.data_set.entity_data, pipeline, 0);
        BindShaderDataSet(command_buffer, state->texture, pipeline, 1);
        BindMeshData(command_buffer, global_rs.mesh.data);
        DrawMesh(command_buffer, state->mesh, state->batch_range.start, state->batch_range.size);
    EndRenderCommands(command_buffer);
}

static void WriteImageToTexture(ShaderData* sd, uint32 index, const char* image_path)
{
    // Load image data and write to staging buffer.
    ImageData image_data = {};
    LoadImageData(&image_data, image_path);
    WriteHostBuffer(&global_rs.staging_buffer, image_data.data, (VkDeviceSize)image_data.size);
    DestroyImageData(&image_data);

    // Copy image data in staging buffer to shader data images.
    uint32 image_count = GetImageCount(sd);
    for (uint32 i = 0; i < image_count; ++i)
    {
        WriteToShaderDataImage(sd, 0, index, &global_rs.staging_buffer);
    }
}

static void LoadTextureData()
{
    WriteImageToTexture(global_rs.data.axis_cube_texture,  0, "images/axis_cube.png");
    WriteImageToTexture(global_rs.data.dirt_block_texture, 0, "images/dirt_block.png");
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
    UpdatePipelineViewports(global_rs.pipeline, free_list, CTK_WRAP_ARRAY(viewports));
}

static void UpdateAllRenderTargetAttachments(Stack* temp_stack, FreeList* free_list)
{
    UpdateRenderTargetAttachments(global_rs.render_target, temp_stack, free_list);
}

/// Interface
////////////////////////////////////////////////////////////
static void InitRenderState(Stack* perm_stack, Stack* temp_stack, FreeList* free_list, ThreadPool* thread_pool)
{
    InitVertexLayout(perm_stack);
    InitSampler();

    InitDeviceMemory();

    InitImageState(&perm_stack->allocator, 16);
    InitShaderDatas(perm_stack);
    BackImagesWithMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    LoadTextureData();

    InitShaderDataSets(perm_stack, temp_stack);
    InitRenderTargets(perm_stack, temp_stack, free_list);
    InitShaders(perm_stack, temp_stack);
    InitPipelines(perm_stack, temp_stack, free_list);
    InitMeshes(perm_stack);
    InitThreadPoolJobs(perm_stack, thread_pool);

}

static void UpdateMVPMatrixes(ThreadPool* thread_pool)
{
    Job<MVPMatrixState>* job = &global_rs.job.update_mvp_matrixes;
    Matrix view_projection_matrix = GetViewProjectionMatrix(&game_state.view);
    auto frame_vs_buffer = GetCurrentFrameBufferMem<VSBuffer>(global_rs.data.vs_buffer, 0);
    uint32 thread_count = thread_pool->size;

    // Initialize thread states and submit tasks.
    for (uint32 thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        MVPMatrixState* state = GetPtr(&job->states, thread_index);
        state->batch_range            = GetBatchRange(thread_index, thread_count, game_state.entity_data.count);
        state->view_projection_matrix = view_projection_matrix;
        state->frame_vs_buffer        = frame_vs_buffer;
        state->entity_data            = &game_state.entity_data;

        Set(&job->tasks, thread_index, SubmitTask(thread_pool, state, UpdateMVPMatrixesThread));
    }

    // Wait for tasks to complete.
    for (uint32 i = 0; i < thread_count; ++i)
    {
        Wait(thread_pool, Get(&job->tasks, i));
    }
}

static void RecordRenderCommands(ThreadPool* thread_pool)
{
    Job<RenderCommandState>* job = &global_rs.job.record_render_commands;

    // Initialize thread states and submit tasks.
    static ShaderDataSet* textures[] =
    {
        global_rs.data_set.axis_cube_texture,
        global_rs.data_set.dirt_block_texture,
    };
    static Mesh* meshes[] =
    {
        global_rs.mesh.cube,
        global_rs.mesh.quad,
    };
    static constexpr uint32 TEXTURE_COUNT = CTK_ARRAY_SIZE(textures);
    static constexpr uint32 MESH_COUNT    = CTK_ARRAY_SIZE(meshes);
    static constexpr uint32 THREAD_COUNT  = TEXTURE_COUNT * MESH_COUNT;
    static_assert(THREAD_COUNT <= RENDER_THREAD_COUNT);
    for (uint32 texture_index = 0; texture_index < TEXTURE_COUNT; ++texture_index)
    {
        for (uint32 mesh_index = 0; mesh_index < MESH_COUNT; ++mesh_index)
        {
            uint32 thread_index = (texture_index * MESH_COUNT) + mesh_index;
            RenderCommandState* state = GetPtr(&job->states, thread_index);
            state->thread_index = thread_index;
            state->texture      = textures[texture_index];
            state->mesh         = meshes[mesh_index];
            state->batch_range  = GetBatchRange(thread_index, THREAD_COUNT, game_state.entity_data.count);

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
    return global_rs.render_target;
}

}
