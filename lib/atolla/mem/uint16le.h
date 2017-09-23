#ifndef MSG_MEM_UINT16LE_H
#define MSG_MEM_UINT16LE_H

#include "../atolla/primitives.h"

#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)

// Detect endianness with preprocessor macros, or, if none available, detect at runtime
#if defined(__BIG_ENDIAN__)
    #define mem_uint16le_to(in)   ((uint16_t) (((in) << 8) | (((in) & 0xFFFF) >> 8)) )
    #define mem_uint16le_from(in) ((uint16_t) (((in) << 8) | (((in) & 0xFFFF) >> 8)) )
#elif defined(__LITTLE_ENDIAN__)
    #define mem_uint16le_to(in)   ((uint16_t) (in))
    #define mem_uint16le_from(in) ((uint16_t) (in))
#else
    #define mem_uint16le_to(in)  ( (IS_BIG_ENDIAN) ? ((uint16_t) (((in) << 8) | (((in) & 0xFFFF) >> 8)) ) : (in) )
    #define mem_uint16le_from(in)  ( (IS_BIG_ENDIAN) ? ((uint16_t) (((in) << 8) | (((in) & 0xFFFF) >> 8)) ) : (in) )
#endif

#endif // MSG_MEM_UINT16LE_H
