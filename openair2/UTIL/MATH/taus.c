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

/*! \file taus.c
* \brief random number generator per OAI component
* \author Navid Nikaein
* \date 2011 - 2014
* \version 0.1
* \email navid.nikaein@eurecom.fr
* \warning
* @ingroup util
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "oml.h"

unsigned int s0[MAX_NUM_COMPS], s1[MAX_NUM_COMPS], s2[MAX_NUM_COMPS], b[MAX_NUM_COMPS], r[MAX_NUM_COMPS];




inline unsigned int taus(unsigned int comp) {
  b[comp] = (((s0[comp] << 13) ^ s0[comp]) >> 19);
  s0[comp] = (((s0[comp] & 0xFFFFFFFE) << 12)^  b[comp]);
  b[comp] = (((s1[comp] << 2) ^ s1[comp]) >> 25);
  s1[comp] = (((s1[comp] & 0xFFFFFFF8) << 4)^  b[comp]);
  b[comp] = (((s2[comp] << 3) ^ s2[comp]) >> 11);
  s2[comp] = (((s2[comp] & 0xFFFFFFF0) << 17)^  b[comp]);
  r[comp] = s0[comp] ^ s1[comp] ^ s2[comp];
  return r[comp];
}

void set_taus_seed(unsigned int seed_type) {
  unsigned int i; // i index of component

  for (i=MIN_NUM_COMPS; i < MAX_NUM_COMPS  ; i ++)  {
    switch (seed_type) {
      case 0: // use rand func
        if (i == 0) srand(time(NULL));

        s0[i] = ((unsigned int)rand());
        s1[i] = ((unsigned int)rand());
        s2[i] = ((unsigned int)rand());
        printf("Initial seeds use rand: s0[%u] = 0x%x, s1[%u] = 0x%x, s2[%u] = 0x%x\n", i, s0[i], i, s1[i], i, s2[i]);
        break;

      case 1: // use rand with seed
        if (i == 0) srand(0x1e23d851);

        s0[i] = ((unsigned int)rand());
        s1[i] = ((unsigned int)rand());
        s2[i] = ((unsigned int)rand());
        printf("Initial seeds use rand with seed : s0[%u] = 0x%x, s1[%u] = 0x%x, s2[%u] = 0x%x\n", i, s0[i], i, s1[i], i, s2[i]);
        break;

      default:
        break;
    }
  }
}

int get_rand (unsigned int comp) {
  if ((comp > MIN_NUM_COMPS) && (comp < MAX_NUM_COMPS))
    return r[comp];
  else {
    //LOG_E(RNG,"unknown component %d\n",comp);
    return -1;
  }
}

unsigned int dtaus(unsigned int comp, unsigned int a, unsigned b) {
  return (int) (((double)taus(comp)/(double)0xffffffff)* (double)(b-a) + (double)a);
}
/*
#ifdef STANDALONE
main() {

  unsigned int i,randomg, randphy;

  set_taus_seed(0);
  printf("dtaus %d \n",dtaus(PHY, 1000, 1000000));

  do {//for (i=0;i<10;i++){
    randphy = taus(PHY);
    randomg = taus(OTG);
    i++;
    // printf("rand for OMG (%d,0x%x) PHY (%d,0x%x)\n",OMG, randomg, PHY, randphy);
  } while (randphy != randomg);
  printf("after %d run: get rand for (OMG 0x%x, PHY 0x%x)\n",i, get_rand(OTG), get_rand(PHY));

}
#endif

*/
