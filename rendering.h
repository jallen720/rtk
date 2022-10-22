#pragma once

#include "rtk/vulkan.h"
#include "rtk/rtk_context.h"
#include "rtk/render_target.h"
#include "ctk2/ctk.h"
#include "ctk2/containers.h"

using namespace CTK;

namespace RTK
{

/// Interface
////////////////////////////////////////////////////////////
static VkCommandBuffer BeginRecordingRenderCommands(RTKContext* rtk, RenderTarget* render_target,
                                                    uint32 render_thread_index)
{
    Frame* frame = rtk->frame;
    VkCommandBuffer render_command_buffer = Get(&frame->render_command_buffers, render_thread_index);

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
    VkCommandBufferBeginInfo command_buffer_begin_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritance_info,
    };
    VkResult res = vkBeginCommandBuffer(render_command_buffer, &command_buffer_begin_info);
    Validate(res, "vkBeginCommandBuffer() failed");

    return render_command_buffer;
}

static void EndRecordingRenderCommands(VkCommandBuffer render_command_buffer)
{
    VkResult res = vkEndCommandBuffer(render_command_buffer);
    Validate(res, "vkEndCommandBuffer() failed");
}

static void SubmitRenderCommands(RTKContext* rtk, RenderTarget* render_target)
{
    Frame* frame = rtk->frame;
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
        .renderArea =
        {
            .offset = { 0, 0 },
            .extent = rtk->swapchain.extent,
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
    res = vkQueueSubmit(rtk->graphics_queue, 1, &submit_info, frame->in_progress);
    Validate(res, "vkQueueSubmit() failed");

    // Queue swapchain image for presentation.
    VkPresentInfoKHR present_info =
    {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &frame->render_finished,
        .swapchainCount     = 1,
        .pSwapchains        = &rtk->swapchain.hnd,
        .pImageIndices      = &frame->swapchain_image_index,
        .pResults           = NULL,
    };
    res = vkQueuePresentKHR(rtk->present_queue, &present_info);
    Validate(res, "vkQueuePresentKHR() failed");

    vkDeviceWaitIdle(rtk->device);
}

}
