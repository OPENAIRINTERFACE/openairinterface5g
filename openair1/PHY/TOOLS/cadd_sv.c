/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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
