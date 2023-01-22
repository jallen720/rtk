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
struct Image;
struct ShaderData;
struct ShaderDataSet;
struct MeshData;
struct Mesh;
struct RenderTarget;
struct Pipeline;

using BufferHnd        = PoolHnd<Buffer>;
using ImageHnd         = PoolHnd<Image>;
using ShaderDataHnd    = PoolHnd<ShaderData>;
using ShaderDataSetHnd = PoolHnd<ShaderDataSet>;
using MeshDataHnd      = PoolHnd<MeshData>;
using MeshHnd          = PoolHnd<Mesh>;
using RenderTargetHnd  = PoolHnd<RenderTarget>;
using PipelineHnd      = PoolHnd<Pipeline>;

/// Data
////////////////////////////////////////////////////////////
struct ResourcesInfo
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

struct Resources
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
static Resources global_resources;

/// Interface
////////////////////////////////////////////////////////////
static void InitResources(Stack* perm_stack, ResourcesInfo* info)
{
    InitPool(&global_resources.buffers,          perm_stack, info->max_buffers);
    InitPool(&global_resources.images,           perm_stack, info->max_images);
    InitPool(&global_resources.shader_datas,     perm_stack, info->max_shader_datas);
    InitPool(&global_resources.shader_data_sets, perm_stack, info->max_shader_data_sets);
    InitPool(&global_resources.mesh_datas,       perm_stack, info->max_mesh_datas);
    InitPool(&global_resources.meshes,           perm_stack, info->max_meshes);
    InitPool(&global_resources.render_targets,   perm_stack, info->max_render_targets);
    InitPool(&global_resources.pipelines,        perm_stack, info->max_pipelines);
}

/// Allocate Functions
////////////////////////////////////////////////////////////
static BufferHnd AllocateBuffer()
{
    return Allocate(&global_resources.buffers);
}

static ImageHnd AllocateImage()
{
    return Allocate(&global_resources.images);
}

static ShaderDataHnd AllocateShaderData()
{
    return Allocate(&global_resources.shader_datas);
}

static ShaderDataSetHnd AllocateShaderDataSet()
{
    return Allocate(&global_resources.shader_data_sets);
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

/// Get Functions
////////////////////////////////////////////////////////////
static Buffer* GetBuffer(BufferHnd hnd)
{
    return GetData(&global_resources.buffers, hnd);
}

static Image* GetImage(ImageHnd hnd)
{
    return GetData(&global_resources.images, hnd);
}

static ShaderData* GetShaderData(ShaderDataHnd hnd)
{
    return GetData(&global_resources.shader_datas, hnd);
}

static ShaderDataSet* GetShaderDataSet(ShaderDataSetHnd hnd)
{
    return GetData(&global_resources.shader_data_sets, hnd);
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
