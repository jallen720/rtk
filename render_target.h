#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/context.h"
#include "rtk/image.h"
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/containers.h"

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
    bool   depth_testing;
    uint32 color_attachment_count;
};

struct RenderTarget
{
    VkRenderPass         render_pass;
    Array<ImageHnd>      depth_images;
    Array<VkFramebuffer> framebuffers;
    Array<VkClearValue>  attachment_clear_values;
};

/// Utils
////////////////////////////////////////////////////////////
static void InitRenderPassAttachments(RenderPassAttachments* attachments, Stack* mem, uint32 max_color_attachments,
                                      bool depth_attachment)
{
    InitArray(&attachments->descriptions, mem, max_color_attachments + (depth_attachment ? 1 : 0));
    InitArray(&attachments->color, mem, max_color_attachments);
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

static VkAttachmentReference* GetDepthAttachment(RenderPassAttachments* attachments)
{
    return attachments->depth.exists ? &attachments->depth.value : NULL;
}

static void InitRenderPass(RenderTarget* render_target, Stack temp_mem, bool depth_testing)
{
    RenderPassAttachments attachments = {};
    InitRenderPassAttachments(&attachments, &temp_mem, 1, depth_testing);
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

    if (depth_testing)
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
            .pDepthStencilAttachment = GetDepthAttachment(&attachments),
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
}

static void InitDepthImages(RenderTarget* render_target, Stack* mem)
{
    Swapchain* swapchain = &global_ctx.swapchain;
    VkFormat depth_image_format = global_ctx.physical_device->depth_image_format;
    InitArray(&render_target->depth_images, mem, swapchain->image_count);
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
                .width  = swapchain->extent.width,
                .height = swapchain->extent.height,
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
        Push(&render_target->depth_images, CreateImage(&depth_image_info));
}

static void InitFramebuffers(RenderTarget* render_target, Stack* mem, Stack temp_mem, uint32 attachment_count,
                             bool depth_testing)
{
    Swapchain* swapchain = &global_ctx.swapchain;
    InitArray(&render_target->framebuffers, mem, global_ctx.swapchain.image_count);
    auto attachments = CreateArray<VkImageView>(&temp_mem, attachment_count);
    for (uint32 i = 0; i < global_ctx.swapchain.image_count; ++i)
    {
        Push(attachments, Get(&swapchain->image_views, i));

        if (depth_testing)
            Push(attachments, GetImage(Get(&render_target->depth_images, i))->view);

        VkFramebufferCreateInfo info =
        {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = render_target->render_pass,
            .attachmentCount = attachments->count,
            .pAttachments    = attachments->data,
            .width           = swapchain->extent.width,
            .height          = swapchain->extent.height,
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
static RenderTargetHnd CreateRenderTarget(Stack* mem, Stack temp_mem, RenderTargetInfo* info)
{
    RenderTargetHnd render_target_hnd = AllocateRenderTarget();
    RenderTarget* render_target = GetRenderTarget(render_target_hnd);
    InitRenderPass(render_target, temp_mem, info->depth_testing);
    if (info->depth_testing)
        InitDepthImages(render_target, mem);

    uint32 depth_attachment_count = info->depth_testing ? 1 : 0;
    uint32 total_attachment_count = info->color_attachment_count + depth_attachment_count;
    InitFramebuffers(render_target, mem, temp_mem, total_attachment_count, info->depth_testing);
    InitArray(&render_target->attachment_clear_values, mem, total_attachment_count);

    return render_target_hnd;
}

static void PushClearValue(RenderTargetHnd render_target_hnd, VkClearValue clear_value)
{
    Push(&GetRenderTarget(render_target_hnd)->attachment_clear_values, clear_value);
}

}
