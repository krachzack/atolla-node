#include "now.h"

#ifdef ARDUINO_ARCH_ESP8266
    #include <Arduino.h>
#else
    #include "gettime.h"
#endif

unsigned int time_now()
{
#ifdef ARDUINO_ARCH_ESP8266
    return millis();
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return ts.tv_sec * 1000 +
           ts.tv_nsec / 1000000;
#endif
}