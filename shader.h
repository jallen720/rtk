#pragma once

#include "rtk/vulkan.h"
#include "rtk/debug.h"
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/containers.h"
#include "ctk/file.h"

using namespace ctk;

namespace RTK {

/// Data
////////////////////////////////////////////////////////////
struct Shader {
    VkShaderModule        hnd;
    VkShaderStageFlagBits stage;
};

/// Interface
////////////////////////////////////////////////////////////
static void LoadShader(Shader* shader, Stack temp_mem, VkDevice device, cstr path, VkShaderStageFlagBits stage) {
    Array<u32>* bytecode = read_file<u32>(&temp_mem, path);
    if (bytecode == NULL) {
        CTK_FATAL("failed to load bytecode from \"%s\"", path);
    }

    shader->stage = stage;
    VkShaderModuleCreateInfo info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .codeSize = byte_size(bytecode),
        .pCode    = bytecode->data,
    };
    VkResult res = vkCreateShaderModule(device, &info, NULL, &shader->hnd);
    Validate(res, "failed to create shader from SPIR-V file \"%p\"", path);
}

}
