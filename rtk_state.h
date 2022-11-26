#pragma once

#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/pool.h"

using namespace CTK;

namespace RTK
{

/// Resource Definitions
////////////////////////////////////////////////////////////
#define RTK_RESOURCE_DEFINITION(TYPE) \
    struct TYPE; \
    using TYPE##Hnd = PoolHnd<TYPE>;

RTK_RESOURCE_DEFINITION(Buffer)
RTK_RESOURCE_DEFINITION(Image)
RTK_RESOURCE_DEFINITION(ShaderData)
RTK_RESOURCE_DEFINITION(ShaderDataSet)
RTK_RESOURCE_DEFINITION(MeshData)
RTK_RESOURCE_DEFINITION(Mesh)
RTK_RESOURCE_DEFINITION(RenderTarget)
RTK_RESOURCE_DEFINITION(Pipeline)

/// Data
////////////////////////////////////////////////////////////
struct RTKStateInfo
{
    uint32 max_buffers;
    uint32 max_images;
    uint32 max_shader_datas;
    uint32 max_shader_data_sets;
    uint32 max_mesh_datas;
    uint32 max_meshes;
    uint32 max_render_targets;
    uint32 max_pipelines;
};

struct RTKState
{
    Pool<Buffer>        buffers;
    Pool<Image>         images;
    Pool<ShaderData>    shader_datas;
    Pool<ShaderDataSet> shader_data_sets;
    Pool<MeshData>      mesh_datas;
    Pool<Mesh>          meshes;
    Pool<RenderTarget>  render_targets;
    Pool<Pipeline>      pipelines;
};

/// Instance
////////////////////////////////////////////////////////////
static RTKState rtk_state;

/// Interface
////////////////////////////////////////////////////////////
static void InitRTKState(Stack* mem, RTKStateInfo* info)
{
    InitPool(&rtk_state.buffers,          mem, info->max_buffers);
    InitPool(&rtk_state.images,           mem, info->max_images);
    InitPool(&rtk_state.shader_datas,     mem, info->max_shader_datas);
    InitPool(&rtk_state.shader_data_sets, mem, info->max_shader_data_sets);
    InitPool(&rtk_state.mesh_datas,       mem, info->max_mesh_datas);
    InitPool(&rtk_state.meshes,           mem, info->max_meshes);
    InitPool(&rtk_state.render_targets,   mem, info->max_render_targets);
    InitPool(&rtk_state.pipelines,        mem, info->max_pipelines);
}

#define RTK_RESOURCE_INTERFACE(TYPE, POOL_NAME) \
    static TYPE##Hnd Allocate##TYPE() \
    { \
        return Allocate(&rtk_state.##POOL_NAME); \
    } \
    static TYPE* Get##TYPE(TYPE##Hnd hnd) \
    { \
        return GetData(&rtk_state.##POOL_NAME, hnd); \
    }

RTK_RESOURCE_INTERFACE(Buffer, buffers)
RTK_RESOURCE_INTERFACE(Image, images)
RTK_RESOURCE_INTERFACE(ShaderData, shader_datas)
RTK_RESOURCE_INTERFACE(ShaderDataSet, shader_data_sets)
RTK_RESOURCE_INTERFACE(MeshData, mesh_datas)
RTK_RESOURCE_INTERFACE(Mesh, meshes)
RTK_RESOURCE_INTERFACE(RenderTarget, render_targets)
RTK_RESOURCE_INTERFACE(Pipeline, pipelines)

}
