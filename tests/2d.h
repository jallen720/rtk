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

struct NDCCoordinates
{
    float32 x;
    float32 y;
    float32 width;
    float32 height;
};

struct VSBuffer
{
    NDCCoordinates ndc_coordinates[MAX_ENTITIES];
};

struct Game
{
    uint32 _;
};

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
    CTK_UNUSED(game);
    Controls(window);
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
            UpdateMVPMatrixes(rs, game, thread_pool);
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "RecordRenderCommands()");
            RecordRenderCommands(game, rs, thread_pool);
            EndProfile(prof_mgr);

            StartProfile(prof_mgr, "SubmitRenderCommands()");
            SubmitRenderCommands(rs->render_target.main);
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
