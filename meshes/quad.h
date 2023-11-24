Vertex quad_vertexes[] =
{
    // -Z
    { .position = { 0.0f, 0.0f, 0.0f }, .uv = { 0.50f, 1.00f } },
    { .position = { 1.0f, 0.0f, 0.0f }, .uv = { 0.75f, 1.00f } },
    { .position = { 1.0f, 1.0f, 0.0f }, .uv = { 0.75f, 0.50f } },
    { .position = { 0.0f, 1.0f, 0.0f }, .uv = { 0.50f, 0.50f } },
};

uint32 quad_indexes[] =
{
    0, 1, 2,
    0, 2, 3,
};
