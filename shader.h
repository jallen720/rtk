#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "rtk/context.h"
#include "ctk3/ctk3.h"
#include "ctk3/stack.h"
#include "ctk3/array.h"
#include "ctk3/file.h"

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
static VkShaderModule LoadShaderModule(Stack temp_stack, const char* path)
{
    Array<uint32>* bytecode = ReadFile<uint32>(&temp_stack, path);
    VkShaderModuleCreateInfo info =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .codeSize = ByteSize(bytecode),
        .pCode    = bytecode->data,
    };

    VkShaderModule module = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(global_ctx.device, &info, NULL, &module);
    Validate(res, "vkCreateShaderModule() failed");

    return module;
}

}
