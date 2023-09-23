
/// Data
////////////////////////////////////////////////////////////
struct DescriptorData
{
    VkDescriptorType type;
    uint32           count;
    uint32           binding_index;
    union
    {
        Buffer* buffers;
        Image*  images;
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
static void BindDescriptorDatas(DescriptorSet* descriptor_set, Array<DescriptorData> descriptor_datas)
{
    // Validate datas to make sure the match bindings.
    bool all_descriptor_datas_valid = true;
    for (uint32 i = 0; i < descriptor_datas.count; ++i)
    {
        DescriptorData* data = GetPtr(&descriptor_datas, i);
        if (data->binding_index >= descriptor_set->bindings.count)
        {
            CTK_FATAL("descriptor data %u binding index %u exceeds descriptor set binding count %u",
                      i, data->binding_index, descriptor_set->bindings);
        }

        DescriptorBinding* binding = GetPtr(&descriptor_set->bindings, data->binding_index);
        if (data->type != binding->type)
        {
            PrintError("descriptor data [%u] type (%s) does not match descriptor binding [%u] type (%s)",
                       i, VkDescriptorTypeName(data->type), data->binding_index, VkDescriptorTypeName(binding->type));
        }
    }
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
            .binding            = i;
            .descriptorType     = binding->type,
            .descriptorCount    = binding->count,
            .stageFlags         = binding->stages,
            .pImmutableSamplers = NULL,
        })
        GetPtr(&layout_bindings, i)->binding = i;
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
