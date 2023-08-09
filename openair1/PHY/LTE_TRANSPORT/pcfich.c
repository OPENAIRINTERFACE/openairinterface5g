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

/*! \file PHY/LTE_TRANSPORT/pcfich.c
* \brief Top-level routines for generating and decoding  the PCFICH/CFI physical/transport channel V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include "PHY/impl_defs_top.h"
#include "PHY/defs_eNB.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "openair1/PHY/LTE_TRANSPORT/transport_vars.h"

//#define DEBUG_PCFICH
void pcfich_scrambling(LTE_DL_FRAME_PARMS *frame_parms, uint8_t subframe, const uint8_t *b, uint8_t *bt)
{
  uint32_t i;
  uint8_t reset;
  uint32_t x1=0, x2, s=0;

  reset = 1;
  // x1 is set in lte_gold_generic
  x2 = ((((2*frame_parms->Nid_cell)+1)*(1+subframe))<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.7.1

  for (i=0; i<32; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    bt[i] = (b[i]&1) ^ ((s>>(i&0x1f))&1);
  }
}



void generate_pcfich(uint8_t num_pdcch_symbols,
                     int16_t amp,
                     LTE_DL_FRAME_PARMS *frame_parms,
                     int32_t **txdataF,
                     uint8_t subframe)
{

  uint8_t pcfich_bt[32],nsymb,pcfich_quad;
  int32_t pcfich_d[2][16];
  uint8_t i;
  uint32_t symbol_offset,m,re_offset,reg_offset;
  int16_t gain_lin_QPSK;
  uint16_t *pcfich_reg = frame_parms->pcfich_reg;

  int nushiftmod3 = frame_parms->nushift%3;
#ifdef DEBUG_PCFICH
  LOG_D(PHY,"Generating PCFICH in subfrmae %d for %d PDCCH symbols, AMP %d, p %d, Ncp %d\n",
	subframe,num_pdcch_symbols,amp,frame_parms->nb_antenna_ports_eNB,frame_parms->Ncp);
#endif

  memset(pcfich_bt, 0, sizeof(pcfich_bt));

  // scrambling
  if ((num_pdcch_symbols>0) && (num_pdcch_symbols<4))
    pcfich_scrambling(frame_parms,subframe,pcfich_b[num_pdcch_symbols-1],pcfich_bt);

  // modulation
  if (frame_parms->nb_antenna_ports_eNB==1)
    gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);
  else
    gain_lin_QPSK = amp/2;

  if (frame_parms->nb_antenna_ports_eNB==1) { // SISO

    for (i=0; i<16; i++) {
      ((int16_t*)(&(pcfich_d[0][i])))[0]   = ((pcfich_bt[2*i] == 1) ? -gain_lin_QPSK : gain_lin_QPSK);
      ((int16_t*)(&(pcfich_d[1][i])))[0]   = ((pcfich_bt[2*i] == 1) ? -gain_lin_QPSK : gain_lin_QPSK);
      ((int16_t*)(&(pcfich_d[0][i])))[1]   = ((pcfich_bt[2*i+1] == 1) ? -gain_lin_QPSK : gain_lin_QPSK);
      ((int16_t*)(&(pcfich_d[1][i])))[1]   = ((pcfich_bt[2*i+1] == 1) ? -gain_lin_QPSK : gain_lin_QPSK);
    }
  } else { // ALAMOUTI
    for (i=0; i<16; i+=2) {
      // first antenna position n -> x0
      ((int16_t*)(&(pcfich_d[0][i])))[0]   = ((pcfich_bt[2*i] == 1) ? -gain_lin_QPSK : gain_lin_QPSK);
      ((int16_t*)(&(pcfich_d[0][i])))[1]   = ((pcfich_bt[2*i+1] == 1) ? -gain_lin_QPSK : gain_lin_QPSK);
      // second antenna position n -> -x1*
      ((int16_t*)(&(pcfich_d[1][i])))[0]   = ((pcfich_bt[2*i+2] == 1) ? gain_lin_QPSK : -gain_lin_QPSK);
      ((int16_t*)(&(pcfich_d[1][i])))[1]   = ((pcfich_bt[2*i+3] == 1) ? -gain_lin_QPSK : gain_lin_QPSK);
      // fill in the rest of the ALAMOUTI precoding
      ((int16_t*)&pcfich_d[0][i+1])[0] = -((int16_t*)&pcfich_d[1][i])[0];
      ((int16_t*)&pcfich_d[0][i+1])[1] =  ((int16_t*)&pcfich_d[1][i])[1];
      ((int16_t*)&pcfich_d[1][i+1])[0] =  ((int16_t*)&pcfich_d[0][i])[0];
      ((int16_t*)&pcfich_d[1][i+1])[1] = -((int16_t*)&pcfich_d[0][i])[1];


    }
  }


  // mapping
  nsymb = (frame_parms->Ncp==0) ? 14:12;

  symbol_offset = (uint32_t)frame_parms->ofdm_symbol_size*(subframe*nsymb);
  re_offset = frame_parms->first_carrier_offset;

  // loop over 4 quadruplets and lookup REGs
  m=0;

  for (pcfich_quad=0; pcfich_quad<4; pcfich_quad++) {
    reg_offset = re_offset+((uint16_t)pcfich_reg[pcfich_quad]*6);

    if (reg_offset>=frame_parms->ofdm_symbol_size)
      reg_offset=1 + reg_offset-frame_parms->ofdm_symbol_size;

    for (i=0; i<6; i++) {
      if ((i!=nushiftmod3)&&(i!=(nushiftmod3+3))) {
        txdataF[0][symbol_offset+reg_offset+i] = pcfich_d[0][m];

        if (frame_parms->nb_antenna_ports_eNB>1)
          txdataF[1][symbol_offset+reg_offset+i] = pcfich_d[1][m];

        m++;
      }
    }
  }

}


