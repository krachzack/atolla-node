// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.

const atolla = require('atolla')
const mdns = require('mdns')

const defaultPort = 10042
const readyColor = 'green'
const errorColor = 'red'
const lentColor = 'yellow'

let ad = false

makeSink(defaultPort)

function makeSink (port, doNotTryAgain) {
  console.log('making')

  const sinkHandle = atolla.sink({
    port,
    lightsCount: 1,
    painter (jsColors) {
      document.body.style.backgroundColor = jsColors[0]
    },
    onReady () {
      console.log('Sink is currently open and ready for sources to connect…')
      document.body.style.backgroundColor = readyColor
      if (!ad) {
        // Make service known via MDNS when first entering ready state
        ad = mdns.createAdvertisement(mdns.udp('atolla'), port);
        ad.start();
      }
    },
    onLent () {
      console.log('Sink is now lent to a source and will regularly call the painter with color data from the source')
      document.body.style.backgroundColor = lentColor
    },
    onError (errorMsg) {
      document.body.style.backgroundColor = errorColor
      console.log('Sink entered unrecoveable error state:', errorMsg)

      // If failed, try other port, but just do this once
      if(!doNotTryAgain) {
        setTimeout(function() {
          const randomPort = (11000 + Math.random() * (50000-11000)) | 0
          makeSink(randomPort, true)
        }, 0);
      }
    }
  })
}
