#pragma once

#include "ctk3/ctk3.h"
#include "ctk3/debug.h"
#include "ctk3/math.h"
#include "ctk3/window.h"
#include "rtk/rtk.h"
#include "rtk/tests/shared.h"
#include "rtk/tests/3d/defs.h"

using namespace CTK;
using namespace RTK;

namespace Test3D
{

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
static void LocalTranslate(View* view, Vec3<float32> translation)
{
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
}

static void ViewControls()
{
    View* view = &game_state.view;

    // Translation
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

    LocalTranslate(view, translation);

    // Rotation
    if (MouseButtonDown(1))
    {
        static constexpr float32 ROTATION_SPEED = 0.2f;
        view->rotation.x -= game_state.mouse.delta.y * ROTATION_SPEED;
        view->rotation.y -= game_state.mouse.delta.x * ROTATION_SPEED;
        view->rotation.x = Clamp(view->rotation.x, -view->max_x_angle, view->max_x_angle);
    }
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

/// Interface
////////////////////////////////////////////////////////////
static void InitGameState()
{
    // View
    game_state.view =
    {
        .position     = { 0, 0, -1 },
        .rotation     = { 0, 0, 0 },
        .vertical_fov = 90.0f,
        .z_near       = 0.1f,
        .z_far        = 1000.0f,
        .max_x_angle  = 85.0f,
    };

    // Entities
    static constexpr uint32 CUBE_SIZE = 6;
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

static Transform* GetTransformPtr(EntityData* entity_data, uint32 index)
{
    if (index >= entity_data->count)
    {
        CTK_FATAL("can't access entity data at index %u: index exceeds highest index %u", index,
                  entity_data->count - 1);
    }
    return entity_data->transforms + index;
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
    ViewControls();

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

}
