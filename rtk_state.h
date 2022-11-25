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

/// Data
////////////////////////////////////////////////////////////
struct RTKStateInfo
{
    uint32 max_buffers;
    uint32 max_images;
    uint32 max_shader_datas;
};

struct RTKState
{
    Pool<Buffer>     buffers;
    Pool<Image>      images;
    Pool<ShaderData> shader_datas;
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

}
