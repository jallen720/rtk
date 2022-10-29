#include <windows.h>
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/multithreading.h"
#include "ctk2/math.h"
#include "stk2/stk.h"

#define STB_IMAGE_IMPLEMENTATION
#include "rtk/stb/stb_image.h"

#define RTK_ENABLE_VALIDATION
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

struct Entity
{
    Vec3<float32> position;
    Vec3<float32> rotation;
};

struct EntityData
{
    Entity entities[MAX_ENTITIES];
    uint32 count;
};

struct VSBuffer
{
    Matrix mvp_matrixes[MAX_ENTITIES];
};

struct Game
{
    // Graphics State
    RenderTarget     render_target;
    VertexLayout     vertex_layout;
    Pipeline         pipeline;
    uint32           test_mesh_indexes_offset;
    Buffer           host_buffer;
    Buffer           device_buffer;
    VkDescriptorPool descriptor_pool;
    VkSampler        sampler;
    ShaderData       vs_buffer;
    ShaderData       textures;
    ShaderDataSet    vs_data_set;
    ShaderDataSet    fs_data_set;

    // Sim State
    Mouse      mouse;
    View       view;
    EntityData entity_data;
};

struct Vertex
{
    Vec3<float32> position;
    Vec2<float32> uv;
};

/// Game Functions
////////////////////////////////////////////////////////////
static Entity* Push(EntityData* entity_data, Entity entity)
{
    if (entity_data->count == MAX_ENTITIES)
        CTK_FATAL("can't push another entity: entity count (%u) at max (%u)", entity_data->count, MAX_ENTITIES);

    Entity* new_entity = entity_data->entities + entity_data->count;
    *new_entity = entity;
    ++entity_data->count;
    return new_entity;
}

static void ValidateIndex(EntityData* entity_data, uint32 index)
{
    if (index >= entity_data->count)
        CTK_FATAL("can't access entity at index %u: index exceeds highest index %u", index, entity_data->count - 1);
}

static Entity* GetEntityPtr(EntityData* entity_data, uint32 index)
{
    ValidateIndex(entity_data, index);
    return entity_data->entities + index;
}

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

static void InitRTK(RTKContext* rtk, Stack* mem, Stack temp_mem, Window* window)
{
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
        .max_physical_devices = 8,
        .render_thread_count  = 1,
    };
    InitRTKContext(rtk, mem, temp_mem, window, &rtk_info);
}

static void InitRenderTargets(Game* game, Stack* mem, Stack temp_mem, RTKContext* rtk)
{
    RenderTarget* rt = &game->render_target;
    static constexpr bool DEPTH_TESTING = true;
    InitRenderTarget(rt, mem, temp_mem, rtk, DEPTH_TESTING);
    Set(&rt->attachment_clear_values, 0, { 0.0f, 0.1f, 0.2f, 1.0f });
    Set(&rt->attachment_clear_values, 1, { 1.0f });
}

static void InitVertexLayout(Game* game, Stack* mem)
{
    VertexLayout* vertex_layout = &game->vertex_layout;

    // Init pipeline vertex layout.
    InitArray(&vertex_layout->bindings, mem, 1);
    PushBinding(vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);

    InitArray(&vertex_layout->attributes, mem, 4);
    PushAttribute(vertex_layout, 3); // Position
    PushAttribute(vertex_layout, 2); // UV
}

static void InitDescriptorPool(Game* game, Stack temp_mem, RTKContext* rtk)
{
    VkDescriptorPoolSize pool_sizes[] =
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
    game->descriptor_pool = CreateDescriptorPool(rtk, WRAP_ARRAY(&temp_mem, pool_sizes));
}

static void InitSampler(Game* game, RTKContext* rtk)
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
    VkResult res = vkCreateSampler(rtk->device, &info, NULL, &game->sampler);
    Validate(res, "vkCreateSampler() failed");
}

