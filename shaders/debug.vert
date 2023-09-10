#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable
#extension GL_EXT_scalar_block_layout     : enable

#include "../tests/debug/defs.h"

layout(location = 0) in vec3 in_vert_pos;
layout(location = 1) in vec2 in_vert_uv;
layout(location = 0) out vec2 out_vert_uv;
layout(location = 1) out flat uint out_instance_index;

layout(set = 0, binding = 0, std430) uniform EntityBuffer
{
    vec4  position[MAX_ENTITIES];
    float scale[MAX_ENTITIES];
    uint  texture_index[MAX_ENTITIES];
}
entity;

const vec2 screen_offset      = vec2(-1, -1);
const vec2 screen_orientation = vec2( 1, -1);

void main()
{
    vec2 vert_pos = in_vert_pos.xy * entity.scale[gl_InstanceIndex];
    vec2 pos = (vert_pos.xy + entity.position[gl_InstanceIndex].xy + screen_offset) * screen_orientation;
    gl_Position = vec4(pos, 0, 1);
    out_vert_uv = in_vert_uv;
    out_instance_index = gl_InstanceIndex;
}
