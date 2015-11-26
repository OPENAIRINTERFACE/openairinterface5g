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
#include "defs.h"

static  __m128i alpha_128 __attribute__ ((aligned(16)));
static  __m128i shift     __attribute__ ((aligned(16)));

int add_cpx_vector(short *x,
                   short *alpha,
                   short *y,
                   unsigned int N)
{
  unsigned int i;                 // loop counter

  __m128i *x_128;
  __m128i *y_128;

  x_128 = (__m128i *)&x[0];
  y_128 = (__m128i *)&y[0];

  alpha_128 = _mm_set1_epi32(*((int*)alpha));

  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>3); i++) {
    y_128[0] = _mm_adds_epi16(alpha_128, x_128[0]);
    y_128[1] = _mm_adds_epi16(alpha_128, x_128[1]);
    y_128[2] = _mm_adds_epi16(alpha_128, x_128[2]);
    y_128[3] = _mm_adds_epi16(alpha_128, x_128[3]);


    x_128+=4;
    y_128 +=4;

  }

  return (0);
}

int add_vector32_scalar(short *x,
                        int alpha,
                        short *y,
                        unsigned int N)
{
  unsigned int i;                 // loop counter

  __m128i *x_128;
  __m128i *y_128;

  x_128 = (__m128i *)&x[0];
  y_128 = (__m128i *)&y[0];

  alpha_128 = _mm_setr_epi32(alpha,0,alpha,0);

  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>3); i++) {
    y_128[0] = _mm_add_epi32(alpha_128, x_128[0]);
    y_128[1] = _mm_add_epi32(alpha_128, x_128[1]);
    y_128[2] = _mm_add_epi32(alpha_128, x_128[2]);
    y_128[3] = _mm_add_epi32(alpha_128, x_128[3]);


    x_128+=4;
    y_128 +=4;

  }

  return (0);
}


int add_real_vector64_scalar(short *x,
                             long long int a,
                             short *y,
                             unsigned int N)
{
  unsigned int i;                 // loop counter

  __m128i *x_128;
  __m128i *y_128;

  x_128 = (__m128i *)&x[0];
  y_128 = (__m128i *)&y[0];

  alpha_128 = _mm_set1_epi64((__m64) a);

  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>3); i++) {
    y_128[0] = _mm_add_epi64(alpha_128, x_128[0]);
    y_128[1] = _mm_add_epi64(alpha_128, x_128[1]);
    y_128[2] = _mm_add_epi64(alpha_128, x_128[2]);
    y_128[3] = _mm_add_epi64(alpha_128, x_128[3]);


    x_128+=4;
    y_128+=4;

  }

  return(0);
}


#ifdef MAIN
#include <stdio.h>

main ()
{

  short input[256] __attribute__((aligned(16)));
  short output[256] __attribute__((aligned(16)));

  int i;
  struct complex16 alpha;

  Zero_Buffer(output,256*2);

  input[0] = 100;
  input[1] = 200;
  input[2] = 100;
  input[3] = 200;
  input[4] = 1234;
  input[5] = -1234;
  input[6] = 1234;
  input[7] = -1234;
  input[8] = 100;
  input[9] = 200;
  input[10] = 100;
  input[11] = 200;
  input[12] = 1000;
  input[13] = 2000;
  input[14] = 1000;
  input[15] = 2000;

  alpha.r = 10;
  alpha.i = -10;

  add_cpx_vector(input,(short*) &alpha,input,8);

  for (i=0; i<16; i+=2)
    printf("output[%d] = %d + %d i\n",i,input[i],input[i+1]);

}

#endif //MAIN
