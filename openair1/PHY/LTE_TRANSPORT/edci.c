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


/*! \file PHY/LTE_TRANSPORT/edci.c
 * \brief Implements M/EPDCCH physical channel TX/RX procedures (36.211).
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
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "SCHED/sched_eNB.h"
#include "SIMULATION/TOOLS/sim.h"      // for taus
#include "PHY/sse_intrin.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

#include "assertions.h"
#include "T.h"
#include "common/utils/LOG/log.h"

//#define DEBUG_DCI_ENCODING 1
//#define DEBUG_DCI_DECODING 1
//#define DEBUG_PHY

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
void generate_edci_top(PHY_VARS_eNB *eNB, int frame, int subframe) {

}

void mpdcch_scrambling(LTE_DL_FRAME_PARMS * frame_parms, mDCI_ALLOC_t * mdci, uint16_t i, uint8_t * e, uint32_t length)
{
  int             n;
  uint8_t         reset;
  uint32_t        x1, x2, s = 0;
  uint8_t         Nacc = 4;
  uint16_t        j0, j, idelta;
  uint16_t        i0 = mdci->i0;

  // Note: we could actually not do anything if i-i0 < Nacc, save it for later

  reset = 1;
  // x1 is set in lte_gold_generic

  if ((mdci->rnti == 0xFFFE) || (mdci->ce_mode == 2)) // CEModeB Note: also for mdci->rnti==SC_RNTI
    Nacc = frame_parms->frame_type == FDD ? 4 : 10;
  else
    Nacc = 1;

  if (frame_parms->frame_type == FDD || Nacc == 1)
    idelta = 0;
  else
    idelta = Nacc - 2;

  j0 = (i0 + idelta) / Nacc;
  j = (i - i0) / Nacc;


  // rule for BL/CE UEs from Section 6.8.B2 in 36.211
  x2 = ((((j0 + j) * Nacc) % 10) << 9) + mdci->dmrs_scrambling_init;
  LOG_D(PHY,"MPDCCH cinit = %x (mdci->dmrs_scrambling_init = %d), scrambling %d encoded DCI bits\n",
	x2,mdci->dmrs_scrambling_init,length);
  for (n = 0; n < length; n++) {
    if ((n & 0x1f) == 0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    e[n] = (e[n] & 1) ^ ((s >> (n & 0x1f)) & 1);

  }
}

// this table is the allocation of modulated MPDCCH format 5 symbols to REs, antenna ports 107,108
// start symbol is symbol 1 and L'=24 => all 6 PRBs in the set
// 9 symbols without DMRS = 9*12*6 REs = 648 REs
// 4 symbols with DMRS (3 REs stolen per symbol = 4*9*6 REs = 216 REs
// Total = 648+216 = 864 REs = 1728 bits
static uint16_t mpdcch5ss1tab[864];

void init_mpdcch5ss1tab_normal_regular_subframe_evenNRBDL(PHY_VARS_eNB * eNB)
{
  int             l, k, kmod, re=0;

  LOG_D(PHY, "Inititalizing mpdcchss15tab for normal prefix, normal prefix, no PSS/SSS/PBCH, even N_RB_DL\n");

  for (l = 1; l < 14; l++) {
    for (k = 0; k < 72; k++) {
      kmod = k % 12;
      if (((l != 5) && (l != 6) && (l != 12) && (l != 13)) || (kmod == 2) || (kmod == 3) || (kmod == 4) || (kmod == 7) || (kmod == 8) || (kmod == 9)) {
        mpdcch5ss1tab[re] = (l * eNB->frame_parms.ofdm_symbol_size) + k;
        re++;
      } else if ((kmod == 0) || (kmod == 5) || (kmod == 10)) {
	    mpdcch5ss1tab[re++] = (l * eNB->frame_parms.ofdm_symbol_size) + k;
      }
    }
  }
  AssertFatal(re == 864, "RE count not equal to 864 (%d)\n", re);
}

// this table is the allocation of modulated MPDCCH format 5 symbols to REs, antenna ports 107,108
// start symbol is symbol 2 and L'=24 => all 6 PRBs in the set
// 8 symbols without DMRS = 8*12*6 REs = 576 REs
// 4 symbols with DMRS (3 REs stolen per symbol = 4*9*6 REs = 216 REs
// Total = 576+216 = 792 REs = 1584 bits
static uint16_t mpdcch5ss2tab[792];

void init_mpdcch5ss2tab_normal_regular_subframe_evenNRBDL(PHY_VARS_eNB * eNB)
{
  int             l, k, kmod, re=0;
  int nushift = eNB->frame_parms.Nid_cell % 6;
  int nushiftp3 = (eNB->frame_parms.Nid_cell+3) % 6;
  // NOTE : THIS IS FOR TM1 ONLY FOR NOW!!!!!!!
  LOG_D(PHY, "Inititalizing mpdcch5ss2tab for normal prefix, normal prefix, no PSS/SSS/PBCH, even N_RB_DL\n");
  for (l = 2; l < 14; l++) {
    for (k = 0; k < 72; k++) {
      kmod = k % 12;
      if ((((l == 4)||(l==11)) && (kmod != nushiftp3) && (kmod != (nushiftp3+6))) || 
	  ((l == 7) && (kmod != nushift) &&(kmod != (nushift+6)))) {  // CS RS
	mpdcch5ss2tab[re] = (l * eNB->frame_parms.ofdm_symbol_size) + k;
	re++;
      }
      if (((l!=4)&&(l!=7)&&(l!=11)) && 
	  (((l != 5) && (l != 6) && (l != 12) && (l != 13)) || (kmod == 2) || (kmod == 3) || (kmod == 4) || (kmod == 7) || (kmod == 8) || (kmod == 9))) {
	mpdcch5ss2tab[re] = (l * eNB->frame_parms.ofdm_symbol_size) + k;
	re++;
      }
    }
  }
  AssertFatal(re == 684, "RE count not equal to 684\n");
}

// this table is the allocation of modulated MPDCCH format 5 symbols to REs, antenna ports 107,108
// start symbol is symbol 3 and L'=24 => all 6 PRBs in the set
// 7 symbols without DMRS = 7*12*6 REs = 504 REs
// 4 symbols with DMRS (3 REs stolen per symbol = 4*9*6 REs = 216 REs
// Total = 504+216 = 720 REs = 1440 bits
static uint16_t mpdcch5ss3tab[720];
void init_mpdcch5ss3tab_normal_regular_subframe_evenNRBDL(PHY_VARS_eNB * eNB)
{
  int             l, k, kmod, re=0;

  LOG_D(PHY, "Inititalizing mpdcch5ss3tab for normal prefix, normal prefix, no PSS/SSS/PBCH, even N_RB_DL\n");
  for (l = 3; l < 14; l++) {
    for (k = 0; k < 72; k++) {
      kmod = k % 12;
      if (((l != 5) && (l != 6) && (l != 12) && (l != 13)) || (kmod == 2) || (kmod == 3) || (kmod == 4) || (kmod == 7) || (kmod == 8) || (kmod == 9)) {
	mpdcch5ss3tab[re] = (l * eNB->frame_parms.ofdm_symbol_size) + k;
	re++;
      } else if ((kmod == 0) || (kmod == 5) || (kmod == 10)) {
	mpdcch5ss3tab[re++] = (l * eNB->frame_parms.ofdm_symbol_size) + k;
      }
    }
  }
  AssertFatal(re == 720, "RE count not equal to 792\n");
}

// this table is the allocation of modulated MPDCCH format 3 symbols to REs, antenna ports 107,108
// with start symbol 1, using L'=16 => first 4 PRBs in the set
// 8 symbols without DMRS = 9*12*4 REs = 432 REs
// 4 symbols with DMRS (3 REs stolen per symbol = 4*9*4 REs = 144 REs
// Total = 432+144 = 576 = 16CCE*36RE/CCE
static uint16_t mpdcch3ss1tab[576];

void init_mpdcch3ss1tab_normal_regular_subframe_evenNRBDL(PHY_VARS_eNB * eNB)
{
  int             l, k, kmod, re=0;

  LOG_D(PHY, "Inititalizing mpdcch3ss1tab for normal prefix, normal prefix, no PSS/SSS/PBCH, even N_RB_DL\n");
  for (l = 1, re = 0; l < 14; l++) {
    for (k = 0; k < 48; k++) {
      kmod = k % 12;
      if (((l != 5) && (l != 6) && (l != 12) && (l != 13)) || (((l == 5) || (l == 6) || (l == 12) || (l == 13)) && (kmod != 0) && (kmod != 5) && (kmod != 10))) {
	mpdcch3ss1tab[re] = (l * eNB->frame_parms.ofdm_symbol_size) + k;
	re++;
      }
    }
  }
  AssertFatal(re == 576, "RE count not equal to 864\n");
}

// this table is the allocation of modulated MPDCCH format 2 symbols to REs, antenna ports 107,108
// with start symbol 1, using L'=8 => last 2 PRBs in the set
// 8 symbols without DMRS = 9*12*2 REs = 216 REs
// 4 symbols with DMRS (3 REs stolen per symbol = 4*9*2 REs = 72 REs
// Total = 216+72 = 288 = 8CCE*36RE/CCE
static uint16_t mpdcch2ss1tab[288];

void init_mpdcch2ss1tab_normal_regular_subframe_evenNRBDL(PHY_VARS_eNB * eNB)
{
  int             l, k, kmod, re=0;

  LOG_D(PHY, "Inititalizing mpdcch2ss1tab for normal prefix, normal prefix, no PSS/SSS/PBCH, even N_RB_DL\n");
  for (l = 1, re = 0; l < 14; l++) {
    for (k = 0; k < 24; k++) {
      kmod = k % 12;
      if (((l != 5) && (l != 6) && (l != 12) && (l != 13)) || (((l == 5) || (l == 6) || (l == 12) || (l == 13)) && (kmod != 0) && (kmod != 5) && (kmod != 10))) {
	mpdcch2ss1tab[re] = (l * eNB->frame_parms.ofdm_symbol_size) + k;
	re++;
      }
    }
  }
  AssertFatal(re == 288, "RE count not equal to 288\n");
}




extern uint8_t *generate_dci0(uint8_t * dci, uint8_t * e, uint8_t DCI_LENGTH, uint16_t coded_bits, uint16_t rnti);


uint16_t        mpdcch_dmrs_tab[12 * 6];

void init_mpdcch_dmrs_tab(uint16_t oss)
{

  int             re = 5 * oss;
  int             pos = 0;

  for (int symb = 0; symb < 4; symb++) {
    for (int prb = 0; prb < 6; prb++, re += 12) {
      mpdcch_dmrs_tab[pos++] = re;
      mpdcch_dmrs_tab[pos++] = re + 5;
      mpdcch_dmrs_tab[pos++] = re + 10;
    }
    if (symb == 0)
      re = 6 * oss;
    else if (symb == 1)
      re = 12 * oss;
    else if (symb == 2)
      re = 13 * oss;
  }

}

void generate_mdci_top(PHY_VARS_eNB * eNB, int frame, int subframe, int16_t amp, int32_t ** txdataF)
{

  LTE_eNB_MPDCCH *mpdcch = &eNB->mpdcch_vars[subframe & 1];
  mDCI_ALLOC_t   *mdci;
  int             coded_bits;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  int             i;
  int             gain_lin_QPSK;
  uint16_t       *mpdcchtab;

  uint32_t        x1, x2, s = 0;
  uint8_t         Nacc = 4;
  uint16_t        j0, j, idelta;
  uint16_t        i0;

  int             off=0;

  // Assumption: only handle a single MPDCCH per narrowband

  int nsymb = (fp->Ncp==0) ? 14:12;

  int symbol_offset = (uint32_t)fp->ofdm_symbol_size*(subframe*nsymb);

  int wp[2][4] = {{1,1,1,1},{1,-1,1,-1}};
  int *w;

  LOG_D(PHY, "generate_mdci_top: num_dci %d\n", mpdcch->num_dci);

  for (i = 0; i < mpdcch->num_dci; i++) {
    mdci = &mpdcch->mdci_alloc[i];

    AssertFatal(fp->frame_type == FDD, "TDD is not yet supported for MPDCCH\n");
    AssertFatal(fp->Ncp == NORMAL, "Extended Prefix not yet supported for MPDCCH\n");
    AssertFatal(mdci->L <= 24, "L is %d\n", mdci->L);
    AssertFatal(fp->N_RB_DL == 50 || fp->N_RB_DL == 100, "Only N_RB_DL=50,100 for MPDCCH\n");
    // Force MPDDCH format 5
    AssertFatal(mdci->number_of_prb_pairs == 6, "2 or 4 PRB pairs not support yet for MPDCCH\n");

    // These are to avoid unimplemented things
    AssertFatal(mdci->ce_mode == 1, "CE mode (%d) B not activated yet\n", mdci->ce_mode);
    AssertFatal(mdci->L == 24, "Only 2+4 and aggregation 24 for now\n");
    int     a_index=mdci->rnti & 3;

    i0 = mdci->i0;
    // antenna index

    if (mdci->start_symbol == 1)  {
      mpdcchtab = mpdcch5ss1tab;
      coded_bits = 756*2;
    } else if (mdci->start_symbol == 2) {
      mpdcchtab = mpdcch5ss2tab;
      coded_bits=684*2;
    } else if (mdci->start_symbol == 3) {
      mpdcchtab = mpdcch5ss3tab;
      coded_bits = 612*2;
    } else
      AssertFatal(1 == 0, "Illegal combination start_symbol %d, a_index %d\n", mdci->start_symbol, a_index);

    LOG_D(PHY, "mdci %d, length %d: rnti %x, L %d, prb_pairs %d, ce_mode %d, transmission type %s, i0 %d, ss %d ,coded_bits %d\n", 
	  i, mdci->dci_length,mdci->rnti, 
	  mdci->L, mdci->number_of_prb_pairs, 
	  mdci->ce_mode, 
	  mdci->transmission_type == 1? "dist" : "loc",
	  mdci->i0, mdci->start_symbol,
	  coded_bits);

    // Note: We only have to run this every Nacc subframes during repetitions, data and scrambling are constant, but we do it for now to simplify during testing

    generate_dci0(mdci->dci_pdu, mpdcch->e, mdci->dci_length, coded_bits, mdci->rnti);



    // scrambling
    uint16_t        absSF = (frame * 10) + subframe;

    AssertFatal(absSF < 10240, "Absolute subframe %d = %d*10 + %d > 10239\n", absSF, frame, subframe);

    mpdcch_scrambling(fp, mdci, absSF, mpdcch->e, coded_bits);
    
    // Modulation for PDCCH
    if (fp->nb_antenna_ports_eNB == 1)
      gain_lin_QPSK = (int16_t) ((amp * ONE_OVER_SQRT2_Q15) >> 15);
    else
      gain_lin_QPSK = amp / 2;

    uint8_t        *e_ptr = mpdcch->e;

    //    if (mdci->transmission_type==0) nprime=mdci->rnti&3; // for Localized 2+4 we use 6.8B.5 rule
    // map directly to one antenna port for now
    // Note: aside from the antenna port mapping, there is no difference between localized and distributed transmission for MPDCCH format 5

    // first RE of narrowband
    // mpdcchtab5 below contains the mapping from each coded symbol to relative RE avoiding the DMRS

    int             nb_i0;
    switch (fp->N_RB_DL) {

    case 6:
    case 25:
      nb_i0 = 0;
      break;
    case 15:
    case 50:
    case 75:
      nb_i0 = 1;
      break;
    case 100:
      nb_i0 = 2;
      break;
    default:
      AssertFatal(1 == 0, "Illegal N_RB_DL %d\n", fp->N_RB_DL);
      break;
    }

    int             re_offset = fp->first_carrier_offset + (12 * nb_i0) + (mdci->narrowband * 12 * 6);
    if (re_offset > fp->ofdm_symbol_size)
      re_offset -= (fp->ofdm_symbol_size - 1);
    int32_t        *txF = &txdataF[0][symbol_offset+re_offset];
    int32_t         yIQ;

    for (i = 0; i < (coded_bits >> 1); i++) {
      // QPSK modulation to yIQ
      ((int16_t *) & yIQ)[0] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
      ((int16_t *) & yIQ)[1] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
      txF[mpdcchtab[i]] = yIQ;
      /*      
      LOG_I(PHY,"Frame %d, subframe %d: mpdcch pos %d (%d,%d) => (%d,%d)\n",
	    frame,subframe,i,mpdcchtab[i]+re_offset,mpdcchtab[i]/fp->ofdm_symbol_size,
	    ((int16_t *) & yIQ)[0],((int16_t *) & yIQ)[1]);*/
    }

    if (mdci->transmission_type == 1) w=0; // distributed
    else w = wp[a_index&1];
    // pilot scrambling initiatlization (note: this is for a single repetition)

    // x1 is set in lte_gold_generic

    // rule for BL/CE UEs from Section 6.10.3A.1 in 36.211

    if ((mdci->rnti == 0xFFFE) || (mdci->ce_mode == 2))     // CEModeB Note: also for mdci->rnti==SC_RNTI
      Nacc = fp->frame_type == FDD ? 4 : 10;
    else
      Nacc = 1;

    if (fp->frame_type == FDD || Nacc == 1)
      idelta = 0;
    else
      idelta = Nacc - 2;

    j0 = (i0 + idelta) / Nacc;
    j = (absSF - i0) / Nacc;



    uint32_t        a = ((((j0 + j) * Nacc) % 10) + 1);
    uint32_t        b = ((mdci->dmrs_scrambling_init << 1) + 1) << 16;
    x2 = a * b;
    x2 = x2 + 2;
    LOG_D(PHY, "mpdcch_dmrs cinit %x (a=%d,b=%d,i0=%d,j0=%d)\n", x2,a,b,i0,j0);

    // add MPDCCH pilots
    int             reset = 1;

    int first_prb = (mdci->narrowband*6) + nb_i0;
    int last_prb  = (mdci->narrowband*6) + nb_i0 + 5;
    int soffset[4] = {5,6,12,13};


    for (int lprime=0,i=0;lprime<4;lprime++) {
      for (int nprb=0;nprb<110;nprb++) {
	if (nprb<fp->N_RB_DL) {
	  re_offset = fp->first_carrier_offset + (12 * nprb);
	  if (re_offset > fp->ofdm_symbol_size)
	    re_offset -= (fp->ofdm_symbol_size - 1);
	  txF = &txdataF[0][symbol_offset + re_offset + fp->ofdm_symbol_size*soffset[lprime]];
	}
	for (int mprime=0;mprime<3;mprime++,i+=2) {
	
	  if ((i & 0x1f) == 0) {
	    s = lte_gold_generic(&x1, &x2, reset);
	    reset = 0;
	  }

	  // select PRBs corresponding to narrowband
	  if ((nprb>= first_prb) && 
	      (nprb<= last_prb)) {
	    ((int16_t *) & yIQ)[0] = (((s >> (i & 0x1f)) & 1) == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
	    ((int16_t *) & yIQ)[1] = (((s >> ((i + 1) & 0x1f)) & 1) == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
	    AssertFatal(mdci->transmission_type==1,"transmission_type %d!=1, handle this ...\n",mdci->transmission_type);
	    if (mdci->transmission_type==1) { // same thing on both 107 and 109
	      txF[(5*mprime)] = yIQ;
	      txF[1+(5*mprime)] = yIQ;
	    }
	    else { // put on selected antenna port with w sequence

	      if (((mprime+nprb)&1) == 0) 
		txF[off+(5*mprime)] = yIQ*w[lprime];
	      else
		txF[off+(5*mprime)] = yIQ*w[3-lprime];
	    }
	    /*
	    LOG_I(PHY, "mpdcch_dmrs pos (dist %d, l %d,nprb %d,mprime %d) %d  => (%d,%d)\n", 
		  mdci->transmission_type, soffset[lprime],nprb,mprime,
		  re_offset + fp->ofdm_symbol_size*soffset[lprime]+(5*mprime), 
		  ((int16_t *) & yIQ)[0], ((int16_t *) & yIQ)[1]);*/
	  } // narrowband condition
	} // RE (m') loop
      } // nprb loop
    } // symbol (l') loop
  } // num_dci loop
}

void init_mpdcch(PHY_VARS_eNB * eNB)
{

  init_mpdcch5ss1tab_normal_regular_subframe_evenNRBDL(eNB);
  init_mpdcch5ss2tab_normal_regular_subframe_evenNRBDL(eNB);
  init_mpdcch5ss3tab_normal_regular_subframe_evenNRBDL(eNB);
  init_mpdcch3ss1tab_normal_regular_subframe_evenNRBDL(eNB);
  init_mpdcch2ss1tab_normal_regular_subframe_evenNRBDL(eNB);
  init_mpdcch_dmrs_tab(eNB->frame_parms.ofdm_symbol_size);
}

#endif
