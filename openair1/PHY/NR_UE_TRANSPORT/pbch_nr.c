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

/**********************************************************************
*
* FILENAME    :  pbch_nr.c
*
* MODULE      :  broacast channel
*
* DESCRIPTION :  generation of pbch
*                3GPP TS 38.211 7.3.3 Physical broadcast channel
*
************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "PHY/defs.h"

#define DEFINE_VARIABLES_PBCH_NR_H
#include "PHY/NR_REFSIG/pbch_nr.h"
#undef DEFINE_VARIABLES_PBCH_NR_H

/*******************************************************************
*
* NAME :         pseudo_random_gold_sequence
*
* PARAMETERS :
*
* RETURN :       generate pseudo-random sequence which is a length-31 Gold sequence
*
* DESCRIPTION :  3GPP TS 38.211 5.2.1 Pseudo-random sequence generation
*                Sequence generation
*
*********************************************************************/

#define NC           (1600)
#define GOLD_LENGTH  (31)

uint32_t *pseudo_random_gold_sequence(length M_PN, uint32_t cinit)
{
  int size = M_PN * sizeof(uint32_t);
  int size_x = sizeof(int)*M_PN + size;
  int *x1 = malloc(size_x);
  int *x2 = malloc(size_x);

  if ((x1 == NULL) || (x2 == NULL)) {
    msg("Fatal memory allocation problem \n");
	assert(0);
  }
  else {
    bzero(x1, size_x);
    bzero(x2, size_x);
  }

  x1[0] = 1;

  for (n = 0; n < 31; n++) {
     x2[n] = (cinit >> n) & 0x1;
  }

  for (int n = 0; n < (NC+M_PN); n++) {
    x1(n+31) = (x1(n+3) + x1(n))%2;
    x2(n+31) = (x2(n+3) + x2(n+2) + x2(n+1) + x2(n))%2;
  }

  int *c = calloc(size);
  if (c != NULL) {
    bzero(c, size);
  }
  else {
   msg("Fatal memory allocation problem \n");
   assert(0);
  }

  for (int n = 0; n < M_PN; n++) {
    c(i) = (x1(n+NC) + x2(n+NC))%2;
  }

  return c;
}
