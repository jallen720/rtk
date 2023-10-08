/// Data
////////////////////////////////////////////////////////////
struct View
{
    Vec3<float32> position;
    Vec3<float32> rotation;
    float32       vertical_fov;
    float32       z_near;
    float32       z_far;
    float32       max_x_angle;
};

struct Transform
{
    Vec3<float32> position;
    Vec3<float32> rotation;
};

struct Entity
{
    uint8 rotate_axis;
    sint8 rotate_direction;
};

struct EntityData
{
    Transform transforms[MAX_ENTITIES];
    Entity    entities[MAX_ENTITIES];
    uint32    texture_indexes[MAX_ENTITIES];
    uint32    count;
};

struct MVPMatrixState
{
    BatchRange    batch_range;
    Matrix        view_projection_matrix;
    EntityBuffer* frame_entity_buffer;
    EntityData*   entity_data;
};

struct GameState
{
    Job<MVPMatrixState> mvp_matrix_job;
    Mouse               mouse;
    View                view;
    EntityData          entity_data;
};

/// Instance
////////////////////////////////////////////////////////////
static GameState game_state;

/// Utils
////////////////////////////////////////////////////////////
static void InitView()
{
    game_state.view =
    {
        .position     = { 32 * 1.5f, 32 * 1.5f, -32 },
        .rotation     = { 0, 0, 0 },
        .vertical_fov = 90.0f,
        .z_near       = 0.1f,
        .z_far        = 1000.0f,
        .max_x_angle  = 85.0f,
    };
}

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

static void InitEntities()
{
    static constexpr uint32 CUBE_SIZE = 66;
    static constexpr uint32 CUBE_ENTITY_COUNT = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE;
    static_assert(CUBE_ENTITY_COUNT <= MAX_ENTITIES);
    EntityData* entity_data = &game_state.entity_data;
    for (uint32 x = 0; x < CUBE_SIZE; ++x)
    for (uint32 y = 0; y < CUBE_SIZE; ++y)
    for (uint32 z = 0; z < CUBE_SIZE; ++z)
    {
        uint32 entity = PushEntity(&game_state.entity_data);
        entity_data->transforms[entity] =
        {
            .position = { x * 1.5f, y * 1.5f, z * 1.5f },
            .rotation = { 0, 0, 0 },
        };
        entity_data->entities[entity] =
        {
            .rotate_axis      = (uint8)RandomRange(0u, 3u),
            .rotate_direction = (sint8)(RandomRange(0u, 2u) ? -1 : 1),
        };
        entity_data->texture_indexes[entity] = entity < CUBE_ENTITY_COUNT / 3 ? 0 :
                                               entity < 2 * (CUBE_ENTITY_COUNT / 3) ? 1 :
                                               2;
    }
}

static void UpdateView()
{
    View* view = &game_state.view;

    // Translation Input
    static constexpr float32 BASE_TRANSLATION_SPEED = 0.01f;
    float32 mod = KeyDown(KEY_SHIFT) ? 8.0f : 1.0f;
    float32 translation_speed = BASE_TRANSLATION_SPEED * mod;
    Vec3<float32> translation = {};

    if (KeyDown(KEY_W)) { translation.z += translation_speed; }
    if (KeyDown(KEY_S)) { translation.z -= translation_speed; }
    if (KeyDown(KEY_D)) { translation.x += translation_speed; }
    if (KeyDown(KEY_A)) { translation.x -= translation_speed; }
    if (KeyDown(KEY_E)) { translation.y += translation_speed; }
    if (KeyDown(KEY_Q)) { translation.y -= translation_speed; }

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
    view->position = view->position + (right * translation.x);
    view->position.y += translation.y;

    // Rotation
    if (MouseButtonDown(1))
    {
        static constexpr float32 ROTATION_SPEED = 0.2f;
        view->rotation.x -= game_state.mouse.delta.y * ROTATION_SPEED;
        view->rotation.y -= game_state.mouse.delta.x * ROTATION_SPEED;
        view->rotation.x = Clamp(view->rotation.x, -view->max_x_angle, view->max_x_angle);
    }
}

