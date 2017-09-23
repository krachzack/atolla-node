#ifndef UINT16_BYTE_H
#define UINT16_BYTE_H

#include <stdint.h>

/** Endianness independent way of getting the lower byte of a 16bit int */
#define mem_uint16_byte_low(in) ((uint8_t) (((uint16_t) (in)) % 256))
/** Endianness independent way of getting the higher byte of a 16bit int */
#define mem_uint16_byte_high(in) ((uint8_t) (((uint16_t) (in)) / 256))

#endif