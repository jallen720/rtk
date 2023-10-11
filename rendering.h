/// Interface
////////////////////////////////////////////////////////////
static VkResult NextFrame()
{
    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;

    // Get next frame and wait for it to be finished (if still in-progress) before proceeding.
    Frame* frame = Next(&global_ctx.frames);

    res = vkWaitForFences(device, 1, &frame->command_buffers_complete, VK_TRUE, UINT64_MAX);
    Validate(res, "vkWaitForFences() failed");

    // // Only reset fence after image is successfully acquired to avoid deadlock if swapchain is recreated.
    // No longer need to wait to reset fences until after successful swapchain image acquisition. If image
    // acquisition fails, a dummy vkQueueSubmit() is called which will re-signal the fence.
    res = vkResetFences(device, 1, &frame->command_buffers_complete);
    Validate(res, "vkResetFences() failed");

    // Once frame is ready, aquire next swapchain image's index.
    res = vkAcquireNextImageKHR(device, global_ctx.swapchain.hnd, UINT64_MAX, frame->image_acquired, VK_NULL_HANDLE,
                                &frame->swapchain_image_index);
    if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Submit dummy vkQueueSubmit() call to unsignal frame->image_acquired semaphore so it can be re-used after
        // swapchain is recreated.
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkSubmitInfo submit_info =
        {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                = NULL,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &frame->image_acquired,
            .pWaitDstStageMask    = &wait_stage,
            .commandBufferCount   = 0,
            .pCommandBuffers      = NULL,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores    = NULL,
        };
        VkResult queue_submit_res = vkQueueSubmit(global_ctx.graphics_queue, 1, &submit_info,
                                                  frame->command_buffers_complete);
        Validate(queue_submit_res, "dummy vkQueueSubmit() to unsignal frame->image_acquired semaphore failed");

        // Reset frame so Next() call gives same frame.
        global_ctx.frames.index = global_ctx.frames.index == 0 ? global_ctx.frames.size - 1 : global_ctx.frames.index - 1;

        return res;
    }

    Validate(res, "vkAcquireNextImageKHR() failed");
    return VK_SUCCESS;
}

static VkCommandBuffer BeginRenderCommands(RenderTarget* render_target, uint32 render_thread_index)
{
    Frame* frame = GetCurrentPtr(&global_ctx.frames);
    VkCommandBuffer command_buffer = Get(&frame->render_command_buffers, render_thread_index);

    VkCommandBufferInheritanceInfo inheritance_info =
    {
        .sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .pNext                = NULL,
        .renderPass           = render_target->render_pass,
        .subpass              = 0,
        .framebuffer          = Get(&render_target->framebuffers, frame->swapchain_image_index),
        .occlusionQueryEnable = VK_FALSE,
        .queryFlags           = 0,
        .pipelineStatistics   = 0,
    };
    VkCommandBufferBeginInfo begin_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritance_info,
    };
    VkResult res = vkBeginCommandBuffer(command_buffer, &begin_info);
    Validate(res, "vkBeginCommandBuffer() failed");

    return command_buffer;
}

static void BindPipeline(VkCommandBuffer command_buffer, Pipeline* pipeline)
{
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->hnd);
}

static void BindDescriptorSets(VkCommandBuffer command_buffer, Pipeline* pipeline, Stack* temp_stack,
                               Array<DescriptorSetHnd> descriptor_set_hnds, uint32 first_binding)
{
    Stack frame = CreateFrame(temp_stack);

    // Create array of handles from descriptor sets to pass to vkCmdBindDescriptorSets().
    uint32 frame_index = GetFrameIndex();
    auto descriptor_sets = CreateArray<VkDescriptorSet>(&frame.allocator, descriptor_set_hnds.count);
    for (uint32 i = 0; i < descriptor_set_hnds.count; ++i)
    {
        Push(descriptor_sets, GetFrameSet(Get(&descriptor_set_hnds, i), frame_index));
    };

    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline->layout,
                            first_binding, // First set
                            descriptor_sets->count, descriptor_sets->data, // Descriptor Set
                            0, NULL); // Dynamic Offsets
}

