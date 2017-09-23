const { Sink } = require('./build/Release/atolla')
const updateInterval = 5

module.exports = function sink (spec) {
  const sink = new Sink(spec)
  let painter = ('painter' in spec && typeof spec.painter === 'function') ? spec.painter : defaultPainter
  let lastState;
  const onStateChange = ('onStateChange' in spec && typeof spec.onStateChange == 'function')
                             ? spec.onStateChange
                             : function() {}
  const frameRawColors = new Uint8Array(spec.lightsCount * 3)
  const frameJsColors = []
  for(let i = 0; i < spec.lightsCount; ++i) {
    frameJsColors.push('black')
  }

  update()

  function update () {
    const state = sink.state()

    if (lastState && state !== lastState) {
      onStateChange(state, lastState)
    }

    if (state === 'ATOLLA_SINK_STATE_LENT') {
      const ok = sink.get(frameRawColors)
      console.log(frameRawColors)
      
      if (ok) {
        for (let jsIdx = 0; jsIdx < frameJsColors.length; ++jsIdx) {
          frameJsColors[jsIdx] = `#${frameRawColors[jsIdx*3].toString(16)}${frameRawColors[jsIdx*3+1].toString(16)}${frameRawColors[jsIdx*3+2].toString(16)}`;
        }

        painter(frameJsColors, frameRawColors)
      }
    }

    lastState = state
    setTimeout(update, updateInterval)
  }

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
}
