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

// 6.10.2.2 MBSFN reference signals Mapping to resource elements

#include <stdio.h>
#include <stdlib.h>

#include "lte_refsig.h"
#include "PHY/defs_eNB.h"
#include "PHY/defs_UE.h"
#include "PHY/impl_defs_top.h"

//extern unsigned int lte_gold_table[10][3][42];
//#define DEBUG_DL_MBSFN

int lte_dl_mbsfn(PHY_VARS_eNB *eNB, int32_t *output,
                 short amp,
                 int subframe,
                 unsigned char l) {
  unsigned int mprime,mprime_dword,mprime_qpsk_symb,m;
  unsigned short k=0,a;
  int32_t qpsk[4];
  a = (amp*ONE_OVER_SQRT2_Q15)>>15;
  ((short *)&qpsk[0])[0] = a;
  ((short *)&qpsk[0])[1] = a;
  ((short *)&qpsk[1])[0] = -a;
  ((short *)&qpsk[1])[1] = a;
  ((short *)&qpsk[2])[0] = a;
  ((short *)&qpsk[2])[1] = -a;
  ((short *)&qpsk[3])[0] = -a;
  ((short *)&qpsk[3])[1] = -a;
  mprime = 3*(110 - eNB->frame_parms.N_RB_DL);

  for (m=0; m<eNB->frame_parms.N_RB_DL*6; m++) {
    if ((l==0) || (l==2))
      k = m<<1;
    else if (l==1)
      k = 1+(m<<1);
    else {
      LOG_E(PHY,"lte_dl_mbsfn: l %d -> ERROR\n",l);
      return(-1);
    }

    k+=eNB->frame_parms.first_carrier_offset;
    mprime_dword     = mprime>>4;
    mprime_qpsk_symb = mprime&0xf;

    if (k >= eNB->frame_parms.ofdm_symbol_size) {
      k++;  // skip DC carrier
      k-=eNB->frame_parms.ofdm_symbol_size;
    }

    output[k] = qpsk[(eNB->lte_gold_mbsfn_table[subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3];
    //output[k] = (lte_gold_table[eNB_offset][subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3;
#ifdef DEBUG_DL_MBSFN
    LOG_D(PHY,"subframe %d, l %d, m %d, mprime %d, mprime_dword %d, mprime_qpsk_symbol %d\n",
          subframe,l,m,mprime,mprime_dword,mprime_qpsk_symb);
    LOG_D(PHY,"index = %d (k %d)(%x)\n",(eNB->lte_gold_mbsfn_table[subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3,k,eNB->lte_gold_mbsfn_table[subframe][l][mprime_dword]);
#endif
    mprime++;
#ifdef DEBUG_DL_MBSFN

    if (m<18)
      printf("subframe %d, l %d output[%d] = (%d,%d)\n",subframe,l,k,((short *)&output[k])[0],((short *)&output[k])[1]);

#endif
  }

  return(0);
}



int lte_dl_mbsfn_rx(PHY_VARS_UE *ue,
                    int *output,
                    int subframe,
                    unsigned char l) {
  unsigned int mprime,mprime_dword,mprime_qpsk_symb,m;
  unsigned short k=0;
  unsigned int qpsk[4];
  // This includes complex conjugate for channel estimation
  ((short *)&qpsk[0])[0] = ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[0])[1] = -ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[1])[0] = -ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[1])[1] = -ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[2])[0] = ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[2])[1] = ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[3])[0] = -ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[3])[1] = ONE_OVER_SQRT2_Q15;
  mprime = 3*(110 - ue->frame_parms.N_RB_DL);

  for (m=0; m<ue->frame_parms.N_RB_DL*6; m++) {
    mprime_dword     = mprime>>4;
    mprime_qpsk_symb = mprime&0xf;
    // this is r_mprime from 3GPP 36-211 6.10.1.2
    output[k] = qpsk[(ue->lte_gold_mbsfn_table[subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3];
#ifdef DEBUG_DL_MBSFN
    printf("subframe %d, l %d, m %u, mprime %u, mprime_dword %u, mprime_qpsk_symbol %u\n",
           subframe,l,m,mprime, mprime_dword,mprime_qpsk_symb);
    printf("index = %d (k %d) (%x)\n",(ue->lte_gold_mbsfn_table[subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3,k,ue->lte_gold_mbsfn_table[subframe][l][mprime_dword]);
#endif
    mprime++;
#ifdef DEBUG_DL_MBSFN

    if (m<18)
      printf("subframe %d l %d output[%d] = (%d,%d)\n",subframe,l,k,((short *)&output[k])[0],((short *)&output[k])[1]);

#endif
    k++;
  }

  return(0);
}

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
int lte_dl_mbsfn_khz_1dot25(PHY_VARS_eNB *eNB, int32_t *output,
                 short amp,
                 int subframe) 
{

  unsigned int mprime,mprime_dword,mprime_qpsk_symb,m;
  unsigned short k=0,a;
  int32_t qpsk[4];

  a = (amp*ONE_OVER_SQRT2_Q15)>>15;
  ((short *)&qpsk[0])[0] = a;
  ((short *)&qpsk[0])[1] = a;

  ((short *)&qpsk[1])[0] = -a;
  ((short *)&qpsk[1])[1] = a;
  ((short *)&qpsk[2])[0] = a;
  ((short *)&qpsk[2])[1] = -a;

  ((short *)&qpsk[3])[0] = -a;
  ((short *)&qpsk[3])[1] = -a;


  mprime = 3*(110 - eNB->frame_parms.N_RB_DL);

  for (m=0; m<eNB->frame_parms.N_RB_DL*24; m++) // m = 0:24*N_RB_DL-1
  {
      if ((subframe&0x1)==0) // n_sf mod 2 == 0: even 
        k = 6*m;
      else
        k = 6*m + 3;


      k+=eNB->frame_parms.first_carrier_offset_khz_1dot25;

      mprime_dword     = mprime>>4;
      mprime_qpsk_symb = mprime&0xf;

      if (k >= eNB->frame_parms.ofdm_symbol_size_khz_1dot25) {
        k++;  // skip DC carrier
        k-=eNB->frame_parms.ofdm_symbol_size_khz_1dot25;
      }
      output[k] = qpsk[(eNB->lte_gold_mbsfn_khz_1dot25_table[subframe][mprime_dword]>>(2*mprime_qpsk_symb))&3];
      mprime++;
   }

  return(0);
}


int lte_dl_mbsfn_khz_1dot25_rx(PHY_VARS_UE *ue,
                    int *output,
                    int subframe) 
{

  unsigned int mprime,mprime_dword,mprime_qpsk_symb,m;
  unsigned short k=0;
  unsigned int qpsk[4];

  // This includes complex conjugate for channel estimation

  ((short *)&qpsk[0])[0] = ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[0])[1] = -ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[1])[0] = -ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[1])[1] = -ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[2])[0] = ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[2])[1] = ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[3])[0] = -ONE_OVER_SQRT2_Q15;
  ((short *)&qpsk[3])[1] = ONE_OVER_SQRT2_Q15;

  mprime = 3*(110 - ue->frame_parms.N_RB_DL);

  for (m=0; m<ue->frame_parms.N_RB_DL*24; m++) // m = 0:24*N_RB_DL-1
  { mprime_dword     = mprime>>4;
    mprime_qpsk_symb = mprime&0xf;

    // this is r_mprime from 3GPP 36-211 6.10.1.2
    output[k] = qpsk[(ue->lte_gold_mbsfn_khz_1dot25_table[subframe][mprime_dword]>>(2*mprime_qpsk_symb))&3];

    mprime++;
    k++;

  }

  return(0);
}

#endif // MAKE Rel14

