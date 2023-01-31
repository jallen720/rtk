#pragma once

#include "rtk/vulkan.h"
#include "rtk/context.h"
#include "rtk/resources.h"
#include "rtk/render_target.h"
#include "rtk/pipeline.h"
#include "rtk/shader_data.h"
#include "rtk/mesh_data.h"
#include "ctk3/ctk3.h"
#include "ctk3/array.h"

using namespace CTK;

namespace RTK
{

/// Interface
////////////////////////////////////////////////////////////
static VkResult NextFrame()
{
    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;

    // Get next frame and wait for it to be finished (if still in-progress) before proceeding.
    Frame* frame = Next(&global_ctx.frames);

    res = vkWaitForFences(device, 1, &frame->in_progress, VK_TRUE, UINT64_MAX);
    Validate(res, "vkWaitForFences() failed");

    res = vkResetFences(device, 1, &frame->in_progress);
    Validate(res, "vkResetFences() failed");

    // Once frame is ready, aquire next swapchain image's index.
    res = vkAcquireNextImageKHR(device, global_ctx.swapchain.hnd, UINT64_MAX, frame->image_acquired, VK_NULL_HANDLE,
                                &frame->swapchain_image_index);
    if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
    {
PrintLine("vkAcquireNextImageKHR() returned %s", res == VK_SUBOPTIMAL_KHR ? "VK_SUBOPTIMAL_KHR" : "VK_ERROR_OUT_OF_DATE_KHR");
        // Recreate swapchain.
        vkDeviceWaitIdle(global_ctx.device);
        // vkDestroySwapchainKHR(global_ctx.device, global_ctx.swapchain.hnd, NULL);
    }
    else
    {
        Validate(res, "vkAcquireNextImageKHR() failed");
vkDeviceWaitIdle(global_ctx.device);
    }

    return res;
}

static VkCommandBuffer BeginRenderCommands(RenderTargetHnd render_target_hnd, uint32 render_thread_index)
{
    RenderTarget* render_target = GetRenderTarget(render_target_hnd);
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
    VkCommandBufferBeginInfo command_buffer_begin_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritance_info,
    };
    VkResult res = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    Validate(res, "vkBeginCommandBuffer() failed");

    return command_buffer;
}

static void BindPipeline(VkCommandBuffer command_buffer, PipelineHnd pipeline_hnd)
{
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetPipeline(pipeline_hnd)->hnd);
}

static void BindShaderDataSet(VkCommandBuffer command_buffer, ShaderDataSetHnd shader_data_set_hnd,
                              PipelineHnd pipeline_hnd, uint32 binding)
{
    VkDescriptorSet descriptor_sets[] =
    {
        GetInstance(shader_data_set_hnd, global_ctx.frames.index)
    };
    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            GetPipeline(pipeline_hnd)->layout,
                            binding, // First set
                            CTK_ARRAY_SIZE(descriptor_sets), descriptor_sets, // Descriptor Set
                            0, NULL); // Dynamic Offsets
}

static void BindMeshData(VkCommandBuffer command_buffer, MeshDataHnd mesh_data_hnd)
{
    MeshData* mesh_data = GetMeshData(mesh_data_hnd);
    Buffer* vertex_buffer = GetBuffer(mesh_data->vertex_buffer);
    Buffer* index_buffer = GetBuffer(mesh_data->index_buffer);
    vkCmdBindVertexBuffers(command_buffer,
                           0, // First Binding
                           1, // Binding Count
                           &vertex_buffer->hnd,
                           &vertex_buffer->offset);
    vkCmdBindIndexBuffer(command_buffer,
                         index_buffer->hnd,
                         index_buffer->offset,
                         VK_INDEX_TYPE_UINT32);
}

static void DrawMesh(VkCommandBuffer command_buffer, MeshHnd mesh_hnd, uint32 instance_start, uint32 instance_count)
{
    Mesh* mesh = GetMesh(mesh_hnd);
    vkCmdDrawIndexed(command_buffer,
                     mesh->index_count,   // Index Count
                     instance_count,      // Instance Count
                     mesh->index_offset,  // Index Offset
                     mesh->vertex_offset, // Vertex Offset
                     instance_start);     // First Instance
}

static void EndRenderCommands(VkCommandBuffer command_buffer)
{
    VkResult res = vkEndCommandBuffer(command_buffer);
    Validate(res, "vkEndCommandBuffer() failed");
}

static void SubmitRenderCommands(RenderTargetHnd render_target_hnd)
{
    RenderTarget* render_target = GetRenderTarget(render_target_hnd);
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
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = NULL,
        .renderPass      = render_target->render_pass,
        .framebuffer     = Get(&render_target->framebuffers, frame->swapchain_image_index),
        .renderArea      = render_target->render_area,
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
    res = vkQueueSubmit(global_ctx.graphics_queue, 1, &submit_info, frame->in_progress);
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
    if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // // Recreate swapchain.
        // vkDeviceWaitIdle(global_ctx.device);
        // vkDestroySwapchainKHR(global_ctx.device, global_ctx.swapchain.hnd, NULL);
    }
    else
    {
        Validate(res, "vkAcquireNextImageKHR() failed");
    }
}

}
