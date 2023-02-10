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
#include "rtk/tests/3d/game.h"

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
    uint32           thread_index;
    ShaderDataSetHnd texture;
    BatchRange       batch_range;
    MeshHnd          mesh;
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
        ShaderHnd vert_3d;
        ShaderHnd frag_3d;
    }
    shader;
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

/// Instance
////////////////////////////////////////////////////////////
static RenderState render_state;

/// Utils
////////////////////////////////////////////////////////////
static void InitBuffers()
{
    BufferInfo host_buffer_info =
    {
        .size               = Megabyte32<64>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    render_state.buffer.host = CreateBuffer(&host_buffer_info);

    BufferInfo device_buffer_info =
    {
        .size               = Megabyte32<64>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    render_state.buffer.device = CreateBuffer(&device_buffer_info);

    render_state.buffer.staging = CreateBuffer(render_state.buffer.host, Megabyte32<4>());
}

static void InitRenderTargets(Stack temp_stack, FreeList* free_list)
{
    VkClearValue attachment_clear_values[] =
    {
        { 0.0f, 0.1f, 0.2f, 1.0f },
        { 1.0f },
    };
    RenderTargetInfo info =
    {
        .depth_testing           = true,
        .attachment_clear_values = CTK_WRAP_ARRAY(attachment_clear_values),
    };
    render_state.render_target.main = CreateRenderTarget(temp_stack, free_list, &info);
}

static void InitVertexLayout(Stack* perm_stack)
{
    VertexLayout* vertex_layout = &render_state.vertex_layout;

    // Init pipeline vertex layout.
    InitArray(&vertex_layout->bindings, perm_stack, 1);
    PushBinding(vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);

    InitArray(&vertex_layout->attributes, perm_stack, 4);
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
    render_state.sampler = CreateSampler(&info);
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
                    .usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                             VK_IMAGE_USAGE_SAMPLED_BIT,
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
                        .levelCount     = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount     = VK_REMAINING_ARRAY_LAYERS,
                    },
                },
                .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            },
            .sampler = sampler,
        },
    };
}

static ShaderDataHnd CreateTexture(Stack* perm_stack, const char* image_path)
{
    ImageData image_data = {};
    LoadImageData(&image_data, image_path);

    ShaderDataInfo info = DefaultTextureInfo(GetSwapchain()->image_format, render_state.sampler, &image_data);
    ShaderDataHnd shader_data_hnd = CreateShaderData(perm_stack, &info);

    // Copy image data into staging buffer.
    Clear(render_state.buffer.staging);
    Write(render_state.buffer.staging, image_data.data, image_data.size);
    uint32 image_count = GetImageCount(shader_data_hnd);
    for (uint32 i = 0; i < image_count; ++i)
    {
        WriteToShaderDataImage(shader_data_hnd, i, render_state.buffer.staging);
    }

    DestroyImageData(&image_data);

    return shader_data_hnd;
}

static void InitShaderDatas(Stack* perm_stack)
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
                .parent_hnd = render_state.buffer.host,
                .size       = sizeof(VSBuffer),
            }
        };
        render_state.data.vs_buffer = CreateShaderData(perm_stack, &info);
    }

    render_state.data.axis_cube_texture  = CreateTexture(perm_stack, "images/axis_cube.png");
    render_state.data.dirt_block_texture = CreateTexture(perm_stack, "images/dirt_block.png");
}

