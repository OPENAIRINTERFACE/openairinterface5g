#ifndef _TIME_UTILS_H_
#define _TIME_UTILS_H_

#include <time.h>
#include <stdint.h>

static inline int64_t clock_difftime_ns(struct timespec start, struct timespec end)
{
  struct timespec temp;
  int64_t temp_ns;

  if ((end.tv_nsec-start.tv_nsec)<0) {
    temp.tv_sec = end.tv_sec-start.tv_sec-1;
    temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec-start.tv_sec;
    temp.tv_nsec = end.tv_nsec-start.tv_nsec;
  }
  temp_ns = (int64_t)(temp.tv_sec) * (int64_t)1000000000 + (temp.tv_nsec);
  return temp_ns;
}

#endif /* _TIME_UTILS_H_ */
