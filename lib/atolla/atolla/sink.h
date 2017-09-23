#ifndef ATOLLA_SINK_H
#define ATOLLA_SINK_H

#include "primitives.h"

enum AtollaSinkState
{
    // Sink is in a state of error that it cannot recover from
    ATOLLA_SINK_STATE_ERROR,
    // Sink is ready for a source to connect to it
    ATOLLA_SINK_STATE_OPEN,
    // Sink is currently connected to a source
    ATOLLA_SINK_STATE_LENT
};
typedef enum AtollaSinkState AtollaSinkState;

/**
 * Represents an endpoint for atolla sources to connect to.
 */
struct AtollaSink
{
    void* internal;
};
typedef struct AtollaSink AtollaSink;

/**
 * Initialization parameters for a new sink.
 */
struct AtollaSinkSpec
{
    /**
     * UDP port that the sink will run on.
     */
    int port;
    /**
     * Maximum amount of color-triplets that will be remembered from enqueue messages,
     * typically at least the amount of physical lights. Reduce this number if packages
     * get too big and quality of service drops.
     *
     * If atolla_sink_get is called with a buffer for more lights, the received pattern
     * is repeated. If atolla_sink_get is called with a buffer for less lights, the
     * received pattern is truncated to fit.
     */
    int lights_count;
};
typedef struct AtollaSinkSpec AtollaSinkSpec;

/**
 * Intializes and creates a new sink.
 */
AtollaSink atolla_sink_make(const AtollaSinkSpec* spec);

/**
 * Frees data and resources associated with the sink. The sink
 * cannot be used anymore, unless it is re-initialized with an additional
 * call to atolla_sink_make.
 */
void atolla_sink_free(AtollaSink sink);

/**
 * Redetermines the state of the sink based on incoming packets.
 */
AtollaSinkState atolla_sink_state(AtollaSink sink);

/**
 * Gets the current frame based on the instant in time upon calling the
 * function.
 *
 * Note that atolla_sink_state must be regularly called to evaluate
 * incoming packets in order for atolla_sink_get to provide results.
 *
 * If returns false, no frame available yet.
 *
 * If atolla_sink_get is called with a buffer for more lights than
 * set in the spec, the stored frame is repeated as a pattern to fill
 * all of the given buffer. If atolla_sink_get is called with a buffer
 * for less lights, the received pattern is truncated to fit.
 */
bool atolla_sink_get(AtollaSink sink, void* frame, size_t frame_len);

#endif // ATOLLA_SINK_H
