#include "block.h"
#include <stdlib.h>
#include "../test/assert.h"

MemBlock mem_block_make(void* data, size_t size)
{
    MemBlock block;

    block.data = data;
    block.size = size;
    block.capacity = size;

    return block;
}

MemBlock mem_block_alloc(size_t initial_capacity)
{
    MemBlock block;

    block.data = (initial_capacity > 0) ? malloc(initial_capacity) : NULL;
    block.size = 0;
    block.capacity = initial_capacity;

    assert(initial_capacity == 0 || block.data != NULL);

    return block;
}

MemBlock mem_block_slice(
    MemBlock* block,
    size_t slice_byte_offset,
    size_t slice_byte_length
)
{
    assert((slice_byte_offset + slice_byte_length) <= block->size);
    uint8_t* bytes = block->data;

    MemBlock slice;
    slice.data = bytes + slice_byte_offset;
    slice.size = slice_byte_length;
    slice.capacity = slice_byte_length;
    return slice;
}

void mem_block_resize(MemBlock* block, size_t new_size)
{
    if(block->capacity < new_size)
    {
        if(block->capacity > 0) {
            free(block->data);
        }

        *block = mem_block_alloc(new_size);
    }
    block->size = new_size;
}

void mem_block_free(MemBlock* block)
{
    if(block->capacity > 0) {
        free(block->data);
        block->data = NULL;
        block->size = 0;
        block->capacity = 0;
    }
}
