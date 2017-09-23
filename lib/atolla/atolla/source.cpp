#include "source.h"
#include "error_codes.h"
#include "../msg/builder.h"
#include "../msg/iter.h"
#include "../test/assert.h"
#include "../time/now.h"
#include "../time/sleep.h"
#include "../udp_socket/udp_socket.h"

#include <stdlib.h>

#ifndef ATOLLA_SOURCE_RECV_BUF_LEN
/**
 * Determines the maximum size of incoming packets.
 * This is enough for about 300 lights.
 *
 */
#define ATOLLA_SOURCE_RECV_BUF_LEN 32
#endif

static const size_t recv_buf_len = ATOLLA_SOURCE_RECV_BUF_LEN;
static const unsigned int retry_timeout_ms_default = 100;
static const unsigned int disconnect_timeout_ms_default = 750;
static const int max_buffered_frames_default = 16;
static const int blocking_make_refresh_interval = 5;
/** Special time value meant to represent no time set */
// FIXME this is actually a valid point in time, maybe use unions with use flag?
static const unsigned int NULL_TIME = ~0;

struct AtollaSourcePrivate
{
    AtollaSourceState state;
    UdpSocket sock;
    uint8_t recv_buf[ATOLLA_SOURCE_RECV_BUF_LEN];
    
    MsgBuilder builder;

    int next_frame_idx;
    unsigned int frame_duration_ms;
    int max_buffered_frames;
    unsigned int retry_timeout_ms;
    unsigned int disconnect_timeout_ms;

    unsigned int first_borrow_time;
    unsigned int last_borrow_time;
    unsigned int last_frame_time;
    unsigned int last_recv_lent_time;

    const char* error_msg;
};
typedef struct AtollaSourcePrivate AtollaSourcePrivate;

static AtollaSourcePrivate* source_private_make(const AtollaSourceSpec* spec);
static void source_await_make_completion(AtollaSourcePrivate* source);
static void source_send_borrow(AtollaSourcePrivate* source);
static void source_update(AtollaSourcePrivate* source);
static void source_iterate_recv_buf(AtollaSourcePrivate* sink, size_t received_bytes);
static void source_lent(AtollaSourcePrivate* source);
static void source_fail(AtollaSourcePrivate* source, const char* error_msg);
static void source_receive(AtollaSourcePrivate* source);
static void source_manage_borrow_packet_loss(AtollaSourcePrivate* source);
static void source_ensure_lent_resent(AtollaSourcePrivate* source);

AtollaSource atolla_source_make(const AtollaSourceSpec* spec)
{
    assert(spec->sink_port >= 0 && spec->sink_port < 65536);

    AtollaSourcePrivate* source = source_private_make(spec);

    msg_builder_init(&source->builder);

    UdpSocketResult result;
    
    result = udp_socket_init(&source->sock);
    if(result.code == UDP_SOCKET_OK) {
        result = udp_socket_set_receiver(&source->sock, spec->sink_hostname, (unsigned short) spec->sink_port);
        if(result.code == UDP_SOCKET_OK) {
            // If hostname could be resolved, send first borrow
            source->first_borrow_time = time_now();
            source_send_borrow(source);
        } else {
            // If resolving failed, immediately enter error state
            source->state = ATOLLA_SOURCE_STATE_ERROR;
            source_fail(source, "Sink hostname could not be resolved.");
        }
    } else {
        source_fail(source, "Sink could not bind to port.");
    }

    if(!spec->async_make)
    {
        source_await_make_completion(source);
    }

    AtollaSource source_handle = { source };
    return source_handle;
}

static AtollaSourcePrivate* source_private_make(const AtollaSourceSpec* spec)
{
    AtollaSourcePrivate* source = (AtollaSourcePrivate*) malloc(sizeof(AtollaSourcePrivate));

    source->state = ATOLLA_SOURCE_STATE_WAITING;
    source->next_frame_idx = 0;
    source->frame_duration_ms = spec->frame_duration_ms;
    source->max_buffered_frames = (spec->max_buffered_frames == 0) ? max_buffered_frames_default : spec->max_buffered_frames;
    source->retry_timeout_ms = (spec->retry_timeout_ms == 0) ? retry_timeout_ms_default : spec->retry_timeout_ms;
    source->disconnect_timeout_ms = (spec->disconnect_timeout_ms == 0) ? disconnect_timeout_ms_default : spec->disconnect_timeout_ms;

    source->first_borrow_time = 0;
    source->last_borrow_time = 0;
    source->last_frame_time = 0;

    return source;
}

