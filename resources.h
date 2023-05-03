#pragma once

#include "ctk3/ctk3.h"
#include "ctk3/stack.h"
#include "ctk3/pool.h"

using namespace CTK;

namespace RTK
{

/// Resource Definitions
////////////////////////////////////////////////////////////
struct Buffer;
struct ShaderData;
struct ShaderDataSet;
struct Shader;
struct MeshData;
struct Mesh;
struct RenderTarget;
struct Pipeline;

using BufferHnd        = PoolHnd<Buffer>;
using ShaderDataHnd    = PoolHnd<ShaderData>;
using ShaderDataSetHnd = PoolHnd<ShaderDataSet>;
using ShaderHnd        = PoolHnd<Shader>;
using MeshDataHnd      = PoolHnd<MeshData>;
using MeshHnd          = PoolHnd<Mesh>;
using RenderTargetHnd  = PoolHnd<RenderTarget>;
using PipelineHnd      = PoolHnd<Pipeline>;

/// Data
////////////////////////////////////////////////////////////
struct ResourcesInfo
{
    uint32 max_buffers;
    uint32 max_shader_datas;
    uint32 max_shader_data_sets;
    uint32 max_shaders;
    uint32 max_mesh_datas;
    uint32 max_meshes;
    uint32 max_render_targets;
    uint32 max_pipelines;
};

struct Resources
{
    Pool<Buffer>        buffers;
    Pool<ShaderData>    shader_datas;
    Pool<ShaderDataSet> shader_data_sets;
    Pool<Shader>        shaders;
    Pool<MeshData>      mesh_datas;
    Pool<Mesh>          meshes;
    Pool<RenderTarget>  render_targets;
    Pool<Pipeline>      pipelines;
};

/// Instance
////////////////////////////////////////////////////////////
static Resources global_resources;

/// Interface
////////////////////////////////////////////////////////////
static void InitResources(Stack* perm_stack, ResourcesInfo* info)
{
    InitPool(&global_resources.buffers,          &perm_stack->allocator, info->max_buffers);
    InitPool(&global_resources.shader_datas,     &perm_stack->allocator, info->max_shader_datas);
    InitPool(&global_resources.shader_data_sets, &perm_stack->allocator, info->max_shader_data_sets);
    InitPool(&global_resources.shaders,          &perm_stack->allocator, info->max_shaders);
    InitPool(&global_resources.mesh_datas,       &perm_stack->allocator, info->max_mesh_datas);
    InitPool(&global_resources.meshes,           &perm_stack->allocator, info->max_meshes);
    InitPool(&global_resources.render_targets,   &perm_stack->allocator, info->max_render_targets);
    InitPool(&global_resources.pipelines,        &perm_stack->allocator, info->max_pipelines);
}

/// Allocate Functions
////////////////////////////////////////////////////////////
static BufferHnd AllocateBuffer()
{
    return Allocate(&global_resources.buffers);
}

static ShaderDataHnd AllocateShaderData()
{
    return Allocate(&global_resources.shader_datas);
}

static ShaderDataSetHnd AllocateShaderDataSet()
{
    return Allocate(&global_resources.shader_data_sets);
}

static ShaderHnd AllocateShader()
{
    return Allocate(&global_resources.shaders);
}

static MeshDataHnd AllocateMeshData()
{
    return Allocate(&global_resources.mesh_datas);
}

static MeshHnd AllocateMesh()
{
    return Allocate(&global_resources.meshes);
}

static RenderTargetHnd AllocateRenderTarget()
{
    return Allocate(&global_resources.render_targets);
}

static PipelineHnd AllocatePipeline()
{
    return Allocate(&global_resources.pipelines);
}

/// Deallocate Functions
////////////////////////////////////////////////////////////
static void DeallocateBuffer(BufferHnd buffer_hnd)
{
    Deallocate(&global_resources.buffers, buffer_hnd);
}

static void DeallocateShaderData(ShaderDataHnd shader_data_hnd)
{
    Deallocate(&global_resources.shader_datas, shader_data_hnd);
}

static void DeallocateShaderDataSet(ShaderDataSetHnd shader_data_set_hnd)
{
    Deallocate(&global_resources.shader_data_sets, shader_data_set_hnd);
}

static void DeallocateShader(ShaderHnd shader_hnd)
{
    Deallocate(&global_resources.shaders, shader_hnd);
}

static void DeallocateMeshData(MeshDataHnd mesh_data_hnd)
{
    Deallocate(&global_resources.mesh_datas, mesh_data_hnd);
}

static void DeallocateMesh(MeshHnd mesh_hnd)
{
    Deallocate(&global_resources.meshes, mesh_hnd);
}

static void DeallocateRenderTarget(RenderTargetHnd render_target_hnd)
{
    Deallocate(&global_resources.render_targets, render_target_hnd);
}

static void DeallocatePipeline(PipelineHnd pipeline_hnd)
{
    Deallocate(&global_resources.pipelines, pipeline_hnd);
}

/// Get Functions
////////////////////////////////////////////////////////////
static Buffer* GetBuffer(BufferHnd hnd)
{
    return GetData(&global_resources.buffers, hnd);
}

static ShaderData* GetShaderData(ShaderDataHnd hnd)
{
    return GetData(&global_resources.shader_datas, hnd);
}

static ShaderDataSet* GetShaderDataSet(ShaderDataSetHnd hnd)
{
    return GetData(&global_resources.shader_data_sets, hnd);
}

static Shader* GetShader(ShaderHnd hnd)
{
    return GetData(&global_resources.shaders, hnd);
}

static MeshData* GetMeshData(MeshDataHnd hnd)
{
    return GetData(&global_resources.mesh_datas, hnd);
}

static Mesh* GetMesh(MeshHnd hnd)
{
    return GetData(&global_resources.meshes, hnd);
}

static RenderTarget* GetRenderTarget(RenderTargetHnd hnd)
{
    return GetData(&global_resources.render_targets, hnd);
}

static Pipeline* GetPipeline(PipelineHnd hnd)
{
    return GetData(&global_resources.pipelines, hnd);
}

}
