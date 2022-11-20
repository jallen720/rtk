#pragma once

#include "ctk2/ctk.h"
#include "ctk2/memory.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
using PoolHnd = uint32;

static constexpr PoolHnd NO_NODE = U32_MAX;

template<typename Type>
union RTKPoolNode
{
    Type    data;
    PoolHnd next;
};

template<typename Type>
struct RTKPool
{
    RTKPoolNode<Type>* nodes;
    uint32             size;
    PoolHnd            next_free;
};

/// Utils
////////////////////////////////////////////////////////////
template<typename Type>
static void ValidatePoolCanAllocate(RTKPool<Type>* pool)
{
    if (pool->next_free == NO_NODE)
        CTK_FATAL("can't allocate node from pool: pool has no free nodes");
}

/// Interface
////////////////////////////////////////////////////////////
template<typename Type>
static void InitRTKPool(RTKPool<Type>* pool, Stack* mem, uint32 size)
{
    pool->nodes     = Allocate<RTKPoolNode<Type>>(mem, size);
    pool->size      = size;
    pool->next_free = 0;

    // Point all nodes to next node in array, and last node to nothing.
    for (PoolHnd hnd = 0; hnd < size - 1; ++hnd)
        pool->nodes[hnd].next = hnd + 1;

    pool->nodes[size - 1].next = NO_NODE;
}

template<typename Type>
static PoolHnd AllocGetHnd(RTKPool<Type>* pool)
{
    ValidatePoolCanAllocate(pool);
    PoolHnd allocated_node = pool->next_free;
    pool->next_free = pool->nodes[allocated_node].next;
    return allocated_node;
}

template<typename Type>
static Type* AllocGetPtr(RTKPool<Type>* pool)
{
    ValidatePoolCanAllocate(pool);
    RTKPoolNode<Type>* allocated_node = pool->nodes + pool->next_free;
    pool->next_free = allocated_node->next;
    return &allocated_node->data;
}

template<typename Type>
static Type* GetDataPtr(RTKPool<Type>* pool, PoolHnd hnd)
{
    if (hnd >= pool->size)
        CTK_FATAL("can't access pool node at handle %u: handle exceeds max handle %u", hnd, pool->size - 1);

    return &pool->nodes[hnd].data;
}

}
