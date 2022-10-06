#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
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
    VkShaderModule        hnd;
    VkShaderStageFlagBits stage;
};

/// Interface
////////////////////////////////////////////////////////////
static void LoadShader(Shader* shader, Stack temp_mem, VkDevice device, cstring path, VkShaderStageFlagBits stage)
{
    Array<uint32>* bytecode = ReadFile<uint32>(&temp_mem, path);
    shader->stage = stage;
    VkShaderModuleCreateInfo info =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .codeSize = ByteSize(bytecode),
        .pCode    = bytecode->data,
    };
    VkResult res = vkCreateShaderModule(device, &info, NULL, &shader->hnd);
    Validate(res, "failed to create shader from SPIR-V file \"%p\"", path);
}

}
