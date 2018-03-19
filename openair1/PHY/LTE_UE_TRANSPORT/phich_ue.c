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

/*! \file PHY/LTE_TRANSPORT/phich.c
* \brief Top-level routines for generating and decoding  the PHICH/HI physical/transport channel V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "defs.h"

#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/defs.h"

#include "T.h"

//#define DEBUG_PHICH 1

//extern unsigned short pcfich_reg[4];
//extern unsigned char pcfich_first_reg_idx;

//unsigned short phich_reg[MAX_NUM_PHICH_GROUPS][3];


uint8_t rv_table[4] = {0, 2, 3, 1};

uint8_t get_mi(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe)
{

  // for FDD
  if (frame_parms->frame_type == FDD)
    return 1;

  // for TDD
  switch (frame_parms->tdd_config) {

  case 0:
    if ((subframe==0) || (subframe==5))
      return(2);
    else return(1);

    break;

  case 1:
    if ((subframe==0) || (subframe==5))
      return(0);
    else return(1);

    break;

  case 2:
    if ((subframe==3) || (subframe==8))
      return(1);
    else return(0);

    break;

  case 3:
    if ((subframe==0) || (subframe==8) || (subframe==9))
      return(1);
    else return(0);

    break;

  case 4:
    if ((subframe==8) || (subframe==9))
      return(1);
    else return(0);

    break;

  case 5:
    if (subframe==8)
      return(1);
    else return(0);

    break;

  case 6:
    return(1);
    break;

  default:
    return(0);
  }
}

unsigned char subframe2_ul_harq(LTE_DL_FRAME_PARMS *frame_parms,unsigned char subframe)
{

  if (frame_parms->frame_type == FDD)
    return(subframe&7);

  switch (frame_parms->tdd_config) {
  case 3:
    if ( (subframe == 8) || (subframe == 9) ) {
      return(subframe-8);
    } else if (subframe==0)
      return(2);
    else {
      LOG_E(PHY,"phich.c: subframe2_ul_harq, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      return(0);
    }

    break;

  case 4:
     if ( (subframe == 8) || (subframe == 9) ) {
       return(subframe-8);
     } else {
       LOG_E(PHY,"phich.c: subframe2_ul_harq, illegal subframe %d for tdd_config %d\n",
             subframe,frame_parms->tdd_config);
       return(0);
     }

     break;

  }

  return(0);
}

int phich_frame2_pusch_frame(LTE_DL_FRAME_PARMS *frame_parms, int frame, int subframe)
{
  int pusch_frame;

  if (frame_parms->frame_type == FDD) {
    pusch_frame = subframe<4 ? frame + 1024 - 1 : frame;
  } else {
    // Note this is not true, but it doesn't matter, the frame number is irrelevant for TDD!
    pusch_frame = (frame);
  }

  //LOG_D(PHY, "frame %d subframe %d: PUSCH frame = %d\n", frame, subframe, pusch_frame);
  return pusch_frame % 1024;
}

uint8_t phich_subframe2_pusch_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe)
{
  uint8_t pusch_subframe = 255;

  if (frame_parms->frame_type == FDD)
    return subframe < 4 ? subframe + 6 : subframe - 4;

  switch (frame_parms->tdd_config) {
  case 0:
    if (subframe == 0)
      pusch_subframe = (3);
    else if (subframe == 5) {
      pusch_subframe = (8);
    } else if (subframe == 6)
      pusch_subframe = (2);
    else if (subframe == 1)
      pusch_subframe = (7);
    else {
      AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
		  subframe,frame_parms->tdd_config);
      pusch_subframe = (0);
    }

    break;

  case 1:
    if (subframe == 6)
      pusch_subframe = (2);
    else if (subframe == 9)
      pusch_subframe = (3);
    else if (subframe == 1)
      pusch_subframe = (7);
    else if (subframe == 4)
      pusch_subframe = (8);
    else {
      AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      pusch_subframe = (0);
    }

    break;

  case 2:
    if (subframe == 8)
      pusch_subframe = (2);
    else if (subframe == 3)
      pusch_subframe = (7);
    else {
      AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      pusch_subframe = (0);
    }

    break;

  case 3:
    if ( (subframe == 8) || (subframe == 9) ) {
      pusch_subframe = (subframe-6);
    } else if (subframe==0)
      pusch_subframe = (4);
    else {
      AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      pusch_subframe = (0);
    }

    break;

  case 4:
    if ( (subframe == 8) || (subframe == 9) ) {
      pusch_subframe = (subframe-6);
    } else {
      AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      pusch_subframe = (0);
    }

    break;

  case 5:
    if (subframe == 8) {
      pusch_subframe = (2);
    } else {
      AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      pusch_subframe = (0);
    }

    break;

  case 6:
    if (subframe == 6) {
      pusch_subframe = (2);
    } else if (subframe == 9) {
      pusch_subframe = (3);
    } else if (subframe == 0) {
      pusch_subframe = (4);
    } else if (subframe == 1) {
      pusch_subframe = (7);
    } else if (subframe == 5) {
      pusch_subframe = (8);
    } else {
      AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      pusch_subframe = (0);
    }

    break;

  default:
    AssertFatal(1==0, "no implementation for TDD UL/DL-config = %d!\n", frame_parms->tdd_config);
    pusch_subframe = (0);
  }

  LOG_D(PHY, "subframe  %d: PUSCH subframe = %d\n", subframe, pusch_subframe);
  return pusch_subframe;
}

int check_pcfich(LTE_DL_FRAME_PARMS *frame_parms,uint16_t reg)
{

  if ((reg == frame_parms->pcfich_reg[0]) ||
      (reg == frame_parms->pcfich_reg[1]) ||
      (reg == frame_parms->pcfich_reg[2]) ||
      (reg == frame_parms->pcfich_reg[3]))
    return(1);

  return(0);
}

void generate_phich_reg_mapping(LTE_DL_FRAME_PARMS *frame_parms)
{

  unsigned short n0 = (frame_parms->N_RB_DL * 2) - 4;  // 2 REG per RB less the 4 used by PCFICH in first symbol
  unsigned short n1 = (frame_parms->N_RB_DL * 3);      // 3 REG per RB in second and third symbol
  unsigned short n2 = n1;
  unsigned short mprime = 0;
  unsigned short Ngroup_PHICH;
  //  uint16_t *phich_reg = frame_parms->phich_reg;
  uint16_t *pcfich_reg = frame_parms->pcfich_reg;

  // compute Ngroup_PHICH (see formula at beginning of Section 6.9 in 36-211
  Ngroup_PHICH = (frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)/48;


  if (((frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)%48) > 0)
    Ngroup_PHICH++;

  // check if Extended prefix
  if (frame_parms->Ncp == 1) {
    Ngroup_PHICH<<=1;
  }

#ifdef DEBUG_PHICH
  LOG_D(PHY,"Ngroup_PHICH %d (phich_config_common.phich_resource %d,phich_config_common.phich_duration %s, NidCell %d,Ncp %d, frame_type %d), smallest pcfich REG %d, n0 %d, n1 %d (first PHICH REG %d)\n",
    ((frame_parms->Ncp == NORMAL)?Ngroup_PHICH:(Ngroup_PHICH>>1)),
    frame_parms->phich_config_common.phich_resource,
    frame_parms->phich_config_common.phich_duration==normal?"normal":"extended",
    frame_parms->Nid_cell,frame_parms->Ncp,frame_parms->frame_type,
    pcfich_reg[frame_parms->pcfich_first_reg_idx],
    n0,
    n1,
    ((frame_parms->Nid_cell))%n0);
#endif

  // This is the algorithm from Section 6.9.3 in 36-211, it works only for normal PHICH duration for now ...

  for (mprime=0; 
       mprime<((frame_parms->Ncp == NORMAL)?Ngroup_PHICH:(Ngroup_PHICH>>1)); 
       mprime++) {

    if (frame_parms->phich_config_common.phich_duration==normal) { // normal PHICH duration

      frame_parms->phich_reg[mprime][0] = (frame_parms->Nid_cell + mprime)%n0;

      if (frame_parms->phich_reg[mprime][0]>=pcfich_reg[frame_parms->pcfich_first_reg_idx])
        frame_parms->phich_reg[mprime][0]++;

      if (frame_parms->phich_reg[mprime][0]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+1)&3])
        frame_parms->phich_reg[mprime][0]++;

      if (frame_parms->phich_reg[mprime][0]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+2)&3])
        frame_parms->phich_reg[mprime][0]++;

      if (frame_parms->phich_reg[mprime][0]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+3)&3])
        frame_parms->phich_reg[mprime][0]++;

      frame_parms->phich_reg[mprime][1] = (frame_parms->Nid_cell + mprime + (n0/3))%n0;

      
      if (frame_parms->phich_reg[mprime][1]>=pcfich_reg[frame_parms->pcfich_first_reg_idx])
        frame_parms->phich_reg[mprime][1]++;
      
      if (frame_parms->phich_reg[mprime][1]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+1)&3])
        frame_parms->phich_reg[mprime][1]++;

      if (frame_parms->phich_reg[mprime][1]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+2)&3])
        frame_parms->phich_reg[mprime][1]++;

      if (frame_parms->phich_reg[mprime][1]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+3)&3])
        frame_parms->phich_reg[mprime][1]++;
      

      frame_parms->phich_reg[mprime][2] = (frame_parms->Nid_cell + mprime + (2*n0/3))%n0;
      
      if (frame_parms->phich_reg[mprime][2]>=pcfich_reg[frame_parms->pcfich_first_reg_idx])
        frame_parms->phich_reg[mprime][2]++;

      if (frame_parms->phich_reg[mprime][2]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+1)&3])
        frame_parms->phich_reg[mprime][2]++;

      if (frame_parms->phich_reg[mprime][2]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+2)&3])
        frame_parms->phich_reg[mprime][2]++;

      if (frame_parms->phich_reg[mprime][2]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+3)&3])
        frame_parms->phich_reg[mprime][2]++;
      
#ifdef DEBUG_PHICH
      printf("phich_reg :%d => %d,%d,%d\n",mprime,frame_parms->phich_reg[mprime][0],frame_parms->phich_reg[mprime][1],frame_parms->phich_reg[mprime][2]);
#endif
    } else { // extended PHICH duration
      frame_parms->phich_reg[mprime<<1][0] = (frame_parms->Nid_cell + mprime)%n0;
      frame_parms->phich_reg[1+(mprime<<1)][0] = (frame_parms->Nid_cell + mprime)%n0;

      frame_parms->phich_reg[mprime<<1][1] = ((frame_parms->Nid_cell*n1/n0) + mprime + (n1/3))%n1;
      frame_parms->phich_reg[mprime<<1][2] = ((frame_parms->Nid_cell*n2/n0) + mprime + (2*n2/3))%n2;

      frame_parms->phich_reg[1+(mprime<<1)][1] = ((frame_parms->Nid_cell*n1/n0) + mprime + (n1/3))%n1;
      frame_parms->phich_reg[1+(mprime<<1)][2] = ((frame_parms->Nid_cell*n2/n0) + mprime + (2*n2/3))%n2;
      //#ifdef DEBUG_PHICH
      printf("phich_reg :%d => %d,%d,%d\n",mprime<<1,frame_parms->phich_reg[mprime<<1][0],frame_parms->phich_reg[mprime][1],frame_parms->phich_reg[mprime][2]);
      printf("phich_reg :%d => %d,%d,%d\n",1+(mprime<<1),frame_parms->phich_reg[1+(mprime<<1)][0],frame_parms->phich_reg[1+(mprime<<1)][1],frame_parms->phich_reg[1+(mprime<<1)][2]);
      //#endif
    }
  } // mprime loop
}  // num_pdcch_symbols loop


void generate_phich_emul(LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t HI,
                         uint8_t subframe)
{


}

int32_t alam_bpsk_perm1[4] = {2,1,4,3}; // -conj(x) 1 (-1-j) -> 2 (1-j), 2->1, 3 (-1+j) -> (4) 1+j, 4->3
int32_t alam_bpsk_perm2[4] = {3,4,2,1}; // conj(x) 1 (-1-j) -> 3 (-1+j), 3->1, 2 (1-j) -> 4 (1+j), 4->2

// This routine generates the PHICH

void generate_phich(LTE_DL_FRAME_PARMS *frame_parms,
                    int16_t amp,
                    uint8_t nseq_PHICH,
                    uint8_t ngroup_PHICH,
                    uint8_t HI,
                    uint8_t subframe,
                    int32_t **y)
{

  int16_t d[24],*dp;
  //  unsigned int i,aa;
  unsigned int re_offset;
  int16_t y0_16[8],y1_16[8];
  int16_t *y0,*y1;
  // scrambling
  uint32_t x1, x2, s=0;
  uint8_t reset = 1;
  int16_t cs[12];
  uint32_t i,i2,i3,m,j;
  int16_t gain_lin_QPSK;
  uint32_t subframe_offset=((frame_parms->Ncp==0)?14:12)*frame_parms->ofdm_symbol_size*subframe;

  memset(d,0,24*sizeof(int16_t));

  if (frame_parms->nb_antenna_ports_eNB==1)
    gain_lin_QPSK = (int16_t)(((int32_t)amp*ONE_OVER_SQRT2_Q15)>>15);
  else
    gain_lin_QPSK = amp/2;

  //printf("PHICH : gain_lin_QPSK %d\n",gain_lin_QPSK);

  // BPSK modulation of HI input (to be repeated 3 times, 36-212 Section 5.3.5, p. 56 in v8.6)
  if (HI>0)
    HI=1;


  //  c = (1-(2*HI))*SSS_AMP;
  // x1 is set in lte_gold_generic
  x2 = (((subframe+1)*((frame_parms->Nid_cell<<1)+1))<<9) + frame_parms->Nid_cell;

  s = lte_gold_generic(&x1, &x2, reset);

  // compute scrambling sequence
  for (i=0; i<12; i++) {
    cs[i] = (uint8_t)((s>>(i&0x1f))&1);
    cs[i] = (cs[i] == 0) ? (1-(HI<<1)) : ((HI<<1)-1);
  }

  if (frame_parms->Ncp == 0) { // Normal Cyclic Prefix

    //    printf("Doing PHICH : Normal CP, subframe %d\n",subframe);
    // 12 output symbols (Msymb)
    for (i=0,i2=0,i3=0; i<3; i++,i2+=4,i3+=8) {
      switch (nseq_PHICH) {
      case 0: // +1 +1 +1 +1
        d[i3]   = cs[i2];
        d[1+i3] = cs[i2];
        d[2+i3] = cs[1+i2];
        d[3+i3] = cs[1+i2];
        d[4+i3] = cs[2+i2];
        d[5+i3] = cs[2+i2];
        d[6+i3] = cs[3+i2];
        d[7+i3] = cs[3+i2];
        break;

      case 1: // +1 -1 +1 -1
        d[i3] = cs[i2];
        d[1+i3] = cs[i2];
        d[2+i3] = -cs[1+i2];
        d[3+i3] = -cs[1+i2];
        d[4+i3] = cs[2+i2];
        d[5+i3] = cs[2+i2];
        d[6+i3] = -cs[3+i2];
        d[7+i3] = -cs[3+i2];
        break;

      case 2: // +1 +1 -1 -1
        d[i3]   = cs[i2];
        d[1+i3]   = cs[i2];
        d[2+i3] = cs[1+i2];
        d[3+i3] = cs[1+i2];
        d[4+i3] = -cs[2+i2];
        d[5+i3] = -cs[2+i2];
        d[6+i3] = -cs[3+i2];
        d[7+i3] = -cs[3+i2];
        break;

      case 3: // +1 -1 -1 +1
        d[i3]   = cs[i2];
        d[1+i3]   = cs[i2];
        d[2+i3] = -cs[1+i2];
        d[3+i3] = -cs[1+i2];
        d[4+i3] = -cs[2+i2];
        d[5+i3] = -cs[2+i2];
        d[6+i3] = cs[3+i2];
        d[7+i3] = cs[3+i2];
        break;

      case 4: // +j +j +j +j
        d[i3]   = -cs[i2];
        d[1+i3] = cs[i2];
        d[2+i3] = -cs[1+i2];
        d[3+i3] = cs[1+i2];
        d[4+i3] = -cs[2+i2];
        d[5+i3] = cs[2+i2];
        d[6+i3] = -cs[3+i2];
        d[7+i3] = cs[3+i2];
        break;

      case 5: // +j -j +j -j
        d[1+i3] = cs[i2];
        d[3+i3] = -cs[1+i2];
        d[5+i3] = cs[2+i2];
        d[7+i3] = -cs[3+i2];
        d[i3]   = -cs[i2];
        d[2+i3] = cs[1+i2];
        d[4+i3] = -cs[2+i2];
        d[6+i3] = cs[3+i2];
        break;

      case 6: // +j +j -j -j
        d[1+i3] = cs[i2];
        d[3+i3] = cs[1+i2];
        d[5+i3] = -cs[2+i2];
        d[7+i3] = -cs[3+i2];
        d[i3]   = -cs[i2];
        d[2+i3] = -cs[1+i2];
        d[4+i3] = cs[2+i2];
        d[6+i3] = cs[3+i2];
        break;

      case 7: // +j -j -j +j
        d[1+i3] = cs[i2];
        d[3+i3] = -cs[1+i2];
        d[5+i3] = -cs[2+i2];
        d[7+i3] = cs[3+i2];
        d[i3]   = -cs[i2];
        d[2+i3] = cs[1+i2];
        d[4+i3] = cs[2+i2];
        d[6+i3] = -cs[3+i2];
        break;

      default:
        AssertFatal(1==0,"phich_coding.c: Illegal PHICH Number\n");
      } // nseq_PHICH
    }

#ifdef DEBUG_PHICH
    LOG_D(PHY,"[PUSCH 0]PHICH d = ");

    for (i=0; i<24; i+=2)
      LOG_D(PHY,"(%d,%d)",d[i],d[i+1]);

    LOG_D(PHY,"\n");
#endif

    // modulation here
    if (frame_parms->nb_antenna_ports_eNB != 1) {
      // do Alamouti precoding here

      // Symbol 0
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][0]*6);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];
      y1 = (int16_t*)&y[1][re_offset+subframe_offset];

      // first antenna position n -> x0
      y0_16[0]   = d[0]*gain_lin_QPSK;
      y0_16[1]   = d[1]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[0]   = -d[2]*gain_lin_QPSK;
      y1_16[1]   = d[3]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[2] = -y1_16[0];
      y0_16[3] = y1_16[1];
      y1_16[2] = y0_16[0];
      y1_16[3] = -y0_16[1];

      // first antenna position n -> x0
      y0_16[4]   = d[4]*gain_lin_QPSK;
      y0_16[5]   = d[5]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[4]   = -d[6]*gain_lin_QPSK;
      y1_16[5]   = d[7]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[6] = -y1_16[4];
      y0_16[7] = y1_16[5];
      y1_16[6] = y0_16[4];
      y1_16[7] = -y0_16[5];

      for (i=0,j=0,m=0; i<6; i++,j+=2) {
        if ((i!=(frame_parms->nushift))&&(i!=(frame_parms->nushift+3))) {
          y0[j]   += y0_16[m];
          y1[j]   += y1_16[m++];
          y0[j+1] += y0_16[m];
          y1[j+1] += y1_16[m++];
        }
      }

      // Symbol 1
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][1]*6);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];
      y1 = (int16_t*)&y[1][re_offset+subframe_offset];

      // first antenna position n -> x0
      y0_16[0]   = d[8]*gain_lin_QPSK;
      y0_16[1]   = d[9]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[0]   = -d[10]*gain_lin_QPSK;
      y1_16[1]   = d[11]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[2] = -y1_16[0];
      y0_16[3] = y1_16[1];
      y1_16[2] = y0_16[0];
      y1_16[3] = -y0_16[1];

      // first antenna position n -> x0
      y0_16[4]   = d[12]*gain_lin_QPSK;
      y0_16[5]   = d[13]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[4]   = -d[14]*gain_lin_QPSK;
      y1_16[5]   = d[15]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[6] = -y1_16[4];
      y0_16[7] = y1_16[5];
      y1_16[6] = y0_16[4];
      y1_16[7] = -y0_16[5];

      for (i=0,j=0,m=0; i<6; i++,j+=2) {
        if ((i!=(frame_parms->nushift))&&(i!=(frame_parms->nushift+3))) {
          y0[j]   += y0_16[m];
          y1[j]   += y1_16[m++];
          y0[j+1] += y0_16[m];
          y1[j+1] += y1_16[m++];
        }
      }

      // Symbol 2
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][2]*6);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];
      y1 = (int16_t*)&y[1][re_offset+subframe_offset];

      // first antenna position n -> x0
      y0_16[0]   = d[16]*gain_lin_QPSK;
      y0_16[1]   = d[17]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[0]   = -d[18]*gain_lin_QPSK;
      y1_16[1]   = d[19]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[2] = -y1_16[0];
      y0_16[3] = y1_16[1];
      y1_16[2] = y0_16[0];
      y1_16[3] = -y0_16[1];

      // first antenna position n -> x0
      y0_16[4]   = d[20]*gain_lin_QPSK;
      y0_16[5]   = d[21]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[4]   = -d[22]*gain_lin_QPSK;
      y1_16[5]   = d[23]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[6] = -y1_16[4];
      y0_16[7] = y1_16[5];
      y1_16[6] = y0_16[4];
      y1_16[7] = -y0_16[5];

      for (i=0,j=0,m=0; i<6; i++,j+=2) {
        if ((i!=(frame_parms->nushift))&&(i!=(frame_parms->nushift+3))) {
          y0[j]   += y0_16[m];
          y1[j]   += y1_16[m++];
          y0[j+1] += y0_16[m];
          y1[j+1] += y1_16[m++];
        }
      }

    } // nb_antenna_ports_eNB

    else {
      // Symbol 0
      //      printf("[PUSCH 0]PHICH REG %d\n",frame_parms->phich_reg[ngroup_PHICH][0]);
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][0]*6);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];
      //      printf("y0 %p\n",y0);

      y0_16[0]   = d[0]*gain_lin_QPSK;
      y0_16[1]   = d[1]*gain_lin_QPSK;
      y0_16[2]   = d[2]*gain_lin_QPSK;
      y0_16[3]   = d[3]*gain_lin_QPSK;
      y0_16[4]   = d[4]*gain_lin_QPSK;
      y0_16[5]   = d[5]*gain_lin_QPSK;
      y0_16[6]   = d[6]*gain_lin_QPSK;
      y0_16[7]   = d[7]*gain_lin_QPSK;

      for (i=0,j=0,m=0; i<6; i++,j+=2) {
        if ((i!=(frame_parms->nushift))&&(i!=(frame_parms->nushift+3))) {
          y0[j]   += y0_16[m++];
          y0[j+1] += y0_16[m++];
        }
      }

      // Symbol 1
      //      printf("[PUSCH 0]PHICH REG %d\n",frame_parms->phich_reg[ngroup_PHICH][1]);
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][1]*6);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];

      y0_16[0]   = d[8]*gain_lin_QPSK;
      y0_16[1]   = d[9]*gain_lin_QPSK;
      y0_16[2]   = d[10]*gain_lin_QPSK;
      y0_16[3]   = d[11]*gain_lin_QPSK;
      y0_16[4]   = d[12]*gain_lin_QPSK;
      y0_16[5]   = d[13]*gain_lin_QPSK;
      y0_16[6]   = d[14]*gain_lin_QPSK;
      y0_16[7]   = d[15]*gain_lin_QPSK;

      for (i=0,j=0,m=0; i<6; i++,j+=2) {
        if ((i!=(frame_parms->nushift))&&(i!=(frame_parms->nushift+3))) {
          y0[j]   += y0_16[m++];
          y0[j+1] += y0_16[m++];
        }
      }

      // Symbol 2
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][2]*6);

      //      printf("[PUSCH 0]PHICH REG %d\n",frame_parms->phich_reg[ngroup_PHICH][2]);
      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];

      y0_16[0]   = d[16]*gain_lin_QPSK;
      y0_16[1]   = d[17]*gain_lin_QPSK;
      y0_16[2]   = d[18]*gain_lin_QPSK;
      y0_16[3]   = d[19]*gain_lin_QPSK;
      y0_16[4]   = d[20]*gain_lin_QPSK;
      y0_16[5]   = d[21]*gain_lin_QPSK;
      y0_16[6]   = d[22]*gain_lin_QPSK;
      y0_16[7]   = d[23]*gain_lin_QPSK;

      for (i=0,j=0,m=0; i<6; i++,j+=2) {
        if ((i!=(frame_parms->nushift))&&(i!=(frame_parms->nushift+3))) {
          y0[j]   += y0_16[m++];
          y0[j+1] += y0_16[m++];
        }
      }

      /*
      for (i=0;i<512;i++)
      printf("re %d (%d): %d,%d\n",i,subframe_offset+i,((int16_t*)&y[0][subframe_offset+i])[0],((int16_t*)&y[0][subframe_offset+i])[1]);
      */
    } // nb_antenna_ports_eNB
  } else { // extended prefix

    // 6 output symbols
    if ((ngroup_PHICH & 1) == 1)
      dp = &d[4];
    else
      dp = d;

    switch (nseq_PHICH) {
    case 0: // +1 +1
      dp[0]  = cs[0];
      dp[2]  = cs[1];
      dp[8]  = cs[2];
      dp[10] = cs[3];
      dp[16] = cs[4];
      dp[18] = cs[5];
      dp[1]  = cs[0];
      dp[3]  = cs[1];
      dp[9]  = cs[2];
      dp[11] = cs[3];
      dp[17] = cs[4];
      dp[19] = cs[5];
      break;

    case 1: // +1 -1
      dp[0]  = cs[0];
      dp[2]  = -cs[1];
      dp[8]  = cs[2];
      dp[10] = -cs[3];
      dp[16] = cs[4];
      dp[18] = -cs[5];
      dp[1]  = cs[0];
      dp[3]  = -cs[1];
      dp[9]  = cs[2];
      dp[11] = -cs[3];
      dp[17] = cs[4];
      dp[19] = -cs[5];
      break;

    case 2: // +j +j
      dp[1]  = cs[0];
      dp[3]  = cs[1];
      dp[9]  = cs[2];
      dp[11] = cs[3];
      dp[17] = cs[4];
      dp[19] = cs[5];
      dp[0]  = -cs[0];
      dp[2]  = -cs[1];
      dp[8]  = -cs[2];
      dp[10] = -cs[3];
      dp[16] = -cs[4];
      dp[18] = -cs[5];

      break;

    case 3: // +j -j
      dp[1]  = cs[0];
      dp[3]  = -cs[1];
      dp[9]  = cs[2];
      dp[11] = -cs[3];
      dp[17] = cs[4];
      dp[19] = -cs[5];
      dp[0]  = -cs[0];
      dp[2]  = cs[1];
      dp[8]  = -cs[2];
      dp[10] = cs[3];
      dp[16] = -cs[4];
      dp[18] = cs[5];
      break;

    default:
      AssertFatal(1==0,"phich_coding.c: Illegal PHICH Number\n");
    }



    if (frame_parms->nb_antenna_ports_eNB != 1) {
      // do Alamouti precoding here
      // Symbol 0
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][0]*6);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];
      y1 = (int16_t*)&y[1][re_offset+subframe_offset];

      // first antenna position n -> x0
      y0_16[0]   = d[0]*gain_lin_QPSK;
      y0_16[1]   = d[1]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[0]   = -d[2]*gain_lin_QPSK;
      y1_16[1]   = d[3]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[2] = -y1_16[0];
      y0_16[3] = y1_16[1];
      y1_16[2] = y0_16[0];
      y1_16[3] = -y0_16[1];

      // first antenna position n -> x0
      y0_16[4]   = d[4]*gain_lin_QPSK;
      y0_16[5]   = d[5]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[4]   = -d[6]*gain_lin_QPSK;
      y1_16[5]   = d[7]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[6] = -y1_16[4];
      y0_16[7] = y1_16[5];
      y1_16[6] = y0_16[4];
      y1_16[7] = -y0_16[5];

      for (i=0,j=0,m=0; i<6; i++,j+=2) {
        if ((i!=(frame_parms->nushift))&&(i!=(frame_parms->nushift+3))) {
          y0[j] += y0_16[m];
          y1[j] += y1_16[m++];
          y0[j+1] += y0_16[m];
          y1[j+1] += y1_16[m++];
        }
      }

      // Symbol 1
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][1]<<2);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      re_offset += (frame_parms->ofdm_symbol_size);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];
      y1 = (int16_t*)&y[1][re_offset+subframe_offset];

      // first antenna position n -> x0
      y0_16[0]   = d[8]*gain_lin_QPSK;
      y0_16[1]   = d[9]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[0]   = -d[10]*gain_lin_QPSK;
      y1_16[1]   = d[11]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[2] = -y1_16[0];
      y0_16[3] = y1_16[1];
      y1_16[2] = y0_16[0];
      y1_16[3] = -y0_16[1];

      // first antenna position n -> x0
      y0_16[4]   = d[12]*gain_lin_QPSK;
      y0_16[5]   = d[13]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[4]   = -d[14]*gain_lin_QPSK;
      y1_16[5]   = d[15]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[6] = -y1_16[4];
      y0_16[7] = y1_16[5];
      y1_16[6] = y0_16[4];
      y1_16[7] = -y0_16[5];

      for (i=0,j=0,m=0; i<4; i++,j+=2) {
        y0[j] += y0_16[m];
        y1[j] += y1_16[m++];
        y0[j+1] += y0_16[m];
        y1[j+1] += y1_16[m++];
      }

      // Symbol 2
      re_offset = frame_parms->first_carrier_offset +  (frame_parms->phich_reg[ngroup_PHICH][2]<<2);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      re_offset += (frame_parms->ofdm_symbol_size<<1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];
      y1 = (int16_t*)&y[1][re_offset+subframe_offset];

      // first antenna position n -> x0
      y0_16[0]   = d[16]*gain_lin_QPSK;
      y0_16[1]   = d[17]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[0]   = -d[18]*gain_lin_QPSK;
      y1_16[1]   = d[19]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[2] = -y1_16[0];
      y0_16[3] = y1_16[1];
      y1_16[2] = y0_16[0];
      y1_16[3] = -y0_16[1];

      // first antenna position n -> x0
      y0_16[4]   = d[20]*gain_lin_QPSK;
      y0_16[5]   = d[21]*gain_lin_QPSK;
      // second antenna position n -> -x1*
      y1_16[4]   = -d[22]*gain_lin_QPSK;
      y1_16[5]   = d[23]*gain_lin_QPSK;
      // fill in the rest of the ALAMOUTI precoding
      y0_16[6] = -y1_16[4];
      y0_16[7] = y1_16[5];
      y1_16[6] = y0_16[4];
      y1_16[7] = -y0_16[5];

      for (i=0,j=0,m=0; i<4; i++,j+=2) {
        y0[j]   += y0_16[m];
        y1[j]   += y1_16[m++];
        y0[j+1] += y0_16[m];
        y1[j+1] += y1_16[m++];
      }
    } else {

      // Symbol 0
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][0]*6);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];

      y0_16[0]   = d[0]*gain_lin_QPSK;
      y0_16[1]   = d[1]*gain_lin_QPSK;
      y0_16[2]   = d[2]*gain_lin_QPSK;
      y0_16[3]   = d[3]*gain_lin_QPSK;
      y0_16[4]   = d[4]*gain_lin_QPSK;
      y0_16[5]   = d[5]*gain_lin_QPSK;
      y0_16[6]   = d[6]*gain_lin_QPSK;
      y0_16[7]   = d[7]*gain_lin_QPSK;

      for (i=0,j=0,m=0; i<6; i++,j+=2) {
        if ((i!=(frame_parms->nushift))&&(i!=(frame_parms->nushift+3))) {
          y0[j] += y0_16[m++];
          y0[j+1] += y0_16[m++];
        }
      }

      // Symbol 1
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][1]<<2);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      re_offset += (frame_parms->ofdm_symbol_size);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];

      y0_16[0]   = d[8]*gain_lin_QPSK;
      y0_16[1]   = d[9]*gain_lin_QPSK;
      y0_16[2]   = d[10]*gain_lin_QPSK;
      y0_16[3]   = d[11]*gain_lin_QPSK;
      y0_16[4]   = d[12]*gain_lin_QPSK;
      y0_16[5]   = d[13]*gain_lin_QPSK;
      y0_16[6]   = d[14]*gain_lin_QPSK;
      y0_16[7]   = d[15]*gain_lin_QPSK;

      for (i=0,j=0,m=0; i<4; i++,j+=2) {
        y0[j] += y0_16[m++];
        y0[j+1] += y0_16[m++];
      }


      // Symbol 2
      re_offset = frame_parms->first_carrier_offset + (frame_parms->phich_reg[ngroup_PHICH][2]<<2);

      if (re_offset > frame_parms->ofdm_symbol_size)
        re_offset -= (frame_parms->ofdm_symbol_size-1);

      re_offset += (frame_parms->ofdm_symbol_size<<1);

      y0 = (int16_t*)&y[0][re_offset+subframe_offset];

      y0_16[0]   = d[16]*gain_lin_QPSK;
      y0_16[1]   = d[17]*gain_lin_QPSK;
      y0_16[2]   = d[18]*gain_lin_QPSK;
      y0_16[3]   = d[19]*gain_lin_QPSK;
      y0_16[4]   = d[20]*gain_lin_QPSK;
      y0_16[5]   = d[21]*gain_lin_QPSK;
      y0_16[6]   = d[22]*gain_lin_QPSK;
      y0_16[7]   = d[23]*gain_lin_QPSK;

      for (i=0,j=0,m=0; i<4; i++) {
        y0[j]   += y0_16[m++];
        y0[j+1] += y0_16[m++];
      }

    } // nb_antenna_ports_eNB
  } // normal/extended prefix
}

