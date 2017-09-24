// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.

const { sink } = require('atolla')
const mdns = require('mdns')

const port = (10000 + Math.random() * (50000-10000))|0

var ad = mdns.createAdvertisement(mdns.udp('atolla'), port);
ad.start();

sink({
  // Note name resolution blocks, this should be resolved beforehand
  port,
  lightsCount: 1,
  painter: function (jsColors) {
    document.body.style.backgroundColor = jsColors[0]
    console.log(jsColors[0], document.body.style.backgroundColor)
  }
})
