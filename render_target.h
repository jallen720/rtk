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
    // Config
    bool   depth_testing;
    uint32 total_attachment_count;

    // State
    VkRenderPass         render_pass;
    VkExtent2D           extent;
    Array<VkFramebuffer> framebuffers;
    Array<VkClearValue>  attachment_clear_values;
    ResourceGroupHnd     depth_image_group;
    ImageMemoryHnd       depth_image_mem;
    ImageHnd             depth_image;
};

/// Utils
////////////////////////////////////////////////////////////
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

static void SetupRenderTarget(RenderTarget* render_target, Stack* temp_stack, FreeList* free_list)
{
    Stack frame = CreateFrame(temp_stack);

    Swapchain* swapchain = GetSwapchain();
    VkDevice device = GetDevice();
    VkResult res = VK_SUCCESS;

    // Set render target to cover entire swapchain extent.
    render_target->extent = swapchain->surface_extent;

    // Create depth image if depth testing is enabled.
    if (render_target->depth_testing)
    {
        ImageInfo depth_image_info =
        {
            .extent =
            {
                .width  = render_target->extent.width,
                .height = render_target->extent.height,
                .depth  = 1
            },
            .type           = VK_IMAGE_TYPE_2D,
            .mip_levels     = 1,
            .array_layers   = 1,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .per_frame      = false,
        };
        ImageMemoryInfo depth_image_mem_info =
        {
            .size       = 0,
            .flags      = 0,
            .usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .format     = GetPhysicalDevice()->depth_image_format,
            .tiling     = VK_IMAGE_TILING_OPTIMAL,
        };
        depth_image_mem_info.size = GetImageSize(&depth_image_info, &depth_image_mem_info);
        render_target->depth_image_mem = CreateImageMemory(render_target->depth_image_group, &depth_image_mem_info);

        AllocateResourceGroup(render_target->depth_image_group);

        ImageViewInfo depth_image_view_info =
        {
            .flags      = 0,
            .type       = VK_IMAGE_VIEW_TYPE_2D,
            .components = RGBA_COMPONENT_SWIZZLE_IDENTITY,
            .subresource_range =
            {
                .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                .baseMipLevel   = 0,
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        render_target->depth_image =
            CreateImage(render_target->depth_image_mem, &depth_image_info, &depth_image_view_info);
    }

    // Init framebuffers.
    InitArray(&render_target->framebuffers, &free_list->allocator, swapchain->image_views.count);
    auto attachments = CreateArray<VkImageView>(&frame.allocator, render_target->total_attachment_count);
    for (uint32 i = 0; i < swapchain->image_views.count; ++i)
    {
        Push(attachments, Get(&swapchain->image_views, i));

        if (render_target->depth_testing)
        {
            Push(attachments, GetImageView(render_target->depth_image, 0));
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
        res = vkCreateFramebuffer(GetDevice(), &info, NULL, Push(&render_target->framebuffers));
        Validate(res, "vkCreateFramebuffer() failed");

        // Clear attachments for next iteration.
        Clear(attachments);
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitRenderTarget(RenderTarget* render_target, Stack* perm_stack, Stack* temp_stack, FreeList* free_list,
                             RenderTargetInfo* info)
{
    Stack frame = CreateFrame(temp_stack);

    render_target->depth_testing = info->depth_testing;
    if (render_target->depth_testing)
    {
        ResourceGroupInfo depth_image_group_info =
        {
            .max_image_mems  = 1,
            .max_images      = 1,
        };
        render_target->depth_image_group = CreateResourceGroup(&free_list->allocator, &depth_image_group_info);
    }

    uint32 depth_attachment_count = render_target->depth_testing ? 1u : 0u;
    uint32 color_attachment_count = 1;
    render_target->total_attachment_count = depth_attachment_count + color_attachment_count;

    // Init Render Pass
    RenderPassAttachments attachments = {};
    InitArray(&attachments.descriptions, &frame.allocator, render_target->total_attachment_count);
    InitArray(&attachments.color, &frame.allocator, color_attachment_count);
    PushColorAttachment(&attachments,
                        {
                            .flags          = 0,
                            .format         = GetSwapchain()->surface_format.format,
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
                               .format         = GetPhysicalDevice()->depth_image_format,
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
    VkResult res = vkCreateRenderPass(GetDevice(), &create_info, NULL, &render_target->render_pass);
    Validate(res, "vkCreateRenderPass() failed");

    // Copy attachment clear values.
    InitArray(&render_target->attachment_clear_values, &perm_stack->allocator, &info->attachment_clear_values);

    // Create depth images and framebuffers based on swapchain extent.
    SetupRenderTarget(render_target, &frame, free_list);
}

static void UpdateRenderTargetAttachments(RenderTarget* render_target, Stack* temp_stack, FreeList* free_list)
{
    // Destroy depth images.
    if (render_target->depth_testing)
    {
        DeallocateResourceGroup(render_target->depth_image_group);
    }

    // Destroy framebuffers.
    for (uint32 i = 0; i < render_target->framebuffers.count; ++i)
    {
        vkDestroyFramebuffer(GetDevice(), Get(&render_target->framebuffers, i), NULL);
    }
    DeinitArray(&render_target->framebuffers, &free_list->allocator);

    // Re-create depth images and framebuffers with new swapchain extent.
    SetupRenderTarget(render_target, temp_stack, free_list);
}