static void InitShaderDatas(Game* game, Stack* mem, RTKContext* rtk)
{
    InitShaderData(&game->vs_buffer, mem, rtk,
    {
        .stages = VK_SHADER_STAGE_VERTEX_BIT,
        .type   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .buffer_info =
        {
            .size               = sizeof(VSBuffer),
            .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
            .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        },
    });

    // Load image.
    sint32 width;
    sint32 height;
    sint32 channel_count;
    uint8* image_data = stbi_load("images/dir_cube.png", &width, &height, &channel_count, 0);

    // Init shader data for images.
    Swapchain* swapchain = &rtk->swapchain;
    InitShaderData(&game->textures, mem, rtk,
    {
        .stages = VK_SHADER_STAGE_FRAGMENT_BIT,
        .type   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .image_info =
        {
            .image =
            {
                .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext     = NULL,
                .flags     = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = swapchain->image_format,
                .extent =
                {
                    .width  = (uint32)width,
                    .height = (uint32)height,
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
                .format   = swapchain->image_format,
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
            .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        },
        .sampler = game->sampler,
    });

    // Write image to staging buffer.
    sint32 image_data_size = width * height * channel_count;
    for (uint32 i = 0; i < game->textures.images.count; ++i)
        memcpy(GetPtr(&game->textures.images, i)->mapped_mem, image_data, image_data_size);

    // Image data copied to staging buffer, can be freed now.
    stbi_image_free(image_data);

    // Transition texture image layouts to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    BeginTempCommandBuffer(rtk);
        for (uint32 i = 0; i < game->textures.images.count; ++i)
        {
            VkImageMemoryBarrier image_mem_barrier =
            {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask       = 0,
                .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = GetPtr(&game->textures.images, i)->hnd,
                .subresourceRange =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            };
            vkCmdPipelineBarrier(rtk->temp_command_buffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0, // Dependency Flags
                                 0, // Memory Barrier Count
                                 NULL, // Memory Barriers
                                 0, // Buffer Memory Barrier Count
                                 NULL, // Buffer Memory Barriers
                                 1, // Image Memory Count
                                 &image_mem_barrier); // Image Memory Barriers
        }
    SubmitTempCommandBuffer(rtk);
}

static void InitShaderDataSets(Game* game, Stack* mem, Stack temp_mem, RTKContext* rtk)
{
    // vs_data_set
    {
        ShaderData* datas[] =
        {
            &game->vs_buffer,
        };
        InitShaderDataSet(&game->vs_data_set, mem, temp_mem, game->descriptor_pool, rtk, WRAP_ARRAY(&temp_mem, datas));
    }

    // fs_data_set
    {
        ShaderData* datas[] =
        {
            &game->textures,
        };
        InitShaderDataSet(&game->fs_data_set, mem, temp_mem, game->descriptor_pool, rtk, WRAP_ARRAY(&temp_mem, datas));
    }
}

static void InitPipelines(Game* game, Stack temp_mem, RTKContext* rtk)
{
    PipelineInfo pipeline_info = {};
    pipeline_info.vertex_layout = &game->vertex_layout;

    // Load pipeline shaders.
    InitArray(&pipeline_info.shaders, &temp_mem, 2);
    LoadShader(Push(&pipeline_info.shaders), temp_mem, rtk->device, "shaders/bin/3d.vert.spv",
               VK_SHADER_STAGE_VERTEX_BIT);
    LoadShader(Push(&pipeline_info.shaders), temp_mem, rtk->device, "shaders/bin/3d.frag.spv",
               VK_SHADER_STAGE_FRAGMENT_BIT);

    // Init descriptor set layouts.
    InitArray(&pipeline_info.descriptor_set_layouts, &temp_mem, 2);
    Push(&pipeline_info.descriptor_set_layouts, game->vs_data_set.layout);
    Push(&pipeline_info.descriptor_set_layouts, game->fs_data_set.layout);

    // Init push constant ranges.
    InitArray(&pipeline_info.push_constant_ranges, &temp_mem, 1);
    Push(&pipeline_info.push_constant_ranges,
    {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset     = 0,
        .size       = sizeof(Matrix),
    });

    InitPipeline(&game->pipeline, temp_mem, &game->render_target, rtk, &pipeline_info);
}

static void InitGraphicMem(Game* game, RTKContext* rtk)
{
    InitBuffer(&game->host_buffer, rtk,
    {
        .size               = Megabyte(128),
        .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    });
    // InitBuffer(&game->device_buffer, rtk,
    // {
    //     .size               = Megabyte(256),
    //     .sharing_mode       = VK_SHARING_MODE_EXCLUSIVE,
    //     .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
    //                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
    //                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
    //                           VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    //     .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    // });
}

static void LoadMeshData(Game* game)
{
    // Load test mesh into host memory.
    {
        #include "rtk/meshes/cube.h"
        game->test_mesh_indexes_offset = sizeof(vertexes);
        memcpy(game->host_buffer.mapped_mem, vertexes, sizeof(vertexes));
        memcpy(game->host_buffer.mapped_mem + sizeof(vertexes), indexes, sizeof(indexes));
    }
}

static void InitGraphicsState(Game* game, Stack* mem, Stack temp_mem, RTKContext* rtk)
{
    InitRenderTargets(game, mem, temp_mem, rtk);
    InitVertexLayout(game, mem);
    InitDescriptorPool(game, temp_mem, rtk);
    InitSampler(game, rtk);
    InitShaderDatas(game, mem, rtk);
    InitShaderDataSets(game, mem, temp_mem, rtk);
    InitPipelines(game, temp_mem, rtk);
    InitGraphicMem(game, rtk);
    LoadMeshData(game);
}

static void InitSimState(Game* game, Stack* mem, RTKContext* rtk)
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
    static constexpr uint32 CUBE_SIZE = 4;
    static constexpr uint32 CUBE_ENTITY_COUNT = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE;
    static_assert(CUBE_ENTITY_COUNT < MAX_ENTITIES);
    for (uint32 z = 0; z < CUBE_SIZE; ++z)
    for (uint32 y = 0; y < CUBE_SIZE; ++y)
    for (uint32 x = 0; x < CUBE_SIZE; ++x)
    {
        Push(&game->entity_data,
        {
            .position = { x * 1.5f, y * 1.5f, z * 1.5f },
            .rotation = { 0, 0, 0 },
        });
    }
}

static void InitGame(Game* game, Stack* mem, Stack temp_mem, RTKContext* rtk)
{
    InitGraphicsState(game, mem, temp_mem, rtk);
    InitSimState(game, mem, rtk);
}

static void LocalTranslate(View* view, Vec3<float32> translation)
{
    Matrix matrix = ID_MATRIX;
    matrix = RotateX(matrix, view->rotation.x);
    matrix = RotateY(matrix, view->rotation.y);
    matrix = RotateZ(matrix, view->rotation.z);

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
    float32 mod = KeyDown(window, Key::SHIFT) ? 8 : 1;
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

static void UpdateMVPMatrixes(Game* game, RTKContext* rtk)
{
    // Update entity MVP matrixes.
    Matrix view_projection_matrix = CreateViewProjectionMatrix(&game->view);
    auto frame_vs_buffer = GetPtr<VSBuffer>(&game->vs_buffer, rtk->frames.index);
    for (uint32 i = 0; i < game->entity_data.count; ++i)
    {
        Entity* entity = GetEntityPtr(&game->entity_data, i);
        Matrix model_matrix = ID_MATRIX;
        model_matrix = Translate(model_matrix, entity->position);
        model_matrix = RotateX(model_matrix, entity->rotation.x);
        model_matrix = RotateY(model_matrix, entity->rotation.y);
        model_matrix = RotateZ(model_matrix, entity->rotation.z);
        // model_matrix = Scale(model_matrix, entity->scale);

        frame_vs_buffer->mvp_matrixes[i] = view_projection_matrix * model_matrix;
    }
}

static void UpdateGame(Game* game, RTKContext* rtk, Window* window)
{
    UpdateMouse(&game->mouse, window);

    Controls(game, window);
    if (!window->open)
        return; // Controls closed window.

    UpdateMVPMatrixes(game, rtk);
}

static void RecordRenderCommands(Game* game, RTKContext* rtk)
{
    Pipeline* pipeline = &game->pipeline;
    VkDeviceSize vertexes_offset = 0;
    VkDeviceSize indexes_offset = game->test_mesh_indexes_offset;

    VkCommandBuffer render_command_buffer = BeginRecordingRenderCommands(rtk, &game->render_target, 0);
        vkCmdBindPipeline(render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->hnd);

        // Bind mesh data buffers.
        vkCmdBindVertexBuffers(render_command_buffer,
                               0, // First Binding
                               1, // Binding Count
                               &game->host_buffer.hnd,
                               &vertexes_offset);

        vkCmdBindIndexBuffer(render_command_buffer,
                             game->host_buffer.hnd,
                             indexes_offset,
                             VK_INDEX_TYPE_UINT32);\

        BindShaderDataSet(&game->vs_data_set, rtk, pipeline, render_command_buffer, 0);
        BindShaderDataSet(&game->fs_data_set, rtk, pipeline, render_command_buffer, 1);

        for (uint32 i = 0; i < game->entity_data.count; ++i)
        {
            // Matrix* mvp_matrix = GetPtr<VSBuffer>(&game->vs_buffer, rtk->frames.index)->mvp_matrixes + i;
            vkCmdPushConstants(render_command_buffer, pipeline->layout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(Matrix), &i);
            vkCmdDrawIndexed(render_command_buffer,
                             36, // Index Count
                             1, // Instance Count
                             0, // Index Offset
                             0, // Vertex Offset
                             0); // First Instance
        }
    EndRecordingRenderCommands(render_command_buffer);
}

void TestMain()
{
    Stack* mem = CreateStack(Megabyte(4));
    Stack* temp_mem = CreateStack(mem, Megabyte(1));

    auto window = Allocate<Window>(mem, 1);
    InitWin32();
    InitWindow(window,
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
    });

    auto rtk = Allocate<RTKContext>(mem, 1);
    InitRTK(rtk, mem, *temp_mem, window);

    auto game = Allocate<Game>(mem, 1);
    InitGame(game, mem, *temp_mem, rtk);

    // Run game.
    while (1)
    {
        ProcessEvents(window);
        if (!window->open)
            break; // Quit event closed window.

        if (IsActive(window))
        {
            NextFrame(rtk);
            UpdateGame(game, rtk, window);
            RecordRenderCommands(game, rtk);
            SubmitRenderCommands(rtk, &game->render_target);
        }
        else
        {
            Sleep(1);
        }
    }
}
