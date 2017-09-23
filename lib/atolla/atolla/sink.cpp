
// FIXME change udp_socket so it does not need c++ linkage

#include "sink.h"
#include "error_codes.h"
#include "../mem/ring.h"
#include "../msg/builder.h"
#include "../msg/iter.h"
#include "../udp_socket/udp_socket.h"
#include "../time/now.h"
#include "../test/assert.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef ATOLLA_SINK_RECV_BUF_LEN
/**
 * Determines the maximum size of incoming packets.
 * This is enough for about 300 lights.
 *
 */
#define ATOLLA_SINK_RECV_BUF_LEN 1024
#endif

static const size_t recv_buf_len = ATOLLA_SINK_RECV_BUF_LEN;
static const size_t color_channel_count = 3;
/** Size of the pending_frames ring buffer in frames */
static const size_t pending_frames_capacity = 128;
/** After drop_timeout milliseconds of not receiving anything, the source is assumed to have shut down the connection */
static const unsigned int drop_timeout = 1500;
/** Determines in milliseconds how often the LENT package will be repeatedly sent to the current borrower */
static const unsigned int lent_send_interval = 500;
/**
 * If the frame duration sent with the borrow packet implies a shorter frame duration than this,
 * report an unrecoverable error to the sink.
 */
static const int frame_length_ms_min = 10;

static const unsigned int NULL_TIME = ~0;

struct AtollaSinkPrivate
{
    AtollaSinkState state;

    UdpSocket socket;
    UdpEndpoint borrower_endpoint;

    unsigned int lights_count;
    unsigned int frame_duration_ms;

    MsgBuilder builder;

    uint8_t recv_buf[ATOLLA_SINK_RECV_BUF_LEN];
    MemBlock current_frame;
    // Holds preliminary data when assembling frame from msg
    MemBlock received_frame;
    MemRing pending_frames;

    unsigned int time_origin;
    int last_enqueued_frame_idx;

    unsigned int last_recv_time;
    unsigned int last_send_lent_time;
};
typedef struct AtollaSinkPrivate AtollaSinkPrivate;

static AtollaSinkPrivate* sink_private_make(const AtollaSinkSpec* spec); 
static void sink_iterate_recv_buf(AtollaSinkPrivate* sink, size_t received_bytes, UdpEndpoint* sender);
static void sink_handle_borrow(AtollaSinkPrivate* sink, uint16_t msg_id, int frame_length_ms, size_t buffer_length, UdpEndpoint* sender);
static void sink_handle_enqueue(AtollaSinkPrivate* sink, uint16_t msg_id, size_t frame_idx, MemBlock frame, UdpEndpoint* sender);
static void sink_enqueue(AtollaSinkPrivate* sink, MemBlock frame);
static void sink_send_lent(AtollaSinkPrivate* sink);
static void sink_send_fail(AtollaSinkPrivate* sink, uint16_t offending_msg_id, uint8_t error_code);
static void sink_send_fail_to(AtollaSinkPrivate* sink, uint16_t offending_msg_id, uint8_t error_code, UdpEndpoint* to);
static void sink_update(AtollaSinkPrivate* sink);
static void sink_receive(AtollaSinkPrivate* sink);
static void sink_send(AtollaSinkPrivate* sink);
static void sink_drop_borrow(AtollaSinkPrivate* sink);

static void fill_with_pattern(void* target, size_t target_len, void* pattern, size_t pattern_len);
static int bounded_diff(int from, int to, int cap);


AtollaSink atolla_sink_make(const AtollaSinkSpec* spec)
{
    AtollaSinkPrivate* sink = sink_private_make(spec);
    
    UdpSocketResult result = udp_socket_init_on_port(&sink->socket, (unsigned short) spec->port);
    assert(result.code == UDP_SOCKET_OK);

    msg_builder_init(&sink->builder);

    AtollaSink sink_handle = { sink };
    return sink_handle;
}

static AtollaSinkPrivate* sink_private_make(const AtollaSinkSpec* spec)
{
    assert(spec->port >= 0 && spec->port < 65536);
    assert(spec->lights_count >= 1);
    // The recv buffer must be large enough to hold messages that overwrite
    // all of the lights with new colors
    assert((spec->lights_count * color_channel_count + 10) < recv_buf_len);

    AtollaSinkPrivate* sink = (AtollaSinkPrivate*) malloc(sizeof(AtollaSinkPrivate));
    assert(sink != NULL);

    // Initialize everything to zero
    memset(sink, 0, sizeof(AtollaSinkPrivate));

    // Except these fields, which are pre-filled
    sink->state = ATOLLA_SINK_STATE_OPEN;
    sink->lights_count = spec->lights_count;
    sink->current_frame = mem_block_alloc(spec->lights_count * color_channel_count);
    sink->received_frame = mem_block_alloc(spec->lights_count * color_channel_count);
    sink->pending_frames = mem_ring_alloc(spec->lights_count * color_channel_count * pending_frames_capacity);

    return sink;
}

