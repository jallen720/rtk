#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_vert_uv;
layout(location = 1) in flat uint in_instance_index;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler2D texture_sampler[2];

void main()
{
    out_color = texture(texture_sampler[in_instance_index], in_vert_uv);
}
