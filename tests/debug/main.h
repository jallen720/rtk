#pragma once

#include <windows.h>
#include "ctk3/ctk3.h"
#include "ctk3/stack.h"
#include "ctk3/free_list.h"
#include "ctk3/thread_pool.h"
#include "ctk3/profile.h"
#include "ctk3/window.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk.h"

#include "rtk/tests/shared.h"
#include "rtk/tests/debug/defs.h"

using namespace CTK;
using namespace RTK;

namespace TestDebug
{

struct RenderState
{
    DeviceStack*   host_stack;
    DeviceStack*   device_stack;
    Buffer*        staging_buffer;
    ImageMemory*   texture_memory;
    RenderTarget*  render_target;
    ShaderData*    textures;
    ShaderData*    entity_buffer;
    ShaderDataSet* shader_data_set;
    Shader*        vert_shader;
    Shader*        frag_shader;
    Pipeline*      pipeline;
    MeshData*      mesh_data;
    Mesh*          quad_mesh;
};

struct EntityBuffer
{
    Vec4<float32> position[MAX_ENTITIES];
    uint32        texture_index[MAX_ENTITIES];
};

static void WriteImageToTexture(ShaderData* sd, uint32 index, Buffer* staging_buffer, const char* image_path)
{
    // Load image data and write to staging buffer.
    ImageData image_data = {};
    LoadImageData(&image_data, image_path);
    WriteHostBuffer(staging_buffer, image_data.data, (VkDeviceSize)image_data.size);
    DestroyImageData(&image_data);

    // Copy image data in staging buffer to shader data images.
    uint32 image_count = GetImageCount(sd);
    for (uint32 i = 0; i < image_count; ++i)
    {
        WriteToShaderDataImage(sd, 0, index, staging_buffer);
    }
}

static RenderState* CreateRenderState(Stack* perm_stack, Stack* temp_stack, FreeList* free_list)
{
    Stack frame = CreateFrame(temp_stack);

    auto rs = Allocate<RenderState>(perm_stack, 1);

    // Device Memory
    DeviceStackInfo host_stack_info =
    {
        .size               = Megabyte32<16>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    rs->host_stack = CreateDeviceStack(&perm_stack->allocator, &host_stack_info);
    DeviceStackInfo device_stack_info =
    {
        .size               = Megabyte32<16>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    rs->device_stack = CreateDeviceStack(&perm_stack->allocator, &device_stack_info);
    rs->staging_buffer = CreateBuffer(&perm_stack->allocator, rs->host_stack, Megabyte32<4>());
    ImageMemoryInfo image_mem_info =
    {
        .image_info =
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
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .max_image_count    = 4,
    };
    rs->texture_memory = CreateImageMemory(&perm_stack->allocator, &image_mem_info, NULL);

    // Render Target
    VkClearValue attachment_clear_values[] = { { .color = { 0.0f, 0.1f, 0.2f, 1.0f } } };
    RenderTargetInfo rt_info =
    {
        .depth_testing           = false,
        .attachment_clear_values = CTK_WRAP_ARRAY(attachment_clear_values),
    };
    rs->render_target = CreateRenderTarget(&perm_stack->allocator, &frame, free_list, &rt_info);

    // Shader Data
    ShaderDataInfo entity_buffer_info =
    {
        .stages    = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .type      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .per_frame = false,
        .count     = 1,
        .buffer =
        {
            .stack = rs->host_stack,
            .size  = sizeof(EntityBuffer),
        },
    };
    rs->entity_buffer = CreateShaderData(&perm_stack->allocator, &entity_buffer_info);

    VkSamplerCreateInfo texture_sampler_info =
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
    VkSampler texture_sampler = CreateSampler(&texture_sampler_info);
    ShaderDataInfo texture_info =
    {
        .stages    = VK_SHADER_STAGE_FRAGMENT_BIT,
        .type      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .per_frame = false,
        .count     = 2,
        .image =
        {
            .memory  = rs->texture_memory,
            .sampler = texture_sampler,
            .view =
            {
                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext    = NULL,
                .flags    = 0,
                .image    = VK_NULL_HANDLE,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = GetSwapchain()->surface_format.format,
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
        },
    };
    rs->textures = CreateShaderData(&perm_stack->allocator, &texture_info);
    WriteImageToTexture(rs->textures, 0, rs->staging_buffer, "images/axis_cube.png");
    WriteImageToTexture(rs->textures, 1, rs->staging_buffer, "images/dirt_block.png");

    ShaderData* shader_datas[] =
    {
        rs->entity_buffer,
        rs->textures,
    };
    rs->shader_data_set = CreateShaderDataSet(&perm_stack->allocator, &frame, CTK_WRAP_ARRAY(shader_datas));

    // Shaders
    rs->vert_shader = CreateShader(&perm_stack->allocator, &frame, "shaders/bin/debug.vert.spv",
                                   VK_SHADER_STAGE_VERTEX_BIT);
    rs->frag_shader = CreateShader(&perm_stack->allocator, &frame, "shaders/bin/debug.frag.spv",
                                   VK_SHADER_STAGE_FRAGMENT_BIT);

    // Pipeline
    Shader* shaders[] = { rs->vert_shader, rs->frag_shader, };
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
    VertexLayout vertex_layout = {};
    InitArray(&vertex_layout.bindings, &frame.allocator, 1);
    PushBinding(&vertex_layout, VK_VERTEX_INPUT_RATE_VERTEX);
    InitArray(&vertex_layout.attributes, &frame.allocator, 2);
    PushAttribute(&vertex_layout, 3, ATTRIBUTE_TYPE_FLOAT32); // Position
    PushAttribute(&vertex_layout, 2, ATTRIBUTE_TYPE_FLOAT32); // UV
    PipelineInfo pipeline_info =
    {
        .shaders       = CTK_WRAP_ARRAY(shaders),
        .viewports     = CTK_WRAP_ARRAY(viewports),
        .vertex_layout = &vertex_layout,
        .depth_testing = false,
        .render_target = rs->render_target,
    };
    ShaderDataSet* shader_data_sets[] = { rs->shader_data_set };
    // VkPushConstantRange push_constant_ranges[] =
    // {
    //     {
    //         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    //         .offset     = 0,
    //         .size       = 4,
    //     },
    // };
    PipelineLayoutInfo pipeline_layout_info =
    {
        .shader_data_sets     = CTK_WRAP_ARRAY(shader_data_sets),
        // .push_constant_ranges = CTK_WRAP_ARRAY(push_constant_ranges),
    };
    rs->pipeline = CreatePipeline(&perm_stack->allocator, &frame, free_list, &pipeline_info, &pipeline_layout_info);

    // Meshes
    MeshDataInfo mesh_data_info =
    {
        .vertex_buffer_size = Megabyte32<1>(),
        .index_buffer_size  = Megabyte32<1>(),
    };
    rs->mesh_data = CreateMeshData(&perm_stack->allocator, rs->device_stack, &mesh_data_info);
    {
        #include "rtk/meshes/quad.h"
        rs->quad_mesh = CreateDeviceMesh(&perm_stack->allocator, rs->mesh_data,
                                         CTK_WRAP_ARRAY(vertexes), CTK_WRAP_ARRAY(indexes),
                                         rs->staging_buffer);
    }

    return rs;
}

static void Run()
{
    Stack* perm_stack = CreateStack(&win32_allocator, Megabyte32<8>());
    Stack* temp_stack = CreateStack(&perm_stack->allocator, Megabyte32<1>());
    FreeList* free_list = CreateFreeList(&perm_stack->allocator, Kilobyte32<16>(), 16, 256);

    ///
    /// Initialization
    ///

    // Make win32 process DPI aware so windows scale properly.
    SetProcessDPIAware();

    WindowInfo window_info =
    {
        .x        = 0,
        .y        = 90, // Taskbar height @ 1.5x zoom (laptop).
        .width    = 1080,
        .height   = 720,
        .title    = "Debug Test",
        .callback = DefaultWindowCallback,
    };
    OpenWindow(&window_info);

    // Init RTK Context + Resources
    ContextInfo context_info = {};
    context_info.instance_info.application_name       = "Debug Test";
    context_info.instance_info.extensions             = {};
#ifdef RTK_ENABLE_VALIDATION
    context_info.instance_info.debug_callback         = DefaultDebugCallback;
    context_info.instance_info.debug_message_severity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    context_info.instance_info.debug_message_type     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
#endif

    DeviceFeatures enabled_features[] =
    {
        DeviceFeatures::GEOMETRY_SHADER,
        DeviceFeatures::SAMPLER_ANISOTROPY,
        DeviceFeatures::SCALAR_BLOCK_LAYOUT,
    };
    context_info.enabled_features = CTK_WRAP_ARRAY(enabled_features);

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
    context_info.descriptor_pool_sizes = CTK_WRAP_ARRAY(descriptor_pool_sizes),

    context_info.render_thread_count = 1,
    InitContext(perm_stack, temp_stack, free_list, &context_info);

    ///
    /// Test
    ///
    RenderState* rs = CreateRenderState(perm_stack, temp_stack, free_list);

// VkImageCreateFlags test_flags  = 0;
// VkFormat           test_format = GetSwapchain()->surface_format.format;
// VkImageTiling      test_tiling = VK_IMAGE_TILING_OPTIMAL;
// VkImageUsageFlags  test_usage  = VK_IMAGE_USAGE_SAMPLED_BIT;
// VkImageCreateInfo  default_info =
// {
//     .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
//     .pNext                 = NULL,
//     .flags                 = test_flags,
//     .imageType             = VK_IMAGE_TYPE_2D,
//     .format                = test_format,
//     .extent                = { 16, 16, 1 },
//     .mipLevels             = 1,
//     .arrayLayers           = 1,
//     .samples               = VK_SAMPLE_COUNT_1_BIT,
//     .tiling                = test_tiling,
//     .usage                 = test_usage,
//     .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
//     .queueFamilyIndexCount = 0,
//     .pQueueFamilyIndices   = NULL,
//     .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
// };
// static constexpr uint32 IMAGE_COUNT = 8;
// VkImageCreateInfo image_create_infos[IMAGE_COUNT] = {};
// uint32 size = 16;
// for (uint32 i = 0; i < IMAGE_COUNT; ++i)
// {
//     image_create_infos[i] = default_info;
//     image_create_infos[i].extent = { size, size, 1 };
//     size *= 2;
// }

// InitImageState(&perm_stack->allocator, IMAGE_COUNT);
// CTK_ITERATE_PTR(info, image_create_infos, IMAGE_COUNT)
// {
//     CreateImage(info);
// }
// BackImagesWithMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
// LogImageState();

    // Initialize entities.
    auto entity_buffer = GetCurrentFrameBufferMem<EntityBuffer>(rs->entity_buffer, 0u);
    for (uint32 i = 0; i < 2; ++i)
    {
        entity_buffer->position[i] = { (float32)i, (float32)i, 0, 1 };
        entity_buffer->texture_index[i] = i;
    }

    for (;;)
    {
        ProcessWindowEvents();
        if (!WindowIsOpen())
        {
            break; // Quit event closed window.
        }

        if (KeyPressed(KEY_ESCAPE))
        {
            CloseWindow();
            break;
        }

        if (!WindowIsActive())
        {
            Sleep(1);
            continue;
        }

        VkResult next_frame_result = NextFrame();
        if (next_frame_result != VK_SUCCESS)
        {
            CTK_FATAL("next_frame_result != VK_SUCCESS");
        }

        VkCommandBuffer command_buffer = BeginRenderCommands(rs->render_target, 0);
            BindPipeline(command_buffer, rs->pipeline);
            BindShaderDataSet(command_buffer, rs->shader_data_set, rs->pipeline, 0);
            BindMeshData(command_buffer, rs->mesh_data);
            DrawMesh(command_buffer, rs->quad_mesh, 0, 2);
        EndRenderCommands(command_buffer);

        SubmitRenderCommands(rs->render_target);
    }
}

}
