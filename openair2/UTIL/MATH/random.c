/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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
