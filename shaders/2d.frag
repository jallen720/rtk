#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable

#include "../tests/defs_2d.h"
#include "common_2d.glsl"

layout (location = 0) in vec2 in_vert_uv;
layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0) uniform sampler2D sprite_sheet;

void main()
{
    out_color = textureLod(sprite_sheet, ivec2(in_vert_uv), 0);
}
