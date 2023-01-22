#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/context.h"
#include "rtk/resources.h"
#include "rtk/buffer.h"
#include "rtk/image.h"
#include "ctk3/ctk3.h"
#include "ctk3/stack.h"
#include "ctk3/array.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct ShaderDataInfo
{
    VkShaderStageFlags stages;
    VkDescriptorType   type;
    bool               per_frame;
    union
    {
        struct
        {
            BufferHnd parent;
            uint32    size;
        }
        buffer;
        struct
        {
            ImageInfo info;
            VkSampler sampler;
        }
        image;
    };
};

struct ShaderData
{
    VkShaderStageFlags stages;
    VkDescriptorType   type;
    bool               per_frame;
    union
    {
        Array<BufferHnd> buffer_hnds;
        struct
        {
            Array<ImageHnd> image_hnds;
            VkSampler       sampler;
        };
    };
};

struct ShaderDataSet
{
    VkDescriptorSetLayout  layout;
    Array<VkDescriptorSet> instances;
};

/// Utils
////////////////////////////////////////////////////////////
static Buffer* GetBuffer(ShaderData* shader_data, uint32 instance)
{
    return GetBuffer(Get(&shader_data->buffer_hnds, instance));
}

static Image* GetImage(ShaderData* shader_data, uint32 instance)
{
    return GetImage(Get(&shader_data->image_hnds, instance));
}

