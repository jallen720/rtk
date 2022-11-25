#pragma once

#include "ctk2/ctk.h"
#include "ctk2/memory.h"
#include "ctk2/pool.h"
#include "rtk/memory.h"

using namespace CTK;

namespace RTK
{

/// Resource Definitions
////////////////////////////////////////////////////////////
#define RTK_DEFINE_RESOURCE(TYPE) \
    using TYPE##Hnd = PoolHnd<TYPE>; \
    struct TYPE;

RTK_DEFINE_RESOURCE(Buffer)

/// Data
////////////////////////////////////////////////////////////
struct RTKStateInfo
{
    uint32 max_buffers;
};

struct RTKState
{
    Pool<Buffer> buffer_pool;
};

/// Interface
////////////////////////////////////////////////////////////
static void InitRTKState(RTKState* state, Stack* mem, RTKStateInfo* info)
{
    InitPool(&state->buffer_pool, mem, info->max_buffers);
}

}
