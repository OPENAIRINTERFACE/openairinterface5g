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

/*! \file PHY/LTE_TRANSPORT/dlsch_scrambling.c
* \brief Routines for the scrambling procedure of the PDSCH physical channel from 36-211, V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/

//#define DEBUG_SCRAMBLING 1

#include "PHY/defs_eNB.h"
#include "PHY/defs_UE.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "transport_eNB.h"
#include "PHY/phy_extern.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

void dlsch_scrambling(LTE_DL_FRAME_PARMS *frame_parms,
                      int mbsfn_flag,
                      LTE_eNB_DLSCH_t *dlsch,
                      int harq_pid,
                      int G,
                      uint8_t q,
                      uint16_t frame,
                      uint8_t Ns) {
  int n;
  //  uint8_t reset;
  uint32_t x1, x2, s=0;
  uint8_t *dlsch_e=dlsch->harq_processes[harq_pid]->eDL;
  uint8_t *e=dlsch_e;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_SCRAMBLING, VCD_FUNCTION_IN);
  // Rule for accumulation of subframes for BL/CE UEs
  uint8_t Nacc=4;
  uint16_t j0,j,idelta;
  uint16_t i  = (Ns>>1) + (10*frame);
#ifdef PHY_TX_THREAD
  uint16_t i0 = dlsch->harq_processes[harq_pid]->i0;
#else
  uint16_t i0 = dlsch->i0;
#endif
#ifdef PHY_TX_THREAD

  if (dlsch->harq_processes[harq_pid]->sib1_br_flag==1)                              Nacc=1;

#else

  if (dlsch->sib1_br_flag==1)                              Nacc=1;

#endif
  else if (dlsch->rnti == 0xFFFF || dlsch->rnti == 0xFFFE) Nacc = (frame_parms->frame_type == TDD) ? 10 : 4;

#ifdef PHY_TX_THREAD
  // Note: above SC-RNTI will also have to be added when/if implemented
  else if (dlsch->harq_processes[harq_pid]->CEmode == CEmodeA)                       Nacc=1;
  else if (dlsch->harq_processes[harq_pid]->CEmode == CEmodeB)                       Nacc = (frame_parms->frame_type == TDD) ? 10 : 4;

#else
  // Note: above SC-RNTI will also have to be added when/if implemented
  else if (dlsch->CEmode == CEmodeA)                       Nacc=1;
  else if (dlsch->CEmode == CEmodeB)                       Nacc = (frame_parms->frame_type == TDD) ? 10 : 4;

#endif

  if (frame_parms->frame_type == FDD || Nacc == 1) idelta = 0;
  else                                             idelta = Nacc-2;

  j0 = (i0+idelta)/Nacc;
  j  = (i - i0)/Nacc;

  //  reset = 1;
  // x1 is set in lte_gold_generic
  if (mbsfn_flag == 0) {
#ifdef PHY_TX_THREAD

    if (dlsch->harq_processes[harq_pid]->i0 != 0xFFFF) {
#else

    if (dlsch->i0 != 0xFFFF) {
#endif
      // rule for BL/CE UEs from Section 6.3.1 in 36.211
      x2=  (dlsch->rnti<<14) + (q<<13) + ((((j0+j)*Nacc)%10)<<9) + frame_parms->Nid_cell;

      if ((frame&1023) < 200) LOG_D(PHY,"Scrambling init for (i0 %d, i %d, j0 %d, j %d, Nacc %d) => x2 %d\n",i0,i,j0,j,Nacc,x2);
    } else
      x2 = (dlsch->rnti<<14) + (q<<13) + ((Ns>>1)<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.3.1 for PDSCH
  } else {
    x2 = ((Ns>>1)<<9) + frame_parms->Nid_cell_mbsfn; //this is c_init in 36.211 Sec 6.3.1 for PMCH
  }

  s = lte_gold_generic(&x1, &x2, 1);

  for (n=0; n<(1+(G>>5)); n++) {
#ifdef DEBUG_SCRAMBLING

    for (int k=0; k<32; k++) printf("scrambling %d : %x xor %x = %d\n",k+(n<<5),e[k],(s>>k)&1,e[k]^((s>>k)&1));

#endif
    e[0] = (e[0]) ^ (s&1);
    e[1] = (e[1]) ^ ((s>>1)&1);
    e[2] = (e[2]) ^ ((s>>2)&1);
    e[3] = (e[3]) ^ ((s>>3)&1);
    e[4] = (e[4]) ^ ((s>>4)&1);
    e[5] = (e[5]) ^ ((s>>5)&1);
    e[6] = (e[6]) ^ ((s>>6)&1);
    e[7] = (e[7]) ^ ((s>>7)&1);
    e[8] = (e[8]) ^ ((s>>8)&1);
    e[9] = (e[9]) ^ ((s>>9)&1);
    e[10] = (e[10]) ^ ((s>>10)&1);
    e[11] = (e[11]) ^ ((s>>11)&1);
    e[12] = (e[12]) ^ ((s>>12)&1);
    e[13] = (e[13]) ^ ((s>>13)&1);
    e[14] = (e[14]) ^ ((s>>14)&1);
    e[15] = (e[15]) ^ ((s>>15)&1);
    e[16] = (e[16]) ^ ((s>>16)&1);
    e[17] = (e[17]) ^ ((s>>17)&1);
    e[18] = (e[18]) ^ ((s>>18)&1);
    e[19] = (e[19]) ^ ((s>>19)&1);
    e[20] = (e[20]) ^ ((s>>20)&1);
    e[21] = (e[21]) ^ ((s>>21)&1);
    e[22] = (e[22]) ^ ((s>>22)&1);
    e[23] = (e[23]) ^ ((s>>23)&1);
    e[24] = (e[24]) ^ ((s>>24)&1);
    e[25] = (e[25]) ^ ((s>>25)&1);
    e[26] = (e[26]) ^ ((s>>26)&1);
    e[27] = (e[27]) ^ ((s>>27)&1);
    e[28] = (e[28]) ^ ((s>>28)&1);
    e[29] = (e[29]) ^ ((s>>29)&1);
    e[30] = (e[30]) ^ ((s>>30)&1);
    e[31] = (e[31]) ^ ((s>>31)&1);
    // This is not faster for some unknown reason
    //    ((__m128i *)e)[0] = _mm_xor_si128(((__m128i *)e)[0],((__m128i *)scrambling_lut)[s&65535]);
    //    ((__m128i *)e)[1] = _mm_xor_si128(((__m128i *)e)[1],((__m128i *)scrambling_lut)[s>>16]);
    s = lte_gold_generic(&x1, &x2, 0);
    e += 32;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_SCRAMBLING, VCD_FUNCTION_OUT);
}



void init_scrambling_lut(void) {
  uint32_t s;
  int i=0,j;

  for (s=0; s<=65535; s++) {
    for (j=0; j<16; j++) {
      scrambling_lut[i++] = (uint8_t)((s>>j)&1);
    }
  }
}

void dlsch_unscrambling(LTE_DL_FRAME_PARMS *frame_parms,
                        int mbsfn_flag,
                        LTE_UE_DLSCH_t *dlsch,
                        int G,
                        int16_t *llr,
                        uint8_t q,
                        uint8_t Ns) {
  int i,j,k=0;
  //  uint8_t reset;
  uint32_t x1, x2, s=0;

  //  reset = 1;
  // x1 is set in first call to lte_gold_generic

  if (mbsfn_flag == 0)
    x2 = (dlsch->rnti<<14) + (q<<13) + ((Ns>>1)<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.3.1
  else
    x2 = ((Ns>>1)<<9) + frame_parms->Nid_cell_mbsfn; //this is c_init in 36.211 Sec 6.3.1

#ifdef DEBUG_SCRAMBLING
  printf("unscrambling: rnti %x, q %d, Ns %d, Nid_cell %d G %d, x2 %x\n",dlsch->rnti,q,Ns,frame_parms->Nid_cell,G,x2);
#endif
  s = lte_gold_generic(&x1, &x2, 1);

  for (i=0; i<(1+(G>>5)); i++) {
    for (j=0; j<32; j++,k++) {
#ifdef DEBUG_SCRAMBLING
      printf("unscrambling %d : %d xor %d =",k,llr[k],(s>>j)&1);
#endif
      llr[k] = ((2*((s>>j)&1))-1)*llr[k];
#ifdef DEBUG_SCRAMBLING
      printf("%d\n",llr[k]);
#endif
    }

    s = lte_gold_generic(&x1, &x2, 0);
  }
}

void init_unscrambling_lut(void) {
  uint32_t s;
  int i=0,j;

  for (s=0; s<=65535; s++) {
    for (j=0; j<16; j++) {
      unscrambling_lut[i++] = (int16_t)((((s>>j)&1)<<1)-1);
    }
  }
}
