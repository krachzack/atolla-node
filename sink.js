const { Sink } = require('./build/Release/atolla')

const noop = () => {}
const stateUpdateIntervalMs = 20
// If requestAnimationFrame is not available, the painter will be scheduled in this interval
const frameGetIntervalMs = 15

module.exports = function sink (spec) {
  let sink = new Sink(spec)
  let painter = ('painter' in spec && typeof spec.painter === 'function') ? spec.painter : noop
  let lastState
  const onStateChange = ('onStateChange' in spec && typeof spec.onStateChange === 'function')
                             ? spec.onStateChange
                             : noop

  const onReady = ('onReady' in spec && typeof spec.onReady === 'function')
                             ? spec.onReady
                             : noop

  const onLent = ('onLent' in spec && typeof spec.onLent === 'function')
                             ? spec.onLent
                             : noop

  const onError = ('onError' in spec && typeof spec.onError === 'function')
                             ? spec.onError
                             : noop

  const frameRawColors = new Uint8Array(spec.lightsCount * 3)
  const frameJsColors = []
  for (let i = 0; i < spec.lightsCount; ++i) {
    frameJsColors.push('black')
  }

  updateState()
  setInterval(updateState, stateUpdateIntervalMs)
  setInterval(updateFrame, frameGetIntervalMs)

  return {
    get painter () {
      return painter
    },
    set painter (newPainter) {
      painter = (typeof newPainter === 'function') ? newPainter : noop
    },
    get state () {
      return lastState
    },
    close () { sink = undefined }
  }

  /**
   * Receives frames from the network and tells the connected source
   * that the connection is still open.
   */
  function updateState () {
    if (!sink) { return } // Sink was closed

    const state = sink.state()

    if (state !== lastState) {
      handleStateChange(state, lastState)
    }

    lastState = state
  }

  /**
   * Requests the current frame from the sink and calls the painter
   * with the new frame.
   *
   * If available, calling the painter through the indirection of
   * requestAnimationFrame.
   */
  function updateFrame () {
    if (!sink) { return } // Sink was closed

    if (lastState === 'ATOLLA_SINK_STATE_LENT') {
      const ok = sink.get(frameRawColors)
      if (ok) {
        for (let jsIdx = 0; jsIdx < frameJsColors.length; ++jsIdx) {
          frameJsColors[jsIdx] = `rgb(${frameRawColors[jsIdx * 3]}, ${frameRawColors[jsIdx * 3 + 1]}, ${frameRawColors[jsIdx * 3 + 2]})`
        }

        if (typeof requestAnimationFrame === 'undefined') {
          painter(frameJsColors, frameRawColors)
        } else {
          requestAnimationFrame(() => painter(frameJsColors, frameRawColors))
        }
      }
    }
  }

  function handleStateChange (newState, oldState) {
    onStateChange(newState, oldState)

    switch (newState) {
      case 'ATOLLA_SINK_STATE_OPEN':
        onReady()
        break

      case 'ATOLLA_SINK_STATE_LENT':
        onLent()
        break

      case 'ATOLLA_SINK_STATE_ERROR':
        onError(sink.errorMsg())
        break
    }
  }
}