// This routine demodulates the PHICH and updates PUSCH/ULSCH parameters

void rx_phich(PHY_VARS_UE *ue,
	      UE_rxtx_proc_t *proc,
              uint8_t subframe,
              uint8_t eNB_id)
{


  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  LTE_UE_PDCCH **pdcch_vars = &ue->pdcch_vars[ue->current_thread_id[subframe]][eNB_id];

  //  uint8_t HI;
  uint8_t harq_pid = phich_subframe_to_harq_pid(frame_parms,proc->frame_rx,subframe);
  LTE_UE_ULSCH_t *ulsch = ue->ulsch[eNB_id];
  int16_t phich_d[24],*phich_d_ptr,HI16;
  //  unsigned int i,aa;
  int8_t d[24],*dp;
  uint16_t reg_offset;

  // scrambling
  uint32_t x1, x2, s=0;
  uint8_t reset = 1;
  int16_t cs[12];
  uint32_t i,i2,i3,phich_quad;
  int32_t **rxdataF_comp = pdcch_vars[eNB_id]->rxdataF_comp;
  uint8_t Ngroup_PHICH,ngroup_PHICH,nseq_PHICH;
  uint8_t NSF_PHICH = 4;
  uint8_t pusch_subframe;

  int8_t delta_PUSCH_acc[4] = {-1,0,1,3};

  // check if we're expecting a PHICH in this subframe
  LOG_D(PHY,"[UE  %d][PUSCH %d] Frame %d subframe %d PHICH RX\n",ue->Mod_id,harq_pid,proc->frame_rx,subframe);

  if (!ulsch)
    return;

  LOG_D(PHY,"[UE  %d][PUSCH %d] Frame %d subframe %d PHICH RX Status: %d \n",ue->Mod_id,harq_pid,proc->frame_rx,subframe, ulsch->harq_processes[harq_pid]->status);

  if (ulsch->harq_processes[harq_pid]->status == ACTIVE) {
     LOG_D(PHY,"[UE  %d][PUSCH %d] Frame %d subframe %d PHICH RX ACTIVE\n",ue->Mod_id,harq_pid,proc->frame_rx,subframe);
    Ngroup_PHICH = (frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)/48;

    if (((frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)%48) > 0)
      Ngroup_PHICH++;

    if (frame_parms->Ncp == 1)
      NSF_PHICH = 2;


    ngroup_PHICH = (ulsch->harq_processes[harq_pid]->first_rb +
                    ulsch->harq_processes[harq_pid]->n_DMRS)%Ngroup_PHICH;

    if ((frame_parms->tdd_config == 0) && (frame_parms->frame_type == TDD) ) {
      pusch_subframe = phich_subframe2_pusch_subframe(frame_parms,subframe);

      if ((pusch_subframe == 4) || (pusch_subframe == 9))
        ngroup_PHICH += Ngroup_PHICH;
    }

    nseq_PHICH = ((ulsch->harq_processes[harq_pid]->first_rb/Ngroup_PHICH) +
                  ulsch->harq_processes[harq_pid]->n_DMRS)%(2*NSF_PHICH);
  } else {
    LOG_D(PHY,"[UE  %d][PUSCH %d] Frame %d subframe %d PHICH RX %s\n",
        ue->Mod_id,
        harq_pid,
        proc->frame_rx,
        subframe,
        (ulsch->harq_processes[harq_pid]->status==SCH_IDLE?   "SCH_IDLE"  :
        (ulsch->harq_processes[harq_pid]->status==ACTIVE?     "ACTIVE"    :
        (ulsch->harq_processes[harq_pid]->status==CBA_ACTIVE? "CBA_ACTIVE":
        (ulsch->harq_processes[harq_pid]->status==DISABLED?   "DISABLED"  : "UNKNOWN")))));

    return;
  }

  memset(d,0,24*sizeof(int8_t));
  phich_d_ptr = phich_d;

  // x1 is set in lte_gold_generic
  x2 = (((subframe+1)*((frame_parms->Nid_cell<<1)+1))<<9) + frame_parms->Nid_cell;

  s = lte_gold_generic(&x1, &x2, reset);

  // compute scrambling sequence
  for (i=0; i<12; i++) {
    cs[i] = 1-(((s>>(i&0x1f))&1)<<1);
  }

  if (frame_parms->Ncp == 0) { // Normal Cyclic Prefix


    // 12 output symbols (Msymb)

    for (i=0,i2=0,i3=0; i<3; i++,i2+=4,i3+=8) {
      switch (nseq_PHICH) {
      case 0: // +1 +1 +1 +1
        d[i3]   = cs[i2];
        d[1+i3] = cs[i2];
        d[2+i3] = cs[1+i2];
        d[3+i3] = cs[1+i2];
        d[4+i3] = cs[2+i2];
        d[5+i3] = cs[2+i2];
        d[6+i3] = cs[3+i2];
        d[7+i3] = cs[3+i2];
        break;

      case 1: // +1 -1 +1 -1
        d[i3] = cs[i2];
        d[1+i3] = cs[i2];
        d[2+i3] = -cs[1+i2];
        d[3+i3] = -cs[1+i2];
        d[4+i3] = cs[2+i2];
        d[5+i3] = cs[2+i2];
        d[6+i3] = -cs[3+i2];
        d[7+i3] = -cs[3+i2];
        break;

      case 2: // +1 +1 -1 -1
        d[i3]   = cs[i2];
        d[1+i3]   = cs[i2];
        d[2+i3] = cs[1+i2];
        d[3+i3] = cs[1+i2];
        d[4+i3] = -cs[2+i2];
        d[5+i3] = -cs[2+i2];
        d[6+i3] = -cs[3+i2];
        d[7+i3] = -cs[3+i2];
        break;

      case 3: // +1 -1 -1 +1
        d[i3]   = cs[i2];
        d[1+i3]   = cs[i2];
        d[2+i3] = -cs[1+i2];
        d[3+i3] = -cs[1+i2];
        d[4+i3] = -cs[2+i2];
        d[5+i3] = -cs[2+i2];
        d[6+i3] = cs[3+i2];
        d[7+i3] = cs[3+i2];
        break;

      case 4: // +j +j +j +j
        d[i3]   = -cs[i2];
        d[1+i3] = cs[i2];
        d[2+i3] = -cs[1+i2];
        d[3+i3] = cs[1+i2];
        d[4+i3] = -cs[2+i2];
        d[5+i3] = cs[2+i2];
        d[6+i3] = -cs[3+i2];
        d[7+i3] = cs[3+i2];
        break;

      case 5: // +j -j +j -j
        d[1+i3] = cs[i2];
        d[3+i3] = -cs[1+i2];
        d[5+i3] = cs[2+i2];
        d[7+i3] = -cs[3+i2];
        d[i3]   = -cs[i2];
        d[2+i3] = cs[1+i2];
        d[4+i3] = -cs[2+i2];
        d[6+i3] = cs[3+i2];
        break;

      case 6: // +j +j -j -j
        d[1+i3] = cs[i2];
        d[3+i3] = cs[1+i2];
        d[5+i3] = -cs[2+i2];
        d[7+i3] = -cs[3+i2];
        d[i3]   = -cs[i2];
        d[2+i3] = -cs[1+i2];
        d[4+i3] = cs[2+i2];
        d[6+i3] = cs[3+i2];
        break;

      case 7: // +j -j -j +j
        d[1+i3] = cs[i2];
        d[3+i3] = -cs[1+i2];
        d[5+i3] = -cs[2+i2];
        d[7+i3] = cs[3+i2];
        d[i3]   = -cs[i2];
        d[2+i3] = cs[1+i2];
        d[4+i3] = cs[2+i2];
        d[6+i3] = -cs[3+i2];
        break;

      default:
        AssertFatal(1==0,"phich_coding.c: Illegal PHICH Number\n");
      } // nseq_PHICH
    }

#ifdef DEBUG_PHICH
    LOG_D(PHY,"PHICH =>");

    for (i=0; i<24; i++) {
      LOG_D(PHY,"%2d,",d[i]);
    }

    LOG_D(PHY,"\n");
#endif
    // demodulation here


  } else { // extended prefix

    // 6 output symbols
    if ((ngroup_PHICH & 1) == 1)
      dp = &d[4];
    else
      dp = d;

    switch (nseq_PHICH) {
    case 0: // +1 +1
      dp[0]  = cs[0];
      dp[2]  = cs[1];
      dp[8]  = cs[2];
      dp[10] = cs[3];
      dp[16] = cs[4];
      dp[18] = cs[5];
      dp[1]  = cs[0];
      dp[3]  = cs[1];
      dp[9]  = cs[2];
      dp[11] = cs[3];
      dp[17] = cs[4];
      dp[19] = cs[5];
      break;

    case 1: // +1 -1
      dp[0]  = cs[0];
      dp[2]  = -cs[1];
      dp[8]  = cs[2];
      dp[10] = -cs[3];
      dp[16] = cs[4];
      dp[18] = -cs[5];
      dp[1]  = cs[0];
      dp[3]  = -cs[1];
      dp[9]  = cs[2];
      dp[11] = -cs[3];
      dp[17] = cs[4];
      dp[19] = -cs[5];
      break;

    case 2: // +j +j
      dp[1]  = cs[0];
      dp[3]  = cs[1];
      dp[9]  = cs[2];
      dp[11] = cs[3];
      dp[17] = cs[4];
      dp[19] = cs[5];
      dp[0]  = -cs[0];
      dp[2]  = -cs[1];
      dp[8]  = -cs[2];
      dp[10] = -cs[3];
      dp[16] = -cs[4];
      dp[18] = -cs[5];

      break;

    case 3: // +j -j
      dp[1]  = cs[0];
      dp[3]  = -cs[1];
      dp[9]  = cs[2];
      dp[11] = -cs[3];
      dp[17] = cs[4];
      dp[19] = -cs[5];
      dp[0]  = -cs[0];
      dp[2]  = cs[1];
      dp[8]  = -cs[2];
      dp[10] = cs[3];
      dp[16] = -cs[4];
      dp[18] = cs[5];
      break;

    default:
      AssertFatal(1==0,"phich_coding.c: Illegal PHICH Number\n");
    }
  }

  HI16 = 0;

  //#ifdef DEBUG_PHICH

  //#endif
  /*
  for (i=0;i<200;i++)
    printf("re %d: %d %d\n",i,((int16_t*)&rxdataF_comp[0][i])[0],((int16_t*)&rxdataF_comp[0][i])[1]);
  */
  for (phich_quad=0; phich_quad<3; phich_quad++) {
    if (frame_parms->Ncp == 1)
      reg_offset = (frame_parms->phich_reg[ngroup_PHICH][phich_quad]*4)+ (phich_quad*frame_parms->N_RB_DL*12);
    else
      reg_offset = (frame_parms->phich_reg[ngroup_PHICH][phich_quad]*4);

    //    msg("\n[PUSCH 0]PHICH (RX) quad %d (%d)=>",phich_quad,reg_offset);
    dp = &d[phich_quad*8];;

    for (i=0; i<8; i++) {
      phich_d_ptr[i] = ((int16_t*)&rxdataF_comp[0][reg_offset])[i];

#ifdef DEBUG_PHICH
      LOG_D(PHY,"%d,",((int16_t*)&rxdataF_comp[0][reg_offset])[i]);
#endif

      HI16 += (phich_d_ptr[i] * dp[i]);
    }
  }

#ifdef DEBUG_PHICH
  LOG_D(PHY,"\n");
  LOG_D(PHY,"HI16 %d\n",HI16);
#endif

  if (HI16>0) {   //NACK
    if (ue->ulsch_Msg3_active[eNB_id] == 1) {
      LOG_D(PHY,"[UE  %d][PUSCH %d][RAPROC] Frame %d subframe %d Msg3 PHICH, received NAK (%d) nseq %d, ngroup %d\n",
            ue->Mod_id,harq_pid,
            proc->frame_rx,
            subframe,
            HI16,
            nseq_PHICH,
            ngroup_PHICH);

      ulsch->f_pusch += delta_PUSCH_acc[ulsch->harq_processes[harq_pid]->TPC];

      LOG_D(PHY,"[PUSCH %d] AbsSubframe %d.%d: f_pusch (ACC) %d, adjusting by %d (TPC %d)\n",
                 harq_pid,proc->frame_rx,subframe,ulsch->f_pusch,
                    delta_PUSCH_acc[ulsch->harq_processes[harq_pid]->TPC],
                    ulsch->harq_processes[harq_pid]->TPC);


      ulsch->harq_processes[harq_pid]->subframe_scheduling_flag = 1;
      //      ulsch->harq_processes[harq_pid]->Ndi = 0;
      ulsch->harq_processes[harq_pid]->round++;
      ulsch->harq_processes[harq_pid]->rvidx = rv_table[ulsch->harq_processes[harq_pid]->round&3];

      if (ulsch->harq_processes[harq_pid]->round>=ue->frame_parms.maxHARQ_Msg3Tx) {
        ulsch->harq_processes[harq_pid]->subframe_scheduling_flag =0;
        ulsch->harq_processes[harq_pid]->status = SCH_IDLE;
        // inform MAC that Msg3 transmission has failed
        ue->ulsch_Msg3_active[eNB_id] = 0;
      }
    } else {
#ifdef UE_DEBUG_TRACE
      LOG_D(PHY,"[UE  %d][PUSCH %d] Frame %d subframe %d PHICH, received NAK (%d) nseq %d, ngroup %d round %d (Mlimit %d)\n",
            ue->Mod_id,harq_pid,
            proc->frame_rx%1024,
            subframe,
            HI16,
            nseq_PHICH,
            ngroup_PHICH,
            ulsch->harq_processes[harq_pid]->round,
            ulsch->Mlimit);
#endif

      //      ulsch->harq_processes[harq_pid]->Ndi = 0;
      ulsch->harq_processes[harq_pid]->round++;
      
      if ( ulsch->harq_processes[harq_pid]->round >= (ulsch->Mlimit - 1) )
      {
          // this is last push re transmission
          ulsch->harq_processes[harq_pid]->rvidx = rv_table[ulsch->harq_processes[harq_pid]->round&3];
          ulsch->O_RI = 0;
          ulsch->O    = 0;
          ulsch->uci_format = HLC_subband_cqi_nopmi;

          // disable phich decoding since it is the last retransmission
          ulsch->harq_processes[harq_pid]->status = SCH_IDLE;

          //ulsch->harq_processes[harq_pid]->subframe_scheduling_flag = 0;
          //ulsch->harq_processes[harq_pid]->round  = 0;

          //LOG_I(PHY,"PUSCH MAX Retransmission acheived ==> flush harq buff (%d) \n",harq_pid);
          //LOG_I(PHY,"[HARQ-UL harqId: %d] PHICH NACK MAX RETRANS(%d) ==> subframe_scheduling_flag = %d round: %d\n", harq_pid, ulsch->Mlimit, ulsch->harq_processes[harq_pid]->subframe_scheduling_flag, ulsch->harq_processes[harq_pid]->round);
      }
      else
      {
          // ulsch->harq_processes[harq_pid]->subframe_scheduling_flag = 1;
          ulsch->harq_processes[harq_pid]->rvidx = rv_table[ulsch->harq_processes[harq_pid]->round&3];
          ulsch->O_RI = 0;
          ulsch->O    = 0;
          ulsch->uci_format = HLC_subband_cqi_nopmi;
          //LOG_I(PHY,"[HARQ-UL harqId: %d] PHICH NACK ==> subframe_scheduling_flag = %d round: %d\n", harq_pid, ulsch->harq_processes[harq_pid]->subframe_scheduling_flag,ulsch->harq_processes[harq_pid]->round);
      }
    }
#if T_TRACER
    T(T_UE_PHY_ULSCH_UE_NACK, T_INT(eNB_id), T_INT(proc->frame_rx%1024), T_INT(subframe), T_INT(ulsch->rnti),
      T_INT(harq_pid));
#endif

  } else {  //ACK
    if (ue->ulsch_Msg3_active[eNB_id] == 1) {
      LOG_D(PHY,"[UE  %d][PUSCH %d][RAPROC] Frame %d subframe %d Msg3 PHICH, received ACK (%d) nseq %d, ngroup %d\n\n",
            ue->Mod_id,harq_pid,
            proc->frame_rx,
            subframe,
            HI16,
            nseq_PHICH,ngroup_PHICH);
    } else {
#ifdef UE_DEBUG_TRACE
      LOG_D(PHY,"[UE  %d][PUSCH %d] Frame %d subframe %d PHICH, received ACK (%d) nseq %d, ngroup %d\n\n",
            ue->Mod_id,harq_pid,
            proc->frame_rx%1024,
            subframe, HI16,
            nseq_PHICH,ngroup_PHICH);
#endif
    }

   // LOG_I(PHY,"[HARQ-UL harqId: %d] subframe_scheduling_flag = %d \n",harq_pid, ulsch->harq_processes[harq_pid]->subframe_scheduling_flag);

    // Incase of adaptive retransmission, PHICH is always decoded as ACK (at least with OAI-eNB)
    // Workaround:
    // rely only on DCI0 decoding and check if NDI has toggled
    //   save current harq_processes content in temporary struct
    //   harqId-8 corresponds to the temporary struct. In total we have 8 harq process(0 ..7) + 1 temporary harq process()
    //ulsch->harq_processes[8] = ulsch->harq_processes[harq_pid];


    ulsch->harq_processes[harq_pid]->status                   = SCH_IDLE;
    ulsch->harq_processes[harq_pid]->round                    = 0;
    ulsch->harq_processes[harq_pid]->subframe_scheduling_flag = 0;
    // inform MAC?
    ue->ulsch_Msg3_active[eNB_id] = 0;

#if T_TRACER
    T(T_UE_PHY_ULSCH_UE_ACK, T_INT(eNB_id), T_INT(proc->frame_rx%1024), T_INT(subframe), T_INT(ulsch->rnti),
      T_INT(harq_pid));
#endif

  }

}