static void source_await_make_completion(AtollaSourcePrivate* source)
{
    while(source->state == ATOLLA_SOURCE_STATE_WAITING)
    {
        if(blocking_make_refresh_interval > 0)
        {
            time_sleep(blocking_make_refresh_interval);
        }
        source_update(source);
    }
}

void atolla_source_free(AtollaSource source_handle)
{
    AtollaSourcePrivate* source = (AtollaSourcePrivate*) source_handle.internal;

    udp_socket_free(&source->sock);

    free(source);
}

AtollaSourceState atolla_source_state(AtollaSource source_handle)
{
    AtollaSourcePrivate* source = (AtollaSourcePrivate*) source_handle.internal;

    source_update(source);

    return source->state;
}

const char* atolla_source_error_msg(AtollaSource source_handle)
{
    AtollaSourcePrivate* source = (AtollaSourcePrivate*) source_handle.internal;
    if(source->state == ATOLLA_SOURCE_STATE_ERROR) {
        return source->error_msg;
    } else {
        return NULL;
    }
}

int atolla_source_put_ready_count(AtollaSource source_handle)
{
    AtollaSourcePrivate* source = (AtollaSourcePrivate*) source_handle.internal;

    source_update(source);

    if(source->state == ATOLLA_SOURCE_STATE_ERROR ||
       source->state == ATOLLA_SOURCE_STATE_WAITING)
    {
        // If in unrecoverable error state or not fully connected yet,
        // report lagging behind 0 frames
        return 0;
    }
    else
    {
        assert(source->state == ATOLLA_SOURCE_STATE_OPEN);

        if(source->last_frame_time == NULL_TIME)
        {
            // If connected, but no frame was enqueued yet, report maximum lag
            return source->max_buffered_frames;
        }
        else
        {
            // Otherwise, calculate lag based on the time of the last enqueued frame
            return (time_now() - source->last_frame_time) / source->frame_duration_ms;
        }
    }
}

int atolla_source_put_ready_timeout(AtollaSource source_handle)
{
    AtollaSourcePrivate* source = (AtollaSourcePrivate*) source_handle.internal;
    
    source_update(source);

    if(source->state == ATOLLA_SOURCE_STATE_ERROR ||
       source->state == ATOLLA_SOURCE_STATE_WAITING)
    {
        // If in unrecoverable error state or not fully connected yet,
        // report lagging behind 0 frames
        return -1;
    }
    else if(source->last_frame_time == NULL_TIME)
    {
        return 0;
    }
    else
    {
        int readyCount = (time_now() - source->last_frame_time) / source->frame_duration_ms;

        if(readyCount > 0) {
            return 0;
        } else {
            return (source->last_frame_time + source->frame_duration_ms) - time_now();
        }
    }    
}

bool atolla_source_put(AtollaSource source_handle, void* frame, size_t frame_len)
{
    AtollaSourcePrivate* source = (AtollaSourcePrivate*) source_handle.internal;

    source_update(source);

    if(atolla_source_state(source_handle) != ATOLLA_SOURCE_STATE_OPEN)
    {
        return false;
    }

    // If the receiving device has no space in the buffer to hold new frames,
    // wait until the next frame was dequeued in the sink
    int timeout = atolla_source_put_ready_timeout(source_handle);
    if(timeout > 0)
    {
        time_sleep(timeout);
    }

    MemBlock* enqueue_msg = msg_builder_enqueue(&source->builder, source->next_frame_idx, frame, frame_len);
    UdpSocketResult send_result = udp_socket_send(&source->sock, enqueue_msg->data, enqueue_msg->size);
    if(send_result.code != UDP_SOCKET_OK)
    {
        return false;
    }
    else
    {
        source->next_frame_idx = (source->next_frame_idx + 1) % 256;
        
        if(source->last_frame_time == NULL_TIME)
        {
            source->last_frame_time = time_now() - (source->max_buffered_frames - 1) * source->frame_duration_ms;
        }
        else
        {
            // Otherwise, advance the last frame time, so we get closer to the point where no more
            // frame can be enqueued
            source->last_frame_time += source->frame_duration_ms;
        }
    
        return true;   
    }
}

static void source_send_borrow(AtollaSourcePrivate* source)
{
    source->last_borrow_time = time_now();
    MemBlock* borrow_msg = msg_builder_borrow(&source->builder, source->frame_duration_ms, source->max_buffered_frames);
    udp_socket_send(&source->sock, borrow_msg->data, borrow_msg->size);
}

