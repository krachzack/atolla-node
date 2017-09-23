
const atolla = require('./build/Release/atolla')

module.exports = {
  library: `${atolla.versionLibraryMajor}.${atolla.versionLibraryMinor}.${atolla.versionLibraryPatch}`,
  protocol: `${atolla.versionProtocolMajor}.${atolla.versionProtocolMinor}`
}
