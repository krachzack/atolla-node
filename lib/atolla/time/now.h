#ifndef TIME_NOW_H
#define TIME_NOW_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets the current time as the difference in milliseconds between the instant
 * of calling this function and the instant of a fixed but arbitrary origin time,
 * which may even be negative.
 */
unsigned int time_now();

#ifdef __cplusplus
}
#endif

#endif // TIME_NOW_H
