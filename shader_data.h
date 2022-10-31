#pragma once

#include "rtk/vulkan.h"
#include "rtk/rtk_context.h"
#include "rtk/memory.h"
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/containers.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
union ShaderDataInfo
{
    BufferInfo buffer_info;
    ImageInfo  image_info;
};

struct ShaderData
{
    VkShaderStageFlags stages;
    VkDescriptorType   type;
    bool               per_frame;
    union
    {
        Array<Buffer> buffers;
        struct
        {
            Array<Image> images;
            VkSampler    sampler;
        };
    };
};

struct ShaderDataSet
{
    VkDescriptorSetLayout  layout;
    Array<VkDescriptorSet> hnds;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitShaderData(ShaderData* shader_data, Stack* mem, RTKContext* rtk, ShaderDataInfo* info)
{
    uint32 instance_count = shader_data->per_frame ? rtk->frames.size : 1;

    if (shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
        shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
    {
        InitArray(&shader_data->buffers, mem, instance_count);
        for (uint32 i = 0; i < instance_count; ++i)
            InitBuffer(Push(&shader_data->buffers), rtk, &info->buffer_info);
    }
    else if (shader_data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
    {
        InitArray(&shader_data->images, mem, instance_count);
        for (uint32 i = 0; i < instance_count; ++i)
            InitImage(Push(&shader_data->images), rtk, &info->image_info);
    }
    else
    {
        CTK_FATAL("can't init shader data: data type %u is unsupported", shader_data->type);
    }
}

template<typename Layout>
static Layout* GetPtr(ShaderData* shader_data, uint32 instance)
{
    return (Layout*)GetPtr(&shader_data->buffers, instance)->mapped_mem;
}

static void InitShaderDataSet(ShaderDataSet* set, Stack* mem, Stack temp_mem, RTKContext* rtk, Array<ShaderData*> datas)
{
    VkDevice device = rtk->device;
    VkResult res = VK_SUCCESS;
    uint32 instance_count = rtk->frames.size;

    // Generate descriptor bindings.
    auto bindings = CreateArray<VkDescriptorSetLayoutBinding>(&temp_mem, datas.count);
    for (uint32 i = 0; i < datas.count; ++i)
    {
        ShaderData* data = Get(&datas, i);
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
    res = vkCreateDescriptorSetLayout(device, &info, NULL, &set->layout);
    Validate(res, "vkCreateDescriptorSetLayout() failed");

    // Duplicate layouts for allocation.
    auto desc_set_alloc_layouts = CreateArray<VkDescriptorSetLayout>(&temp_mem, instance_count);
    for (uint32 i = 0; i < instance_count; ++i)
        Push(desc_set_alloc_layouts, set->layout);

    // Allocate descriptor sets.
    VkDescriptorSetAllocateInfo allocate_info =
    {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = NULL,
        .descriptorPool     = rtk->descriptor_pool,
        .descriptorSetCount = instance_count,
        .pSetLayouts        = desc_set_alloc_layouts->data,
    };
    InitArrayFull(&set->hnds, mem, instance_count);
    res = vkAllocateDescriptorSets(device, &allocate_info, set->hnds.data);
    Validate(res, "vkAllocateDescriptorSets() failed");

    // Bind descriptor data.
    uint32 max_writes = datas.count * instance_count;
    auto buffer_infos = CreateArray<VkDescriptorBufferInfo>(&temp_mem, max_writes);
    auto image_infos = CreateArray<VkDescriptorImageInfo>(&temp_mem, max_writes);
    auto writes = CreateArray<VkWriteDescriptorSet>(&temp_mem, max_writes);

    for (uint32 instance_index = 0; instance_index < instance_count; ++instance_index)
    {
        VkDescriptorSet desc_set = Get(&set->hnds, instance_index);
        for (uint32 data_binding = 0; data_binding < datas.count; ++data_binding)
        {
            ShaderData* data = Get(&datas, data_binding);
            uint32 data_instance_index = data->per_frame ? instance_index : 0;

            VkWriteDescriptorSet* write = Push(writes);
            write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->dstSet          = desc_set;
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
                    .imageView   = GetPtr(&data->images, data_instance_index)->view,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                });
            }
            else
            {
                // Map buffer to buffer info for write.
                Buffer* buffer = GetPtr(&data->buffers, data_instance_index);
                write->pBufferInfo = Push(buffer_infos,
                {
                    .buffer = buffer->hnd,
                    .offset = 0,
                    .range  = buffer->size,
                });
            }
        }
    }

    vkUpdateDescriptorSets(device, writes->count, writes->data, 0, NULL);
}

}
