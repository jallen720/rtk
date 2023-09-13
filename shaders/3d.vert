#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable
#extension GL_EXT_scalar_block_layout     : enable

#include "../tests/3d/defs.h"

layout(location = 0) in vec3 in_vert_pos;
layout(location = 1) in vec2 in_vert_uv;
layout(location = 0) out vec2 out_vert_uv;
layout(location = 1) out flat uint out_entity_index;

layout(set = 0, binding = 0, std430) uniform EntityBuffer
{
    mat4 mvp_matrixes[MAX_ENTITIES];
    uint texture_indexes[MAX_ENTITIES];
}
entity_buffer;

layout(push_constant) uniform PushConstants
{
    uint entity_index;
}
pc;

void main()
{
    uint entity_index = pc.entity_index == USE_GL_INSTANCE_INDEX ? gl_InstanceIndex : pc.entity_index;
    gl_Position = entity_buffer.mvp_matrixes[entity_index] * vec4(in_vert_pos, 1);
    out_vert_uv = in_vert_uv;
    out_entity_index = entity_index;
}
