#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable

#include "../tests/defs.h"

layout (location = 0) in vec3 in_vert_pos;
layout (location = 1) in vec2 in_vert_uv;
layout (location = 0) out vec3 out_vert_color;
layout (location = 1) out vec2 out_vert_uv;

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

// layout (push_constant) uniform PushConstants
// {
//     mat4 mvp_matrix;
// }
// push_constants;

void main()
{
    // gl_Position = push_constants.mvp_matrix * vec4(in_vert_pos, 1);
    gl_Position = vs_buffer.mvp_matrixes[push_constants.entity_index] * vec4(in_vert_pos, 1);
    out_vert_color = in_vert_pos;
    out_vert_uv = in_vert_uv;
}
