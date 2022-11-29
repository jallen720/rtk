#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/profile.h"
#include "stk2/stk.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

#include "rtk/tests/shared.h"
#include "rtk/tests/defs_2d.h"

using namespace CTK;
using namespace STK;
using namespace RTK;

static constexpr uint32 WINDOW_WIDTH = 1080;
static constexpr uint32 WINDOW_HEIGHT = 720;
static constexpr float32 WINDOW_ASPECT_RATIO = (float32)WINDOW_WIDTH / WINDOW_WIDTH;

struct NDCoordinates
{
    float32 x;
    float32 y;
    float32 width;
    float32 height;
};

struct VSBuffer
{
    NDCoordinates nd_coordinates[MAX_ENTITIES];
};

struct FSBuffer
{
    Vec2<uint32> sprite_coords[MAX_ENTITIES];
};

struct Game
{
    Mouse        mouse;
    VertexLayout vertex_layout;
    VkSampler    sampler;

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
    // struct
    // {
    //     ShaderDataHnd axis_cube_texture;
    // }
    // data;
    // struct
    // {
    //     ShaderDataSetHnd entity_data;
    //     ShaderDataSetHnd axis_cube_texture;
    // }
    // data_set;
    struct
    {
        PipelineHnd main;
    }
    pipeline;
    struct
    {
        MeshDataHnd data;
        MeshHnd     quad;
    }
    mesh;
};

struct Vertex
{
    Vec2<float32> position;
    Vec2<uint32>  uv;
};

static void InitVertexLayout(Game* game, Stack* mem)
{
    VertexLayout* vertex_layout = &game->vertex_layout;

    // Init pipeline vertex layout.
    InitArray(&vertex_layout->bindings, mem, 1);
    PushBinding(vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);

    InitArray(&vertex_layout->attributes, mem, 2);
    PushAttribute(vertex_layout, 2); // Position
    PushAttribute(vertex_layout, 2); // UV
}

static void InitSampler(Game* game)
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
    game->sampler = CreateSampler(&info);
}

static void InitBuffers(Game* game)
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
    game->buffer.host = CreateBuffer(&host_buffer_info);

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
    game->buffer.device = CreateBuffer(&device_buffer_info);

    game->buffer.staging = CreateBuffer(game->buffer.host, Megabyte(16));
}

static void InitRenderTargets(Game* game, Stack* mem, Stack temp_mem)
{
    RenderTargetInfo info =
    {
        .depth_testing          = true,
        .color_attachment_count = 1,
    };
    game->render_target.main = CreateRenderTarget(mem, temp_mem, &info);
    PushClearValue(game->render_target.main, { 0.0f, 0.1f, 0.2f, 1.0f });
    PushClearValue(game->render_target.main, { 1.0f });
}

// static void InitShaderDatas(Game* game, Stack* mem)
// {
//     // vs_buffer
//     {
//         ShaderDataInfo info =
//         {
//             .stages    = VK_SHADER_STAGE_VERTEX_BIT,
//             .type      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//             .per_frame = true,
//             .buffer =
//             {
//                 .parent = game->buffer.host,
//                 .size   = sizeof(VSBuffer),
//             }
//         };
//         game->data.vs_buffer = CreateShaderData(mem, &info);
//     }

//     game->data.axis_cube_texture = CreateTexture("images/axis_cube.png", game, mem);
// }

// static void InitShaderDataSets(Game* game, Stack* mem, Stack temp_mem)
// {
//     // data_set.vs_buffer
//     {
//         ShaderDataHnd datas[] =
//         {
//             game->data.vs_buffer,
//         };
//         game->data_set.entity_data = CreateShaderDataSet(mem, temp_mem, WRAP_ARRAY(datas));
//     }

//     // data_set.axis_cube_texture
//     {
//         ShaderDataHnd datas[] =
//         {
//             game->data.axis_cube_texture,
//         };
//         game->data_set.axis_cube_texture = CreateShaderDataSet(mem, temp_mem, WRAP_ARRAY(datas));
//     }
// }

