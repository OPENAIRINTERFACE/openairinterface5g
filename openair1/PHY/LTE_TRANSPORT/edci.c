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

/*! \file PHY/LTE_TRANSPORT/dci.c
* \brief Implements PDCCH physical channel TX/RX procedures (36.211) and DCI encoding/decoding (36.212/36.213). Current LTE compliance V8.6 2009-03.
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SIMULATION/TOOLS/defs.h" // for taus 
#include "PHY/sse_intrin.h"

#include "assertions.h" 
#include "T.h"
#include "UTIL/LOG/log.h"

//#define DEBUG_DCI_ENCODING 1
//#define DEBUG_DCI_DECODING 1
//#define DEBUG_PHY

#ifdef Rel14
void generate_edci_top(PHY_VARS_eNB *eNB, int frame, int subframe) {

}

void mpdcch_scrambling(LTE_DL_FRAME_PARMS *frame_parms,
		       mDCI_ALLOC_t *mdci,
		       uint16_t i,
		       uint8_t *e,
		       uint32_t length)
{
  int n;
  uint8_t reset;
  uint32_t x1, x2, s=0;
  uint8_t Nacc=4;
  uint16_t j0,j,idelta;
  uint16_t i0 = mdci->i0;

  // Note: we could actually not do anything if i-i0 < Nacc, save it for later

  reset = 1;
  // x1 is set in lte_gold_generic

  if ((mdci->rnti == 0xFFFE) || 
      (mdci->ce_mode == 2)) // CEModeB Note: also for mdci->rnti==SC_RNTI
    Nacc=frame_parms->frame_type == FDD ? 4 : 10;
  else Nacc=1;
  
  if (frame_parms->frame_type == FDD || Nacc == 1) idelta = 0;
  else                                             idelta = Nacc-2;
  
  j0 = (i0+idelta)/Nacc;
  j  = (i - i0)/Nacc; 

  
  // rule for BL/CE UEs from Section 6.8.B2 in 36.211
  x2=  ((((j0+j)*Nacc)%10)<<9) +  mdci->dmrs_scrambling_init;
  
  for (n=0; n<length; n++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }
    e[i] = (e[i]&1) ^ ((s>>(i&0x1f))&1);
  }
}

// this table is the allocation of modulated MPDCCH format 5 symbols to REs
// There are in total 36 REs/ECCE * 4 ECCE/PRB_pair = 144 REs in total/PRB_pair, total is 168 REs => 24 REs for DMRS
// For format 5 there are 6 PRB pairs => 864 REs for 24 total ECCE
static uint16_t mpdcch5tab[864];

void init_mpdcch5tab_normal_regular_subframe_evenNRBDL(PHY_VARS_eNB *eNB) {
  int l,k,kmod,re;

  LOG_I(PHY,"Inititalizing mpdcch5tab for normal prefix, normal prefix, no PSS/SSS/PBCH, even N_RB_DL\n");
  for (l=0,re=0;l<14;l++) {
    for (k=0;k<72;k++){
      kmod = k % 12; 
      if (((l!=5) && (l!=6) && (l!=12) && (l!=13)) ||
	  (((l==5)||(l==6)||(l==12)||(l==13))&&(kmod!=0)&&(kmod!=5)&&(kmod!=10)))
	mpdcch5tab[re++]=(l*eNB->frame_parms.ofdm_symbol_size)+k;
    }
  }
  AssertFatal(re==864,"RE count not equal to 864\n");
}

extern uint8_t *generate_dci0(uint8_t *dci,
			      uint8_t *e,
			      uint8_t DCI_LENGTH,
			      uint8_t aggregation_level,
			      uint16_t rnti);

void generate_mdci_top(PHY_VARS_eNB *eNB, int frame, int subframe,int16_t amp,int32_t **txdataF) {

  LTE_eNB_MPDCCH *mpdcch= &eNB->mpdcch_vars[subframe&2];
  mDCI_ALLOC_t *mdci;
  int coded_bits;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int i;
  int gain_lin_QPSK;

  for (i=0;i<mpdcch->num_dci;i++) {
    mdci = &mpdcch->mdci_alloc[i];


    AssertFatal(fp->frame_type==FDD,"TDD is not yet supported for MPDCCH\n");
    AssertFatal(fp->Ncp == NORMAL,"Extended Prefix not yet supported for MPDCCH\n");
    AssertFatal(mdci->L<=24,"L is %d\n",mdci->L);
    AssertFatal(fp->N_RB_DL==50 || fp->N_RB_DL==100,"Only N_RB_DL=50,100 for MPDCCH\n");
    // Force MPDDCH format 5
    AssertFatal(mdci->number_of_prb_pairs==6,"2 or 4 PRB pairs not support yet for MPDCCH\n");
    AssertFatal(mdci->reps>0,"mdci->reps==0\n");

    // 9 REs/EREG * 4 EREG/ECCE => 36 REs/ECCE => 72 bits/ECCE, so same as regular PDCCH channel encoding

    // Note: We only have to run this every Nacc subframes during repetitions, data and scrambling are constant, but we do it for now to simplify during testing

    generate_dci0(mdci->dci_pdu,
		  mpdcch->e+(72*mdci->firstCCE),
		  mdci->dci_length,
		  mdci->L,
		  mdci->rnti);

    
    coded_bits = 72 * mdci->L;

    // scrambling
    uint16_t absSF = (frame*10)+subframe; 

    AssertFatal(absSF < 1024,
		"Absolute subframe %d = %d*10 + %d > 1023\n",
		absSF,frame,subframe);

    mpdcch_scrambling(fp,
		      mdci,
		      absSF,
		      mpdcch->e+(72*mdci->firstCCE),
		      coded_bits);

    // Modulation for PDCCH
    if (fp->nb_antenna_ports_eNB==1)
      gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);
    else
      gain_lin_QPSK = amp/2;

    uint8_t *e_ptr = mpdcch->e;

    //    if (mdci->transmission_type==0) nprime=mdci->rnti&3; // for Localized 2+4 we use 6.8B.5 rule
    // map directly to one antenna port for now
    // Note: aside from the antenna port mapping, there is no difference between localized and distributed transmission for MPDCCH format 5

    // first RE of narrowband
    // mpdcchtab5 below contains the mapping from each coded symbol to relative RE avoiding the DMRS


    int re_offset = fp->first_carrier_offset + 1 + ((fp->N_RB_DL==100)?1:0) + mdci->narrowband*12*6;
    if (re_offset>fp->ofdm_symbol_size) re_offset-=(fp->ofdm_symbol_size-1);
    int32_t *txF = &txdataF[0][re_offset];
    int32_t yIQ;

    for (i=0; i<(coded_bits>>1); i++) {
      // QPSK modulation to yIQ
      ((int16_t*)&yIQ)[0] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK; e_ptr++;
      ((int16_t*)&yIQ)[1] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK; e_ptr++;
      txF[mpdcch5tab[i+(36*mdci->firstCCE)]] = yIQ;
    }

  }
} 

#endif
