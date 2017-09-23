const tinycolor = require('tinycolor2')
const { Source } = require('./build/Release/atolla')

module.exports = function source (spec) {
  const source = new Source(spec)
  const defaultColor = new Uint8Array([0,0,0])
  const defaultPainter = function() { return defaultColor }
  let painter = ('painter' in spec && typeof spec.painter === 'function') ? spec.painter : defaultPainter
  let onStateChange = ('onStateChange' in spec && typeof spec.onStateChange == 'function')
                          ? spec.onStateChange
                          : function() {}
  let time = 0
  const frameDurationSeconds = spec.frameDurationMs / 1000
  let updateTimeout
  let lastState
  let errorMsg

  update()

  function update ()
  {
    const state = source ? source.state() : 'ATOLLA_SOURCE_STATE_CLOSED'

    if (lastState && state !== lastState) {
      onStateChange(state, lastState)
    }

    switch (state) {
      case "ATOLLA_SOURCE_STATE_WAITING":
        errorMsg = undefined
        setTimeout(update, 10)
        break

      case "ATOLLA_SOURCE_STATE_OPEN":
        errorMsg = undefined
        stream()
        break

      case "ATOLLA_SOURCE_STATE_ERROR":
        errorMsg = source.errorMsg()
        break
    }

    lastState = state
  }

  function stream()
  {
    for (let readyCount = source.putReadyCount(); readyCount > 0; readyCount = source.putReadyCount()) {
      putFrame()
    }

    updateTimeout = setTimeout(update, source.putReadyTimeout())
  }

  function putFrame () {
    let painted = painter(time)

    if (!(painted instanceof Uint8Array)) {
      if(typeof painted == 'string') {
        const { r, g, b } = tinycolor(painted).toRgb()
        painted = new Uint8Array([r,g,b])
      } else if (Array.isArray(painted) && painted.length >= 3) {
        if(typeof painted[0] == "string") {
          painted = new Uint8Array(
            painted.reduce((painted, next) => {
              if(next instanceof Uint8Array) {
                painted.concat(Array.from(next))
              } else if(typeof next == 'string') {
                const { r, g, b } = tinycolor(next).toRgb()
                painted.concat([r,g,b])
              }
            }, [])
          )
        } else if (typeof painted[0] == "number") {
          painted = new Uint8Array(painted)
        } else {
          painted = defaultColor
        }
      } else {
        painted = defaultColor
      }
    }

    if(source.put(painted)) {
      time += frameDurationSeconds
    }
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
    get errorMessage () {
      return errorMsg
    },
    close () {
      if (updateTimeout) {
        clearTimeout(updateTimeout)
      }
    }
  }
}
