Vertex vertexes[] =
{
    // -Z
    { .position = { 0.0f, 0.0f, 0.0f } },
    { .position = { 0.0f, 1.0f, 0.0f } },
    { .position = { 1.0f, 1.0f, 0.0f } },
    { .position = { 1.0f, 0.0f, 0.0f } },

    // +Z
    { .position = { 1.0f, 0.0f, 1.0f } },
    { .position = { 1.0f, 1.0f, 1.0f } },
    { .position = { 0.0f, 1.0f, 1.0f } },
    { .position = { 0.0f, 0.0f, 1.0f } },

    // -X
    { .position = { 0.0f, 0.0f, 1.0f } },
    { .position = { 0.0f, 1.0f, 1.0f } },
    { .position = { 0.0f, 1.0f, 0.0f } },
    { .position = { 0.0f, 0.0f, 0.0f } },

    // +X
    { .position = { 1.0f, 0.0f, 0.0f } },
    { .position = { 1.0f, 1.0f, 0.0f } },
    { .position = { 1.0f, 1.0f, 1.0f } },
    { .position = { 1.0f, 0.0f, 1.0f } },

    // -Y
    { .position = { 0.0f, 0.0f, 1.0f } },
    { .position = { 0.0f, 0.0f, 0.0f } },
    { .position = { 1.0f, 0.0f, 0.0f } },
    { .position = { 1.0f, 0.0f, 1.0f } },

    // +Y
    { .position = { 0.0f, 1.0f, 0.0f } },
    { .position = { 0.0f, 1.0f, 1.0f } },
    { .position = { 1.0f, 1.0f, 1.0f } },
    { .position = { 1.0f, 1.0f, 0.0f } },
};

uint32 indexes[] =
{
    // -Z
    0, 1, 2,
    0, 2, 3,

    // +Z
    4, 5, 6,
    4, 6, 7,

    // -X
    8, 9, 10,
    8, 10, 11,

    // +X
    12, 13, 14,
    12, 14, 15,

    // -Y
    16, 17, 18,
    16, 18, 19,

    // +Y
    20, 21, 22,
    20, 22, 23,
};
