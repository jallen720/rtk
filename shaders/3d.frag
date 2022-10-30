#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 in_vert_color;
layout (location = 1) in vec2 in_vert_uv;
layout (location = 0) out vec4 out_color;

layout (set = 1, binding = 0) uniform sampler2D texture_sampler;

void main()
{
    // out_color = vec4(in_vert_color, 1);
    out_color = texture(texture_sampler, in_vert_uv);
}
