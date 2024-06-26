#pragma once

#include "allocator.h"

namespace muli
{

// Stack allocator used for transient, predictable allocations.
// You muse nest allocate/free pairs
class StackAllocator : public Allocator
{
    static constexpr inline int32 stack_size = 100 * 1024;
    static constexpr inline int32 max_stack_entries = 32;

public:
    StackAllocator();
    ~StackAllocator();

    virtual void* Allocate(int32 size) override;
    virtual void Free(void* p, int32 size) override;
    virtual void Clear() override;

    int32 GetAllocation() const;
    int32 GetMaxAllocation() const;

private:
    struct StackEntry
    {
        int8* data;
        int32 size;
        bool mallocUsed;
    };

    int8 stack[stack_size];
    int32 index;

    int32 allocation;
    int32 maxAllocation;

    StackEntry entries[max_stack_entries];
    int32 entryCount;
};

inline int32 StackAllocator::GetAllocation() const
{
    return allocation;
}

inline int32 StackAllocator::GetMaxAllocation() const
{
    return maxAllocation;
}

} // namespace muli
