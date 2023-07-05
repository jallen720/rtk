#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_vert_uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D texture_sampler;
// layout(set = 0, binding = 0) uniform sampler2D texture_sampler[2];

// layout(push_constant) uniform PushConstants
// {
//     layout(offset = 16) uint texture_index;
// }
// push_constants;

void main()
{
    out_color = texture(texture_sampler, in_vert_uv);
    // out_color = texture(texture_sampler[push_constants.texture_index], in_vert_uv);
}
