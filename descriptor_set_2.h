
/// Data
////////////////////////////////////////////////////////////
struct DescriptorData
{
    VkDescriptorType type;
    uint32           binding_index;
    uint32           count;
    union
    {
        Buffer* buffers;
        struct
        {
            ImageHnd* image_hnds;
            VkSampler sampler;
        };
    };
};

struct DescriptorBinding
{
    VkDescriptorType   type;
    uint32             count;
    VkShaderStageFlags stages;
};

struct DescriptorSet
{
    Array<VkDescriptorSet>   hnds;
    Array<DescriptorBinding> bindings;
    VkDescriptorSetLayout    layout;
};

/// Interface
////////////////////////////////////////////////////////////
static void BindDescriptorDatas(DescriptorSet* descriptor_set, Stack* temp_stack, Array<DescriptorData> descriptor_datas)
{
    Stack frame = CreateFrame(temp_stack);

    // Validate datas match bindings.
    bool all_descriptor_datas_valid = true;
    for (uint32 i = 0; i < descriptor_datas.count; ++i)
    {
        DescriptorData* data = GetPtr(&descriptor_datas, i);
        if (data->binding_index >= descriptor_set->bindings.count)
        {
            CTK_FATAL("descriptor data [%u] binding index %u exceeds descriptor set binding count %u",
                      i, data->binding_index, descriptor_set->bindings.count);
        }

        DescriptorBinding* binding = GetPtr(&descriptor_set->bindings, data->binding_index);
        if (data->type != binding->type)
        {
            all_descriptor_datas_valid = false;
            PrintError("descriptor data [%u] type (%s) does not match descriptor binding [%u] type (%s)",
                       i, VkDescriptorTypeName(data->type), data->binding_index, VkDescriptorTypeName(binding->type));
        }
        if (data->count != binding->count)
        {
            all_descriptor_datas_valid = false;
            PrintError("descriptor data [%u] count (%u) does not match descriptor binding [%u] count (%u)",
                       i, data->count, data->binding_index, binding->count);
        }
    }

    if (!all_descriptor_datas_valid)
    {
        CTK_FATAL("can't bind descriptor datas: not all descriptor datas match associated descriptor set bindings");
    }

    // Bind datas.
    uint32 frame_count = GetFrameCount();
    uint32 buffer_count = 0;
    uint32 image_count = 0;
    CTK_ITER(data, &descriptor_datas)
    {
        if (data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            buffer_count += data->count;
        }
        else if (data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            image_count += data->count;
        }
        else
        {
            CTK_FATAL("unhandled descriptor type: %u", (uint32)data->type);
        }
    }
    auto buffer_infos = CreateArray<VkDescriptorBufferInfo>(&frame.allocator, buffer_count * frame_count);
    auto image_infos  = CreateArray<VkDescriptorImageInfo> (&frame.allocator, image_count  * frame_count);
    auto writes       = CreateArray<VkWriteDescriptorSet>  (&frame.allocator, buffer_infos->size + image_infos->size);

    for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
    {
        VkDescriptorSet frame_descriptor_set = Get(&descriptor_set->hnds, frame_index);
        CTK_ITER(data, &descriptor_datas)
        {
            VkWriteDescriptorSet* write = Push(writes);
            write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->dstSet          = frame_descriptor_set;
            write->dstBinding      = data->binding_index;
            write->dstArrayElement = 0;
            write->descriptorType  = data->type;
            write->descriptorCount = data->count;

            if (data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            {
                // Map buffer to buffer info for write.
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
            else if (data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            {
                // Map combined image sampler to image info for write.
                write->pImageInfo = End(image_infos);
                CTK_ITER_PTR(image_hnd, data->image_hnds, data->count)
                {
                    Image* image = GetImage(*image_hnd);
                    Push(image_infos,
                    {
                        .sampler     = data->sampler,
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

static void InitDescriptorSet(DescriptorSet* descriptor_set, Stack* perm_stack, Stack* temp_stack,
                              Array<DescriptorBinding> bindings)
{
    Stack frame = CreateFrame(temp_stack);

    VkResult res = VK_SUCCESS;
    VkDevice device = GetDevice();

    // Set all layout binding indexes to their index in the array.
    Array<VkDescriptorSetLayoutBinding> layout_bindings = {};
    InitArray(&layout_bindings, &frame.allocator, bindings.count);
    for (uint32 i = 0; i < bindings.count; ++i)
    {
        DescriptorBinding* binding = GetPtr(&bindings, i);
        Push(&layout_bindings,
            {
                .binding = i,
                .descriptorType = binding->type,
                .descriptorCount = binding->count,
                .stageFlags = binding->stages,
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
    uint32 frame_count = GetFrameCount();
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

    // Copy bindings.
    InitArray(&descriptor_set->bindings, &perm_stack->allocator, &bindings);
}

// static void BindBuffers(DescriptorSet* descriptor_set, Stack* temp_stack, Array<Buffer*> buffers, uint32 binding,
//                         uint32 offset, VkDescriptorType type)
// {
//     Stack frame = CreateFrame(temp_stack);

//     Array<VkDescriptorBufferInfo> buffer_infos = {};
//     InitArray(&buffer_infos, &frame.allocator, buffers.count);
//     for (uint32 i = 0; i < buffers.count; ++i)
//     {
//         Buffer* buffer = Get(&buffers, i);
//         Push(&buffer_infos,
//         {
//             .buffer = buffer->hnd,
//             .offset = buffer->offset,
//             .range  = buffer->size,
//         });
//     }
//     VkWriteDescriptorSet write =
//     {
//         .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//         .dstSet          = descriptor_set->hnd,
//         .dstBinding      = binding,
//         .dstArrayElement = offset,
//         .descriptorCount = buffer_infos.count,
//         .descriptorType  = type,
//         .pBufferInfo     = buffer_infos.data,
//     };
//     vkUpdateDescriptorSets(GetDevice(), 1, &write, 0, NULL);
// }

// static void BindImages(DescriptorSet* descriptor_set, Stack* temp_stack, Array<ImageHnd> images, VkSampler sampler,
//                        uint32 binding, uint32 offset, VkDescriptorType type)
// {
//     Stack frame = CreateFrame(temp_stack);

//     Array<VkDescriptorImageInfo> image_infos = {};
//     InitArray(&image_infos, &frame.allocator, images.count);
//     for (uint32 i = 0; i < images.count; ++i)
//     {
//         Push(&image_infos,
//         {
//             .sampler     = sampler,
//             .imageView   = GetImageView(Get(&images, i)),
//             .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//         });
//     }
//     VkWriteDescriptorSet write =
//     {
//         .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//         .dstSet          = descriptor_set->hnd,
//         .dstBinding      = binding,
//         .dstArrayElement = offset,
//         .descriptorCount = image_infos.count,
//         .descriptorType  = type,
//         .pImageInfo      = image_infos.data,
//     };
//     vkUpdateDescriptorSets(GetDevice(), 1, &write, 0, NULL);
// }