void atolla_sink_free(AtollaSink sink_handle)
{
    AtollaSinkPrivate* sink = (AtollaSinkPrivate*) sink_handle.internal;

    msg_builder_free(&sink->builder);

    udp_socket_free(&sink->socket);

    mem_block_free(&sink->current_frame);
    mem_block_free(&sink->received_frame);
    mem_ring_free(&sink->pending_frames);

    free(sink);
}

AtollaSinkState atolla_sink_state(AtollaSink sink_handle)
{
    AtollaSinkPrivate* sink = (AtollaSinkPrivate*) sink_handle.internal;

    sink_update(sink);

    return sink->state;
}

bool atolla_sink_get(AtollaSink sink_handle, void* frame, size_t frame_len)
{
    AtollaSinkPrivate* sink = (AtollaSinkPrivate*) sink_handle.internal;

    bool lent = sink->state == ATOLLA_SINK_STATE_LENT;

    if(lent)
    {
        if(sink->time_origin == NULL_TIME)
        {
            // Set origin on first dequeue
            bool ok = mem_ring_dequeue(&sink->pending_frames, sink->current_frame.data, sink->current_frame.capacity);
            if(ok) {
                sink->time_origin = time_now();
            } else {
                // nothing available yet
                return false;
            }
        }
        else
        {
            unsigned int now = time_now();
            while((now - sink->time_origin) > sink->frame_duration_ms) {
                bool ok = mem_ring_dequeue(&sink->pending_frames, sink->current_frame.data, sink->current_frame.capacity);
                if(ok) {
                    sink->time_origin += sink->frame_duration_ms;
                } else {
                    // TODO Experiencing lag, maybe disconnect at this point, not when trying to receive
                    //      this way the unfinished buffer can finish showing
                    break;
                }
            }
        }

        fill_with_pattern(frame, frame_len, sink->current_frame.data, sink->current_frame.capacity);
    }

    return lent;
}

static void sink_update(AtollaSinkPrivate* sink)
{
    sink_receive(sink);
    sink_send(sink);
}

static void sink_receive(AtollaSinkPrivate* sink)
{
    UdpSocketResult result;

    const int max_receives = 1;

    // Try receiving until would block or received max_receives times
    for(int i = 0; i < max_receives; ++i) {
        UdpEndpoint sender;
        size_t received_bytes = 0;
        result = udp_socket_receive_from(
            &sink->socket,
            sink->recv_buf, recv_buf_len,
            &received_bytes,
            &sender
        );
    
        if(result.code == UDP_SOCKET_OK)
        {
            // If another packet available, iterate contained messages
            sink_iterate_recv_buf(sink, received_bytes, &sender);
            sink->last_recv_time = time_now();
        }
        else
        {
            // drop connections if have not received packets in a while
            if(sink->state == ATOLLA_SINK_STATE_LENT && (time_now() - sink->last_recv_time) > drop_timeout)
            {
                sink_send_fail(sink, 0, ATOLLA_ERROR_CODE_TIMEOUT);
                sink_drop_borrow(sink);
            }
            return;
        }
    }
}

static void sink_iterate_recv_buf(AtollaSinkPrivate* sink, size_t received_bytes, UdpEndpoint* sender)
{
    MsgIter iter = msg_iter_make(sink->recv_buf, received_bytes);

    for(; msg_iter_has_msg(&iter); msg_iter_next(&iter))
    {
        MsgType type = msg_iter_type(&iter);
        uint16_t msg_id = msg_iter_msg_id(&iter);

        switch(type)
        {
            case MSG_TYPE_BORROW:
            {
                uint8_t frame_len = msg_iter_borrow_frame_length(&iter);
                uint8_t buffer_len = msg_iter_borrow_buffer_length(&iter);
                sink_handle_borrow(sink, msg_id, frame_len, buffer_len, sender);
                break;
            }

            case MSG_TYPE_ENQUEUE:
            {
                uint8_t frame_idx = msg_iter_enqueue_frame_idx(&iter);
                MemBlock frame = msg_iter_enqueue_frame(&iter);
                sink_handle_enqueue(sink, msg_id, frame_idx, frame, sender);
                break;
            }

            default:
            {
                sink_send_fail_to(sink, msg_id, ATOLLA_ERROR_CODE_BAD_MSG, sender);
                break;
            }
        }
    }
}