static void source_update(AtollaSourcePrivate* source)
{
    source_receive(source);
    source_manage_borrow_packet_loss(source);
    source_ensure_lent_resent(source);
}

static void source_receive(AtollaSourcePrivate* source)
{
    size_t received_len;
    UdpSocketResult result;

    result = udp_socket_receive(
        &source->sock,
        source->recv_buf, recv_buf_len,
        &received_len,
        false
    );

    if(result.code == UDP_SOCKET_OK)
    {
        source_iterate_recv_buf(source, received_len);
    }
}

static void source_manage_borrow_packet_loss(AtollaSourcePrivate* source)
{
    if(source->state == ATOLLA_SOURCE_STATE_WAITING)
    {
        unsigned int now = time_now();
        unsigned int time_since_first_borrow = now - source->first_borrow_time;
        unsigned int time_since_last_borrow = now - source->last_borrow_time;

        if(time_since_first_borrow > source->disconnect_timeout_ms)
        {
            // If no lent message was received after the disconnect timeout,
            // enter unrecoverable error state
            source_fail(source, "Tried to borrow the sink, but the attempt timed out.");
        }
        else if(time_since_last_borrow > source->retry_timeout_ms)
        {
            // If no lent message was received after the retry timeout, try borrowing again
            source_send_borrow(source);
        }
    }
}

static void source_ensure_lent_resent(AtollaSourcePrivate* source)
{
    if(source->state == ATOLLA_SOURCE_STATE_OPEN &&
       (time_now() - source->last_recv_lent_time) >= source->disconnect_timeout_ms)
    {
        source_fail(source, "The connection to the sink was lost.");
    }
}

static void source_iterate_recv_buf(AtollaSourcePrivate* source, size_t received_bytes)
{
    MsgIter iter = msg_iter_make(source->recv_buf, received_bytes);

    for(; msg_iter_has_msg(&iter); msg_iter_next(&iter))
    {
        MsgType type = msg_iter_type(&iter);

        switch(type)
        {
            case MSG_TYPE_LENT:
            {
                source_lent(source);
                break;
            }

            case MSG_TYPE_FAIL:
            {
                switch(msg_iter_fail_error_code(&iter))
                {
                    case ATOLLA_ERROR_CODE_NOT_BORROWED:
                        source_fail(source, "The sink signalled that is not currently borrowed by this source.");
                        break;

                    case ATOLLA_ERROR_CODE_REQUESTED_BUFFER_TOO_LARGE:
                        source_fail(source, "The sink does not have enough memory for a frame queue of the requested length.");
                        break;

                    case ATOLLA_ERROR_CODE_REQUESTED_FRAME_DURATION_TOO_SHORT:
                        source_fail(source, "The sink cannot accomodate the reqest for the given frame duration because it is too short. Try a shorter frame duration.");
                        break;

                    case ATOLLA_ERROR_CODE_LENT_TO_OTHER_SOURCE:
                        source_fail(source, "The sink refused a request to borrow or enqueue because it is currently lent to another source. Try again later, when the other source has stopped transmission.");
                        break;

                    case ATOLLA_ERROR_CODE_BAD_MSG:
                        source_fail(source, "The sink signalled that it could not understand a message or that a message contained a not further specified invalid value. This might be due to incompatible versions of the atolla protocol.");
                        break;

                    case ATOLLA_ERROR_CODE_TIMEOUT:
                        source_fail(source, "The sink signalled that it did not receive packets for so long, it deems the connection no longer working. This might be due to bad signal quality or the source failing to enqueue frames for too long.");
                        break;

                    default:
                        source_fail(source, "The sink signalled an unrecoverable error state.");
                        break;
                }

                
                break;
            }

            default:
            {
                source_fail(source, "Malformed or unknown message type received from sink. This might be due to incompatible versions of the atolla protocol.");
                break;
            }
        }
    }
}

static void source_lent(AtollaSourcePrivate* source)
{
    if(source->state == ATOLLA_SOURCE_STATE_WAITING)
    {
        source->state = ATOLLA_SOURCE_STATE_OPEN;
        source->last_frame_time = NULL_TIME;
        source->last_recv_lent_time = time_now();
    }
    else if(source->state == ATOLLA_SOURCE_STATE_OPEN)
    {
        source->last_recv_lent_time = time_now();
    }
}

static void source_fail(AtollaSourcePrivate* source, const char* error_msg)
{
    source->state = ATOLLA_SOURCE_STATE_ERROR;
    source->error_msg = error_msg;
}
