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

/*! \file taus.c
* \brief random number generator per OAI component
* \author Navid Nikaein
* \date 2011 - 2014
* \version 0.1
* \email navid.nikaein@eurecom.fr
* \warning
* @ingroup util
*/

#ifndef RTAI_ENABLED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "oml.h"
#else
#include <asm/io.h>
#include <asm/rtai.h>
#include <rtai.h>
#include <rtai_sched.h>
#define time(x) (unsigned int)(rt_get_time_ns())
#endif


unsigned int s0[MAX_NUM_COMPS], s1[MAX_NUM_COMPS], s2[MAX_NUM_COMPS], b[MAX_NUM_COMPS], r[MAX_NUM_COMPS];




inline unsigned int taus(unsigned int comp)
{

  b[comp] = (((s0[comp] << 13) ^ s0[comp]) >> 19);
  s0[comp] = (((s0[comp] & 0xFFFFFFFE) << 12)^  b[comp]);
  b[comp] = (((s1[comp] << 2) ^ s1[comp]) >> 25);
  s1[comp] = (((s1[comp] & 0xFFFFFFF8) << 4)^  b[comp]);
  b[comp] = (((s2[comp] << 3) ^ s2[comp]) >> 11);
  s2[comp] = (((s2[comp] & 0xFFFFFFF0) << 17)^  b[comp]);
  r[comp] = s0[comp] ^ s1[comp] ^ s2[comp];
  return r[comp];
}

void set_taus_seed(unsigned int seed_type)
{

  unsigned int i; // i index of component

  for (i=MIN_NUM_COMPS; i < MAX_NUM_COMPS  ; i ++)  {

    switch (seed_type) {
    case 0: // use rand func
      if (i == 0) srand(time(NULL));

      s0[i] = ((unsigned int)rand());
      s1[i] = ((unsigned int)rand());
      s2[i] = ((unsigned int)rand());
      printf("Initial seeds use rand: s0[%d] = 0x%x, s1[%d] = 0x%x, s2[%d] = 0x%x\n", i, s0[i], i, s1[i], i, s2[i]);
      break;

    case 1: // use rand with seed
      if (i == 0) srand(0x1e23d851);

      s0[i] = ((unsigned int)rand());
      s1[i] = ((unsigned int)rand());
      s2[i] = ((unsigned int)rand());
      printf("Initial seeds use rand with seed : s0[%d] = 0x%x, s1[%d] = 0x%x, s2[%d] = 0x%x\n", i, s0[i], i, s1[i], i, s2[i]);

      break;

    default:
      break;

    }
  }
}

int get_rand (unsigned int comp)
{
  if ((comp > MIN_NUM_COMPS) && (comp < MAX_NUM_COMPS))
    return r[comp];
  else {
    //LOG_E(RNG,"unknown component %d\n",comp);
    return -1;
  }
}

unsigned int dtaus(unsigned int comp, unsigned int a, unsigned b)
{

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