static void InitShaderDataSets(Stack* perm_stack, Stack temp_stack)
{
    // data_set.vs_buffer
    {
        ShaderDataHnd datas[] =
        {
            render_state.data.vs_buffer,
        };
        render_state.data_set.entity_data = CreateShaderDataSet(perm_stack, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    // data_set.axis_cube_texture
    {
        ShaderDataHnd datas[] =
        {
            render_state.data.axis_cube_texture,
        };
        render_state.data_set.axis_cube_texture = CreateShaderDataSet(perm_stack, temp_stack, CTK_WRAP_ARRAY(datas));
    }

    // data_set.dirt_block_texture
    {
        ShaderDataHnd datas[] =
        {
            render_state.data.dirt_block_texture,
        };
        render_state.data_set.dirt_block_texture = CreateShaderDataSet(perm_stack, temp_stack, CTK_WRAP_ARRAY(datas));
    }
}

static void InitShaders(Stack temp_stack)
{
    render_state.shader.vert_3d = CreateShader(temp_stack, "shaders/bin/3d.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    render_state.shader.frag_3d = CreateShader(temp_stack, "shaders/bin/3d.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

static void InitPipelines(Stack temp_stack, FreeList* free_list)
{
    // Pipeline Layout
    ShaderDataSetHnd shader_data_sets[] =
    {
        render_state.data_set.entity_data,
        render_state.data_set.axis_cube_texture,
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
    VkExtent2D swapchain_extent = GetSwapchain()->extent;
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
    ShaderHnd shaders[] =
    {
        render_state.shader.vert_3d,
        render_state.shader.frag_3d,
    };
    PipelineInfo pipeline_info =
    {
        .shaders           = CTK_WRAP_ARRAY(shaders),
        .viewports         = CTK_WRAP_ARRAY(viewports),
        .vertex_layout     = &render_state.vertex_layout,
        .depth_testing     = true,
        .render_target_hnd = render_state.render_target.main,
    };

    render_state.pipeline.main = CreatePipeline(temp_stack, free_list, &pipeline_info, &pipeline_layout_info);
}

static void InitMeshes()
{
    MeshDataInfo mesh_data_info =
    {
        .parent_buffer_hnd  = render_state.buffer.host,
        .vertex_buffer_size = Megabyte32<1>(),
        .index_buffer_size  = Megabyte32<1>(),
    };
    render_state.mesh.data = CreateMeshData(&mesh_data_info);

    {
        #include "rtk/meshes/cube.h"
        render_state.mesh.cube = CreateMesh(render_state.mesh.data,
                                             CTK_WRAP_ARRAY(vertexes),
                                             CTK_WRAP_ARRAY(indexes));
    }

    {
        #include "rtk/meshes/quad_3d.h"
        render_state.mesh.quad = CreateMesh(render_state.mesh.data,
                                             CTK_WRAP_ARRAY(vertexes),
                                             CTK_WRAP_ARRAY(indexes));
    }
}

template<typename StateType>
static void InitThreadPoolJob(Job<StateType>* job, Stack* perm_stack, uint32 thread_count)
{
    InitArrayFull(&job->states, perm_stack, thread_count);
    InitArrayFull(&job->tasks, perm_stack, thread_count);
}

static void InitThreadPoolJobs(Stack* perm_stack, ThreadPool* thread_pool)
{
    InitThreadPoolJob(&render_state.job.update_mvp_matrixes, perm_stack, thread_pool->size);
    InitThreadPoolJob(&render_state.job.record_render_commands, perm_stack, RENDER_THREAD_COUNT);
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
    VkExtent2D swapchain_extent = GetSwapchain()->extent;
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
    VkCommandBuffer command_buffer = BeginRenderCommands(render_state.render_target.main, state->thread_index);
        PipelineHnd pipeline = render_state.pipeline.main;
        BindPipeline(command_buffer, pipeline);
        BindShaderDataSet(command_buffer, render_state.data_set.entity_data, pipeline, 0);
        BindShaderDataSet(command_buffer, state->texture, pipeline, 1);
        BindMeshData(command_buffer, render_state.mesh.data);
        DrawMesh(command_buffer, state->mesh, state->batch_range.start, state->batch_range.size);
    EndRenderCommands(command_buffer);
}

static void UpdateAllPipelines(FreeList* free_list)
{
    VkExtent2D swapchain_extent = GetSwapchain()->extent;
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
    UpdateViewports(render_state.pipeline.main, free_list, CTK_WRAP_ARRAY(viewports));
}

static void UpdateAllRenderTargets(Stack temp_stack, FreeList* free_list)
{
    UpdateRenderTarget(render_state.render_target.main, temp_stack, free_list);
}

/// Interface
////////////////////////////////////////////////////////////
static void InitRenderState(Stack* perm_stack, Stack temp_stack, FreeList* free_list, ThreadPool* thread_pool)
{
    InitBuffers();
    InitRenderTargets(temp_stack, free_list);
    InitVertexLayout(perm_stack);
    InitSampler();
    InitShaderDatas(perm_stack);
    InitShaderDataSets(perm_stack, temp_stack);
    InitShaders(temp_stack);
    InitPipelines(temp_stack, free_list);
    InitMeshes();
    InitThreadPoolJobs(perm_stack, thread_pool);
}

static void UpdateMVPMatrixes(ThreadPool* thread_pool)
{
    Job<MVPMatrixState>* job = &render_state.job.update_mvp_matrixes;
    Matrix view_projection_matrix = GetViewProjectionMatrix(&game.view);
    auto frame_vs_buffer = GetBufferMem<VSBuffer>(render_state.data.vs_buffer);
    uint32 thread_count = thread_pool->size;

    // Initialize thread states and submit tasks.
    for (uint32 thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        MVPMatrixState* state = GetPtr(&job->states, thread_index);
        state->batch_range            = GetBatchRange(thread_index, thread_count, game.entity_data.count);
        state->view_projection_matrix = view_projection_matrix;
        state->frame_vs_buffer        = frame_vs_buffer;
        state->entity_data            = &game.entity_data;

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
    Job<RenderCommandState>* job = &render_state.job.record_render_commands;

    // Initialize thread states and submit tasks.
    static ShaderDataSetHnd textures[] =
    {
        render_state.data_set.axis_cube_texture,
        render_state.data_set.dirt_block_texture,
    };
    static MeshHnd meshes[] =
    {
        render_state.mesh.cube,
        render_state.mesh.quad,
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
            state->batch_range  = GetBatchRange(thread_index, THREAD_COUNT, game.entity_data.count);

            Set(&job->tasks, thread_index, SubmitTask(thread_pool, state, RecordRenderCommandsThread));
        }
    }

    // Wait for tasks to complete.
    for (uint32 i = 0; i < THREAD_COUNT; ++i)
    {
        Wait(thread_pool, Get(&job->tasks, i));
    }
}

}
