Vertex cube_vertexes[] =
{
    // +X
    { .position = { 1.0f, 0.0f, 0.0f }, .uv = { 0.00f, 0.50f } },
    { .position = { 1.0f, 0.0f, 1.0f }, .uv = { 0.25f, 0.50f } },
    { .position = { 1.0f, 1.0f, 1.0f }, .uv = { 0.25f, 0.00f } },
    { .position = { 1.0f, 1.0f, 0.0f }, .uv = { 0.00f, 0.00f } },

    // -X
    { .position = { 0.0f, 0.0f, 1.0f }, .uv = { 0.00f, 1.00f } },
    { .position = { 0.0f, 0.0f, 0.0f }, .uv = { 0.25f, 1.00f } },
    { .position = { 0.0f, 1.0f, 0.0f }, .uv = { 0.25f, 0.50f } },
    { .position = { 0.0f, 1.0f, 1.0f }, .uv = { 0.00f, 0.50f } },

    // +Y
    { .position = { 0.0f, 1.0f, 0.0f }, .uv = { 0.25f, 0.50f } },
    { .position = { 1.0f, 1.0f, 0.0f }, .uv = { 0.50f, 0.50f } },
    { .position = { 1.0f, 1.0f, 1.0f }, .uv = { 0.50f, 0.00f } },
    { .position = { 0.0f, 1.0f, 1.0f }, .uv = { 0.25f, 0.00f } },

    // -Y
    { .position = { 0.0f, 0.0f, 1.0f }, .uv = { 0.25f, 1.00f } },
    { .position = { 1.0f, 0.0f, 1.0f }, .uv = { 0.50f, 1.00f } },
    { .position = { 1.0f, 0.0f, 0.0f }, .uv = { 0.50f, 0.50f } },
    { .position = { 0.0f, 0.0f, 0.0f }, .uv = { 0.25f, 0.50f } },

    // +Z
    { .position = { 1.0f, 0.0f, 1.0f }, .uv = { 0.50f, 0.50f } },
    { .position = { 0.0f, 0.0f, 1.0f }, .uv = { 0.75f, 0.50f } },
    { .position = { 0.0f, 1.0f, 1.0f }, .uv = { 0.75f, 0.00f } },
    { .position = { 1.0f, 1.0f, 1.0f }, .uv = { 0.50f, 0.00f } },

    // -Z
    { .position = { 0.0f, 0.0f, 0.0f }, .uv = { 0.50f, 1.00f } },
    { .position = { 1.0f, 0.0f, 0.0f }, .uv = { 0.75f, 1.00f } },
    { .position = { 1.0f, 1.0f, 0.0f }, .uv = { 0.75f, 0.50f } },
    { .position = { 0.0f, 1.0f, 0.0f }, .uv = { 0.50f, 0.50f } },
};

uint32 cube_indexes[] =
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
