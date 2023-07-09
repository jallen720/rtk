/// Data
////////////////////////////////////////////////////////////
struct BufferShaderDataInfo
{
    DeviceStack* stack;
    VkDeviceSize size;
};

struct ImageShaderDataInfo
{
    ImageMemory*          memory;
    VkSampler             sampler;
    VkImageViewCreateInfo view;
};

struct ShaderDataInfo
{
    VkShaderStageFlags stages;
    VkDescriptorType   type;
    bool               per_frame;
    uint32             count;
    union
    {
        BufferShaderDataInfo buffer;
        ImageShaderDataInfo  image;
    };
};

struct ShaderData
{
    VkShaderStageFlags stages;
    VkDescriptorType   type;
    bool               per_frame;
    uint32             count;
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
    Array<VkDescriptorSet> instances;
};

/// Interface
////////////////////////////////////////////////////////////
static ShaderData* CreateShaderData(const Allocator* allocator, ShaderDataInfo* info)
{
    CTK_ASSERT(info->count != 0);

    auto shader_data = Allocate<ShaderData>(allocator, 1);

    shader_data->stages    = info->stages;
    shader_data->type      = info->type;
    shader_data->per_frame = info->per_frame;
    shader_data->count     = info->count;

    uint32 instance_count = shader_data->per_frame ? global_ctx.frames.size : 1;
    uint32 total_count = instance_count * shader_data->count;

    if (shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
        shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
    {
        BufferShaderDataInfo* buffer_info = &info->buffer;
        InitArray(&shader_data->buffers, allocator, total_count);
        for (uint32 i = 0; i < total_count; ++i)
        {
            InitBuffer(Push(&shader_data->buffers), buffer_info->stack, buffer_info->size);
        }
    }
    else if (shader_data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
    {
        ImageShaderDataInfo* image_info = &info->image;
        InitArray(&shader_data->images, allocator, total_count);
        for (uint32 i = 0; i < total_count; ++i)
        {
            InitImage(Push(&shader_data->images), image_info->memory, &image_info->view);
        }
        shader_data->sampler = image_info->sampler;
    }
    else
    {
        CTK_FATAL("can't init shader-data: type %u is unsupported", shader_data->type);
    }

    return shader_data;
}

template<typename Type>
static Type* GetBufferMem(ShaderData* shader_data, uint32 instance_index, uint32 index)
{
    CTK_ASSERT(shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
               shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

    uint32 instance_offset = instance_index * shader_data->count;
    return (Type*)GetPtr(&shader_data->buffers, instance_offset + index)->mapped_mem;
}

template<typename Type>
static Type* GetCurrentFrameBufferMem(ShaderData* shader_data, uint32 index)
{
    return GetBufferMem<Type>(shader_data, global_ctx.frames.index, index);
}

static void
WriteToShaderDataImage(ShaderData* shader_data, uint32 instance_index, uint32 image_index, Buffer* image_data_buffer)
{
    Image* image = GetPtr(&shader_data->images, (instance_index * shader_data->count) + image_index);

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

static ShaderDataSet*
CreateShaderDataSet(const Allocator* allocator, Stack* temp_stack, Array<ShaderData*> shader_datas)
{
    Stack frame = CreateFrame(temp_stack);

    auto shader_data_set = Allocate<ShaderDataSet>(allocator, 1);

    VkDevice device = global_ctx.device;
    VkResult res = VK_SUCCESS;
    uint32 instance_count = global_ctx.frames.size;

    // Generate descriptor bindings.
    auto bindings = CreateArray<VkDescriptorSetLayoutBinding>(&frame.allocator, shader_datas.count);
    for (uint32 i = 0; i < shader_datas.count; ++i)
    {
        ShaderData* shader_data = Get(&shader_datas, i);
        Push(bindings,
        {
            .binding            = i,
            .descriptorType     = shader_data->type,
            .descriptorCount    = shader_data->count,
            .stageFlags         = shader_data->stages,
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
    auto desc_set_alloc_layouts = CreateArray<VkDescriptorSetLayout>(&frame.allocator, instance_count);
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
    InitArrayFull(&shader_data_set->instances, allocator, instance_count);
    res = vkAllocateDescriptorSets(device, &allocate_info, shader_data_set->instances.data);
    Validate(res, "vkAllocateDescriptorSets() failed");

    // Bind descriptor data.
    uint32 buffer_count = 0;
    uint32 image_count = 0;
    CTK_ITERATE(shader_data_ptr, &shader_datas)
    {
        ShaderData* shader_data = *shader_data_ptr;
        if (shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            buffer_count += shader_data->count;
        }
        else if (shader_data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            image_count += shader_data->count;
        }
        else
        {
            CTK_FATAL("unhandled shader-data type: %u", (uint32)shader_data->type);
        }
    }
    auto buffer_infos = CreateArray<VkDescriptorBufferInfo>(&frame.allocator, buffer_count * instance_count);
    auto image_infos = CreateArray<VkDescriptorImageInfo>(&frame.allocator, image_count * instance_count);
    auto writes = CreateArray<VkWriteDescriptorSet>(&frame.allocator, buffer_infos->size + image_infos->size);

    for (uint32 instance_index = 0; instance_index < instance_count; ++instance_index)
    {
        VkDescriptorSet instance_desc_set = Get(&shader_data_set->instances, instance_index);
        for (uint32 data_binding = 0; data_binding < shader_datas.count; ++data_binding)
        {
            ShaderData* shader_data = Get(&shader_datas, data_binding);
            uint32 data_instance_index = shader_data->per_frame ? instance_index : 0;
            uint32 data_instance_offset = data_instance_index * shader_data->count;

            VkWriteDescriptorSet* write = Push(writes);
            write->sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->dstSet          = instance_desc_set;
            write->dstBinding      = data_binding;
            write->dstArrayElement = 0;
            write->descriptorType  = shader_data->type;
            write->descriptorCount = shader_data->count;

            if (shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                shader_data->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            {
                // Map buffer to buffer info for write.
                write->pBufferInfo = End(buffer_infos);
                for (uint32 buffer_index = 0; buffer_index < shader_data->count; ++buffer_index)
                {

                    Buffer* buffer = GetPtr(&shader_data->buffers, data_instance_offset + buffer_index);
                    Push(buffer_infos,
                    {
                        .buffer = buffer->hnd,
                        .offset = buffer->offset,
                        .range  = buffer->size,
                    });
                }
            }
            else if (shader_data->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            {
                // Map combined image sampler to image info for write.
                write->pImageInfo = End(image_infos);
                for (uint32 image_index = 0; image_index < shader_data->count; ++image_index)
                {
                    Image* image = GetPtr(&shader_data->images, data_instance_offset + image_index);
                    Push(image_infos,
                    {
                        .sampler     = shader_data->sampler,
                        .imageView   = image->view,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    });
                }
            }
            else
            {
                CTK_FATAL("unhandled shader-data type: %u", (uint32)shader_data->type);
            }
        }
    }

    vkUpdateDescriptorSets(device, writes->count, writes->data, 0, NULL);

    return shader_data_set;
}

static uint32 GetImageCount(ShaderData* shader_data)
{
    return shader_data->images.count;
}
