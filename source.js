const { Source } = require('./build/Release/atolla')
const toUint8ArrayColors = require('./to-uint8-colors')

const defaultColor = new Uint8Array([0, 0, 0])
const defaultPainter = () => defaultColor
const noop = () => {}

/**
 * Creates a new source to stream color information over the network to the source
 * specified with the 'hostname' and 'port' in the spec object.
 *
 * The source will in regular intervals stream colors to the sink provided by a
 * painter function. The frequency of calls to the painter is decided upon calling
 * source by the values of frameDurationMs and maxBufferedFrames in the spec.
 *
 *    import { source } from 'atolla'
 *
 *    // Stream a sine-like animation to a sink running at localhost
 *    source({
 *      hostname: 'localhost',
 *      port: 10042,
 *      frameDurationMs: 30,
 *      maxBufferedFrames: 10,
 *      painter: (time) => `hsl(0, 100%, ${100 * Math.abs(Math.sin(time))}%)`,
 *      onReady: () => console.log('Source successfully connected to sink'),
 *      onError: (msg) => console.log('Source encountered unrecoverable error and is now disconnected', msg)
 *    })
 */
module.exports = function source (spec) {
  // Immediately start the source with the given spec
  // If no spec provided, or the spec has erroneous properties,
  // the source constructor will throw an exception from inside the source
  // If succeeds, spec.frameDurationMs is guaranteed to be defined and be an integer
  let source = new Source(spec)

  const frameDurationSeconds = spec.frameDurationMs / 1000

  let painter = ('painter' in spec && typeof spec.painter === 'function')
                    ? spec.painter
                    : defaultPainter

  let onStateChange = ('onStateChange' in spec && typeof spec.onStateChange === 'function')
                          ? spec.onStateChange
                          : noop

  let onReady = ('onReady' in spec && typeof spec.onReady === 'function')
                          ? spec.onReady
                          : noop

  let onError = ('onError' in spec && typeof spec.onError === 'function')
                          ? spec.onError
                          : noop

  // Time in seconds that will be counted up when successfully sending a frame and passed
  // to the painter function when requesting a new frame
  let time = 0

  let updateTimeout
  let lastState
  let errorMsg

  update()

  return {
    get painter () {
      return painter
    },
    set painter (newPainter) {
      painter = (typeof newPainter === 'function') ? newPainter : defaultPainter
    },
    get state () {
      return lastState
    },
    get errorMessage () {
      return errorMsg
    },
    /**
     * Frees associated resources of the source. The source will cease to call
     * the painter after calling this function.
     */
    close () {
      if (lastState !== 'ATOLLA_SOURCE_STATE_CLOSED') {
        const oldState = lastState
        if (updateTimeout) {
          clearTimeout(updateTimeout)
          updateTimeout = undefined
        }
        source = undefined
        errorMsg = undefined
        lastState = 'ATOLLA_SOURCE_STATE_CLOSED'
        onStateChange('ATOLLA_SOURCE_STATE_CLOSED', oldState)
      }
    }
  }

  function update () {
    updateTimeout = undefined

    const state = source.state()

    if (lastState && state !== lastState) {
      handleStateChange(state, lastState)
    }

    switch (state) {
      case 'ATOLLA_SOURCE_STATE_WAITING':
        errorMsg = undefined
        scheduleUpdate(10)
        break

      case 'ATOLLA_SOURCE_STATE_OPEN':
        errorMsg = undefined
        stream()
        break

      case 'ATOLLA_SOURCE_STATE_ERROR':
        errorMsg = source.errorMsg()
        break
    }

    lastState = state
  }

  function handleStateChange (newState, oldState) {
    onStateChange(newState, oldState)

    switch (newState) {
      case 'ATOLLA_SOURCE_STATE_OPEN':
        onReady()
        break

      case 'ATOLLA_SOURCE_STATE_ERROR':
        onError(source.errorMsg())
        break
    }
  }

  function stream () {
    for (let readyCount = source.putReadyCount(); readyCount > 0; readyCount = source.putReadyCount()) {
      putFrame()
    }

    scheduleUpdate(source.putReadyTimeout())
  }

  function putFrame () {
    let painted = toUint8ArrayColors(painter(time)) || defaultColor

    if (source.put(painted)) {
      time += frameDurationSeconds
    }
  }

  function scheduleUpdate (ms) {
    updateTimeout = setTimeout(update, ms)
  }
}
