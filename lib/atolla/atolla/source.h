#ifndef ATOLLA_SOURCE_H
#define ATOLLA_SOURCE_H

#include "primitives.h"

enum AtollaSourceState
{
    /**
     * The source has sent a message to the sink that communicates the desire
     * of the source to stream light information to the sink, which should
     * then be displayed. While in this state, the source waits for the
     * first response of the sink, ensuring that the sink exists and is
     * functioning.
     */
    ATOLLA_SOURCE_STATE_WAITING,
    /**
     * The sink is lent to the source and light information can be streamed with
     * atolla_source_put.
     */
    ATOLLA_SOURCE_STATE_OPEN,
    /**
     * Channel is in a state of error that it cannot recover from.
     * Possible reasons for this are:
     *
     * - The sink did not respond when trying to establish a connection,
     * - the connection was lost later,
     * - the sink has been lent to another source,
     * - the sink has communicated an unrecoverable error.
     */
    ATOLLA_SOURCE_STATE_ERROR
};
typedef enum AtollaSourceState AtollaSourceState;

/**
 * Represents the source of streaming light information that is connected
 * to a sink.
 */
struct AtollaSource
{
    void* internal;
};
typedef struct AtollaSource AtollaSource;

/**
 * Provides initialization parameters for a source that can be passed to
 * atolla_source_make.
 */
struct AtollaSourceSpec
{
    /**
     * IP address or hostname of the sink to connect the source to.
     */
    const char* sink_hostname;
    /**
     * UDP port that the sink running on sink_hostname is expected to run on.
     */
    int sink_port;
    /**
     * Time in milliseconds that one frame remains valid in the sink. E.g. a
     * frame duration of 17ms implies a refresh rate of 1000ms/17ms = 59 frames
     * per second on the sink.
     *
     * Note that the sink might refuse the borrow request if this duration is
     * too short.
     */
    int frame_duration_ms;
    /**
     * Determines how many frames should be sent to the sink in advance and
     * stored in a buffer for later use. This serves to compensate for spikes in
     * package throughput and makes the communication more stable.
     *
     * A value of zero lets the implementation pick a default value.
     */
    int max_buffered_frames;
    /**
     * Holds the time in milliseconds after which a new borrow message is
     * sent in reaction to a suspected packet loss.
     *
     * A value of zero lets the implementation pick a default value.
     */
    int retry_timeout_ms;
    /**
     * Holds the time in milliseconds after which no further attempts are
     * made to contact the sink. If the sink is unresponsive for this amount
     * of milliseconds, the source enters the error state.
     *
     * A value of zero lets the implementation pick a default value.
     */
    int disconnect_timeout_ms;
    /**
     * If set to true, atolla_source_make will not await completion of the
     * borrowing process before returning from atolla_source_make. After returning,
     * the state will be ATOLLA_SOURCE_STATE_WAITING until the connection is either
     * open or the connection attempt failed. In order for the source to manage
     * the asnychroneous borrowing, atolla_source_state should be called in
     * regular intervals to evaluate incoming packets.
     *
     * If set to false, the atolla_source_make function will block until the
     * borrowing process either failed or completed successfully. After
     * returning, the source is guaranteed to be in state ATOLLA_SOURCE_STATE_OPEN
     * or ATOLLA_SOURCE_STATE_ERROR.
     */
    bool async_make;
};
typedef struct AtollaSourceSpec AtollaSourceSpec;

/**
 * Creates a new atolla source using the parameters in the given spec struct.
 *
 * Upon completion, the source has initiated the borrowing process and will
 * be in state ATOLLA_SOURCE_STATE_WAITING.
 *
 * After a while, the source will enter either ATOLLA_SOURCE_STATE_OPEN if the
 * borrowing process was successful, or enter ATOLLA_SOURCE_STATE_ERROR if the
 * source does not exist or reported an error.
 *
 * TODO add option to block until connected
 */
AtollaSource atolla_source_make(const AtollaSourceSpec* spec);

/**
 * Orderly shuts down the source first and then frees associated resources.
 * The source referenced by the given source handle may not be used again
 * after calling this function unless it is re-initialized with atolla_source_make.
 */
void atolla_source_free(AtollaSource source);

/**
 * Gets the current state of the source based on the connection to the sink.
 *
 * ATOLLA_SOURCE_STATE_WAITING is returned when the source is trying to connect
 * to the sink after calling atolla_source_make.
 *
 * ATOLLA_SOURCE_STATE_OPEN is returned when the source has an active connection
 * to the sink.
 *
 * ATOLLA_SOURCE_STATE_ERROR is returned when either the borrowing process failed,
 * the sink reported an unrecoverable error or if the connection to the sink was
 * lost.
 */
AtollaSourceState atolla_source_state(AtollaSource source);

/**
 * If source is in error state, returns a pointer to a human readable error message.
 * If source is not in error state, returns null.
 */
const char* atolla_source_error_msg(AtollaSource source);

/**
 * With a source in the open state, determines how many frames can be sent to
 * the sink using atolla_source_put without blocking.
 *
 * If in error or waiting state, returns zero.
 *
 * If this function returns a non-zero value, a call to atolla_source_put
 * will not block. This also implies that atolla_source_put_ready_timeout will
 * return 0 if atolla_source_put_ready_count is non-zero.
 */
int atolla_source_put_ready_count(AtollaSource source);

/**
 * With a source in the open state, returns an amount of milliseconds to wait after
 * which atolla_source_put is guaranteed to be ready to immediately accept the next frame
 * without blocking.
 *
 * If the source is in waiting or error state, returns -1 instead.
 */
int atolla_source_put_ready_timeout(AtollaSource source);

/**
 * Tries to enqueue the given frame in the connected sink.
 *
 * The call might block for some time in order to wait for the device to catch
 * up displaying the previously sent frames. If atolla_source_put_ready_count returns
 * a non-zero value, atolla_source_put is guaranteed not to wait. The same
 * guarantee applies if atolla_source_put_ready_timeout returns zero.
 *
 * If the source is in open state, the frame will either immediately be sent,
 * or, if max_buffered_frames are already enqueued in the connected sink,
 * will block until atolla_source_put_ready_count returns a non-zero value and
 * then send the frame. The function returns true in this case.
 *
 * If the source is in waiting state, that is, if the sink has not responded
 * to the borrow request from the source yet, this function will return false
 * and not try to enqueue the frame.
 */
bool atolla_source_put(AtollaSource source, void* frame, size_t frame_len);

#endif // ATOLLA_SOURCE_H
