#version 450
#extension GL_ARB_separate_shader_objects : require
#extension GL_GOOGLE_include_directive    : require
#extension GL_EXT_scalar_block_layout     : require

#include "../tests/debug/defs.h"

layout(location = 0) in vec3 in_vert_pos;
layout(location = 1) in vec2 in_vert_uv;
layout(location = 0) out vec2 out_vert_uv;
layout(location = 1) out flat uint out_entity_index;

layout(set = 0, binding = 0, std430) uniform EntityBuffer
{
    vec4  positions[MAX_ENTITIES];
    float scales[MAX_ENTITIES];
    uint  texture_indexes[MAX_ENTITIES];
}
entity;

layout(push_constant) uniform PushConstants
{
    uint entity_index;
}
pc;

const vec2 screen_offset      = vec2(-1, -1);
const vec2 screen_orientation = vec2( 1, -1);

void main()
{
    uint entity_index = pc.entity_index == USE_GL_INSTANCE_INDEX ? gl_InstanceIndex : pc.entity_index;
    vec2 vert_pos = in_vert_pos.xy * entity.scales[entity_index];
    vec2 pos = (vert_pos.xy + entity.positions[entity_index].xy + screen_offset) * screen_orientation;
    gl_Position = vec4(pos, 1, 1);
    out_vert_uv = in_vert_uv;
    out_entity_index = entity_index;
}
