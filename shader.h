#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/rtk_context.h"
#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/containers.h"
#include "ctk2/file.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
struct Shader
{
    VkShaderModule        module;
    VkShaderStageFlagBits stage;
};

/// Interface
////////////////////////////////////////////////////////////
static VkShaderModule LoadShaderModule(RTKContext* rtk, Stack temp_mem, cstring path)
{
    Array<uint32>* bytecode = ReadFile<uint32>(&temp_mem, path);
    VkShaderModuleCreateInfo info =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .codeSize = ByteSize(bytecode),
        .pCode    = bytecode->data,
    };

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(rtk->device, &info, NULL, &module);
    Validate(res, "vkCreateShaderModule() failed");

    return module;
}

}
