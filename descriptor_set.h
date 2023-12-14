/// Data
////////////////////////////////////////////////////////////
struct DescriptorSetHnd { uint32 index; };

struct DescriptorData
{
    VkDescriptorType   type;
    VkShaderStageFlags stages;
    uint32             count;
    union
    {
        BufferHnd* buffer_hnds;
        ImageHnd*  image_hnds;
        VkSampler* samplers;
        struct
        {
            ImageHnd* image_hnds;
            VkSampler sampler;
        }
        image_samplers;
    };
};

struct DescriptorState
{
    Array<DescriptorData>* data_bindings;
    VkDescriptorSetLayout* layouts;
    VkDescriptorSet*       sets;
    VkDescriptorPool       pool;
    uint32                 max_sets;
    uint32                 set_count;
    uint32                 frame_count;
};

struct DescriptorSetModuleInfo
{
    uint32 max_descriptor_sets;
};

static DescriptorState g_desc_state;

/// Interface
////////////////////////////////////////////////////////////
static void InitDescriptorSetModule(const Allocator* allocator, DescriptorSetModuleInfo info)
{
    uint32 frame_count = GetFrameCount();
    g_desc_state.data_bindings = Allocate<Array<DescriptorData>>(allocator, info.max_descriptor_sets);
    g_desc_state.layouts       = Allocate<VkDescriptorSetLayout>(allocator, info.max_descriptor_sets);
    g_desc_state.sets          = Allocate<VkDescriptorSet>(allocator, info.max_descriptor_sets * frame_count);
    g_desc_state.pool          = VK_NULL_HANDLE;
    g_desc_state.max_sets      = info.max_descriptor_sets;
    g_desc_state.set_count     = 0;
    g_desc_state.frame_count   = frame_count;
}

static DescriptorSetHnd CreateDescriptorSet(const Allocator* allocator, Stack* temp_stack, Array<DescriptorData> datas)
{
    if (g_desc_state.set_count >= g_desc_state.max_sets)
    {
        CTK_FATAL("can't create descriptor set: already at max of %u", g_desc_state.max_sets);
    }

    Stack frame = CreateFrame(temp_stack);

    // Assign handle.
    DescriptorSetHnd hnd = { .index = g_desc_state.set_count };
    ++g_desc_state.set_count;

    // Copy data bindings.
    InitArray(&g_desc_state.data_bindings[hnd.index], allocator, &datas);

    // Generate layout bindings.
    Array<VkDescriptorSetLayoutBinding> layout_bindings = {};
    InitArray(&layout_bindings, &frame.allocator, datas.count);
    for (uint32 i = 0; i < datas.count; ++i)
    {
        DescriptorData* data = GetPtr(&datas, i);
        Push(&layout_bindings,
             {
                 .binding            = i,
                 .descriptorType     = data->type,
                 .descriptorCount    = data->count,
                 .stageFlags         = data->stages,
                 .pImmutableSamplers = NULL,
             });
    }

    // Create layout.
    VkDescriptorSetLayoutCreateInfo layout_info =
    {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = NULL,
        .flags        = 0,
        .bindingCount = layout_bindings.count,
        .pBindings    = layout_bindings.data,
    };
    VkResult res = vkCreateDescriptorSetLayout(GetDevice(), &layout_info, NULL, &g_desc_state.layouts[hnd.index]);
    Validate(res, "vkCreateDescriptorSetLayout() failed");

    return hnd;
}

