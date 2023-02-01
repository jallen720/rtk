#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/context.h"
#include "rtk/image.h"
#include "ctk3/ctk3.h"
#include "ctk3/stack.h"
#include "ctk3/free_list.h"
#include "ctk3/array.h"
#include "ctk3/optional.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct RenderPassAttachments
{
    Array<VkAttachmentDescription>  descriptions;
    Array<VkAttachmentReference>    color;
    Optional<VkAttachmentReference> depth;
};

struct RenderTargetInfo
{
    bool                depth_testing;
    Array<VkClearValue> attachment_clear_values;
};

struct RenderTarget
{
    VkRenderPass         render_pass;
    VkExtent2D           extent;
    Array<Image>         depth_images;
    Array<VkFramebuffer> framebuffers;
    Array<VkClearValue>  attachment_clear_values;

    // Cached Init State
    bool depth_testing;
};

/// Utils
////////////////////////////////////////////////////////////
static void InitRenderPassAttachments(RenderPassAttachments* attachments, Stack temp_stack,
                                      uint32 max_color_attachments, bool depth_attachment)
{
    InitArray(&attachments->descriptions, &temp_stack, max_color_attachments + (depth_attachment ? 1 : 0));
    InitArray(&attachments->color, &temp_stack, max_color_attachments);
}

static void PushColorAttachment(RenderPassAttachments* attachments, VkAttachmentDescription description)
{
    uint32 attachment_index = attachments->descriptions.count;
    Push(&attachments->descriptions, description);
    Push(&attachments->color,
    {
        .attachment = attachment_index,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    });
}

static void SetDepthAttachment(RenderPassAttachments* attachments, VkAttachmentDescription description)
{
    uint32 attachment_index = attachments->descriptions.count;
    Push(&attachments->descriptions, description);
    Set(&attachments->depth,
    {
        .attachment = attachment_index,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    });
}

static void SetupRenderTarget(RenderTarget* render_target, Stack temp_stack, FreeList* free_list)
{
    Swapchain* swapchain = &global_ctx.swapchain;

    // Set render target to cover entire swapchain extent.
    render_target->extent = global_ctx.swapchain.extent;

    // Init depth images if depth testing is enabled.
    if (render_target->depth_testing)
    {
        VkFormat depth_image_format = global_ctx.physical_device->depth_image_format;
        InitArrayFull(&render_target->depth_images, free_list, swapchain->image_count);
        ImageInfo depth_image_info =
        {
            .image =
            {
                .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext     = NULL,
                .flags     = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = depth_image_format,
                .extent =
                {
                    .width  = render_target->extent.width,
                    .height = render_target->extent.height,
                    .depth  = 1
                },
                .mipLevels             = 1,
                .arrayLayers           = 1,
                .samples               = VK_SAMPLE_COUNT_1_BIT,
                .tiling                = VK_IMAGE_TILING_OPTIMAL,
                .usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
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
                .format   = depth_image_format,
                .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            },
            .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };
        for (uint32 i = 0; i < swapchain->image_count; ++i)
        {
            InitImage(GetPtr(&render_target->depth_images, i), &depth_image_info);
        }
    }

    // Init framebuffers.
    uint32 depth_attachment_count = render_target->depth_testing ? 1 : 0;
    uint32 color_attachment_count = 1;
    uint32 total_attachment_count = color_attachment_count + depth_attachment_count;
    InitArray(&render_target->framebuffers, free_list, swapchain->image_count);
    auto attachments = CreateArray<VkImageView>(&temp_stack, total_attachment_count);
    for (uint32 i = 0; i < swapchain->image_count; ++i)
    {
        Push(attachments, Get(&swapchain->image_views, i));

        if (render_target->depth_testing)
        {
            Push(attachments, GetPtr(&render_target->depth_images, i)->view);
        }

        VkFramebufferCreateInfo info =
        {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = render_target->render_pass,
            .attachmentCount = attachments->count,
            .pAttachments    = attachments->data,
            .width           = render_target->extent.width,
            .height          = render_target->extent.height,
            .layers          = 1,
        };
        VkResult res = vkCreateFramebuffer(global_ctx.device, &info, NULL, Push(&render_target->framebuffers));
        Validate(res, "vkCreateFramebuffer() failed");

        // Clear attachments for next iteration.
        Clear(attachments);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static RenderTargetHnd CreateRenderTarget(Stack temp_stack, FreeList* free_list, RenderTargetInfo* info)
{
    RenderTargetHnd render_target_hnd = AllocateRenderTarget();
    RenderTarget* render_target = GetRenderTarget(render_target_hnd);

    render_target->depth_testing = info->depth_testing;

    // Init Render Pass
    RenderPassAttachments attachments = {};
    InitRenderPassAttachments(&attachments, temp_stack, 1, render_target->depth_testing);
    PushColorAttachment(&attachments,
    {
        .flags          = 0,
        .format         = global_ctx.swapchain.image_format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,

        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    });

    if (render_target->depth_testing)
    {
        SetDepthAttachment(&attachments,
        {
            .flags          = 0,
            .format         = global_ctx.physical_device->depth_image_format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,

            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        });
    }

    VkSubpassDescription subpass_descriptions[] =
    {
        {
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = attachments.color.count,
            .pColorAttachments       = attachments.color.data,
            .pDepthStencilAttachment = attachments.depth.exists ? &attachments.depth.value : NULL,
        }
    };
    VkRenderPassCreateInfo create_info =
    {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachments.descriptions.count,
        .pAttachments    = attachments.descriptions.data,
        .subpassCount    = CTK_ARRAY_SIZE(subpass_descriptions),
        .pSubpasses      = subpass_descriptions,
        .dependencyCount = 0,
        .pDependencies   = NULL,
    };
    VkResult res = vkCreateRenderPass(global_ctx.device, &create_info, NULL, &render_target->render_pass);
    Validate(res, "vkCreateRenderPass() failed");

    // Copy attachment clear values.
    InitArray(&render_target->attachment_clear_values, free_list, &info->attachment_clear_values);

    // Create depth images and framebuffers based on swapchain extent.
    SetupRenderTarget(render_target, temp_stack, free_list);

    return render_target_hnd;
}

static void UpdateRenderTarget(RenderTargetHnd render_target_hnd, Stack temp_stack, FreeList* free_list)
{
    RenderTarget* render_target = GetRenderTarget(render_target_hnd);

    // Destory depth images.
    if (render_target->depth_testing)
    {
        for (uint32 i = 0; i < render_target->depth_images.count; ++i)
        {
            DeinitImage(GetPtr(&render_target->depth_images, i));
        }
        DeinitArray(&render_target->depth_images, free_list);
    }

    // Destory framebuffers.
    for (uint32 i = 0; i < render_target->framebuffers.count; ++i)
    {
        vkDestroyFramebuffer(global_ctx.device, Get(&render_target->framebuffers, i), NULL);
    }
    DeinitArray(&render_target->framebuffers, free_list);

    // Re-create depth images and framebuffers with new swapchain extent.
    SetupRenderTarget(render_target, temp_stack, free_list);;
}

}
