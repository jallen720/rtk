/// Data
////////////////////////////////////////////////////////////
struct Shader
{
    VkShaderModule        module;
    VkShaderStageFlagBits stage;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitShader(Shader* shader, Stack* temp_stack, const char* path, VkShaderStageFlagBits stage)
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
    VkResult res = vkCreateShaderModule(GetDevice(), &info, NULL, &shader->module);
    Validate(res, "vkCreateShaderModule() failed");

    shader->stage = stage;
}

static Shader* CreateShader(const Allocator* allocator, Stack* temp_stack, const char* path,
                            VkShaderStageFlagBits stage)
{
    auto shader = Allocate<Shader>(allocator, 1);
    InitShader(shader, temp_stack, path, stage);
    return shader;
}