static void sink_handle_borrow(AtollaSinkPrivate* sink, uint16_t msg_id, int frame_length_ms, size_t buffer_length, UdpEndpoint* sender)
{
    if(sink->state == ATOLLA_SINK_STATE_OPEN ||
       (sink->state == ATOLLA_SINK_STATE_LENT && udp_endpoint_equal(sender, &sink->borrower_endpoint))
      )
    {
        size_t required_frame_buf_size = buffer_length * (sink->lights_count * color_channel_count);
        
        if(required_frame_buf_size > sink->pending_frames.buf.capacity)
        {
            sink_send_fail_to(sink, msg_id, ATOLLA_ERROR_CODE_REQUESTED_BUFFER_TOO_LARGE, sender);
            if(sink->state == ATOLLA_SINK_STATE_LENT) { sink_drop_borrow(sink); }
        }
        else if(frame_length_ms < frame_length_ms_min)
        {
            sink_send_fail_to(sink, msg_id, ATOLLA_ERROR_CODE_REQUESTED_FRAME_DURATION_TOO_SHORT, sender);
            if(sink->state == ATOLLA_SINK_STATE_LENT) { sink_drop_borrow(sink); }
        }
        else
        {
            sink->borrower_endpoint = *sender;
            sink->frame_duration_ms = frame_length_ms;
            sink->time_origin = NULL_TIME;
            sink->last_enqueued_frame_idx = NULL_TIME;
            sink->last_recv_time = NULL_TIME;
            sink->state = ATOLLA_SINK_STATE_LENT;

            sink_send_lent(sink);
        }
    }
    else if(sink->state == ATOLLA_SINK_STATE_LENT)
    {
        sink_send_fail_to(sink, msg_id, ATOLLA_ERROR_CODE_LENT_TO_OTHER_SOURCE, sender);
    }
    else
    {
        return; // In error state, do not bother to respond
    }
}

static void sink_handle_enqueue(AtollaSinkPrivate* sink, uint16_t msg_id, size_t frame_idx, MemBlock frame, UdpEndpoint* sender)
{
    if(sink->state == ATOLLA_SINK_STATE_ERROR)
    {
        return; // In error state, do not bother to respond
    }
    else if(sink->state == ATOLLA_SINK_STATE_OPEN)
    {
        sink_send_fail_to(sink, msg_id, ATOLLA_ERROR_CODE_NOT_BORROWED, sender);
    }
    else
    {
        if(frame.size < 3)
        {
            // Minimum enqueue length is 3, drop connection after illegal message
            sink_send_fail_to(sink, msg_id, ATOLLA_ERROR_CODE_BAD_MSG, sender);
            sink_drop_borrow(sink);
        }
        else
        {
            if(udp_endpoint_equal(sender, &sink->borrower_endpoint))
            {
                int diff = bounded_diff(sink->last_enqueued_frame_idx, frame_idx, 256);
                if(diff > 128)
                {
                    // If would have to skip more than 128, this is an out of order package.
                    // We just drop it.
                    return;
                }
            
                while(diff > 0) {
                    sink_enqueue(sink, frame);
                    diff = bounded_diff(sink->last_enqueued_frame_idx, frame_idx, 256);
                }
            }
            else
            {
                sink_send_fail_to(sink, msg_id, ATOLLA_ERROR_CODE_LENT_TO_OTHER_SOURCE, sender);
            }
        }
    }
}

static void sink_enqueue(AtollaSinkPrivate* sink, MemBlock frame)
{
    fill_with_pattern(
        sink->received_frame.data, sink->received_frame.capacity,
        frame.data, frame.size
    );

    if(mem_ring_enqueue(&sink->pending_frames, sink->received_frame.data, sink->received_frame.capacity)) {
        sink->last_enqueued_frame_idx = (sink->last_enqueued_frame_idx + 1) % 256;
    }
}

static void sink_send(AtollaSinkPrivate* sink)
{
    if(sink->state == ATOLLA_SINK_STATE_LENT)
    {
        if((time_now() - sink->last_send_lent_time) > lent_send_interval)
        {
            sink_send_lent(sink);
        }
    }
}

static void sink_send_lent(AtollaSinkPrivate* sink)
{
    MemBlock* lent_msg = msg_builder_lent(&sink->builder);
    udp_socket_send_to(&sink->socket, lent_msg->data, lent_msg->size, &sink->borrower_endpoint);
    sink->last_send_lent_time = time_now();
}

static void sink_send_fail(AtollaSinkPrivate* sink, uint16_t offending_msg_id, uint8_t error_code)
{
    sink_send_fail_to(sink, offending_msg_id, error_code, &sink->borrower_endpoint);
}

static void sink_send_fail_to(AtollaSinkPrivate* sink, uint16_t offending_msg_id, uint8_t error_code, UdpEndpoint* to)
{
    MemBlock* lent_msg = msg_builder_fail(&sink->builder, offending_msg_id, error_code);
    udp_socket_send_to(&sink->socket, lent_msg->data, lent_msg->size, to);
}

static void sink_drop_borrow(AtollaSinkPrivate* sink)
{
    sink->state = ATOLLA_SINK_STATE_OPEN;
}

static void fill_with_pattern(void* target, size_t target_len, void* pattern, size_t pattern_len)
{
    if(target_len == 0) return;
    if(pattern_len == 0) return;

    uint8_t* offset_target = (uint8_t*) target;
    
    while(target_len > 0)
    {
        if(pattern_len >= target_len)
        {
            memcpy(offset_target, pattern, target_len);
            target_len = 0;
        }
        else
        {
            memcpy(offset_target, pattern, pattern_len);
            offset_target += pattern_len;
            target_len -= pattern_len;
        }
    }
}

static int bounded_diff(int from, int to, int cap)
{
    if(to < from)
    {
        return cap - from + to;
    }
    else
    {
        return to - from;
    }
}
