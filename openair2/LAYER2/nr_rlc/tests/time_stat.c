#include "../../../../common/utils/time_stat.h"

time_average_t *time_average_new(int duration, int initial_size)
{
  return (void *)0;
}

void time_average_free(time_average_t *t)
{
}

void time_average_add(time_average_t *t, uint64_t time, uint64_t value)
{
}

double time_average_get_average(time_average_t *t, uint64_t time)
{
  return 0;
}

uint64_t time_average_now(void)
{
  return 0;
}