static void InitPipelines(Game* game, Stack temp_mem)
{
    // Pipeline info arrays.
    Shader shaders[] =
    {
        {
            .module = LoadShaderModule(temp_mem, "shaders/bin/2d.vert.spv"),
            .stage  = VK_SHADER_STAGE_VERTEX_BIT
        },
        {
            .module = LoadShaderModule(temp_mem, "shaders/bin/2d.frag.spv"),
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT
        },
    };
    // ShaderDataSetHnd shader_data_sets[] =
    // {
    //     game->data_set.entity_data,
    //     game->data_set.axis_cube_texture,
    // };
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
        .vertex_layout        = &game->vertex_layout,
        .shaders              = WRAP_ARRAY(shaders),
        // .shader_data_sets     = WRAP_ARRAY(shader_data_sets),
        // .push_constant_ranges = WRAP_ARRAY(push_constant_ranges),
    };
    game->pipeline.main = CreatePipeline(temp_mem, game->render_target.main, &pipeline_info);
}

static void InitMeshes(Game* game)
{
    MeshDataInfo mesh_data_info =
    {
        .parent_buffer      = game->buffer.host,
        .vertex_buffer_size = Kilobyte(1),
        .index_buffer_size  = Kilobyte(1),
    };
    game->mesh.data = CreateMeshData(&mesh_data_info);

    {
        #include "rtk/meshes/quad_2d.h"
        game->mesh.quad = CreateMesh(game->mesh.data, WRAP_ARRAY(vertexes), WRAP_ARRAY(indexes));
    }
}

static void InitGame(Game* game, Stack* mem, Stack temp_mem)
{
    InitVertexLayout(game, mem);
    InitSampler(game);
    InitBuffers(game);
    InitRenderTargets(game, mem, temp_mem);
    // InitShaderDatas(game, mem);
    // InitShaderDataSets(game, mem, temp_mem);
    InitPipelines(game, temp_mem);
    InitMeshes(game);
}

static void Controls(Window* window)
{
    if (KeyDown(window, Key::ESCAPE))
    {
        window->open = false;
        return;
    }
}

static void UpdateGame(Game* game, Window* window)
{
    Controls(window);
    UpdateMouse(&game->mouse, window);
}

void TestMain()
{
    Stack* mem = CreateStack(Megabyte(1));
    Stack temp_mem = {};
    InitStack(&temp_mem, mem, Kilobyte(100));

    // Init Win32 + Window
    InitWin32();
    WindowInfo window_info =
    {
        .surface =
        {
            .x      = 0,
            .y      = 90, // Taskbar height.
            .width  = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
        },
        .title    = L"3D Test",
        .callback = DefaultWindowCallback,
    };
    auto window = Allocate<Window>(mem, 1);
    InitWindow(window, &window_info);

    // Init RTK Context + Resources
    VkDescriptorPoolSize descriptor_pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                16 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          16 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          16 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   16 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   16 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         16 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         16 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 16 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 16 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       16 },
    };
    ContextInfo context_info =
    {
        .instance_info =
        {
            .application_name = "RTK 2D Test",
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
        },
        .required_features =
        {
            .as_struct =
            {
                .geometryShader    = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
            },
        },
        .max_physical_devices  = 4,
        .render_thread_count   = 1,
        .descriptor_pool_sizes = WRAP_ARRAY(descriptor_pool_sizes),
    };
    InitContext(mem, temp_mem, window, &context_info);

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
    InitResources(mem, &resources_info);

    auto prof_mgr = Allocate<ProfileManager>(mem, 1);
    InitProfileManager(prof_mgr, mem, 64);

    auto game = Allocate<Game>(mem, 1);
    InitGame(game, mem, temp_mem);

    // Run game.
    while (1)
    {
        StartProfile(prof_mgr, "Frame");

        ProcessEvents(window);
        if (!window->open)
            break; // Quit event closed window.

        if (IsActive(window))
        {
#if 1
            StartProfile(prof_mgr, "UpdateGame()");
            UpdateGame(game, window);
            EndProfile(prof_mgr);
#else
            StartProfile(prof_mgr, "NextFrame()");
            NextFrame();
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "UpdateMVPMatrixes()");
            UpdateMVPMatrixes(game, thread_pool);
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "RecordRenderCommands()");
            RecordRenderCommands(game, thread_pool);
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "SubmitRenderCommands()");
            SubmitRenderCommands(game->render_target.main);
            EndProfile(prof_mgr);
#endif
        }
        else
        {
            Sleep(1);
        }

        EndProfile(prof_mgr);
        PrintProfiles(prof_mgr);
        ClearProfiles(prof_mgr);
    }
}
