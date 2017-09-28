const tinycolor = require('tinycolor2')

/**
 * Converts the given value to an Uint8Array of color-triplets. If parsing failed,
 * undefined is returned. If parsing succeeded, the returned Uint8Array is guaranteed
 * to contain at least one RGB color-triplet and have a size that is divisible by
 * three.
 *
 * If Uint8Array given where length >= 3 and the length is divisible by three,
 * that same array will be returned. If either criterion is violated, undefined
 * is returned.
 *
 * If a plain JS array of numbers is given, it will be converted to an Uint8Array.
 * Note that both a length of three or more and a length divisble by three is required,
 * otherwise the function will return undefined.
 *
 * If a plain JS array of strings is given, it will be converted to an Uint8Array of
 * color triplets, assuming that each string represents a CSS color. If any of the
 * contained colors is invalid, the return value will be the value undefined.
 *
 * If plain string given, it will be parsed by tinycolor and converted to a single
 * RGB triplet returned as a three-component Uint8Array. If the color is invalid,
 * undefined will be returned instead of the array.
 *
 * @param {*} convertee the value to obtain colors from, can be one of CSS string,
 *                      flat array of RGB triplets, flat array of CSS strings, typed
 *                      Uint8Array with RGB triplets
 * @return Uint8Array of color triplets on success, otherwise undefined
 */
module.exports = function toUint8Colors (convertee) {
  if (convertee instanceof Uint8Array) {
    return (convertee.length >= 3 && convertee.length % 3 === 0)
              ? convertee
              : undefined
  }

  if (typeof convertee === 'string') {
    const color = tinycolor(convertee)

    if (!color.isValid()) {
      return undefined
    }

    const { r, g, b } = color.toRgb()
    return new Uint8Array([ r, g, b ])
  }

  if (Array.isArray(convertee)) {
    if(typeof convertee[0] === 'number') {
      return (convertee.length >= 3 && convertee.length % 3 === 0)
                ? new Uint8Array(convertee)
                : undefined
    } else if(typeof convertee[0] === 'string') {
      let allValid = true

      const colors = convertee.reduce((convertee, next) => {
        const color = tinycolor(next)
        allValid = allValid && color.isValid()
        const { r, g, b } = color.toRgb()
        convertee.concat([r, g, b])
      }, [])

      return allValid ? new Uint8Array(colors) : undefined
    }
  }

  return undefined
}
