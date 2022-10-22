#pragma once

#include "rtk/vulkan.h"
#include "rtk/rtk_context.h"
#include "ctk2/ctk.h"
#include "ctk2/containers.h"

using namespace CTK;

namespace RTK
{

/// Interface
////////////////////////////////////////////////////////////
static VkDescriptorPool CreateDescriptorPool(RTKContext* rtk, Array<VkDescriptorPoolSize>* pool_sizes)
{
    // Count total descriptors from pool sizes.
    uint32 total_descriptors = 0;
    for (uint32 i = 0; i < pool_sizes->count; ++i)
        total_descriptors += GetPtr(pool_sizes, i)->descriptorCount;

    VkDescriptorPoolCreateInfo pool_info =
    {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = 0,
        .maxSets       = total_descriptors,
        .poolSizeCount = pool_sizes->count,
        .pPoolSizes    = pool_sizes->data,
    };
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkResult res = vkCreateDescriptorPool(rtk->device, &pool_info, NULL, &descriptor_pool);
    Validate(res, "failed to create descriptor pool");

    return descriptor_pool;
}

}
