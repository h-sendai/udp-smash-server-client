// #define _GNU_SOURCE

#include "timespecsub.h"

int timespecsub(struct timespec *a, struct timespec *b, struct timespec *r)
{
    r->tv_sec  = a->tv_sec  - b->tv_sec;
    r->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (r->tv_nsec < 0) {
        r->tv_nsec += 1000000000;
        r->tv_sec  -= 1;
    }

    return 0;
}
