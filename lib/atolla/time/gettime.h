#ifndef TIME_GETTIME_H
#define TIME_GETTIME_H

#ifdef __MACH__
    // On systems with mach microkernel such as OS X or iOS use mach timing functions
    #include "mach_gettime.h"
#else
    // otherwise use posix
    #include <time.h>
#endif

#endif // TIME_GETTIME_H
