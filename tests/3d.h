#include <windows.h>
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/multithreading.h"
#include "ctk2/math.h"
#include "stk2/stk.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

#include "rtk/tests/shared.h"

using namespace CTK;
using namespace STK;
using namespace RTK;

/// Data
////////////////////////////////////////////////////////////
static constexpr uint32 MAX_PHYSICAL_DEVICES        = 8;
static constexpr uint32 MAX_RENDER_PASS_ATTACHMENTS = 8;
static constexpr uint32 MAX_HOST_MEMORY             = Megabyte(128);
static constexpr uint32 MAX_DEVICE_MEMORY           = Megabyte(256);
static constexpr uint32 RENDER_THREAD_COUNT         = 1;
static constexpr bool   DEPTH_TESTING               = true;

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
    Matrix        mvp_matrix;
};

struct Game
{
    // Graphics
    RenderTarget render_target;
    VertexLayout vertex_layout;
    Pipeline     pipeline;
    uint32       test_mesh_indexes_offset;

    // Sim State
    View          view;
    Array<Entity> entities;
    Mouse         mouse;
};

struct Vertex
{
    Vec3<float32> position;
};

/// Test Functions
////////////////////////////////////////////////////////////
static void SelectPhysicalDevice(RTKContext* rtk)
{
    // Default to first physical device.
    UsePhysicalDevice(rtk, 0);

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
    InitRTKContext(rtk, mem, MAX_PHYSICAL_DEVICES);

    // Initialize RTK instance.
    InitInstance(rtk,
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
    });
    InitSurface(rtk, window);

    // Initialize device state.
    DeviceFeatures required_features =
    {
        .as_struct =
        {
            .geometryShader    = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
        },
    };
    LoadCapablePhysicalDevices(rtk, temp_mem, &required_features);
    SelectPhysicalDevice(rtk);
    InitDevice(rtk, &required_features);
    InitQueues(rtk);
    GetSurfaceInfo(rtk, mem);
    InitMemory(rtk, MAX_HOST_MEMORY, MAX_DEVICE_MEMORY);
    InitMainCommandState(rtk);

    // Initialize rendering state.
    InitSwapchain(rtk, mem, temp_mem);

    InitRenderCommandPools(rtk, mem, RENDER_THREAD_COUNT);

    uint32 frame_count = rtk->swapchain.image_count + 1;
    InitFrames(rtk, mem, frame_count);
}

static void InitRenderTargets(Game* game, Stack* mem, Stack temp_mem, RTKContext* rtk)
{
    InitRenderTarget(&game->render_target, mem, temp_mem, rtk, DEPTH_TESTING);
}

static void InitVertexLayout(Game* game, Stack* mem)
{
    VertexLayout* vertex_layout = &game->vertex_layout;

    // Init pipeline vertex layout.
    InitArray(&vertex_layout->bindings, mem, 1);
    PushBinding(vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);

    InitArray(&vertex_layout->attributes, mem, 4);
    PushAttribute(vertex_layout, 3); // Position
    // PushAttribute(&pipeline_info.vertex_layout, 3); // Color
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

static void LoadMeshData(Game* game, RTKContext* rtk)
{
    // Load test mesh into host memory.
    {
        #include "rtk/meshes/cube.h"
        game->test_mesh_indexes_offset = sizeof(vertexes);
        memcpy(rtk->host_buffer.mapped_mem, vertexes, sizeof(vertexes));
        memcpy(rtk->host_buffer.mapped_mem + sizeof(vertexes), indexes, sizeof(indexes));
    }
}

static void InitGameState(Game* game, Stack* mem)
{
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

    static constexpr uint32 CUBE_SIZE = 4;
    static constexpr uint32 CUBE_ENTITY_COUNT = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE;
    InitArray(&game->entities, mem, CUBE_ENTITY_COUNT);
    for (uint32 x = 0; x < CUBE_SIZE; ++x)
    for (uint32 y = 0; y < CUBE_SIZE; ++y)
    for (uint32 z = 0; z < CUBE_SIZE; ++z)
    {
        Push(&game->entities,
        {
            .position = { x * 1.5f, y * 1.5f, z * 1.5f },
            .rotation = { 0, 0, 0 },
        });
    }
}

static void InitGame(Game* game, Stack* mem, Stack temp_mem, RTKContext* rtk)
{
    InitRenderTargets(game, mem, temp_mem, rtk);
    InitVertexLayout(game, mem);
    InitPipelines(game, temp_mem, rtk);
    LoadMeshData(game, rtk);
    InitGameState(game, mem);
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

static void UpdateMVPMatrixes(Game* game)
{
    // Update entity MVP matrixes.
    Matrix view_projection_matrix = CreateViewProjectionMatrix(&game->view);
    for (uint32 i = 0; i < game->entities.count; ++i)
    {
        Entity* entity = GetPtr(&game->entities, i);
        Matrix model_matrix = ID_MATRIX;
        model_matrix = Translate(model_matrix, entity->position);
        model_matrix = RotateX(model_matrix, entity->rotation.x);
        model_matrix = RotateY(model_matrix, entity->rotation.y);
        model_matrix = RotateZ(model_matrix, entity->rotation.z);
        // model_matrix = Scale(model_matrix, entity->scale);

        entity->mvp_matrix = view_projection_matrix * model_matrix;
    }
}

static void UpdateGame(Game* game, Window* window)
{
    UpdateMouse(&game->mouse, window);

    Controls(game, window);
    if (!window->open)
        return; // Controls closed window.

    UpdateMVPMatrixes(game);
}

static void RecordRenderCommands(Game* game, RTKContext* rtk)
{
    VkCommandBuffer render_command_buffer = BeginRecordingRenderCommands(rtk, &game->render_target, 0);
        Pipeline* pipeline = &game->pipeline;
        vkCmdBindPipeline(render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->hnd);

        // Bind mesh data buffers.
        VkDeviceSize vertexes_offset = 0;
        vkCmdBindVertexBuffers(render_command_buffer,
                               0, // First Binding
                               1, // Binding Count
                               &rtk->host_buffer.hnd,
                               &vertexes_offset);

        VkDeviceSize indexes_offset = game->test_mesh_indexes_offset;
        vkCmdBindIndexBuffer(render_command_buffer,
                             rtk->host_buffer.hnd,
                             indexes_offset,
                             VK_INDEX_TYPE_UINT32);

        for (uint32 i = 0; i < game->entities.count; ++i)
        {
            Entity* entity = GetPtr(&game->entities, i);
            vkCmdPushConstants(render_command_buffer, pipeline->layout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(entity->mvp_matrix), &entity->mvp_matrix);
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
            UpdateGame(game, window);
            NextFrame(rtk);
            RecordRenderCommands(game, rtk);
            SubmitRenderCommands(rtk, &game->render_target);
        }
        else
        {
            Sleep(1);
        }
    }
}
