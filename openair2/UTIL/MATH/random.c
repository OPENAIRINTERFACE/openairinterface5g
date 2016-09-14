/*
                                 random.c
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 ***************************************************************************/
#include "rtos_header.h"
#include "platform_types.h"

#ifdef USER_MODE
#    include <sys/time.h>
#else
#include <rtai_sched.h>
#endif


/* Random generators */
#define FACTOR       16807
#define LASTXN       127773
#define UPTOMOD     -2836

static int      seed;


void
init_uniform (void)
{
#ifdef USER_MODE
  struct timeval  tv;
  struct timezone tz;

  gettimeofday (&tv, &tz);
  seed = (int) tv.tv_usec;
#ifdef NODE_MT
#warning TO DO seed = mobileId
  //seed += mobileId;
#endif
#ifdef NODE_RG
#warning TO DO seed = rgId
  //seed += rgId;
#endif
#else
  seed = rt_get_time_ns();
#endif
}


int
uniform (void)
{
  static int      times, rest, prod1, prod2;

  times = seed / LASTXN;
  rest = seed - times * LASTXN;
  prod1 = times * UPTOMOD;
  prod2 = rest * FACTOR;
  seed = prod1 + prod2;

  return seed;
}