void generate_phich_top(PHY_VARS_eNB *eNB,
                        eNB_rxtx_proc_t *proc,
			int16_t amp)
{


  LTE_DL_FRAME_PARMS *frame_parms=&eNB->frame_parms;
  int32_t **txdataF = eNB->common_vars.txdataF;
  uint8_t harq_pid;
  uint8_t Ngroup_PHICH,ngroup_PHICH,nseq_PHICH;
  uint8_t NSF_PHICH = 4;
  uint8_t pusch_subframe;
  uint8_t i;
  uint32_t pusch_frame;
  int subframe = proc->subframe_tx;
  phich_config_t *phich;

  // compute Ngroup_PHICH (see formula at beginning of Section 6.9 in 36-211

  Ngroup_PHICH = (frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)/48;

  if (((frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)%48) > 0)
    Ngroup_PHICH++;

  if (frame_parms->Ncp == 1)
    NSF_PHICH = 2;

  if (eNB->phich_vars[subframe&1].num_hi > 0) {
    pusch_frame = phich_frame2_pusch_frame(frame_parms,proc->frame_tx,subframe);
    pusch_subframe = phich_subframe2_pusch_subframe(frame_parms,subframe);
    harq_pid = subframe2harq_pid(frame_parms,pusch_frame,pusch_subframe);
  }

  for (i=0; i<eNB->phich_vars[subframe&1].num_hi; i++) {

    phich = &eNB->phich_vars[subframe&1].config[i];
    
    ngroup_PHICH = (phich->first_rb +
		    phich->n_DMRS)%Ngroup_PHICH;
    
    if ((frame_parms->tdd_config == 0) && (frame_parms->frame_type == TDD) ) {
      
      if ((pusch_subframe == 4) || (pusch_subframe == 9))
	ngroup_PHICH += Ngroup_PHICH;
    }
    
    nseq_PHICH = ((phich->first_rb/Ngroup_PHICH) +
		  phich->n_DMRS)%(2*NSF_PHICH);
    LOG_D(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d Generating PHICH, AMP %d  ngroup_PHICH %d/%d, nseq_PHICH %d : HI %d, first_rb %d)\n",
	  eNB->Mod_id,harq_pid,proc->frame_tx,
	  subframe,amp,ngroup_PHICH,Ngroup_PHICH,nseq_PHICH,
	  phich->hi,
	  phich->first_rb);
    
    T(T_ENB_PHY_PHICH, T_INT(eNB->Mod_id), T_INT(proc->frame_tx), T_INT(subframe),
      T_INT(-1 /* TODO: rnti */), T_INT(harq_pid),
      T_INT(Ngroup_PHICH), T_INT(NSF_PHICH),
      T_INT(ngroup_PHICH), T_INT(nseq_PHICH),
      T_INT(phich->hi),
      T_INT(phich->first_rb),
      T_INT(phich->n_DMRS));
    
    generate_phich(frame_parms,
		   amp,//amp*2,
		   nseq_PHICH,
		   ngroup_PHICH,
		   phich->hi,
		   subframe,
		   txdataF);
  }//  for (i=0; i<eNB->phich_vars[subframe&1].num_hi; i++) { 
  eNB->phich_vars[subframe&1].num_hi=0;
}
