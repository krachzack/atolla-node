#include "builder.h"
#include "type.h"
#include "../mem/uint16_byte.h"
#include "../test/assert.h"
#include <string.h>

static const size_t header_len = sizeof(uint8_t)  + // message type
                                 sizeof(uint16_t) + // message ID
                                 sizeof(uint16_t);  // payload size

static const size_t max_payload_len = 65535;

static const size_t initial_block_capacity = 32;

static MemBlock* build(
    MsgBuilder* builder,
    MsgType type,
    void* payload,
    size_t payload_len
);

static void set_uint8(
    MemBlock* msg_buf,
    size_t byte_offset,
    uint8_t value
);

static void set_uint16(
    MemBlock* msg_buf,
    size_t byte_offset,
    uint16_t value
);

static void set_data(
    MemBlock* msg_buf,
    size_t byte_offset,
    void* data,
    size_t data_len
);

void msg_builder_init(
    MsgBuilder* builder
)
{
    builder->msg_buf = mem_block_alloc(initial_block_capacity);
    builder->next_msg_id = 0;
}

void msg_builder_free(
    MsgBuilder* builder
)
{
    mem_block_free(&builder->msg_buf);
}

static MemBlock* build(
    MsgBuilder* builder,
    MsgType type,
    void* payload,
    size_t payload_len
)
{
    assert(payload_len <= max_payload_len);

    const size_t msg_len = header_len + payload_len;
    MemBlock* block = &builder->msg_buf;

    mem_block_resize(block, msg_len);

    set_uint8(block, 0, (uint8_t) type);
    set_uint16(block, 1, builder->next_msg_id++);
    set_data(block, 3, payload, payload_len);

    return block;
}

MemBlock* msg_builder_borrow(
    MsgBuilder* builder,
    uint8_t frame_length,
    uint8_t buffer_length
)
{
    uint8_t payload[] = { frame_length, buffer_length };
    size_t payload_len = sizeof(payload) / sizeof(uint8_t);
    return build(builder, MSG_TYPE_BORROW, payload, payload_len);
}

MemBlock* msg_builder_lent(
    MsgBuilder* builder
)
{
    return build(builder, MSG_TYPE_LENT, NULL, 0);
}

MemBlock* msg_builder_enqueue(
    MsgBuilder* builder,
    uint8_t frame_idx,
    void* frame,
    size_t frame_len
)
{
    const size_t frame_idx_len = sizeof(uint8_t);
    const size_t frame_len_len = sizeof(uint16_t);
    const size_t payload_len = frame_idx_len + frame_len_len + frame_len;
    uint8_t payload[payload_len];
    payload[0] = frame_idx;
    payload[1] = mem_uint16_byte_low(frame_len);
    payload[2] = mem_uint16_byte_high(frame_len);
    
    void* payload_frame = (void*) &payload[3];
    memcpy(payload_frame, frame, frame_len);    

    return build(builder, MSG_TYPE_ENQUEUE, payload, payload_len);
}

MemBlock* msg_builder_fail(
    MsgBuilder* builder,
    uint16_t causing_message_id,
    uint8_t error_code
)
{
    uint8_t payload[3] = {
        mem_uint16_byte_low(causing_message_id),
        mem_uint16_byte_high(causing_message_id),
        error_code
    };
    const size_t payload_len = sizeof(payload) / sizeof(uint8_t);
    return build(builder, MSG_TYPE_FAIL, payload, payload_len);
}

static void set_uint8(
    MemBlock* block,
    size_t byte_offset,
    uint8_t value
)
{
    assert(block->size >= (byte_offset + sizeof(uint8_t)));
    uint8_t* at = ((uint8_t*) block->data) + byte_offset;
    *at = value;
}

static void set_uint16(
    MemBlock* block,
    size_t byte_offset,
    uint16_t value
)
{
    assert(block->size >= (byte_offset + sizeof(uint16_t)));
    uint8_t* target = ((uint8_t*) block->data) + byte_offset;
    target[0] = mem_uint16_byte_low(value);
    target[1] = mem_uint16_byte_high(value);
}

static void set_data(
    MemBlock* block,
    size_t byte_offset,
    void* data,
    size_t data_len
)
{
    assert(block->size >= (byte_offset + sizeof(uint16_t) + data_len));

    uint8_t* data_header_ptr = ((uint8_t*) block->data) + byte_offset;
    data_header_ptr[0] = mem_uint16_byte_low(data_len);
    data_header_ptr[1] = mem_uint16_byte_high(data_len);

    if(data_len > 0) {
        void* data_payload_ptr = data_header_ptr + 2;
        memcpy(data_payload_ptr, data, data_len);
    }
}
