#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable
#extension GL_EXT_scalar_block_layout     : enable

#include "../tests/debug/defs.h"

layout(location = 0) in vec2 in_vert_uv;
layout(location = 1) in flat uint in_instance_index;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0, std430) uniform EntityBuffer
{
    vec4  position[MAX_ENTITIES];
    float scale[MAX_ENTITIES];
    uint  texture_index[MAX_ENTITIES];
}
entity;

layout(set = 0, binding = 1) uniform sampler2D texture_sampler[MAX_TEXTURES];

void main()
{
    out_color = texture(texture_sampler[entity.texture_index[in_instance_index]], in_vert_uv);
}
