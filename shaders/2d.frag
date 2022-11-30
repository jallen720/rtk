#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 in_vert_uv;
layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0) uniform sampler2D texture_sampler;

void main()
{
    out_color = textureLod(texture_sampler, ivec2(in_vert_uv), 0);
}