static void UpdateEntities()
{
    EntityData* entity_data = &game_state.entity_data;
    for (uint32 entity_index = 0; entity_index < entity_data->count; ++entity_index)
    {
        Transform* transform = entity_data->transforms + entity_index;
        Entity* entity = entity_data->entities + entity_index;

        static constexpr float32 ROTATION_SPEED = 0.1f;
        float32 rotation = entity->rotate_direction * ROTATION_SPEED;
        if (entity->rotate_axis == 0)
        {
            transform->rotation.x += rotation;
        }
        else if (entity->rotate_axis == 1)
        {
            transform->rotation.y += rotation;
        }
        else
        {
            transform->rotation.z += rotation;
        }
    }
}

static Matrix GetViewProjectionMatrix()
{
    View* view = &game_state.view;

    // View Matrix
    Matrix view_model_matrix = ID_MATRIX;
    view_model_matrix = RotateX(view_model_matrix, view->rotation.x);
    view_model_matrix = RotateY(view_model_matrix, view->rotation.y);
    view_model_matrix = RotateZ(view_model_matrix, view->rotation.z);
    Vec3<float32> forward =
    {
        .x = Get(&view_model_matrix, 0, 2),
        .y = Get(&view_model_matrix, 1, 2),
        .z = Get(&view_model_matrix, 2, 2),
    };
    Matrix view_matrix = LookAt(view->position, view->position + forward, { 0.0f, -1.0f, 0.0f });

    // Projection Matrix
    VkExtent2D swapchain_extent = GetSwapchain()->surface_extent;
    float32 swapchain_aspect_ratio = (float32)swapchain_extent.width / swapchain_extent.height;
    Matrix projection_matrix = GetPerspectiveMatrix(view->vertical_fov, swapchain_aspect_ratio,
                                                    view->z_near, view->z_far);

    return projection_matrix * view_matrix;
}

static void UpdateMVPMatrixesThread(void* data)
{
    auto state = (MVPMatrixState*)data;
    BatchRange batch_range = state->batch_range;
    Matrix view_projection_matrix = state->view_projection_matrix;
    EntityBuffer* frame_entity_buffer = state->frame_entity_buffer;
    EntityData* entity_data = state->entity_data;

    // Update entity MVP matrixes.
    for (uint32 i = batch_range.start; i < batch_range.start + batch_range.size; ++i)
    {
        Transform* entity_transform = game_state.entity_data.transforms + i;
        Matrix model_matrix = ID_MATRIX;
        model_matrix = Translate(model_matrix, entity_transform->position);
        model_matrix = RotateX(model_matrix, entity_transform->rotation.x);
        model_matrix = RotateY(model_matrix, entity_transform->rotation.y);
        model_matrix = RotateZ(model_matrix, entity_transform->rotation.z);
        // model_matrix = Scale(model_matrix, entity_transform->scale);

        frame_entity_buffer->mvp_matrixes[i] = view_projection_matrix * model_matrix;
        frame_entity_buffer->texture_indexes[i] = entity_data->texture_indexes[i];
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitGameState(Stack* perm_stack, uint32 job_task_count)
{
    InitThreadPoolJob(&game_state.mvp_matrix_job, perm_stack, job_task_count);
    InitView();
    InitEntities();
}

static void UpdateGame()
{
    // If window will close, skip all other controls.
    if (KeyDown(KEY_ESCAPE))
    {
        CloseWindow();
        return;
    }

    UpdateMouse(&game_state.mouse);
    UpdateView();
    UpdateEntities();
}

static void UpdateMVPMatrixes(ThreadPool* thread_pool)
{
    Job<MVPMatrixState>* job = &game_state.mvp_matrix_job;
    Matrix view_projection_matrix = GetViewProjectionMatrix();
    auto frame_entity_buffer = GetHostMemory<EntityBuffer>(GetEntityBuffer(), GetFrameIndex());
    uint32 thread_count = thread_pool->size;

    // Initialize thread states and submit tasks.
    for (uint32 thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        MVPMatrixState* state = GetPtr(&job->states, thread_index);
        state->batch_range            = GetBatchRange(thread_index, thread_count, game_state.entity_data.count);
        state->view_projection_matrix = view_projection_matrix;
        state->frame_entity_buffer    = frame_entity_buffer;
        state->entity_data            = &game_state.entity_data;

        Set(&job->tasks, thread_index, SubmitTask(thread_pool, state, UpdateMVPMatrixesThread));
    }

    // Wait for tasks to complete.
    CTK_ITER(task, &job->tasks)
    {
        Wait(thread_pool, *task);
    }
}

static uint32 GetEntityCount()
{
    return game_state.entity_data.count;
}
