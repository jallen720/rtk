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

/// Data
////////////////////////////////////////////////////////////
struct RTKStateInfo
{
    uint32 max_buffers;
};

struct RTKState
{
    Pool<Buffer> buffers;
};

/// Instance
////////////////////////////////////////////////////////////
static RTKState rtk_state;

/// Interface
////////////////////////////////////////////////////////////
static void InitRTKState(Stack* mem, RTKStateInfo* info)
{
    InitPool(&rtk_state.buffers, mem, info->max_buffers);
}

static BufferHnd AllocateBuffer()
{
    return Allocate(&rtk_state.buffers);
}

static Buffer* GetBuffer(BufferHnd hnd)
{
    return GetData(&rtk_state.buffers, hnd);
}

}
