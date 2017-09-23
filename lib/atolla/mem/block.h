#ifndef MSG_BLOCK_H
#define MSG_BLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../atolla/primitives.h"

/**
 * References a block of dynamically allocated memory.
 */
struct MemBlock
{
    void* data;
    size_t size;
    size_t capacity;
};
typedef struct MemBlock MemBlock;

/**
 * Creates a new memory block with the given data. Size and capacity are both
 * set to the given size. No dynamic allocation is performed by this function.
 * Note that memory blocks created with this function can only be resized if the
 * given pointer was allocated by malloc and ownership is effectively
 * transferred to the memory block from the user code.
 */
MemBlock mem_block_make(void* data, size_t size);

/**
 * Creates a new memory block with the given capacity. A capacity of 0 is valid
 * and results in no calls to malloc.
 */
MemBlock mem_block_alloc(size_t initial_capacity);

/**
 * Returns a MemBlock referencing memory inside the given block. No heap
 * allocation takes place, the returned block references the same memory as the
 * original.
 */
MemBlock mem_block_slice(
    MemBlock* block,
    size_t slice_byte_offset,
    size_t slice_byte_length
);

/**
 * Ensures a minimum size for the memory block. If the current capacity is
 * lower than the desired size, frees the memory currently referenced by the
 * memory block and replaces it with newly allocated memory with a capacity
 * equal to the given size. Does not reallocate if the memory block is already
 * large enough.
 */
void mem_block_resize(MemBlock* block, size_t new_size);

/**
 * Frees the memory referenced by the memory block. This effectively resizes
 * the memory block to zero. Note that the memory occupied by the MemBlock
 * structure itself is managed by the module user.
 */
void mem_block_free(MemBlock* block);

#ifdef __cplusplus
}
#endif

#endif // MSG_BLOCK_H
