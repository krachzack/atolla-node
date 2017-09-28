const { Sink } = require('./build/Release/atolla')

const noop = () => {}

module.exports = function sink (spec) {
  const sink = new Sink(spec)
  let painter = ('painter' in spec && typeof spec.painter === 'function') ? spec.painter : defaultPainter
  let lastState
  const onStateChange = ('onStateChange' in spec && typeof spec.onStateChange == 'function')
                             ? spec.onStateChange
                             : noop

  const onReady = ('onReady' in spec && typeof spec.onReady == 'function')
                             ? spec.onReady
                             : noop

  const onLent = ('onLent' in spec && typeof spec.onLent == 'function')
                             ? spec.onLent
                             : noop

  const onError = ('onError' in spec && typeof spec.onError == 'function')
                             ? spec.onError
                             : noop

  const frameRawColors = new Uint8Array(spec.lightsCount * 3)
  const frameJsColors = []
  for(let i = 0; i < spec.lightsCount; ++i) {
    frameJsColors.push('black')
  }

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
    close () { sink = undefined }
  }

  function update () {
    if (!sink) { return } // Sink was closed

    const state = sink.state()

    if (state !== lastState) {
      handleStateChange(state, lastState)
    }

    if (state === 'ATOLLA_SINK_STATE_LENT') {
      const ok = sink.get(frameRawColors)
      if (ok) {
        for (let jsIdx = 0; jsIdx < frameJsColors.length; ++jsIdx) {
          frameJsColors[jsIdx] = `rgb(${frameRawColors[jsIdx*3]}, ${frameRawColors[jsIdx*3+1]}, ${frameRawColors[jsIdx*3+2]})`
        }
        painter(frameJsColors, frameRawColors)
      }
    }

    lastState = state
    requestAnimationFrame(update)
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
