/// Data
////////////////////////////////////////////////////////////
struct Shader
{
    VkShaderModule        module;
    VkShaderStageFlagBits stage;
};

/// Utils
////////////////////////////////////////////////////////////
static VkShaderModule LoadShaderModule(Stack* temp_stack, const char* path)
{
    Stack frame = CreateFrame(temp_stack);
    Array<uint32>* bytecode = ReadFile<uint32>(&frame.allocator, path);
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

/// Interface
////////////////////////////////////////////////////////////
static Shader* CreateShader(const Allocator* allocator, Stack* temp_stack, const char* path,
                            VkShaderStageFlagBits stage)
{
    auto shader = Allocate<Shader>(allocator, 1);

    shader->module = LoadShaderModule(temp_stack, path);
    shader->stage  = stage;

    return shader;
}
