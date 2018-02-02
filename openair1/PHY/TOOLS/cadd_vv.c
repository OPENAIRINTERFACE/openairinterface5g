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


int add_vector16(short *x,
                 short *y,
                 short *z,
                 unsigned int N)
{
  unsigned int i;                 // loop counter

  __m128i *x_128;
  __m128i *y_128;
  __m128i *z_128;

  x_128 = (__m128i *)&x[0];
  y_128 = (__m128i *)&y[0];
  z_128 = (__m128i *)&z[0];


  for(i=0; i<(N>>5); i++) {
    /*
    printf("i = %d\n",i);
    print_shorts(x_128[0],"x[0]=");
    print_shorts(y_128[0],"y[0]=");
    */

    z_128[0] = _mm_adds_epi16(x_128[0],y_128[0]);
    /*
    print_shorts(z_128[0],"z[0]=");

    print_shorts(x_128[1],"x[1]=");
    print_shorts(y_128[1],"y[1]=");
    */

    z_128[1] = _mm_adds_epi16(x_128[1],y_128[1]);
    /*
    print_shorts(z_128[1],"z[1]=");

    print_shorts(x_128[2],"x[2]=");
    print_shorts(y_128[2],"y[2]=");
    */

    z_128[2] = _mm_adds_epi16(x_128[2],y_128[2]);
    /*
    print_shorts(z_128[2],"z[2]=");

    print_shorts(x_128[3],"x[3]=");
    print_shorts(y_128[3],"y[3]=");
    */

    z_128[3] = _mm_adds_epi16(x_128[3],y_128[3]);
    /*
    print_shorts(z_128[3],"z[3]=");
    */





    x_128 +=4;
    y_128 +=4;
    z_128 +=4;

  }

  _mm_empty();
  _m_empty();

  return(0);
}

/*
print_shorts64(__m64 x,char *s) {

  printf("%s ",s);
  printf("%d %d %d %d\n",((short *)&x)[0],((short *)&x)[1],((short *)&x)[2],((short *)&x)[3]);

}
*/

int add_vector16_64(short *x,
                    short *y,
                    short *z,
                    unsigned int N)
{
  unsigned int i;                 // loop counter

  __m64 *x_64;
  __m64 *y_64;
  __m64 *z_64;

  x_64 = (__m64 *)&x[0];
  y_64 = (__m64 *)&y[0];
  z_64 = (__m64 *)&z[0];


  for(i=0; i<(N>>2); i++) {
    /*
    printf("i = %d\n",i);
    print_shorts64(x_64[i],"x[i]=");
    print_shorts64(y_64[i],"y[i]=");
    */

    z_64[i] = _mm_adds_pi16(x_64[i],y_64[i]);
    /*
    print_shorts64(z_64[i],"z[i]=");
    */


  }

  _mm_empty();
  _m_empty();
  return(0);
}

int add_cpx_vector32(short *x,
                     short *y,
                     short *z,
                     unsigned int N)
{
  unsigned int i;                 // loop counter

  __m128i *x_128;
  __m128i *y_128;
  __m128i *z_128;

  x_128 = (__m128i *)&x[0];
  y_128 = (__m128i *)&y[0];
  z_128 = (__m128i *)&z[0];


  for(i=0; i<(N>>3); i++) {
    z_128[0] = _mm_add_epi32(x_128[0],y_128[0]);
    z_128[1] = _mm_add_epi32(x_128[1],y_128[1]);
    z_128[2] = _mm_add_epi32(x_128[2],y_128[2]);
    z_128[3] = _mm_add_epi32(x_128[3],y_128[3]);


    x_128+=4;
    y_128 +=4;
    z_128 +=4;

  }

  _mm_empty();
  _m_empty();
  return(0);
}

int32_t sub_cpx_vector16(int16_t *x,
                     int16_t *y,
                     int16_t *z,
                     uint32_t N)
{
  unsigned int i;                 // loop counter

  __m128i *x_128;
  __m128i *y_128;
  __m128i *z_128;

  x_128 = (__m128i *)&x[0];
  y_128 = (__m128i *)&y[0];
  z_128 = (__m128i *)&z[0];

 for(i=0; i<(N>>3); i++) {
    z_128[0] = _mm_subs_epi16(x_128[0],y_128[0]);

    x_128++;
    y_128++;
    z_128++;

  }

  _mm_empty();
  _m_empty();
  return(0);
}



int add_real_vector64(short *x,
                      short* y,
                      short *z,
                      unsigned int N)
{
  unsigned int i;                 // loop counter

  __m128i *x_128;
  __m128i *y_128;
  __m128i *z_128;

  x_128 = (__m128i *)&x[0];
  y_128 = (__m128i *)&y[0];
  z_128 = (__m128i *)&z[0];

  for(i=0; i<(N>>3); i++) {
    z_128[0] = _mm_add_epi64(x_128[0], y_128[0]);
    z_128[1] = _mm_add_epi64(x_128[1], y_128[1]);
    z_128[2] = _mm_add_epi64(x_128[2], y_128[2]);
    z_128[3] = _mm_add_epi64(x_128[3], y_128[3]);


    x_128+=4;
    y_128+=4;
    z_128+=4;

  }

  _mm_empty();
  _m_empty();
  return(0);
}

int sub_real_vector64(short *x,
                      short* y,
                      short *z,
                      unsigned int N)
{
  unsigned int i;                 // loop counter

  __m128i *x_128;
  __m128i *y_128;
  __m128i *z_128;

  x_128 = (__m128i *)&x[0];
  y_128 = (__m128i *)&y[0];
  z_128 = (__m128i *)&z[0];

  for(i=0; i<(N>>3); i++) {
    z_128[0] = _mm_sub_epi64(x_128[0], y_128[0]);
    z_128[1] = _mm_sub_epi64(x_128[1], y_128[1]);
    z_128[2] = _mm_sub_epi64(x_128[2], y_128[2]);
    z_128[3] = _mm_sub_epi64(x_128[3], y_128[3]);


    x_128+=4;
    y_128+=4;
    z_128+=4;

  }

  _mm_empty();
  _m_empty();
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
