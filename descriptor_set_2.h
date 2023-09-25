/// Data
////////////////////////////////////////////////////////////
struct DescriptorData
{
    VkDescriptorType   type;
    VkShaderStageFlags stages;
    uint32             count;
    union
    {
        Buffer*    buffers;
        VkSampler* samplers;
        ImageHnd*  image_hnds;
        struct
        {
            ImageHnd* image_hnds;
            VkSampler sampler;
        }
        combined_image_samplers;
    };
};

struct DescriptorSet
{
    Array<VkDescriptorSet> hnds;
    VkDescriptorSetLayout  layout;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitDescriptorSet(DescriptorSet* descriptor_set, Stack* perm_stack, Stack* temp_stack,
                              Array<DescriptorData> datas)
{
    Stack frame = CreateFrame(temp_stack);

    VkResult res = VK_SUCCESS;
    VkDevice device = GetDevice();
    uint32 frame_count = GetFrameCount();

    // Set all layout binding indexes to their index in the array.
    Array<VkDescriptorSetLayoutBinding> layout_bindings = {};
    InitArray(&layout_bindings, &frame.allocator, datas.count);
    for (uint32 i = 0; i < datas.count; ++i)
    {
        DescriptorData* binding = GetPtr(&datas, i);
        Push(&layout_bindings,
             {
                 .binding            = i,
                 .descriptorType     = binding->type,
                 .descriptorCount    = binding->count,
                 .stageFlags         = binding->stages,
                 .pImmutableSamplers = NULL,
             });
    }

    VkDescriptorSetLayoutCreateInfo layout_info =
    {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = NULL,
        .flags        = 0,
        .bindingCount = layout_bindings.count,
        .pBindings    = layout_bindings.data,
    };
    res = vkCreateDescriptorSetLayout(device, &layout_info, NULL, &descriptor_set->layout);
    Validate(res, "vkCreateDescriptorSetLayout() failed");

    // Duplicate layouts for each frame's descriptor set.
    Array<VkDescriptorSetLayout> layouts = {};
    InitArray(&layouts, &frame.allocator, frame_count);
    CTK_REPEAT(frame_count)
    {
        Push(&layouts, descriptor_set->layout);
    }

    // Allocate descriptor sets.
    InitArrayFull(&descriptor_set->hnds, &perm_stack->allocator, frame_count);
    VkDescriptorSetAllocateInfo allocate_info =
    {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = NULL,
        .descriptorPool     = global_ctx.descriptor_pool,
        .descriptorSetCount = layouts.count,
        .pSetLayouts        = layouts.data,
    };
    res = vkAllocateDescriptorSets(device, &allocate_info, descriptor_set->hnds.data);
    Validate(res, "vkAllocateDescriptorSets() failed");

    // Bind datas.
    uint32 buffer_write_count = 0;
    uint32 image_write_count  = 0;
    CTK_ITER(data, &datas)
    {
        if (data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            buffer_write_count += data->count;
        }
        else if (data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                 data->type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                 data->type == VK_DESCRIPTOR_TYPE_SAMPLER)
        {
            image_write_count += data->count;
        }
        else
        {
            CTK_FATAL("unhandled descriptor type: %u", (uint32)data->type);
        }
    }
    auto buffer_infos = CreateArray<VkDescriptorBufferInfo>(&frame.allocator, buffer_write_count * frame_count);
    auto image_infos  = CreateArray<VkDescriptorImageInfo> (&frame.allocator, image_write_count  * frame_count);
    auto writes       = CreateArray<VkWriteDescriptorSet>  (&frame.allocator, buffer_infos->size + image_infos->size);

    for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
    {
        VkDescriptorSet frame_descriptor_set = Get(&descriptor_set->hnds, frame_index);
        for (uint32 binding_index = 0; binding_index < datas.count; ++binding_index)
        {
            DescriptorData* data = GetPtr(&datas, binding_index);
            VkWriteDescriptorSet* write = Push(writes);
            write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->dstSet          = frame_descriptor_set;
            write->dstBinding      = binding_index;
            write->dstArrayElement = 0;
            write->descriptorType  = data->type;
            write->descriptorCount = data->count;

            if (data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            {
                write->pBufferInfo = End(buffer_infos);
                CTK_ITER_PTR(buffer, data->buffers, data->count)
                {
                    uint32 instance_index = buffer->instance_count == frame_count ? frame_index : 0;
                    Push(buffer_infos,
                    {
                        .buffer = buffer->hnd,
                        .offset = buffer->offsets[instance_index],
                        .range  = buffer->size,
                    });
                }
            }
            else if (data->type == VK_DESCRIPTOR_TYPE_SAMPLER)
            {
                write->pImageInfo = End(image_infos);
                CTK_ITER_PTR(sampler, data->samplers, data->count)
                {
                    Push(image_infos,
                    {
                        .sampler     = *sampler,
                        .imageView   = VK_NULL_HANDLE,
                        .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    });
                }
            }
            else if (data->type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
            {
                write->pImageInfo = End(image_infos);
                CTK_ITER_PTR(image_hnd, data->image_hnds, data->count)
                {
                    Image* image = GetImage(*image_hnd);
                    Push(image_infos,
                    {
                        .sampler     = VK_NULL_HANDLE,
                        .imageView   = image->default_view,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    });
                }
            }
            else if (data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            {
                write->pImageInfo = End(image_infos);
                CTK_ITER_PTR(image_hnd, data->combined_image_samplers.image_hnds, data->count)
                {
                    Image* image = GetImage(*image_hnd);
                    Push(image_infos,
                    {
                        .sampler     = data->combined_image_samplers.sampler,
                        .imageView   = image->default_view,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    });
                }
            }
            else
            {
                CTK_FATAL("unhandled descriptor type: %u", (uint32)data->type);
            }
        }
    }

    vkUpdateDescriptorSets(GetDevice(), writes->count, writes->data, 0, NULL);
}

// static void BindDescriptorDatas(DescriptorSet* descriptor_set, Stack* temp_stack, Array<DescriptorData> datas)
// {
//     Stack frame = CreateFrame(temp_stack);

//     // Validate datas match bindings.
//     bool all_descriptor_datas_valid = true;
//     for (uint32 i = 0; i < datas.count; ++i)
//     {
//         DescriptorData* data = GetPtr(&datas, i);
//         if (data->binding_index >= descriptor_set->bindings.count)
//         {
//             CTK_FATAL("descriptor data [%u] binding index %u exceeds descriptor set binding count %u",
//                       i, data->binding_index, descriptor_set->bindings.count);
//         }

//         DescriptorData* binding = GetPtr(&descriptor_set->bindings, data->binding_index);
//         if (data->type != binding->type)
//         {
//             all_descriptor_datas_valid = false;
//             PrintError("descriptor data [%u] type (%s) does not match descriptor binding [%u] type (%s)",
//                        i, VkDescriptorTypeName(data->type), data->binding_index, VkDescriptorTypeName(binding->type));
//         }
//         if (data->count != binding->count)
//         {
//             all_descriptor_datas_valid = false;
//             PrintError("descriptor data [%u] count (%u) does not match descriptor binding [%u] count (%u)",
//                        i, data->count, data->binding_index, binding->count);
//         }
//     }

//     if (!all_descriptor_datas_valid)
//     {
//         CTK_FATAL("can't bind descriptor datas: not all descriptor datas match associated descriptor set bindings");
//     }

//     // Bind datas.
//     uint32 frame_count        = GetFrameCount();
//     uint32 buffer_write_count = 0;
//     uint32 image_write_count  = 0;
//     CTK_ITER(data, &datas)
//     {
//         if (data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
//             data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
//         {
//             buffer_write_count += data->count;
//         }
//         else if (data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
//                  data->type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
//                  data->type == VK_DESCRIPTOR_TYPE_SAMPLER)
//         {
//             image_write_count += data->count;
//         }
//         else
//         {
//             CTK_FATAL("unhandled descriptor type: %u", (uint32)data->type);
//         }
//     }
//     auto buffer_infos = CreateArray<VkDescriptorBufferInfo>(&frame.allocator, buffer_write_count * frame_count);
//     auto image_infos  = CreateArray<VkDescriptorImageInfo> (&frame.allocator, image_write_count  * frame_count);
//     auto writes       = CreateArray<VkWriteDescriptorSet>  (&frame.allocator, buffer_infos->size + image_infos->size);

//     for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
//     {
//         VkDescriptorSet frame_descriptor_set = Get(&descriptor_set->hnds, frame_index);
//         CTK_ITER(data, &datas)
//         {
//             VkWriteDescriptorSet* write = Push(writes);
//             write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//             write->dstSet          = frame_descriptor_set;
//             write->dstBinding      = data->binding_index;
//             write->dstArrayElement = 0;
//             write->descriptorType  = data->type;
//             write->descriptorCount = data->count;

//             if (data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
//                 data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
//             {
//                 write->pBufferInfo = End(buffer_infos);
//                 CTK_ITER_PTR(buffer, data->buffers, data->count)
//                 {
//                     uint32 instance_index = buffer->instance_count == frame_count ? frame_index : 0;
//                     Push(buffer_infos,
//                     {
//                         .buffer = buffer->hnd,
//                         .offset = buffer->offsets[instance_index],
//                         .range  = buffer->size,
//                     });
//                 }
//             }
//             else if (data->type == VK_DESCRIPTOR_TYPE_SAMPLER)
//             {
//                 write->pImageInfo = End(image_infos);
//                 CTK_ITER_PTR(sampler, data->samplers, data->count)
//                 {
//                     Push(image_infos,
//                     {
//                         .sampler     = *sampler,
//                         .imageView   = VK_NULL_HANDLE,
//                         .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
//                     });
//                 }
//             }
//             else if (data->type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
//             {
//                 write->pImageInfo = End(image_infos);
//                 CTK_ITER_PTR(image_hnd, data->image_hnds, data->count)
//                 {
//                     Image* image = GetImage(*image_hnd);
//                     Push(image_infos,
//                     {
//                         .sampler     = VK_NULL_HANDLE,
//                         .imageView   = image->default_view,
//                         .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//                     });
//                 }
//             }
//             else if (data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
//             {
//                 write->pImageInfo = End(image_infos);
//                 CTK_ITER_PTR(image_hnd, data->combined_image_samplers.image_hnds, data->count)
//                 {
//                     Image* image = GetImage(*image_hnd);
//                     Push(image_infos,
//                     {
//                         .sampler     = data->combined_image_samplers.sampler,
//                         .imageView   = image->default_view,
//                         .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//                     });
//                 }
//             }
//             else
//             {
//                 CTK_FATAL("unhandled descriptor type: %u", (uint32)data->type);
//             }
//         }
//     }

//     vkUpdateDescriptorSets(GetDevice(), writes->count, writes->data, 0, NULL);
// }
