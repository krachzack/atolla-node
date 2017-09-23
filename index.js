const atolla = require('./build/Release/atolla');
const sink = require('./sink')
const source = require('./source')
const version = require('./version')

module.exports = {
  version,
  sink,
  source
}