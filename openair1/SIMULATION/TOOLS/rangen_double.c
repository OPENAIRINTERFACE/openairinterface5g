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
#ifdef USER_MODE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#endif

#include  "defs.h"

static unsigned int seed, iy, ir[98];
/*
@defgroup _uniformdouble
@ingroup numerical Uniform linear congruential random number generator.
*/

/*!\brief Initialization routine for Uniform/Gaussian random number generators. */

#define a 1664525lu
#define mod 4294967296.0                /* is 2**32 */


void randominit(unsigned seed_init)
{
  int i;
  // this need to be integrated with the existing rng, like taus: navid
  msg("Initializing random number generator, seed %x\n",seed_init);

  if (seed_init == 0) {
    srand((unsigned)time(NULL));

    seed = (unsigned int) rand();
  } else {
    seed = seed_init;
  }

  if (seed % 2 == 0) seed += 1; /* seed and mod are relative prime */

  for (i=1; i<=97; i++) {
    seed = a*seed;                 /* mod 2**32  */
    ir[i]= seed;                   /* initialize the shuffle table    */
  }

  iy=1;
}

/*!\brief Uniform linear congruential random number generator on \f$[0,1)\f$.  Returns a double-precision floating-point number.*/

double uniformrandom(void)
{
#define a 1664525lu
#define mod 4294967296.0                /* is 2**32 */

  int j;

  j=1 + 97.0*iy/mod;
  iy=ir[j];
  seed = a*seed;                          /* mod 2**32 */
  ir[j] = seed;
  return( (double) iy/mod );
}

/*
@defgroup _gaussdouble Gaussian random number generator based on modified Box-Muller transformation.
@ingroup numerical
*/

/*!\brief Gaussian random number generator based on modified Box-Muller transformation.Returns a double-precision floating-point number. */

double gaussdouble(double mean, double variance)
{
  static int iset=0;
  static double gset;
  double fac,r,v1,v2;

  if (iset == 0) {
    do {
      v1 = 2.0*uniformrandom()-1.0;
      v2 = 2.0*uniformrandom()-1.0;
      r = v1*v1+v2*v2;
    }  while (r >= 1.0);

    fac = sqrt(-2.0*log(r)/r);
    gset= v1*fac;
    iset=1;
    return(sqrt(variance)*v2*fac + mean);
  } else {
    iset=0;
    return(sqrt(variance)*gset + mean);
  }
}



#ifdef MAIN
main(int argc,char **argv)
{

  int i;

  randominit();

  for (i=0; i<10; i++) {
    printf("%f\n",gaussdouble(0.0,1.0));
  }
}
#endif

