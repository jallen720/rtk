#include <windows.h>
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/multithreading.h"
#include "ctk/math.h"
#include "stk/stk.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

using namespace ctk;
using namespace stk;
using namespace RTK;

/// Data
////////////////////////////////////////////////////////////
static constexpr u32 MAX_PHYSICAL_DEVICES        = 8;
static constexpr u32 MAX_RENDER_PASS_ATTACHMENTS = 8;
static constexpr u32 MAX_HOST_MEMORY             = megabyte(128);
static constexpr u32 MAX_DEVICE_MEMORY           = megabyte(256);

struct View {
    Vec3<f32> position;
    Vec3<f32> rotation;
    f32       vertical_fov;
    f32       aspect;
    f32       z_near;
    f32       z_far;
    f32       max_x_angle;
};

struct Entity {
    Vec3<f32> position;
    Vec3<f32> rotation;
    Matrix    mvp_matrix;
};

struct Mouse {
    Vec2<s32> position;
    Vec2<s32> delta;
    Vec2<s32> last_position;
};

struct Game {
    Array<Shader>* shaders;
    Pipeline       pipeline;
    View           view;
    Array<Entity>* entities;
    Mouse          mouse;
};

struct Vertex {
    Vec3<f32> position;
    // Vec3<f32> color;
};

/// Test Functions
////////////////////////////////////////////////////////////
static void SelectPhysicalDevice(RTKContext* rtk) {
    // Default to first physical device.
    UsePhysicalDevice(rtk, 0);

    // Use first discrete device if any are available.
    for (u32 i = 0; i < rtk->physical_devices->count; ++i) {
        if (get(rtk->physical_devices, i)->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            UsePhysicalDevice(rtk, i);
            break;
        }
    }

    LogPhysicalDevice(rtk->physical_device, "selected physical device");
}

