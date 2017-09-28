const atolla = require('atolla')

const runningTimeSecs = 10

const source = atolla.source({
  // Connection settings
  hostname: 'localhost',
  port: 10042,
  frameDurationMs: 30,
  maxBufferedFrames: 10,
  // Callbacks
  painter: (time) => `hsl(0, 100%, ${100 * Math.abs(Math.sin(time))}%)`,
  onReady: () => console.log('Connection successful, streaming colors'),
  onStateChange: (newState, old) => {
    console.log(`Source entered state ${newState}`)
  },
  onError: (msg) => { console.error('Source disconnected because of error: ', msg); process.exit(0) }
})

// Close script after runningTimeSecs seconds
setTimeout(() => source.close(), runningTimeSecs*1000)
