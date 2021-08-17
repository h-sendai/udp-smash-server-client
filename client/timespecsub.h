//#define _BSD_SOURCE 1
//#define _POSIX_C_SOURCE 199309L

#ifndef _TIMESPECSUB
#define _TIMESPECSUB 1

#include <time.h>

extern int timespecsub(struct timespec *a, struct timespec *b, struct timespec *r);

#endif
