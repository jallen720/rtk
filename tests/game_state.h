/// Data
////////////////////////////////////////////////////////////
struct Mouse
{
    Vec2<sint32> position;
    Vec2<sint32> delta;
    Vec2<sint32> last_position;
};

struct Entity
{
    uint8 rotate_axis;
    sint8 rotate_direction;
};

struct EntityData
{
    Transform transforms     [MAX_ENTITIES];
    uint32    texture_indexes[MAX_ENTITIES];
    uint32    sampler_indexes[MAX_ENTITIES];
    Entity    entities       [MAX_ENTITIES];
    uint32    count;
};

struct GameState
{
    Mouse      mouse;
    View       view;
    EntityData entity_data;
};

/// Instance
////////////////////////////////////////////////////////////
static GameState game_state;

/// Utils
////////////////////////////////////////////////////////////
static uint32 PushEntity(EntityData* entity_data)
{
    if (entity_data->count == MAX_ENTITIES)
    {
        CTK_FATAL("can't push another entity: entity count (%u) at max (%u)", entity_data->count, MAX_ENTITIES);
    }

    uint32 new_entity = entity_data->count;
    ++entity_data->count;
    return new_entity;
}

static uint32 TextureIndex(uint32 entity_index)
{
    return (entity_index / (MAX_ENTITIES / TEXTURE_COUNT)) % TEXTURE_COUNT;
}

static uint32 SamplerIndex(uint32 entity_index)
{
    static constexpr uint32 SAMPLER_INDEXES[TEXTURE_COUNT] = { 0, 0, 1, 0, 1, 1 };
    return SAMPLER_INDEXES[TextureIndex(entity_index)];
}

static void UpdateMouse(Mouse* mouse)
{
    mouse->position      = GetMousePosition();
    mouse->delta         = mouse->position - mouse->last_position;
    mouse->last_position = mouse->position;
}

static void UpdateView()
{
    View* view = &game_state.view;

    // Translation Input
    static constexpr float32 BASE_TRANSLATION_SPEED = 0.01f;
    float32 mod = KeyDown(Key::SHIFT) ? 32.0f : 4.0f;
    float32 translation_speed = BASE_TRANSLATION_SPEED * mod;
    Vec3<float32> translation = {};

    if (KeyDown(Key::W)) { translation.z += translation_speed; }
    if (KeyDown(Key::S)) { translation.z -= translation_speed; }
    if (KeyDown(Key::D)) { translation.x += translation_speed; }
    if (KeyDown(Key::A)) { translation.x -= translation_speed; }
    if (KeyDown(Key::E)) { translation.y += translation_speed; }
    if (KeyDown(Key::Q)) { translation.y -= translation_speed; }

    // Local Translation
    Matrix matrix = ID_MATRIX;
    matrix = RotateX(matrix, view->rotation.x);
    matrix = RotateY(matrix, view->rotation.y);
    matrix = RotateZ(matrix, view->rotation.z);
    matrix = Translate(matrix, translation);

    Vec3<float32> forward =
    {
        .x = Get(&matrix, 0, 2),
        .y = Get(&matrix, 1, 2),
        .z = Get(&matrix, 2, 2),
    };
    Vec3<float32> right =
    {
        .x = Get(&matrix, 0, 0),
        .y = Get(&matrix, 1, 0),
        .z = Get(&matrix, 2, 0),
    };
    view->position = view->position + (forward * translation.z);
    view->position = view->position + (right   * translation.x);
    view->position.y += translation.y;

    // Rotation
    if (MouseButtonDown(1))
    {
        static constexpr float32 ROTATION_SPEED = 0.1f;
        view->rotation.x -= game_state.mouse.delta.y * ROTATION_SPEED;
        view->rotation.y -= game_state.mouse.delta.x * ROTATION_SPEED;
        view->rotation.x = Clamp(view->rotation.x, -view->max_x_angle, view->max_x_angle);
    }
}

static void UpdateEntities()
{
    // EntityData* entity_data = &game_state.entity_data;
    // for (uint32 entity_index = 0; entity_index < entity_data->count; ++entity_index)
    // {
    //     Transform* transform = &entity_data->transforms[entity_index];
    //     Entity* entity = &entity_data->entities[entity_index];

    //     static constexpr float32 ROTATION_SPEED = 0.1f;
    //     float32 rotation = entity->rotate_direction * ROTATION_SPEED;
    //     if (entity->rotate_axis == 0)
    //     {
    //         transform->rotation.x += rotation;
    //     }
    //     else if (entity->rotate_axis == 1)
    //     {
    //         transform->rotation.y += rotation;
    //     }
    //     else
    //     {
    //         transform->rotation.z += rotation;
    //     }
    // }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitGameState(Stack* perm_stack)
{
    // View
    game_state.view =
    {
        .position     = { (CUBE_SIZE / 2) * 1.5f, (CUBE_SIZE / 2) * 1.5f, -(CUBE_SIZE / 2) },
        .rotation     = { 0, 0, 0 },
        .vertical_fov = 90.0f,
        .z_near       = 0.1f,
        .z_far        = 1000.0f,
        .max_x_angle  = 85.0f,
    };

    // Entities
    EntityData* entity_data = &game_state.entity_data;
    for (uint32 x = 0; x < CUBE_SIZE; ++x)
    for (uint32 y = 0; y < CUBE_SIZE; ++y)
    for (uint32 z = 0; z < CUBE_SIZE; ++z)
    {
        uint32 entity_index = PushEntity(&game_state.entity_data);
        entity_data->transforms[entity_index] =
        {
            .position = { x * 1.5f, y * 1.5f, z * 1.5f },
            .rotation = { 0, 0, 0 },
        };
        entity_data->entities[entity_index] =
        {
            .rotate_axis      = (uint8)RandomRange(0u, 3u),
            .rotate_direction = (sint8)(RandomRange(0u, 2u) ? -1 : 1),
        };
        entity_data->texture_indexes[entity_index] = TextureIndex(entity_index);
        entity_data->sampler_indexes[entity_index] = SamplerIndex(entity_index);
    }

    // Set texture & sampler indexes for all frames. Move to update step if these could change during update.
    for (uint32 frame_index = 0; frame_index < GetFrameCount(); ++frame_index)
    {
        SetTextureIndexes(entity_data->texture_indexes, entity_data->count, frame_index);
        SetSamplerIndexes(entity_data->sampler_indexes, entity_data->count, frame_index);
    }
}

static void UpdateGame()
{
    // If window will close, skip all other controls.
    if (KeyDown(Key::ESCAPE))
    {
        CloseWindow();
        return;
    }

    UpdateMouse(&game_state.mouse);
    UpdateView();
    UpdateEntities();
}

static View* GetView()
{
    return &game_state.view;
}

static EntityData* GetEntityData()
{
    return &game_state.entity_data;
}
