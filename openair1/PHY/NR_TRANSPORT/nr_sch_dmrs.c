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

/*! \file PHY/NR_TRANSPORT/nr_sch_dmrs.c
* \brief
* \author
* \date
* \version
* \company Eurecom
* \email:
* \note
* \warning
*/

#include "nr_sch_dmrs.h"

/*Table 7.4.1.1.2-1 and 7.4.1.1.2-2 38211 Columns: ap - CDM group - Delta - Wf(0) - Wf(1) - Wt(0) - Wt(1)*/
/*Table 6.4.1.1.3-1 is identical to Table 7.4.1.1.2-1 and Table 6.4.1.1.3-2 is identical to Table 7.4.1.1.2-2. UL DMRS can reuse these tables*/
int8_t pdsch_dmrs_1[8][7] = {{0,0,0,1,1,1,1},
                             {1,0,0,1,-1,1,1},
                             {2,1,1,1,1,1,1},
                             {3,1,1,1,-1,1,1},
                             {4,0,0,1,1,1,-1},
                             {5,0,0,1,-1,1,-1},
                             {6,1,1,1,1,1,-1},
                             {7,1,1,1,-1,1,-1}};

int8_t pdsch_dmrs_2[12][7] = {{0,0,0,1,1,1,1},
                              {1,0,0,1,-1,1,1},
                              {2,1,2,1,1,1,1},
                              {3,1,2,1,-1,1,1},
                              {4,2,4,1,1,1,1},
                              {5,2,4,1,-1,1,1},
                              {6,0,0,1,1,1,-1},
                              {7,0,0,1,-1,1,-1},
                              {8,1,2,1,1,1,-1},
                              {9,1,2,1,-1,1,-1},
                              {10,2,4,1,1,1,-1},
                              {11,2,4,1,-1,1,-1}};

void get_antenna_ports(uint8_t *ap, uint8_t n_symbs, uint8_t config) {
  if (config == NFAPI_NR_DMRS_TYPE1)
    for (int i=0; i<(4+((n_symbs-1)<<2)); i++)
      *(ap+i) = i;
  else
    for (int i=0; i<(7+((n_symbs-1)<<2)); i++)
      *(ap+i) = i;
}

void get_Wt(int8_t *Wt, uint8_t ap, uint8_t config) {
  for (int i=0; i<2; i++)
    *(Wt+i)=(config==NFAPI_NR_DMRS_TYPE1)?(pdsch_dmrs_1[ap][5+i]):(pdsch_dmrs_2[ap][5+i]);
}

void get_Wf(int8_t *Wf, uint8_t ap, uint8_t config) {
  for (int i=0; i<2; i++)
    *(Wf+i)=(config==NFAPI_NR_DMRS_TYPE1)?(pdsch_dmrs_1[ap][3+i]):(pdsch_dmrs_2[ap][3+i]);
}

uint8_t get_delta(uint8_t ap, uint8_t config) {
  return ((config==NFAPI_NR_DMRS_TYPE1)?(pdsch_dmrs_1[ap][2]):(pdsch_dmrs_2[ap][2]));
}

uint16_t get_dmrs_freq_idx(uint16_t n, uint8_t k_prime, uint8_t delta, uint8_t dmrs_type) {
  uint16_t dmrs_idx = (dmrs_type)? (6*n+k_prime+delta):((n<<2)+(k_prime<<1)+delta);
  return dmrs_idx;
}

uint8_t get_l0(uint16_t dlDmrsSymbPos) {

  uint16_t mask=dlDmrsSymbPos;
  int l0;
  for (l0=0;l0<14;l0++) {
    if ((mask&1) == 1) break;
    mask>>=1;
  }
  AssertFatal(l0 < 4,"impossible l0 %d\n",l0);
  return (l0);
}
