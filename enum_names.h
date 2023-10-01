/// Interface
////////////////////////////////////////////////////////////
#define RTK_ENUM_NAME_CASE(NAME) case NAME: return #NAME;

static constexpr const char* VkPresentModeName(VkPresentModeKHR present_mode)
{
    switch (present_mode)
    {
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_IMMEDIATE_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_MAILBOX_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_FIFO_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR)
        default: CTK_FATAL("unknown VkPresentModeKHR value: %u", (uint32)present_mode);
    };
}

static constexpr const char* VkDescriptorTypeName(VkDescriptorType descriptor_type)
{
    switch (descriptor_type)
    {
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_SAMPLER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
        default: CTK_FATAL("unknown VkDescriptorType value: %u", (uint32)descriptor_type);
    }
}

static constexpr const char* VkBufferUsageName(VkBufferUsageFlagBits buffer_usage)
{
    switch (buffer_usage)
    {
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        default: CTK_FATAL("unknown VkBufferUsageFlagBits value: %u", (uint32)buffer_usage);
    }
}