static void InitRTK(RTKContext* rtk, Stack* mem, Stack temp_mem, Window* window) {
    InitRTKContext(rtk, mem, MAX_PHYSICAL_DEVICES);

    // Initialize RTK instance.
    InitInstance(rtk, {
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
    DeviceFeatures required_features = {
        .as_struct = {
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
    InitRenderPass(rtk);
    InitFramebuffers(rtk, mem);
    static constexpr u32 RENDER_THREAD_COUNT = 1;
    InitRenderCommandState(rtk, mem, RENDER_THREAD_COUNT);
    InitSyncState(rtk);
}

static u32 test_mesh_indexes_offset = 0;

static void InitRenderState(Game* game, Stack* mem, Stack temp_mem, RTKContext* rtk) {
    PipelineInfo pipeline_info = {};

    // Load pipeline shaders.
    init_array(&pipeline_info.shaders, mem, 2);
    LoadShader(push(&pipeline_info.shaders), temp_mem, rtk->device, "shaders/3d.vert.spv",
               VK_SHADER_STAGE_VERTEX_BIT);
    LoadShader(push(&pipeline_info.shaders), temp_mem, rtk->device, "shaders/3d.frag.spv",
               VK_SHADER_STAGE_FRAGMENT_BIT);

    // Init pipeline vertex layout.
    InitVertexLayout(&pipeline_info.vertex_layout, mem, 1, 4);
    PushBinding(&pipeline_info.vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);
    PushAttribute(&pipeline_info.vertex_layout, 3); // Position
    // PushAttribute(&pipeline_info.vertex_layout, 3); // Color

    // Init push constant ranges.
    init_array(&pipeline_info.push_constant_ranges, mem, 1);
    push(&pipeline_info.push_constant_ranges, {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset     = 0,
        .size       = sizeof(Matrix),
    });

    InitPipeline(&game->pipeline, temp_mem, rtk, &pipeline_info);

    // Load test mesh into host memory.
    {
        #include "rtk/meshes/cube.h"
        test_mesh_indexes_offset = sizeof(vertexes);
        memcpy(rtk->host_buffer.mapped_mem, vertexes, sizeof(vertexes));
        memcpy(rtk->host_buffer.mapped_mem + sizeof(vertexes), indexes, sizeof(indexes));
    }
}

static void InitGameState(Game* game, Stack* mem) {
    game->view = {
        .position     = { 0, 0, -1 },
        .rotation     = { 0, 0, 0 },
        .vertical_fov = 90.0f,
        .aspect       = 16.0f / 9.0f,
        .z_near       = 0.1f,
        .z_far        = 1000.0f,
        .max_x_angle  = 85.0f,
    };

    static constexpr u32 CUBE_SIZE = 4;
    static constexpr u32 CUBE_ENTITY_COUNT = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE;
    game->entities = create_array<Entity>(mem, CUBE_ENTITY_COUNT);
    for (u32 x = 0; x < CUBE_SIZE; ++x)
    for (u32 y = 0; y < CUBE_SIZE; ++y)
    for (u32 z = 0; z < CUBE_SIZE; ++z) {
        push(game->entities, {
            .position = { x * 1.5f, y * 1.5f, z * 1.5f },
            .rotation = { 0, 0, 0 },
        });
    }
}

static void InitTest(Game* game, Stack* mem, Stack temp_mem, RTKContext* rtk) {
    InitRenderState(game, mem, temp_mem, rtk);
    InitGameState(game, mem);
}

static void UpdateMouse(Game* game, Window* window) {
    game->mouse.position = get_mouse_position(window);
    game->mouse.delta = game->mouse.position - game->mouse.last_position;
    game->mouse.last_position = game->mouse.position;
}

static void LocalTranslate(View* view, Vec3<f32> translation) {
    Matrix matrix = ID_MATRIX;
    matrix = rotate_x(matrix, view->rotation.x);
    matrix = rotate_y(matrix, view->rotation.y);
    matrix = rotate_z(matrix, view->rotation.z);

    Vec3<f32> forward = { matrix[0][2], matrix[1][2], matrix[2][2] };
    Vec3<f32> right = { matrix[0][0], matrix[1][0], matrix[2][0] };
    view->position += translation.z * forward;
    view->position += translation.x * right;
    view->position.y += translation.y;
}

static void ViewControls(Game* game, Window* window) {
    View* view = &game->view;

    // Translation
    static constexpr f32 BASE_TRANSLATION_SPEED = 0.01f;
    f32 mod = key_down(window, Key::SHIFT) ? 8 : 1;
    f32 translation_speed = BASE_TRANSLATION_SPEED * mod;
    Vec3<f32> translation = {};

    if (key_down(window, Key::W)) { translation.z += translation_speed; }
    if (key_down(window, Key::S)) { translation.z -= translation_speed; }
    if (key_down(window, Key::D)) { translation.x += translation_speed; }
    if (key_down(window, Key::A)) { translation.x -= translation_speed; }
    if (key_down(window, Key::E)) { translation.y += translation_speed; }
    if (key_down(window, Key::Q)) { translation.y -= translation_speed; }

    LocalTranslate(view, translation);

    // Rotation
    if (mouse_button_down(window, 1)) {
        static constexpr f32 ROTATION_SPEED = 0.2f;
        view->rotation.x -= game->mouse.delta.y * ROTATION_SPEED;
        view->rotation.y -= game->mouse.delta.x * ROTATION_SPEED;
        view->rotation.x = clamp(view->rotation.x, -view->max_x_angle, view->max_x_angle);
    }
}

static void Controls(Game* game, Window* window) {
    if (key_down(window, Key::ESCAPE)) {
        window->open = false;
    }

    ViewControls(game, window);
}

static Matrix CreateViewProjectionMatrix(View* view) {
    Matrix view_model_matrix = ID_MATRIX;
    view_model_matrix = rotate_x(view_model_matrix, view->rotation.x);
    view_model_matrix = rotate_y(view_model_matrix, view->rotation.y);
    view_model_matrix = rotate_z(view_model_matrix, view->rotation.z);
    Vec3<f32> forward = {
        .x = view_model_matrix[0][2],
        .y = view_model_matrix[1][2],
        .z = view_model_matrix[2][2],
    };
    Matrix view_matrix = look_at(view->position, view->position + forward, { 0.0f, -1.0f, 0.0f });

    // Projection Matrix
    Matrix projection_matrix = perspective_matrix(view->vertical_fov, view->aspect, view->z_near, view->z_far);

    return projection_matrix * view_matrix;
}

static void UpdateGame(Game* game) {
    // Update entity MVP matrixes.
    Matrix view_projection_matrix = CreateViewProjectionMatrix(&game->view);
    for (u32 i = 0; i < game->entities->count; ++i) {
        Entity* entity = get(game->entities, i);
        Matrix model_matrix = ID_MATRIX;
        model_matrix = translate(ID_MATRIX, entity->position);
        model_matrix = rotate_x(model_matrix, entity->rotation.x);
        model_matrix = rotate_y(model_matrix, entity->rotation.y);
        model_matrix = rotate_z(model_matrix, entity->rotation.z);
        // model_matrix = scale(model_matrix, entity->scale);

        entity->mvp_matrix = view_projection_matrix * model_matrix;
    }
}

static void RecordRenderCommands(Game* game, RTKContext* rtk) {
    VkCommandBuffer render_command_buffer = BeginRecordingRenderCommands(rtk, 0);
        Pipeline* pipeline = &game->pipeline;
        vkCmdBindPipeline(render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->hnd);

        // Bind mesh data buffers.
        VkDeviceSize vertexes_offset = 0;
        vkCmdBindVertexBuffers(render_command_buffer,
                               0, // First Binding
                               1, // Binding Count
                               &rtk->host_buffer.hnd,
                               &vertexes_offset);

        VkDeviceSize indexes_offset = test_mesh_indexes_offset;
        vkCmdBindIndexBuffer(render_command_buffer,
                             rtk->host_buffer.hnd,
                             indexes_offset,
                             VK_INDEX_TYPE_UINT32);

        for (u32 i = 0; i < game->entities->count; ++i) {
            Entity* entity = get(game->entities, i);
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
    vkEndCommandBuffer(render_command_buffer);
}

void test_main() {
    Stack* mem = create_stack(megabyte(4));
    Stack* temp_mem = create_stack(mem, megabyte(1));

    win32_init();
    Window* window = create_window(mem, {
        .surface = {
            .x      = 0,
            .y      = 60, // Taskbar height.
            .width  = 1080,
            .height = 720,
        },
        .title    = L"RTK Test",
        .callback = default_window_callback,
    });

    auto rtk = allocate<RTKContext>(mem, 1);
    InitRTK(rtk, mem, *temp_mem, window);

    auto game = allocate<Game>(mem, 1);
    InitTest(game, mem, *temp_mem, rtk);

    // Run game.
    while (1) {
        process_events(window);
        if (!window->open) {
            // Quit event closed window.
            break;
        }

        UpdateMouse(game, window);
        Controls(game, window);
        if (!window->open) {
            // Controls closed window.
            break;
        }

        if (window_is_active(window)) {
            UpdateGame(game);
            NextFrame(rtk);
            RecordRenderCommands(game, rtk);
            SubmitRenderCommands(rtk);
        }
        else {
            Sleep(1);
        }
    }
}
