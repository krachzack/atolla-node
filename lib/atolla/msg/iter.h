#ifndef MSG_ITER_H
#define MSG_ITER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "type.h"
#include "../atolla/primitives.h"
#include "../mem/block.h"

/**
 * Iterates through the messages in the buffer specified by start and end.
 * An instance is typically created with assigning the return value of
 * msg_iter_make.
 *
 * To select the next message use the msg_iter_next function. If it is called
 * after reading the last message or if the buffer was empty to begin with,
 * all functions other than msg_iter_has_msg have undefined behavior, though
 * protected with asserts in debug builds.
 */
struct MsgIter
{
    uint8_t* msg_buf_start;
    uint8_t* msg_buf_end;
};
typedef struct MsgIter MsgIter;

/**
 * Returns a newly initialized message iterator currently pointing to the first
 * message in the buffer.
 */
MsgIter msg_iter_make(
    void* msg_buffer,
    size_t msg_buffer_size
);

/**
 * Checks whether the iterator currently points to a message.
 *
 * The iterator does not point to a message if either the buffer was empty to
 * begin with or if msg_iter_next has been called after the last message in the
 * buffer.
 */
bool msg_iter_has_msg(MsgIter* iter);

/**
 * Selects the next message in the buffer. This function should not be called
 * if msg_iter_has_msg has already returned false.
 */
void msg_iter_next(MsgIter* iter);

/**
 * Returns the message type of the currently selected message.
 *
 * If the iterator is already at the end of the buffer, the behavior of this
 * function is undefined. Do not call it with an iterator if msg_iter_has_msg
 * returns false.
 */
MsgType msg_iter_type(MsgIter* iter);

/**
 * Gets the message ID of the currently selected message in native byte order.
 *
 * If the iterator is already at the end of the buffer, the behavior of this
 * function is undefined. Do not call it with an iterator if msg_iter_has_msg
 * returns false.
 */
uint16_t msg_iter_msg_id(MsgIter* iter);

/**
 * Get the frame length of a currently selected BORROW message.
 *
 * If the iterator is already at the end of the buffer, or if the currently
 * selected message has a type different from MSG_TYPE_BORROW, the behavior of
 * this function is undefined. Do not call it with an iterator if
 * msg_iter_has_msg returns false or if msg_iter_type returns a type different
 * from MSG_TYPE_BORROW.
 */
uint8_t msg_iter_borrow_frame_length(MsgIter* iter);

/**
 * Get the buffer length of a currently selected BORROW message. If no message is
 * selected or if the currently selected message has a different type, the
 * behavior of this function is undefined.
 *
 * If the iterator is already at the end of the buffer, or if the currently
 * selected message has a type different from MSG_TYPE_BORROW, the behavior of
 * this function is undefined. Do not call it with an iterator if
 * msg_iter_has_msg returns false or if msg_iter_type returns a type different
 * from MSG_TYPE_BORROW.
 */
uint8_t msg_iter_borrow_buffer_length(MsgIter* iter);

/**
 * Get the contained frame index of a currently selected ENQUEUE message.
 *
 * If the iterator is already at the end of the buffer, or if the currently
 * selected message has a type different from MSG_TYPE_ENQUEUE, the behavior of
 * this function is undefined. Do not call it with an iterator if
 * msg_iter_has_msg returns false or if msg_iter_type returns a type different
 * from MSG_TYPE_ENQUEUE.
 */
uint8_t msg_iter_enqueue_frame_idx(MsgIter* iter);

/**
 * Get the contained frame of a currently selected ENQUEUE message.
 *
 * If the iterator is already at the end of the buffer, or if the currently
 * selected message has a type different from MSG_TYPE_ENQUEUE, the behavior of
 * this function is undefined. Do not call it with an iterator if
 * msg_iter_has_msg returns false or if msg_iter_type returns a type different
 * from MSG_TYPE_ENQUEUE.
 */
MemBlock msg_iter_enqueue_frame(MsgIter* iter);

/**
 * Get a previously sent message ID that a currently selected FAIL message
 * refers to.
 *
 * If the iterator is already at the end of the buffer, or if the currently
 * selected message has a type different from MSG_TYPE_FAIL, the behavior of
 * this function is undefined. Do not call it with an iterator if
 * msg_iter_has_msg returns false or if msg_iter_type returns a type different
 * from MSG_TYPE_FAIL.
 */
uint16_t msg_iter_fail_offending_msg_id(MsgIter* iter);

/**
 * Get an error code specifying which type of error occurred, that caused the
 * currently selected FAIL message to be sent.
 *
 * If the iterator is already at the end of the buffer, or if the currently
 * selected message has a type different from MSG_TYPE_FAIL, the behavior of
 * this function is undefined. Do not call it with an iterator if
 * msg_iter_has_msg returns false or if msg_iter_type returns a type different
 * from MSG_TYPE_FAIL.
 */
uint8_t msg_iter_fail_error_code(MsgIter* iter);

#ifdef __cplusplus
}
#endif

#endif // MSG_ITER_H