static void BindMeshData(VkCommandBuffer command_buffer, MeshData* mesh_data)
{
    VkDeviceSize vertex_buffer_offset = GetOffset(mesh_data->vertex_buffer, 0);
    vkCmdBindVertexBuffers(command_buffer,
                           0, // First Binding
                           1, // Binding Count
                           &GetBufferMemory(mesh_data->vertex_buffer)->hnd,
                           &vertex_buffer_offset);
    vkCmdBindIndexBuffer(command_buffer,
                         GetBufferMemory(mesh_data->index_buffer)->hnd,
                         GetOffset(mesh_data->index_buffer, 0),
                         VK_INDEX_TYPE_UINT32);
}

static void DrawMesh(VkCommandBuffer command_buffer, Mesh* mesh, uint32 instance_start, uint32 instance_count)
{
    vkCmdDrawIndexed(command_buffer,
                     mesh->index_count,   // Index Count
                     instance_count,      // Instance Count
                     mesh->index_offset,  // Index Offset
                     mesh->vertex_offset, // Vertex Offset (sint32)
                     instance_start);     // First Instance
}

static void EndRenderCommands(VkCommandBuffer command_buffer)
{
    VkResult res = vkEndCommandBuffer(command_buffer);
    Validate(res, "vkEndCommandBuffer() failed");
}

static void SubmitRenderCommands(RenderTarget* render_target)
{
    Frame* frame = GetCurrentPtr(&global_ctx.frames);
    VkResult res = VK_SUCCESS;

    // Record current frame's primary command buffer, including render command buffers.
    VkCommandBuffer command_buffer = frame->primary_render_command_buffer;

    // Begin command buffer.
    VkCommandBufferBeginInfo command_buffer_begin_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = 0,
        .pInheritanceInfo = NULL,
    };
    res = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    Validate(res, "vkBeginCommandBuffer() failed");

    // Begin render pass.
    VkRenderPassBeginInfo render_pass_begin_info =
    {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext       = NULL,
        .renderPass  = render_target->render_pass,
        .framebuffer = Get(&render_target->framebuffers, frame->swapchain_image_index),
        .renderArea  =
        {
            .offset = { 0, 0 },
            .extent = render_target->extent,
        },
        .clearValueCount = render_target->attachment_clear_values.count,
        .pClearValues    = render_target->attachment_clear_values.data,
    };
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // Execute entity render commands.
    vkCmdExecuteCommands(command_buffer, frame->render_command_buffers.count, frame->render_command_buffers.data);

    vkCmdEndRenderPass(command_buffer);

    res = vkEndCommandBuffer(command_buffer);
    Validate(res, "vkEndCommandBuffer() failed");

    // Submit commands for rendering to graphics queue.
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info =
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = NULL,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &frame->image_acquired,
        .pWaitDstStageMask    = &wait_stage,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &frame->render_finished,
    };
    res = vkQueueSubmit(global_ctx.graphics_queue, 1, &submit_info, frame->command_buffers_complete);
    Validate(res, "vkQueueSubmit() failed");

    // Queue swapchain image for presentation.
    VkPresentInfoKHR present_info =
    {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &frame->render_finished,
        .swapchainCount     = 1,
        .pSwapchains        = &global_ctx.swapchain.hnd,
        .pImageIndices      = &frame->swapchain_image_index,
        .pResults           = NULL,
    };
    res = vkQueuePresentKHR(global_ctx.present_queue, &present_info);
    if (res != VK_SUBOPTIMAL_KHR && res != VK_ERROR_OUT_OF_DATE_KHR)
    {
        Validate(res, "vkQueuePresentKHR() failed");
    }
}
