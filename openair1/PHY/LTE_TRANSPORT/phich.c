
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
* \brief Top-level routines for generating the PHICH/HI physical/transport channel V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

#include "PHY/defs_eNB.h"
#include "PHY/impl_defs_top.h"
#include "T.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "transport_common_proto.h"

//#define DEBUG_PHICH 1

//extern unsigned short pcfich_reg[4];
//extern unsigned char pcfich_first_reg_idx;

//unsigned short phich_reg[MAX_NUM_PHICH_GROUPS][3];



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


void generate_phich_top(PHY_VARS_eNB *eNB,
                        L1_rxtx_proc_t *proc,
			int16_t amp)
{


  LTE_DL_FRAME_PARMS *frame_parms=&eNB->frame_parms;
  int32_t **txdataF = eNB->common_vars.txdataF;
  uint8_t Ngroup_PHICH,ngroup_PHICH,nseq_PHICH;
  uint8_t NSF_PHICH = 4;
  uint8_t pusch_subframe=-1;
  uint8_t i;
  uint8_t harq_pid = 0; 
  int subframe = proc->subframe_tx;
  phich_config_t *phich;

  // compute Ngroup_PHICH (see formula at beginning of Section 6.9 in 36-211

  Ngroup_PHICH = (frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)/48;

  if (((frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)%48) > 0)
    Ngroup_PHICH++;

  if (frame_parms->Ncp == 1)
    NSF_PHICH = 2;

  if (eNB->phich_vars[subframe&1].num_hi > 0) {
    pusch_subframe = phich_subframe2_pusch_subframe(frame_parms,subframe);
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
    harq_pid = subframe2harq_pid(frame_parms,phich_frame2_pusch_frame(frame_parms,proc->frame_tx,subframe),pusch_subframe);
	if (harq_pid == 255) {
      LOG_E(PHY,"FATAL ERROR: illegal harq_pid, returning\n");
	  return;
	}
    LOG_D(PHY,"[eNB %d][PUSCH %d] Frame %d subframe %d Generating PHICH, AMP %d  ngroup_PHICH %d/%d, nseq_PHICH %d : HI %d, first_rb %d)\n",
	  eNB->Mod_id,harq_pid,proc->frame_tx,
	  subframe,amp,ngroup_PHICH,Ngroup_PHICH,nseq_PHICH,
	  phich->hi,
	  phich->first_rb);
    
    T(T_ENB_PHY_PHICH, T_INT(eNB->Mod_id), T_INT(proc->frame_tx), T_INT(subframe),
      T_INT(-1 /* TODO: rnti */), 
      T_INT(harq_pid),
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


