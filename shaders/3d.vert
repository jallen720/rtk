#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable

#include "../tests/defs.h"

layout (location = 0) in vec3 in_vert_pos;
layout (location = 1) in vec2 in_vert_uv;
layout (location = 0) out vec2 out_vert_uv;

layout (set = 0, binding = 0) uniform VSBuffer
{
    mat4 mvp_matrixes[MAX_ENTITIES];
}
vs_buffer;

layout (push_constant) uniform PushConstants
{
    uint entity_index;
}
push_constants;

void main()
{
    gl_Position = vs_buffer.mvp_matrixes[push_constants.entity_index] * vec4(in_vert_pos, 1);
    out_vert_uv = in_vert_uv;
}
