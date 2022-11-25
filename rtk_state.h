#pragma once

#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/pool.h"

using namespace CTK;

namespace RTK
{

/// Resource Definitions
////////////////////////////////////////////////////////////
#define RTK_DEFINE_RESOURCE(TYPE) \
    struct TYPE; \
    using TYPE##Hnd = PoolHnd<TYPE>;

RTK_DEFINE_RESOURCE(Buffer)
RTK_DEFINE_RESOURCE(Image)
RTK_DEFINE_RESOURCE(ShaderData)
RTK_DEFINE_RESOURCE(ShaderDataSet)
RTK_DEFINE_RESOURCE(MeshData)
RTK_DEFINE_RESOURCE(Mesh)

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
};

struct RTKState
{
    Pool<Buffer>        buffers;
    Pool<Image>         images;
    Pool<ShaderData>    shader_datas;
    Pool<ShaderDataSet> shader_data_sets;
    Pool<MeshData>      mesh_datas;
    Pool<Mesh>          meshes;
};

/// Instance
////////////////////////////////////////////////////////////
static RTKState rtk_state;

/// Interface
////////////////////////////////////////////////////////////
static void InitRTKState(Stack* mem, RTKStateInfo* info)
{
    InitPool(&rtk_state.buffers, mem, info->max_buffers);
    InitPool(&rtk_state.images, mem, info->max_images);
    InitPool(&rtk_state.shader_datas, mem, info->max_shader_datas);
    InitPool(&rtk_state.shader_data_sets, mem, info->max_shader_data_sets);
    InitPool(&rtk_state.mesh_datas, mem, info->max_mesh_datas);
    InitPool(&rtk_state.meshes, mem, info->max_meshes);
}

static BufferHnd AllocateBuffer()
{
    return Allocate(&rtk_state.buffers);
}

static Buffer* GetBuffer(BufferHnd hnd)
{
    return GetData(&rtk_state.buffers, hnd);
}

static ImageHnd AllocateImage()
{
    return Allocate(&rtk_state.images);
}

static Image* GetImage(ImageHnd hnd)
{
    return GetData(&rtk_state.images, hnd);
}

static ShaderDataHnd AllocateShaderData()
{
    return Allocate(&rtk_state.shader_datas);
}

static ShaderData* GetShaderData(ShaderDataHnd hnd)
{
    return GetData(&rtk_state.shader_datas, hnd);
}

static ShaderDataSetHnd AllocateShaderDataSet()
{
    return Allocate(&rtk_state.shader_data_sets);
}

static ShaderDataSet* GetShaderDataSet(ShaderDataSetHnd hnd)
{
    return GetData(&rtk_state.shader_data_sets, hnd);
}

static MeshDataHnd AllocateMeshData()
{
    return Allocate(&rtk_state.mesh_datas);
}

static MeshData* GetMeshData(MeshDataHnd hnd)
{
    return GetData(&rtk_state.mesh_datas, hnd);
}

static MeshHnd AllocateMesh()
{
    return Allocate(&rtk_state.meshes);
}

static Mesh* GetMesh(MeshHnd hnd)
{
    return GetData(&rtk_state.meshes, hnd);
}

}
