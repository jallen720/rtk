#pragma once

#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/pool.h"

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
static RTKState global_state;

/// Interface
////////////////////////////////////////////////////////////
static void InitRTKState(Stack* mem, RTKStateInfo* info)
{
    InitPool(&global_state.buffers,          mem, info->max_buffers);
    InitPool(&global_state.images,           mem, info->max_images);
    InitPool(&global_state.shader_datas,     mem, info->max_shader_datas);
    InitPool(&global_state.shader_data_sets, mem, info->max_shader_data_sets);
    InitPool(&global_state.mesh_datas,       mem, info->max_mesh_datas);
    InitPool(&global_state.meshes,           mem, info->max_meshes);
    InitPool(&global_state.render_targets,   mem, info->max_render_targets);
    InitPool(&global_state.pipelines,        mem, info->max_pipelines);
}

/// Allocate Functions
////////////////////////////////////////////////////////////
static BufferHnd AllocateBuffer()
{
    return Allocate(&global_state.buffers);
}

static ImageHnd AllocateImage()
{
    return Allocate(&global_state.images);
}

static ShaderDataHnd AllocateShaderData()
{
    return Allocate(&global_state.shader_datas);
}

static ShaderDataSetHnd AllocateShaderDataSet()
{
    return Allocate(&global_state.shader_data_sets);
}

static MeshDataHnd AllocateMeshData()
{
    return Allocate(&global_state.mesh_datas);
}

static MeshHnd AllocateMesh()
{
    return Allocate(&global_state.meshes);
}

static RenderTargetHnd AllocateRenderTarget()
{
    return Allocate(&global_state.render_targets);
}

static PipelineHnd AllocatePipeline()
{
    return Allocate(&global_state.pipelines);
}

/// Get Functions
////////////////////////////////////////////////////////////
static Buffer* GetBuffer(BufferHnd hnd)
{
    return GetData(&global_state.buffers, hnd);
}

static Image* GetImage(ImageHnd hnd)
{
    return GetData(&global_state.images, hnd);
}

static ShaderData* GetShaderData(ShaderDataHnd hnd)
{
    return GetData(&global_state.shader_datas, hnd);
}

static ShaderDataSet* GetShaderDataSet(ShaderDataSetHnd hnd)
{
    return GetData(&global_state.shader_data_sets, hnd);
}

static MeshData* GetMeshData(MeshDataHnd hnd)
{
    return GetData(&global_state.mesh_datas, hnd);
}

static Mesh* GetMesh(MeshHnd hnd)
{
    return GetData(&global_state.meshes, hnd);
}

static RenderTarget* GetRenderTarget(RenderTargetHnd hnd)
{
    return GetData(&global_state.render_targets, hnd);
}

static Pipeline* GetPipeline(PipelineHnd hnd)
{
    return GetData(&global_state.pipelines, hnd);
}

}
