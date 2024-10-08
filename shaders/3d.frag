#version 450
#extension GL_ARB_separate_shader_objects : require
#extension GL_GOOGLE_include_directive    : require
#extension GL_EXT_scalar_block_layout     : require
#extension GL_EXT_nonuniform_qualifier    : require

#include "../tests/defs.h"

layout(location = 0) in vec2 in_vert_uv;
layout(location = 1) in flat uint in_entity_index;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0, std430) uniform EntityBuffer
{
    mat4 mvp_matrixes   [MAX_ENTITIES];
    uint texture_indexes[MAX_ENTITIES];
    uint sampler_indexes[MAX_ENTITIES];
}
entity;

layout(set = 1, binding = 0) uniform texture2D textures[MAX_TEXTURES];
layout(set = 1, binding = 1) uniform sampler   samplers[MAX_SAMPLERS];

void main()
{
    out_color =
        texture(
            sampler2D(
                textures[nonuniformEXT(entity.texture_indexes[in_entity_index])],
                samplers[nonuniformEXT(entity.sampler_indexes[in_entity_index])]),
            in_vert_uv);
}
