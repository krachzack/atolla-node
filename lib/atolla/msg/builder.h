#ifndef MSG_BUILD_H
#define MSG_BUILD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../atolla/primitives.h"
#include "../mem/block.h"

/**
 * Assembles atolla messages in an internal memory block that is managed by the
 * builder.
 *
 * Message assembly functions return a pointer to this internal memory. Note
 * that the returned memory block only remains valid until the next message is
 * generated. This makes MsgBuilder inherently not thread-safe.
 *
 * The builder takes care of some protocol internals such as incrementing the
 * message id when generating messages. There should be exactly one builder per
 * client.
 */
struct MsgBuilder
{
    MemBlock msg_buf;
    uint16_t next_msg_id;
};
typedef struct MsgBuilder MsgBuilder;

/**
 * Initializes the message builder and allocates memory for message generation.
 * Note that the internal memory block might need to re-allocate at a later
 * point in time. Always use the return value of the generation functions to
 * obtain a reference to message memory. The given pointer must always reference
 * valid, uninitialized message builder structs.
 */
void msg_builder_init(
    MsgBuilder* builder
);

/**
 * Frees resources associated with the message builder. The message builder
 * structure itself is managed by the calling code. Be sure to call this
 * function once and only once after it is no longer needed.
 */
void msg_builder_free(
    MsgBuilder* builder
);

/**
 * Generates and returns a borrow message containing the given frame length and
 * buffer size.
 *
 * The returned memory block references internal memory of the message builder
 * and is only valid until the next message generation function is called with
 * the same builder.
 */
MemBlock* msg_builder_borrow(
    MsgBuilder* builder,
    uint8_t frame_length,
    uint8_t buffer_length
);

/**
 * Generates and returns a lent message.
 *
 * The returned memory block references internal memory of the message builder
 * and is only valid until the next message generation function is called with
 * the same builder.
 */
MemBlock* msg_builder_lent(
    MsgBuilder* builder
);

/**
 * Generates and returns an enqueue message containing the given frame. Note
 * that the maximum size of a frame is 65535 bytes, which is equivalent to
 * 21845 colors.
 *
 * The returned memory block references internal memory of the message builder
 * and is only valid until the next message generation function is called with
 * the same builder.
 */
MemBlock* msg_builder_enqueue(
    MsgBuilder* builder,
    uint8_t frame_idx,
    void* frame,
    size_t frame_len
);


/**
 * Generates and returns a fail message with the given causing message ID and
 * the given error code.
 *
 * The returned memory block references internal memory of the message builder
 * and is only valid until the next message generation function is called with
 * the same builder.
 */
MemBlock* msg_builder_fail(
    MsgBuilder* builder,
    uint16_t causing_message_id,
    uint8_t error_code
);

#ifdef __cplusplus
}
#endif

#endif // MSG_BUILD_H
