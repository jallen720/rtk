/// Interface
////////////////////////////////////////////////////////////
static void BindDescriptorSets(VkCommandBuffer command_buffer, Pipeline* pipeline, Stack* temp_stack,
                               Array<DescriptorSet*> descriptor_sets, uint32 first_binding)
{
    Stack frame = CreateFrame(temp_stack);

    // Create array of handles from descriptor sets to pass to vkCmdBindDescriptorSets().
    Array<VkDescriptorSet> descriptor_set_hnds = {};
    InitArray(&descriptor_set_hnds, &frame.allocator, descriptor_sets.count);
    for (uint32 i = 0; i < descriptor_sets.count; ++i)
    {
        Push(&descriptor_set_hnds, Get(&descriptor_sets, i)->hnd);
    };

    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline->layout,
                            first_binding, // First set
                            descriptor_set_hnds.count, descriptor_set_hnds.data, // Descriptor Set
                            0, NULL); // Dynamic Offsets
}
