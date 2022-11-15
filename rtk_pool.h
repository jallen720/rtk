#pragma once

#include "ctk2/ctk.h"
#include "ctk2/memory.h"

using namespace CTK;

namespace RTK
{

/// Data
////////////////////////////////////////////////////////////
static constexpr uint32 NO_NODE = U32_MAX;

template<typename Type>
union RTKPoolNode
{
    Type   data;
    uint32 next;
};

template<typename Type>
struct RTKPool
{
    RTKPoolNode<Type>* nodes;
    uint32             size;
    uint32             next_free;
};

template<typename Type>
static void InitRTKPool(RTKPool<Type>* pool, Stack* mem, uint32 size)
{
    pool->nodes     = Allocate<RTKPoolNode<Type>>(mem, size);
    pool->size      = size;
    pool->next_free = 0;

    // Point all nodes to next node in array, and last node to nothing.
    for (uint32 i = 0; i < size - 1; ++i)
        pool->nodes[i].next = i + 1;

    pool->nodes[size - 1].next = NO_NODE;
}

template<typename Type>
static uint32 Allocate(RTKPool<Type>* pool)
{
    if (pool->next_free == NO_NODE)
        CTK_FATAL("can't allocate node from pool: pool has no free nodes");

    uint32 allocated_node = pool->next_free;
    pool->next_free = pool->nodes[allocated_node].next;
    return allocated_node;
}

template<typename Type>
static Type* GetNodeData(RTKPool<Type>* pool, uint32 index)
{
    if (index >= pool->size)
        CTK_FATAL("can't access pool node at index %u: index exceeds max index %u", index, pool->size - 1);

    return &pool->nodes[index].data;
}

}
