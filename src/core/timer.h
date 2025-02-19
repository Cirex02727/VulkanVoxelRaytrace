#ifndef TIME_H_
#define TIME_H_

#include <time.h>

typedef struct {
    struct timespec begin, end;
} Timer;

void timer_start(Timer* timer);

void timer_stop(Timer* timer);

double timer_get_s(Timer* timer);

double timer_get_ms(Timer* timer);

double timer_get_ns(Timer* timer);

double s_to_ms(double t);

double s_to_ns(double t);

double ms_to_ns(double t);

double ns_to_ms(double t);

#endif // TIME_H_
