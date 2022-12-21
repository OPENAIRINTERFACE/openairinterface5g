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

#include "PHY/defs_common.h"
#include "tools_defs.h"


int32_t sub_cpx_vector16(int16_t *x,
                     int16_t *y,
                     int16_t *z,
                     uint32_t N)
{
  unsigned int i;                 // loop counter

  simde__m128i *x_128;
  simde__m128i *y_128;
  simde__m128i *z_128;

  x_128 = (simde__m128i *)&x[0];
  y_128 = (simde__m128i *)&y[0];
  z_128 = (simde__m128i *)&z[0];

 for(i=0; i<(N>>3); i++) {
    z_128[0] = simde_mm_subs_epi16(x_128[0],y_128[0]);

    x_128++;
    y_128++;
    z_128++;

  }

  simde_mm_empty();
  simde_m_empty();
  return(0);
}



#ifdef MAIN
#include <stdio.h>

main ()
{

  short input[256] __attribute__((aligned(16)));
  short output[256] __attribute__((aligned(16)));

  int i;
  c16_t alpha;

  memset(output,0,256*2);

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
