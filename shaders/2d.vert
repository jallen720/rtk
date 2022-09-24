#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 in_vert_pos;
layout (location = 0) out vec3 out_vert_color;

layout (push_constant) uniform PushConstants {
    mat4 mvp_matrix;
} push_constants;

void main() {
    gl_Position = push_constants.mvp_matrix * vec4(in_vert_pos, 1);
    out_vert_color = in_vert_pos;
}
