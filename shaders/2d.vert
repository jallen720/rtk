#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable

#include "../tests/defs_2d.h"
#include "common_2d.glsl"

layout (location = 0) in vec2 in_vert_pos;
layout (location = 1) in vec2 in_vert_uv;
layout (location = 0) out vec2 out_vert_uv;

vec2 sprite_dim  = vec2(64, 32);
vec2 surface_dim = vec2(1080, 720);
vec2 offset      = vec2(0, 0);

const vec2 NDC_SCALE_FACTOR              = vec2(2, -2);
const vec2 NDC_BOTTOM_LEFT_CORNER_OFFSET = vec2(-1, 1);

// layout (set = 0, binding = 0) uniform Surface
// {
//     vec2 dim;
// }
// surface;

// layout (set = 1, binding = 0) uniform EntityData
// {
//     vec2 position[MAX_ENTITIES];
//     uint sprite[MAX_ENTITIES];
// }
// entity_data;

// layout (set = 2, binding = 0) uniform Sprites
// {
//     Sprite data[MAX_SPRITES];
// }
// sprites;

vec2 SpriteSurfaceOffsetToNDC(vec2 pos)
{
    return ((pos * sprite_dim) + offset) / surface_dim;
}

vec2 ToNDC(vec2 pos)
{
    return (SpriteSurfaceOffsetToNDC(pos) * NDC_SCALE_FACTOR) + NDC_BOTTOM_LEFT_CORNER_OFFSET;
}

void main()
{
    gl_Position = vec4(ToNDC(in_vert_pos), 0, 1);
    out_vert_uv = in_vert_uv;
}
