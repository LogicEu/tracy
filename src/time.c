#define _POSIX_C_SOURCE 199309L
#include <tracy.h>
#include <time.h>

double time_clock()
{
    static struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec + time.tv_nsec / 1000000000.0;
}
