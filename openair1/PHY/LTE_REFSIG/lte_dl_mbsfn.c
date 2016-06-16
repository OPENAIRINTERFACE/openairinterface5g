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
// 6.10.2.2 MBSFN reference signals Mapping to resource elements

#ifdef USER_MODE
#include <stdio.h>
#include <stdlib.h>
#endif

#include "defs.h"
#include "PHY/defs.h"

//extern unsigned int lte_gold_table[10][3][42];
//#define DEBUG_DL_MBSFN

int lte_dl_mbsfn(PHY_VARS_eNB *phy_vars_eNB, int32_t *output,
                 short amp,
                 int subframe,
                 unsigned char l)
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


  mprime = 3*(110 - phy_vars_eNB->lte_frame_parms.N_RB_DL);

  for (m=0; m<phy_vars_eNB->lte_frame_parms.N_RB_DL*6; m++) {

    if ((l==0) || (l==2))
      k = m<<1;
    else if (l==1)
      k = 1+(m<<1);
    else {
      msg("lte_dl_mbsfn: l %d -> ERROR\n",l);
      return(-1);
    }

    k+=phy_vars_eNB->lte_frame_parms.first_carrier_offset;

    mprime_dword     = mprime>>4;
    mprime_qpsk_symb = mprime&0xf;

    if (k >= phy_vars_eNB->lte_frame_parms.ofdm_symbol_size) {
      k++;  // skip DC carrier
      k-=phy_vars_eNB->lte_frame_parms.ofdm_symbol_size;
    }

    output[k] = qpsk[(phy_vars_eNB->lte_gold_mbsfn_table[subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3];
    //output[k] = (lte_gold_table[eNB_offset][subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3;


#ifdef DEBUG_DL_MBSFN
    msg("subframe %d, l %d, m %d, mprime %d, mprime_dword %d, mprime_qpsk_symbol %d\n",
        subframe,l,m,mprime,mprime_dword,mprime_qpsk_symb);
    msg("index = %d (k %d)(%x)\n",(phy_vars_eNB->lte_gold_mbsfn_table[subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3,k,phy_vars_eNB->lte_gold_mbsfn_table[subframe][l][mprime_dword]);
#endif
    mprime++;

#ifdef DEBUG_DL_MBSFN

    if (m<18)
      printf("subframe %d, l %d output[%d] = (%d,%d)\n",subframe,l,k,((short *)&output[k])[0],((short *)&output[k])[1]);

#endif

  }

  return(0);
}



int lte_dl_mbsfn_rx(PHY_VARS_UE *phy_vars_ue,
                    int *output,
                    int subframe,
                    unsigned char l)
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

  mprime = 3*(110 - phy_vars_ue->lte_frame_parms.N_RB_DL);

  for (m=0; m<phy_vars_ue->lte_frame_parms.N_RB_DL*6; m++) {

    mprime_dword     = mprime>>4;
    mprime_qpsk_symb = mprime&0xf;

    // this is r_mprime from 3GPP 36-211 6.10.1.2
    output[k] = qpsk[(phy_vars_ue->lte_gold_mbsfn_table[subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3];

#ifdef DEBUG_DL_MBSFN
    printf("subframe %d, l %d, m %d, mprime %d, mprime_dword %d, mprime_qpsk_symbol %d\n",
           subframe,l,m,mprime, mprime_dword,mprime_qpsk_symb);
    printf("index = %d (k %d) (%x)\n",(phy_vars_ue->lte_gold_mbsfn_table[subframe][l][mprime_dword]>>(2*mprime_qpsk_symb))&3,k,phy_vars_ue->lte_gold_mbsfn_table[subframe][l][mprime_dword]);
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

