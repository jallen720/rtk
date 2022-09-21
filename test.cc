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

static constexpr u32 MAX_PHYSICAL_DEVICES = 8;
static constexpr u32 MAX_RENDER_PASS_ATTACHMENTS = 8;
static constexpr u32 MAX_HOST_MEMORY = megabyte(128);
static constexpr u32 MAX_DEVICE_MEMORY = megabyte(256);

struct Test {
    VertexLayout vertex_layout;
    Array<Shader>* shaders;
    Pipeline pipeline;
};

struct Vertex {
    Vec3<f32> position;
    Vec3<f32> color;
};

static void Controls(Window* window) {
    if (key_down(window, Key::ESCAPE)) {
        window->open = false;
    }
}

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

static constexpr Vertex VERTEXES[] = {
    { { -0.5f, 0.5f, 0.0f }, { 1, 0, 0 } },
    { {  0.0f,-0.5f, 0.0f }, { 0, 1, 0 } },
    { {  0.5f, 0.5f, 0.0f }, { 0, 0, 1 } },
};
static constexpr u32 INDEXES[] = { 0, 1, 2 };

static void InitTest(Test* test, Stack* mem, Stack temp_mem, RTKContext* rtk) {
    InitVertexLayout(&test->vertex_layout, mem, 1, 4);
    PushBinding(&test->vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);
    PushAttribute(&test->vertex_layout, 3); // Position
    PushAttribute(&test->vertex_layout, 3); // Color

    test->shaders = create_array<Shader>(mem, 2);
    LoadShader(push(test->shaders), temp_mem, rtk->device, "shaders/test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    LoadShader(push(test->shaders), temp_mem, rtk->device, "shaders/test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    InitPipeline(&test->pipeline, temp_mem, rtk, &test->vertex_layout, test->shaders);

    memcpy(rtk->host_buffer.mapped_mem, VERTEXES, sizeof(VERTEXES));
    memcpy(rtk->host_buffer.mapped_mem + sizeof(VERTEXES), INDEXES, sizeof(INDEXES));
}

static void RecordRenderCommands(Test* test, RTKContext* rtk) {
    VkCommandBuffer render_command_buffer = BeginRecordingRenderCommands(rtk, 0);
        Pipeline* pipeline = &test->pipeline;
        vkCmdBindPipeline(render_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->hnd);

        // Bind mesh data buffers.
        VkDeviceSize vertexes_offset = 0;
        vkCmdBindVertexBuffers(render_command_buffer,
                               0, // First Binding
                               1, // Binding Count
                               &rtk->host_buffer.hnd,
                               &vertexes_offset);

        VkDeviceSize indexes_offset = sizeof(VERTEXES);
        vkCmdBindIndexBuffer(render_command_buffer,
                             rtk->host_buffer.hnd,
                             indexes_offset,
                             VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(render_command_buffer,
                         3, // Index Count
                         1, // Instance Count
                         0, // Index Offset
                         0, // Vertex Offset
                         0); // First Instance
    vkEndCommandBuffer(render_command_buffer);
}

s32 main() {
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

    // // Create threadpool.
    // SYSTEM_INFO system_info = {};
    // GetSystemInfo(&system_info);
    // ThreadPool* thread_pool = create_thread_pool(mem, system_info.dwNumberOfProcessors);

    RTKContext rtk = {};
    InitRTK(&rtk, mem, *temp_mem, window);

    Test test = {};
    InitTest(&test, mem, *temp_mem, &rtk);

    // Run test.
    while (1) {
        process_events(window);
        if (!window->open) {
            // Quit event closed window.
            break;
        }

        Controls(window);
        if (!window->open) {
            // Controls closed window.
            break;
        }

        if (window_is_active(window)) {
            NextFrame(&rtk);
            RecordRenderCommands(&test, &rtk);
            SubmitRenderCommands(&rtk);
        }
        else {
            Sleep(1);
        }
    }
}
