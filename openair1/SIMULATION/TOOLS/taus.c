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

#include <time.h>
#include <stdlib.h>

unsigned int s0, s1, s2, b;

//----------------------------------------------
//
/*!
*/
//

unsigned int taus(void)
{

  b = (((s0 << 13) ^ s0) >> 19);
  s0 = (((s0 & 0xFFFFFFFE) << 12)^  b);
  b = (((s1 << 2) ^ s1) >> 25);
  s1 = (((s1 & 0xFFFFFFF8) << 4)^  b);
  b = (((s2 << 3) ^ s2) >> 11);
  s2 = (((s2 & 0xFFFFFFF0) << 17)^  b);
  return s0 ^ s1 ^ s2;
}
#if 1
void set_taus_seed(unsigned int seed_init)
{

  struct drand48_data buffer;
  unsigned long result = 0;

  if (seed_init == 0) {
    s0 = (unsigned int)time(NULL);
    s1 = (unsigned int)time(NULL);
    s2 = (unsigned int)time(NULL);
  } else {
    /* Use reentrant version of rand48 to ensure that no conflicts with other generators occur */
    srand48_r((long int)seed_init, &buffer);
    mrand48_r(&buffer, (long int *)&result);
    s0 = result;
    mrand48_r(&buffer, (long int *)&result);
    s1 = result;
    mrand48_r(&buffer, (long int *)&result);
    s2 = result;
  }
}
#endif

#if 0
 void set_taus_seed(unsigned int seed_init)
{

  struct drand48_data buffer;
  unsigned long result = 0;
    s0 = (unsigned int)0x1e23d852;
    s1 = (unsigned int)0x81f38a1c;
    s2 = (unsigned int)0xfe1a133e;

    return;

  if (seed_init == 0) {
    s0 = (unsigned int)time(NULL);
    s1 = (unsigned int)time(NULL);
    s2 = (unsigned int)time(NULL);
  } else {
   // Use reentrant version of rand48 to ensure that no conflicts with other generators occur */
    srand48_r((long int)seed_init, &buffer);
    mrand48_r(&buffer, (long int *)&result);
    s0 = result;
    mrand48_r(&buffer, (long int *)&result);
    s1 = result;
    mrand48_r(&buffer, (long int *)&result);
    s2 = result;
  }
}
#endif

#ifdef MAIN

main()
{

  unsigned int i,rand;

  set_taus_seed();

  for (i=0; i<10; i++) {

    rand = taus();
    printf("%u\n",rand);

  }
}
#endif //MAIN



