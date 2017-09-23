#include "ring.h"

#include <string.h>

 MemRing mem_ring_alloc(size_t capacity)
 {
    MemRing ring = { mem_block_alloc(capacity), 0, 0 };
    ring.buf.size = capacity;
    return ring;
 }
 
 void mem_ring_free(MemRing* ring)
 {
    mem_block_free(&ring->buf);
    ring->front = 0;
    ring->len = 0;
 }

 bool mem_ring_is_empty(MemRing* ring)
 {
     return ring->len > 0;
 }
 
 bool mem_ring_peek(MemRing* ring, void** peek_addr, size_t peek_len)
 {
    if(ring->len < peek_len) {
        return false;
    }

    MemBlock front_block = mem_block_slice(&ring->buf, ring->front, peek_len);
    *peek_addr = front_block.data;

    return true;
 }
 
 bool mem_ring_dequeue(MemRing* ring, void* out_buf, size_t out_buf_len)
 {
    if(ring->len < out_buf_len) return false;

    MemBlock front_block = mem_block_slice(&ring->buf, ring->front, out_buf_len);

    ring->front = (ring->front + out_buf_len) % ring->buf.size;
    ring->len -= out_buf_len;

    memcpy(out_buf, front_block.data, out_buf_len);
    return true;
 }

 bool mem_ring_drop(MemRing* ring, size_t drop_len)
 {
    if(ring->len < drop_len) return false;
    ring->front = (ring->front + drop_len) % ring->buf.size;
    ring->len -= drop_len;
    return true;
 }
 
 bool mem_ring_enqueue(MemRing* ring, void* in_buf, size_t in_buf_len)
 {
    if(ring->buf.size < (ring->len + in_buf_len)) {
        return false;
    }

    size_t back = (ring->front + ring->len) % ring->buf.size;

    MemBlock back_block = mem_block_slice(&ring->buf, back, in_buf_len);

    ring->len += in_buf_len;

    memcpy(back_block.data, in_buf, in_buf_len);

    return true;
 }
 