Vertex cube_repeating_vertexes[] =
{
    // +X
    { .position = { 1.0f, 0.0f, 0.0f }, .uv = { 0, 1 } },
    { .position = { 1.0f, 1.0f, 0.0f }, .uv = { 0, 0 } },
    { .position = { 1.0f, 1.0f, 1.0f }, .uv = { 1, 0 } },
    { .position = { 1.0f, 0.0f, 1.0f }, .uv = { 1, 1 } },

    // -X
    { .position = { 0.0f, 0.0f, 1.0f }, .uv = { 0, 1 } },
    { .position = { 0.0f, 1.0f, 1.0f }, .uv = { 0, 0 } },
    { .position = { 0.0f, 1.0f, 0.0f }, .uv = { 1, 0 } },
    { .position = { 0.0f, 0.0f, 0.0f }, .uv = { 1, 1 } },

    // +Y
    { .position = { 0.0f, 1.0f, 0.0f }, .uv = { 0, 1 } },
    { .position = { 0.0f, 1.0f, 1.0f }, .uv = { 0, 0 } },
    { .position = { 1.0f, 1.0f, 1.0f }, .uv = { 1, 0 } },
    { .position = { 1.0f, 1.0f, 0.0f }, .uv = { 1, 1 } },

    // -Y
    { .position = { 0.0f, 0.0f, 1.0f }, .uv = { 0, 1 } },
    { .position = { 0.0f, 0.0f, 0.0f }, .uv = { 0, 0 } },
    { .position = { 1.0f, 0.0f, 0.0f }, .uv = { 1, 0 } },
    { .position = { 1.0f, 0.0f, 1.0f }, .uv = { 1, 1 } },

    // +Z
    { .position = { 1.0f, 0.0f, 1.0f }, .uv = { 0, 1 } },
    { .position = { 1.0f, 1.0f, 1.0f }, .uv = { 0, 0 } },
    { .position = { 0.0f, 1.0f, 1.0f }, .uv = { 1, 0 } },
    { .position = { 0.0f, 0.0f, 1.0f }, .uv = { 1, 1 } },

    // -Z
    { .position = { 0.0f, 0.0f, 0.0f }, .uv = { 0, 1 } },
    { .position = { 0.0f, 1.0f, 0.0f }, .uv = { 0, 0 } },
    { .position = { 1.0f, 1.0f, 0.0f }, .uv = { 1, 0 } },
    { .position = { 1.0f, 0.0f, 0.0f }, .uv = { 1, 1 } },
};

uint32 cube_repeating_indexes[] =
{
    // +X
    0, 1, 2,
    0, 2, 3,

    // -X
    4, 5, 6,
    4, 6, 7,

    // +Y
    8, 9, 10,
    8, 10, 11,

    // -Y
    12, 13, 14,
    12, 14, 15,

    // +Z
    16, 17, 18,
    16, 18, 19,

    // -Z
    20, 21, 22,
    20, 22, 23,
};
