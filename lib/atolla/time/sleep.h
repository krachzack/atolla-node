#ifndef TIME_SLEEP_H
#define TIME_SLEEP_H

#if defined(ARDUINO_ARCH_ESP8266)
    // On ESP use arduino framework delay function
    #include <Arduino.h>
    #define time_sleep(ms) (delay((ms)))
#elif defined(_WIN32) || defined(WIN32)
    // On windows use os sleep
    #include <windows.h>
    #define time_sleep(ms) (Sleep((ms)))
#else
    // On unixlike use unistd.h
    #include <unistd.h>
    #define time_sleep(ms) (usleep((ms) * 1000))
#endif

#endif // TIME_SLEEP_H
