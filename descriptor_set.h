/// Data
////////////////////////////////////////////////////////////
struct DescriptorSet
{
    VkDescriptorSet       hnd;
    VkDescriptorSetLayout layout;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitDescriptorSet(DescriptorSet* descriptor_set, Array<VkDescriptorSetLayoutBinding> layout_bindings)
{
    VkResult res = VK_SUCCESS;
    VkDevice device = GetDevice();

    // Set all binding indexes to their index in the array.
    for (uint32 i = 0; i < layout_bindings.count; ++i)
    {
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

    // Allocate descriptor sets.
    VkDescriptorSetAllocateInfo allocate_info =
    {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = NULL,
        .descriptorPool     = global_ctx.descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &descriptor_set->layout,
    };
    res = vkAllocateDescriptorSets(device, &allocate_info, &descriptor_set->hnd);
    Validate(res, "vkAllocateDescriptorSets() failed");
}

static void BindBuffers(DescriptorSet* descriptor_set, Stack* temp_stack, Array<Buffer*> buffers, uint32 binding,
                        uint32 offset, VkDescriptorType type)
{
    Stack frame = CreateFrame(temp_stack);

    Array<VkDescriptorBufferInfo> buffer_infos = {};
    InitArray(&buffer_infos, &frame.allocator, buffers.count);
    for (uint32 i = 0; i < buffers.count; ++i)
    {
        Buffer* buffer = Get(&buffers, i);
        Push(&buffer_infos,
        {
            .buffer = buffer->hnd,
            .offset = buffer->offset,
            .range  = buffer->size,
        });
    }
    VkWriteDescriptorSet write =
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = descriptor_set->hnd,
        .dstBinding      = binding,
        .dstArrayElement = offset,
        .descriptorCount = buffer_infos.count,
        .descriptorType  = type,
        .pBufferInfo     = buffer_infos.data,
    };
    vkUpdateDescriptorSets(GetDevice(), 1, &write, 0, NULL);
}

static void BindImages(DescriptorSet* descriptor_set, Stack* temp_stack, Array<ImageHnd> images, VkSampler sampler,
                       uint32 binding, uint32 offset, VkDescriptorType type)
{
    Stack frame = CreateFrame(temp_stack);

    Array<VkDescriptorImageInfo> image_infos = {};
    InitArray(&image_infos, &frame.allocator, images.count);
    for (uint32 i = 0; i < images.count; ++i)
    {
        Push(&image_infos,
        {
            .sampler     = sampler,
            .imageView   = GetImageView(Get(&images, i)),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
    }
    VkWriteDescriptorSet write =
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = descriptor_set->hnd,
        .dstBinding      = binding,
        .dstArrayElement = offset,
        .descriptorCount = image_infos.count,
        .descriptorType  = type,
        .pImageInfo      = image_infos.data,
    };
    vkUpdateDescriptorSets(GetDevice(), 1, &write, 0, NULL);
}
