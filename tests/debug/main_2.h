#pragma once

#include <windows.h>
#include "ctk3/ctk3.h"
#include "ctk3/stack.h"
#include "ctk3/free_list.h"
#include "ctk3/window.h"

#define RTK_ENABLE_VALIDATION
#include "rtk/rtk_2.h"

#include "rtk/tests/shared.h"
#include "rtk/tests/debug/defs.h"

using namespace CTK;
using namespace RTK;

namespace TestDebug
{

struct RenderState
{
    BufferStack      host_stack;
    BufferStack      device_stack;
    Buffer           staging_buffer;
    RenderTarget     render_target;
    Buffer           entity_buffer;
    ImageGroupHnd    textures_group;
    Array<ImageHnd>  textures;
    VkSampler        texture_sampler;
    DescriptorSetHnd descriptor_set;
    Shader           vert_shader;
    Shader           frag_shader;
    Pipeline         pipeline;
    MeshData         mesh_data;
    Mesh             quad_mesh;
};

struct EntityBuffer
{
    Vec4<float32> positions[MAX_ENTITIES];
    float32       scales[MAX_ENTITIES];
    uint32        texture_indexes[MAX_ENTITIES];
};

static constexpr const char* TEXTURE_IMAGE_PATHS[] =
{
    "images/axis_cube.png",
    "images/axis_cube_inv.png",
    "images/dirt_block.png",
};
static constexpr uint32 TEXTURE_COUNT = CTK_ARRAY_SIZE(TEXTURE_IMAGE_PATHS);
static_assert(TEXTURE_COUNT == MAX_TEXTURES);

static void InitRenderState(RenderState* rs, Stack* perm_stack, Stack* temp_stack, FreeList* free_list)
{
    Stack frame = CreateFrame(temp_stack);

    InitImageModule(&perm_stack->allocator, 4);
    InitDescriptorSetModule(&perm_stack->allocator, 16);

    BufferStackInfo device_stack_info =
    {
        .size               = Megabyte32<16>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    InitBufferStack(&rs->device_stack, &device_stack_info);
    BufferStackInfo host_stack_info =
    {
        .size               = Megabyte32<16>(),
        .usage_flags        = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    InitBufferStack(&rs->host_stack, &host_stack_info);
    BufferInfo staging_buffer_info =
    {
        .type             = BufferType::BUFFER,
        .size             = Megabyte32<4>(),
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .instance_count   = 1,
    };
    InitBuffer(&rs->staging_buffer, &rs->host_stack, &staging_buffer_info);

    // Render Target
    VkClearValue attachment_clear_values[] = { { .color = { 0.0f, 0.1f, 0.2f, 1.0f } } };
    RenderTargetInfo rt_info =
    {
        .depth_testing           = false,
        .attachment_clear_values = CTK_WRAP_ARRAY(attachment_clear_values),
    };
    InitRenderTarget(&rs->render_target, &frame, free_list, &rt_info);

    // Shaders
    InitShader(&rs->vert_shader, &frame, "shaders/bin/debug.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    InitShader(&rs->frag_shader, &frame, "shaders/bin/debug.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    // Meshes
    MeshDataInfo mesh_data_info =
    {
        .vertex_buffer_size = Megabyte32<1>(),
        .index_buffer_size  = Megabyte32<1>(),
    };
    InitMeshData(&rs->mesh_data, &rs->device_stack, &mesh_data_info);
    {
        #include "rtk/meshes/quad.h"
        InitDeviceMesh(&rs->quad_mesh, &rs->mesh_data, CTK_WRAP_ARRAY(vertexes), CTK_WRAP_ARRAY(indexes),
                       &rs->staging_buffer);
    }

    // Descriptor Datas

    // Entity Buffer
    BufferInfo entity_buffer_info =
    {
        .type             = BufferType::UNIFORM,
        .size             = sizeof(EntityBuffer),
        .offset_alignment = USE_MIN_OFFSET_ALIGNMENT,
        .instance_count   = GetFrameCount(),
    };
    InitBuffer(&rs->entity_buffer, &rs->host_stack, &entity_buffer_info);

    // Textures
    rs->textures_group = CreateImageGroup(&perm_stack->allocator, 4);
    InitArray(&rs->textures, &perm_stack->allocator, TEXTURE_COUNT);
    VkImageCreateInfo texture_create_info =
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
    };
    for (uint32 i = 0; i < TEXTURE_COUNT; ++i)
    {
        Push(&rs->textures, CreateImage(rs->textures_group, &texture_create_info));
    }
    BackWithMemory(rs->textures_group, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    for (uint32 i = 0; i < TEXTURE_COUNT; ++i)
    {
        LoadImage(Get(&rs->textures, i), &rs->staging_buffer, 0, TEXTURE_IMAGE_PATHS[i]);
    }

    // Texture Sampler
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
    VkResult res = vkCreateSampler(GetDevice(), &texture_sampler_info, NULL, &rs->texture_sampler);
    Validate(res, "vkCreateSampler() failed");

    // Descriptor Sets
    DescriptorData descriptor_datas[] =
    {
        {
            .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stages     = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .count      = 1,
            .buffers    = &rs->entity_buffer,
        },
        {
            .type       = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .stages     = VK_SHADER_STAGE_FRAGMENT_BIT,
            .count      = rs->textures.count,
            .image_hnds = rs->textures.data,
        },
        {
            .type       = VK_DESCRIPTOR_TYPE_SAMPLER,
            .stages     = VK_SHADER_STAGE_FRAGMENT_BIT,
            .count      = 1,
            .samplers   = &rs->texture_sampler,
        },
    };
    rs->descriptor_set = CreateDescriptorSet(&perm_stack->allocator, &frame, CTK_WRAP_ARRAY(descriptor_datas));
    AllocateDescriptorSets();
    BindDescriptorSets(&frame);

    // Pipeline
    Shader* shaders[] =
    {
        &rs->vert_shader,
        &rs->frag_shader,
    };
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
        .render_target = &rs->render_target,
    };
    VkDescriptorSetLayout descriptor_set_layouts[] =
    {
        GetLayout(rs->descriptor_set),
    };
    VkPushConstantRange push_constant_ranges[] =
    {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset     = 0,
            .size       = sizeof(uint32),
        },
    };
    PipelineLayoutInfo pipeline_layout_info =
    {
        .descriptor_set_layouts = CTK_WRAP_ARRAY(descriptor_set_layouts),
        .push_constant_ranges   = CTK_WRAP_ARRAY(push_constant_ranges),
    };
    InitPipeline(&rs->pipeline, &frame, free_list, &pipeline_info, &pipeline_layout_info);
}

static void Run()
{
    Stack* perm_stack = CreateStack(&win32_allocator, Megabyte32<8>());
    Stack* temp_stack = CreateStack(&perm_stack->allocator, Megabyte32<1>());
    FreeList* free_list = CreateFreeList(&perm_stack->allocator, Kilobyte32<16>(),
                                         { .chunk_byte_size = 16, .max_range_count = 256 });

    ///
    /// Initialization
    ///

    // Make win32 process DPI aware so windows scales properly.
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
    context_info.instance_info.application_name = "Debug Test";
    context_info.instance_info.api_version      = VK_API_VERSION_1_3;
    context_info.instance_info.extensions       = {};
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
    context_info.render_thread_count = 1;

    InitDeviceFeatures(&context_info.enabled_features);
    context_info.enabled_features.vulkan_1_0.geometryShader                            = VK_TRUE;
    context_info.enabled_features.vulkan_1_0.samplerAnisotropy                         = VK_TRUE;
    context_info.enabled_features.vulkan_1_2.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    context_info.enabled_features.vulkan_1_2.scalarBlockLayout                         = VK_TRUE;

    InitContext(perm_stack, temp_stack, free_list, &context_info);

    ///
    /// Test
    ///
    RenderState rs = {};
    InitRenderState(&rs, perm_stack, temp_stack, free_list);

    // Initialize entities.
    static constexpr uint32 ENTITY_COUNT = MAX_ENTITIES;
    static_assert(ENTITY_COUNT <= MAX_ENTITIES);
    static constexpr float32 SCALE = 2.0f / ENTITY_COUNT;
    for (uint32 frame_index = 0; frame_index < GetFrameCount(); ++frame_index)
    {
        EntityBuffer* entity_buffer = GetMappedMem<EntityBuffer>(&rs.entity_buffer, frame_index);
        for (uint32 i = 0; i < ENTITY_COUNT; ++i)
        {
            entity_buffer->positions[i] = { SCALE * i, SCALE * i, 0, 1 };
            entity_buffer->scales[i] = SCALE;
            entity_buffer->texture_indexes[i] = i % TEXTURE_COUNT;
        }
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

EntityBuffer* entity_buffer = GetMappedMem<EntityBuffer>(&rs.entity_buffer, GetFrameIndex());
static float32 x = 0.0f;
for (uint32 i = 0; i < ENTITY_COUNT; ++i)
{
    entity_buffer->positions[i].y = fabs(sinf(((float32)i / ENTITY_COUNT) + x));
}
x += 0.005f;

        VkCommandBuffer command_buffer = BeginRenderCommands(&rs.render_target, 0);
            DescriptorSetHnd descriptor_sets[] =
            {
                rs.descriptor_set,
            };
            BindDescriptorSets(command_buffer, &rs.pipeline, temp_stack, CTK_WRAP_ARRAY(descriptor_sets), 0);
            BindPipeline(command_buffer, &rs.pipeline);
            BindMeshData(command_buffer, &rs.mesh_data);
            DrawMesh(command_buffer, &rs.quad_mesh, 0, ENTITY_COUNT);
        EndRenderCommands(command_buffer);
        SubmitRenderCommands(&rs.render_target);
    }
}

}