static void InitDescriptorSets(Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);
    VkDevice device = GetDevice();
    uint32 frame_count = g_desc_state.frame_count;

    ///
    /// Allocate Descriptor Sets
    ///

    // Create descriptor pool based on descriptor set data bindings for all descriptor sets.
    static constexpr uint32 DESCRIPTOR_TYPE_COUNT = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1;
    uint32 pool_size_indexes[DESCRIPTOR_TYPE_COUNT] = {};
    FArray<VkDescriptorPoolSize, DESCRIPTOR_TYPE_COUNT> pool_sizes = {};
    CTK_ITER_PTR(pool_size_index, pool_size_indexes, DESCRIPTOR_TYPE_COUNT)
    {
        *pool_size_index = UINT32_MAX;
    }

    CTK_ITER_PTR(data_bindings, g_desc_state.data_bindings, g_desc_state.set_count)
    {
        CTK_ITER(data_binding, data_bindings)
        {
            VkDescriptorPoolSize* pool_size = pool_size_indexes[data_binding->type] == UINT32_MAX
                                              ? Push(&pool_sizes, { .type = data_binding->type })
                                              : GetPtr(&pool_sizes, pool_size_indexes[data_binding->type]);
            pool_size->descriptorCount += data_binding->count * frame_count;
        }
    }

    VkDescriptorPoolCreateInfo pool_info =
    {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = 0,
        .maxSets       = g_desc_state.set_count * frame_count,
        .poolSizeCount = pool_sizes.count,
        .pPoolSizes    = pool_sizes.data,
    };
    VkResult res = vkCreateDescriptorPool(device, &pool_info, NULL, &g_desc_state.pool);
    Validate(res, "vkCreateDescriptorPool() failed");

    // Allocate all descriptor sets for each frame from pool.
    for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
    {
        uint32 frame_offset = frame_index * g_desc_state.set_count;
        VkDescriptorSetAllocateInfo allocate_info =
        {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = NULL,
            .descriptorPool     = g_desc_state.pool,
            .descriptorSetCount = g_desc_state.set_count,
            .pSetLayouts        = g_desc_state.layouts,
        };
        res = vkAllocateDescriptorSets(device, &allocate_info, &g_desc_state.sets[frame_offset]);
        Validate(res, "vkAllocateDescriptorSets() failed");
    }

    ///
    /// Bind Resources
    ///

    // Add up total number of data bindings for all descriptor sets.
    uint32 buffer_write_count = 0;
    uint32 image_write_count  = 0;
    CTK_ITER_PTR(data_bindings, g_desc_state.data_bindings, g_desc_state.set_count)
    {
        CTK_ITER(data_binding, data_bindings)
        {
            if (data_binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                data_binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            {
                buffer_write_count += data_binding->count;
            }
            else if (data_binding->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                     data_binding->type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                     data_binding->type == VK_DESCRIPTOR_TYPE_SAMPLER)
            {
                image_write_count += data_binding->count;
            }
            else
            {
                CTK_FATAL("unhandled descriptor type: %u", (uint32)data_binding->type);
            }
        }
    }

    // Generate writes from data bindings.
    auto buffer_infos = CreateArray<VkDescriptorBufferInfo>(&frame.allocator, buffer_write_count * frame_count);
    auto image_infos  = CreateArray<VkDescriptorImageInfo> (&frame.allocator, image_write_count  * frame_count);
    auto writes       = CreateArray<VkWriteDescriptorSet>  (&frame.allocator, buffer_infos->size + image_infos->size);
    for (uint32 frame_index = 0; frame_index < frame_count; ++frame_index)
    {
        uint32 frame_offset = frame_index * g_desc_state.set_count;
        for (uint32 set_index = 0; set_index < g_desc_state.set_count; ++set_index)
        {
            Array<DescriptorData>* data_bindings = &g_desc_state.data_bindings[set_index];
            VkDescriptorSet descriptor_set = g_desc_state.sets[frame_offset + set_index];
            for (uint32 binding_index = 0; binding_index < data_bindings->count; ++binding_index)
            {
                DescriptorData* data_binding = GetPtr(data_bindings, binding_index);
                VkWriteDescriptorSet* write = Push(writes);
                write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write->dstSet          = descriptor_set;
                write->dstBinding      = binding_index;
                write->dstArrayElement = 0;
                write->descriptorType  = data_binding->type;
                write->descriptorCount = data_binding->count;

                if (data_binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                    data_binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
                {
                    write->pBufferInfo = End(buffer_infos);
                    CTK_ITER_PTR(buffer_hnd, data_binding->buffer_hnds, data_binding->count)
                    {
                        Push(buffer_infos,
                             {
                                 .buffer = GetBuffer(*buffer_hnd),
                                 .offset = GetOffset(*buffer_hnd, frame_index),
                                 .range  = GetSize(*buffer_hnd),
                             });
                    }
                }
                else if (data_binding->type == VK_DESCRIPTOR_TYPE_SAMPLER)
                {
                    write->pImageInfo = End(image_infos);
                    CTK_ITER_PTR(sampler, data_binding->samplers, data_binding->count)
                    {
                        Push(image_infos,
                             {
                                 .sampler     = *sampler,
                                 .imageView   = VK_NULL_HANDLE,
                                 .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                             });
                    }
                }
                else if (data_binding->type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
                {
                    write->pImageInfo = End(image_infos);
                    CTK_ITER_PTR(image_hnd, data_binding->image_hnds, data_binding->count)
                    {
                        Push(image_infos,
                             {
                                 .sampler     = VK_NULL_HANDLE,
                                 .imageView   = GetView(*image_hnd, frame_index),
                                 .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             });
                    }
                }
                else if (data_binding->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                    write->pImageInfo = End(image_infos);
                    CTK_ITER_PTR(image_hnd, data_binding->image_samplers.image_hnds, data_binding->count)
                    {
                        Push(image_infos,
                             {
                                 .sampler     = data_binding->image_samplers.sampler,
                                 .imageView   = GetView(*image_hnd, frame_index),
                                 .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             });
                    }
                }
                else
                {
                    CTK_FATAL("unhandled descriptor type: %u", (uint32)data_binding->type);
                }
            }
        }
    }

    // Update all descriptor sets with writes from data bindings.
    vkUpdateDescriptorSets(GetDevice(), writes->count, writes->data, 0, NULL);
}

static VkDescriptorSetLayout GetLayout(DescriptorSetHnd hnd)
{
    if (hnd.index >= g_desc_state.set_count)
    {
        CTK_FATAL("can't get descriptor set layout: handle index %u exceeds descriptor set count of %u",
                  hnd.index, g_desc_state.set_count);
    }
    return g_desc_state.layouts[hnd.index];
}

static VkDescriptorSet GetFrameSet(DescriptorSetHnd hnd, uint32 frame_index)
{
    if (hnd.index >= g_desc_state.set_count)
    {
        CTK_FATAL("can't get frame descriptor set: handle index %u exceeds descriptor set count of %u",
                  hnd.index, g_desc_state.set_count);
    }

    if (frame_index >= g_desc_state.frame_count)
    {
        CTK_FATAL("can't get frame descriptor set: frame index %u exceeds frame count of %u that descriptor set module "
                  "was initialized with",
                  frame_index, g_desc_state.frame_count);
    }

    uint32 frame_offset = frame_index * g_desc_state.set_count;
    return g_desc_state.sets[frame_offset + hnd.index];
}