/// Interface
////////////////////////////////////////////////////////////
static ShaderDataHnd CreateShaderData(Stack* perm_stack, ShaderDataInfo* info)
{
    ShaderDataHnd shader_data_hnd = AllocateShaderData();
    ShaderData* shader_data = GetShaderData(shader_data_hnd);

    shader_data->stages    = info->stages;
    shader_data->type      = info->type;
    shader_data->per_frame = info->per_frame;

    uint32 instance_count = shader_data->per_frame ? global_ctx.frames.size : 1;

    if (shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
        shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
    {
        InitArray(&shader_data->buffer_hnds, perm_stack, instance_count);
        for (uint32 i = 0; i < instance_count; ++i)
        {
            Push(&shader_data->buffer_hnds, CreateBuffer(info->buffer.parent, info->buffer.size));
        }
    }
    else if (shader_data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
    {
        shader_data->sampler = info->image.sampler;

        InitArray(&shader_data->image_hnds, perm_stack, instance_count);
        for (uint32 i = 0; i < instance_count; ++i)
        {
            Push(&shader_data->image_hnds, CreateImage(&info->image.info));
        }
    }
    else
    {
        CTK_FATAL("can't init shader data: data type %u is unsupported", shader_data->type);
    }

    return shader_data_hnd;
}

template<typename Type>
static Type* GetBufferMem(ShaderDataHnd shader_data_hnd, uint32 instance)
{
    ShaderData* shader_data = GetShaderData(shader_data_hnd);
    CTK_ASSERT(shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
               shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

    return (Type*)GetBuffer(shader_data, instance)->mapped_mem;
}

static void WriteToShaderDataImage(ShaderDataHnd shader_data_hnd, uint32 instance_index,
                                   BufferHnd image_data_buffer_hnd)
{
    ShaderData* shader_data = GetShaderData(shader_data_hnd);
    Buffer* image_data_buffer = GetBuffer(image_data_buffer_hnd);
    Image* image = GetImage(shader_data, instance_index);

    // Copy image data from buffer memory to image memory.
    BeginTempCommandBuffer();
        // Transition image layout for use in shader.
        VkImageMemoryBarrier image_mem_barrier =
        {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image->hnd,
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        vkCmdPipelineBarrier(global_ctx.temp_command_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Source Stage Mask
                             VK_PIPELINE_STAGE_TRANSFER_BIT, // Destination Stage Mask
                             0, // Dependency Flags
                             0, // Memory Barrier Count
                             NULL, // Memory Barriers
                             0, // Buffer Memory Barrier Count
                             NULL, // Buffer Memory Barriers
                             1, // Image Memory Barrier Count
                             &image_mem_barrier); // Image Memory Barriers

        VkBufferImageCopy copy =
        {
            .bufferOffset      = image_data_buffer->offset,
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .imageOffset =
            {
                .x = 0,
                .y = 0,
                .z = 0,
            },
            .imageExtent = image->extent,
        };
        vkCmdCopyBufferToImage(global_ctx.temp_command_buffer, image_data_buffer->hnd, image->hnd,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        // Transition image layout for use in shader.
        VkImageMemoryBarrier image_mem_barrier2 =
        {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image->hnd,
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        vkCmdPipelineBarrier(global_ctx.temp_command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, // Source Stage Mask
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // Destination Stage Mask
                             0, // Dependency Flags
                             0, // Memory Barrier Count
                             NULL, // Memory Barriers
                             0, // Buffer Memory Barrier Count
                             NULL, // Buffer Memory Barriers
                             1, // Image Memory Barrier Count
                             &image_mem_barrier2); // Image Memory Barriers
    SubmitTempCommandBuffer();
}

static ShaderDataSetHnd CreateShaderDataSet(Stack* perm_stack, Stack temp_mem, Array<ShaderDataHnd> datas)
{
    ShaderDataSetHnd shader_data_set_hnd = AllocateShaderDataSet();
    ShaderDataSet* shader_data_set = GetShaderDataSet(shader_data_set_hnd);

    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;
    uint32 instance_count = global_ctx.frames.size;

    // Generate descriptor bindings.
    auto bindings = CreateArray<VkDescriptorSetLayoutBinding>(&temp_mem, datas.count);
    for (uint32 i = 0; i < datas.count; ++i)
    {
        ShaderData* data = GetShaderData(Get(&datas, i));
        Push(bindings,
        {
            .binding            = i,
            .descriptorType     = data->type,
            .descriptorCount    = 1,
            .stageFlags         = data->stages,
            .pImmutableSamplers = NULL,
        });
    }

    // Create layout from bindings.
    VkDescriptorSetLayoutCreateInfo info =
    {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = NULL,
        .flags        = 0,
        .bindingCount = bindings->count,
        .pBindings    = bindings->data,
    };
    res = vkCreateDescriptorSetLayout(device, &info, NULL, &shader_data_set->layout);
    Validate(res, "vkCreateDescriptorSetLayout() failed");

    // Duplicate layouts for allocation.
    auto desc_set_alloc_layouts = CreateArray<VkDescriptorSetLayout>(&temp_mem, instance_count);
    for (uint32 i = 0; i < instance_count; ++i)
    {
        Push(desc_set_alloc_layouts, shader_data_set->layout);
    }

    // Allocate descriptor sets.
    VkDescriptorSetAllocateInfo allocate_info =
    {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = NULL,
        .descriptorPool     = global_ctx.descriptor_pool,
        .descriptorSetCount = instance_count,
        .pSetLayouts        = desc_set_alloc_layouts->data,
    };
    InitArrayFull(&shader_data_set->instances, perm_stack, instance_count);
    res = vkAllocateDescriptorSets(device, &allocate_info, shader_data_set->instances.data);
    Validate(res, "vkAllocateDescriptorSets() failed");

    // Bind descriptor data.
    uint32 max_writes = datas.count * instance_count;
    auto buffer_infos = CreateArray<VkDescriptorBufferInfo>(&temp_mem, max_writes);
    auto image_infos = CreateArray<VkDescriptorImageInfo>(&temp_mem, max_writes);
    auto writes = CreateArray<VkWriteDescriptorSet>(&temp_mem, max_writes);

    for (uint32 instance_index = 0; instance_index < instance_count; ++instance_index)
    {
        VkDescriptorSet instance_desc_set = Get(&shader_data_set->instances, instance_index);
        for (uint32 data_binding = 0; data_binding < datas.count; ++data_binding)
        {
            ShaderData* data = GetShaderData(Get(&datas, data_binding));
            uint32 data_instance_index = data->per_frame ? instance_index : 0;

            VkWriteDescriptorSet* write = Push(writes);
            write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->dstSet          = instance_desc_set;
            write->dstBinding      = data_binding;
            write->dstArrayElement = 0;
            write->descriptorType  = data->type;
            write->descriptorCount = 1;

            if (data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            {
                // Map combined image sampler to image info for write.
                write->pImageInfo = Push(image_infos,
                {
                    .sampler     = data->sampler,
                    .imageView   = GetImage(data, data_instance_index)->view,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                });
            }
            else
            {
                // Map buffer to buffer info for write.
                Buffer* buffer = GetBuffer(data, data_instance_index);
                write->pBufferInfo = Push(buffer_infos,
                {
                    .buffer = buffer->hnd,
                    .offset = buffer->offset,
                    .range  = buffer->size,
                });
            }
        }
    }

    vkUpdateDescriptorSets(device, writes->count, writes->data, 0, NULL);

    return shader_data_set_hnd;
}

static VkDescriptorSet GetInstance(ShaderDataSetHnd shader_data_set_hnd, uint32 instance)
{
    return Get(&GetShaderDataSet(shader_data_set_hnd)->instances, instance);
}

}
