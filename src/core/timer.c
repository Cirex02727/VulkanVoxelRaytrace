#include "timer.h"

void timer_start(Timer* timer)
{
    clock_gettime(CLOCK_MONOTONIC, &timer->begin);
    timer->end = (struct timespec) {0};
}

void timer_stop(Timer* timer)
{
    clock_gettime(CLOCK_MONOTONIC, &timer->end);
}

double timer_get_ns(Timer* timer)
{
    return
        ((double)timer->end.tv_sec   * 1e9 + timer->end.tv_nsec) - 
        ((double)timer->begin.tv_sec * 1e9 + timer->begin.tv_nsec);
}

double timer_get_micros(Timer* timer)
{
    return
        ((double)timer->end.tv_sec   * 1e6 + timer->end.tv_nsec   * 1e-3) - 
        ((double)timer->begin.tv_sec * 1e6 + timer->begin.tv_nsec * 1e-3);
}

double timer_get_ms(Timer* timer)
{
    return
        ((double)timer->end.tv_sec   * 1e3 + timer->end.tv_nsec   * 1e-6) - 
        ((double)timer->begin.tv_sec * 1e3 + timer->begin.tv_nsec * 1e-6);
}

double timer_get_s(Timer* timer)
{
    return
        ((double)timer->end.tv_sec   + timer->end.tv_nsec   * 1e-9) - 
        ((double)timer->begin.tv_sec + timer->begin.tv_nsec * 1e-9);
}

double s_to_ms(double t)
{
    return t * 1e3;
}

double s_to_ns(double t)
{
    return t * 1e9;
}

double ms_to_ns(double t)
{
    return t * 1e6;
}

double ns_to_ms(double t)
{
    return t * 1e-6;
}
