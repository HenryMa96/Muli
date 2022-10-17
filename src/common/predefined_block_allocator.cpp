#include "muli/predefined_block_allocator.h"
#include "muli/block_allocator.h"

namespace muli
{

//                   Sizes
// circle           : 248
// capsule          : 264
// polygon          : 272

// Angle joint      : 216
// Grab joint       : 256
// Distance joint   : 256
// Revolute joint   : 264
// Prismatic joint  : 280
// Pulley joint     : 288
// Weld joint       : 296

// Predefined block sizes
static constexpr int32 blockSizes[predefinedBlockSizeCount] = {
    16,  // 0
    32,  // 1
    64,  // 2
    96,  // 3
    128, // 4
    160, // 5
    192, // 6
    224, // 7
    256, // 8
    320, // 9
    384, // 10
    448, // 11
    512, // 12
    640, // 13
};

static constexpr int32 chunkSize = 16 * 1024;
static constexpr int32 maxPredefinedBlockSize = blockSizes[predefinedBlockSizeCount - 1];

struct SizeMap
{
    SizeMap()
    {
        int32 j = 0;
        values[0] = 0;
        for (int32 i = 1; i <= maxPredefinedBlockSize; i++)
        {
            if (i <= blockSizes[j])
            {
                values[i] = j;
            }
            else
            {
                ++j;
                values[i] = j;
            }
        }
    }

    int32 values[maxPredefinedBlockSize + 1];
};

static const SizeMap sizeMap;

PredefinedBlockAllocator::PredefinedBlockAllocator()
{
    blockCount = 0;
    chunkCount = 0;
    chunks = nullptr;
    memset(freeList, 0, sizeof(freeList));
}

PredefinedBlockAllocator::~PredefinedBlockAllocator()
{
    Clear();
}

void* PredefinedBlockAllocator::Allocate(int32 size)
{
    if (size == 0)
    {
        return nullptr;
    }
    if (size > maxPredefinedBlockSize)
    {
        return malloc(size);
    }

    int32 index = sizeMap.values[size];
    assert(0 <= index && index <= predefinedBlockSizeCount);

    if (freeList[index] == nullptr)
    {
        Block* blocks = (Block*)malloc(chunkSize);
        int32 blockSize = blockSizes[index];
        int32 blockCapacity = chunkSize / blockSize;

        // Build a linked list for the free list.
        for (int32 i = 0; i < blockCapacity - 1; ++i)
        {
            Block* block = (Block*)((int8*)blocks + blockSize * i);
            Block* next = (Block*)((int8*)blocks + blockSize * (i + 1));
            block->next = next;
        }
        Block* last = (Block*)((int8*)blocks + blockSize * (blockCapacity - 1));
        last->next = nullptr;

        Chunk* newChunk = (Chunk*)malloc(sizeof(Chunk));
        newChunk->blockSize = blockSize;
        newChunk->blocks = blocks;
        newChunk->next = chunks;
        chunks = newChunk;
        ++chunkCount;

        freeList[index] = newChunk->blocks;
    }

    Block* block = freeList[index];
    freeList[index] = block->next;
    ++blockCount;

    return block;
}

void PredefinedBlockAllocator::Free(void* p, int32 size)
{
    if (size == 0)
    {
        return;
    }

    if (size > maxPredefinedBlockSize)
    {
        free(p);
        return;
    }

    assert(0 < size && size <= maxPredefinedBlockSize);

    int32 index = sizeMap.values[size];
    assert(0 <= index && index <= predefinedBlockSizeCount);

#if defined(_DEBUG)
    // Verify the memory address and size is valid.
    int32 blockSize = blockSizes[index];
    bool found = false;

    Chunk* chunk = chunks;
    while (chunk)
    {
        if (chunk->blockSize != blockSize)
        {
            assert((int8*)p + blockSize <= (int8*)chunk->blocks || (int8*)chunk->blocks + chunkSize <= (int8*)p);
        }
        else
        {
            if (((int8*)chunk->blocks <= (int8*)p && (int8*)p + blockSize <= (int8*)chunk->blocks + chunkSize))
            {
                found = true;
                break;
            }
        }

        chunk = chunk->next;
    }

    assert(found);
#endif

    Block* block = (Block*)p;
    block->next = freeList[index];
    freeList[index] = block;
    --blockCount;
}

void PredefinedBlockAllocator::Clear()
{
    Chunk* chunk = chunks;
    while (chunk)
    {
        Chunk* c0 = chunk;
        chunk = c0->next;
        free(c0->blocks);
        free(c0);
    }

    blockCount = 0;
    chunkCount = 0;
    chunks = nullptr;
    memset(freeList, 0, sizeof(freeList));
}

} // namespace  muli