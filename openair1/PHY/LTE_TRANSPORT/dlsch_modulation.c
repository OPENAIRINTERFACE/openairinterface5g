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

/*! \file PHY/LTE_TRANSPORT/dlsch_modulation.c
 * \brief Top-level routines for generating the PDSCH physical channel from 36-211, V8.6 2009-03, V14.1 2017 (FeMBMS)
 * \author R. Knopp, F. Kaltenberger, J. Morgade
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, javier.morgade@ieee.irg
 * \note
 * \warning
 */
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/LTE_TRANSPORT/transport_eNB.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"
//#define DEBUG_DLSCH_MODULATION
//#define NEW_ALLOC_RE

//#define is_not_pilot(pilots,re,nushift,use2ndpilots) ((pilots==0) || ((re!=nushift) && (re!=nushift+6)&&((re!=nushift+3)||(use2ndpilots==1))&&((re!=nushift+9)||(use2ndpilots==1)))?1:0)

uint8_t is_not_pilot(uint8_t pilots, uint8_t re, uint8_t nushift, uint8_t use2ndpilots)
{

  uint8_t offset = (pilots==2)?3:0;
  int nushiftmod3 = nushift%3;

  if (pilots==0)
    return(1);

  if (use2ndpilots==1) {  // This is for SISO (mode 1)
    if ((re!=nushift+offset) && (re!=((nushift+6+offset)%12)))
      return(1);
  } else { // 2 antenna pilots
    if ((re!=nushiftmod3) && (re!=nushiftmod3+6) && (re!=nushiftmod3+3) && (re!=nushiftmod3+9))
      return(1);
  }

  return(0);
}

/*uint8_t is_not_UEspecRS(int first_layer,int re)
{
  return(1);
}*/
uint8_t is_not_UEspecRS(int8_t lprime, uint8_t re, uint8_t nushift, uint8_t Ncp, uint8_t beamforming_mode)
{
  uint8_t offset = (lprime==1||lprime==3)?2:0;
  if (lprime==-1)
    return(1);

  switch (beamforming_mode) {
    case 7:
      if (Ncp == NORMAL){
        if ((re!=nushift+offset) && (re!=((nushift+4+offset)%12)) &&  (re!=((nushift+8+offset)%12)))
          return(1);
        /*else{
          LOG_I(PHY,"(is_no_UEspec_RS):lprime=%d, re=%d, nushift=%d, offset=%d\n",lprime, re,nushift,offset);
        }*/
      } else {
        if ((re!=nushift+offset) && (re!=((nushift+3+offset)%12)) && (re!=((nushift+6+offset)%12)) && (re!=((nushift+9+offset)%12)))
          return(1);
      }
      break;

    default:
      LOG_E(PHY,"is_not_UEspecRS() [dlsch_modulation.c] : ERROR, unknown beamforming_mode %d\n",beamforming_mode);
      return(-1);
  }

  return(0);
}






void layer1prec2A(int32_t *antenna0_sample, int32_t *antenna1_sample, uint8_t precoding_index)
{

  switch (precoding_index) {

    case 0: // 1 1
      *antenna1_sample=*antenna0_sample;
      break;

    case 1: // 1 -1
      ((int16_t *)antenna1_sample)[0] = -((int16_t *)antenna0_sample)[0];
      ((int16_t *)antenna1_sample)[1] = -((int16_t *)antenna0_sample)[1];
      break;

    case 2: // 1 j
      ((int16_t *)antenna1_sample)[0] = -((int16_t *)antenna0_sample)[1];
      ((int16_t *)antenna1_sample)[1] = ((int16_t *)antenna0_sample)[0];
      break;

    case 3: // 1 -j
      ((int16_t *)antenna1_sample)[0] = ((int16_t *)antenna0_sample)[1];
      ((int16_t *)antenna1_sample)[1] = -((int16_t *)antenna0_sample)[0];
      break;
  }

  // normalize
  /*  ((int16_t *)antenna0_sample)[0] = (int16_t)((((int16_t *)antenna0_sample)[0]*ONE_OVER_SQRT2_Q15)>>15);
      ((int16_t *)antenna0_sample)[1] = (int16_t)((((int16_t *)antenna0_sample)[1]*ONE_OVER_SQRT2_Q15)>>15);  ((int16_t *)antenna1_sample)[0] = (int16_t)((((int16_t *)antenna1_sample)[0]*ONE_OVER_SQRT2_Q15)>>15);
      ((int16_t *)antenna1_sample)[1] = (int16_t)((((int16_t *)antenna1_sample)[1]*ONE_OVER_SQRT2_Q15)>>15);  */
}

uint32_t FOUR[2]={0,4};
uint32_t TWO[2]={0,2};

int allocate_REs_in_RB_no_pilots_QPSK_siso(PHY_VARS_eNB* phy_vars_eNB,
                                           int **txdataF,
                                           uint32_t *jj,
                                           uint32_t *jj2,
                                           uint16_t re_offset,
                                           uint32_t symbol_offset,
                                           LTE_DL_eNB_HARQ_t *dlsch0_harq,
                                           LTE_DL_eNB_HARQ_t *dlsch1_harq,
                                           uint8_t pilots,
                                           int16_t amp,
                                           uint8_t precoder_index,
                                           int16_t *qam_table_s0,
                                           int16_t *qam_table_s1,
                                           uint32_t *re_allocated,
                                           uint8_t skip_dc,
                                           uint8_t skip_half,
                                           uint8_t lprime,
                                           uint8_t mprime,
                                           uint8_t Ns,
                                           int *P1_SHIFT,
                                           int *P2_SHIFT)
{

  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNB->frame_parms;
  uint8_t *x0             = dlsch0_harq->eDL;
  uint32_t qpsk_table_offset_re = 0;
  uint32_t qpsk_table_offset_im = 0;

  uint32_t tti_offset;
  uint8_t re;
  uint8_t *x0p;
  uint8_t first_re,last_re;

  last_re=12;
  first_re=0;
  if (skip_half==1)
    last_re=6;
  else if (skip_half==2)
    first_re=6;
  re=first_re;

  if (skip_dc == 0) {
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+re;
         re<last_re;
         re++,x0p+=2,tti_offset++) {

      qpsk_table_offset_re=x0p[0];
      qpsk_table_offset_im=x0p[1];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qpsk_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qpsk_table_offset_im];
    }
  }
  else {
    // 1st half of PRB
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset,re=0;
         re<6;
         re++,x0p+=2,tti_offset++) {

      qpsk_table_offset_re=x0p[0];
      qpsk_table_offset_im=x0p[1];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qpsk_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qpsk_table_offset_im];
    }
    // 2nd half of PRB
    for (tti_offset=symbol_offset+re_offset-frame_parms->ofdm_symbol_size+7;
         re<12;
         re++,x0p+=2,tti_offset++) {

      qpsk_table_offset_re=x0p[0];
      qpsk_table_offset_im=x0p[1];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qpsk_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qpsk_table_offset_im];
    }
  }
  if(skip_half!=0)
  {
    *re_allocated = *re_allocated + 6;
    *jj=*jj + 12;
  }
  else
  {
    *re_allocated = *re_allocated + 12;
    *jj=*jj + 24;
  }

    return(0);
}

int allocate_REs_in_RB_pilots_QPSK_siso(PHY_VARS_eNB* phy_vars_eNB,
                                        int **txdataF,
                                        uint32_t *jj,
                                        uint32_t *jj2,
                                        uint16_t re_offset,
                                        uint32_t symbol_offset,
                                        LTE_DL_eNB_HARQ_t *dlsch0_harq,
                                        LTE_DL_eNB_HARQ_t *dlsch1_harq,
                                        uint8_t pilots,
                                        int16_t amp,
                                        uint8_t precoder_index,
                                        int16_t *qam_table_s0,
                                        int16_t *qam_table_s1,
                                        uint32_t *re_allocated,
                                        uint8_t skip_dc,
                                        uint8_t skip_half,
                                        uint8_t lprime,
                                        uint8_t mprime,
                                        uint8_t Ns,
                                        int *P1_SHIFT,
                                        int *P2_SHIFT)
{


  LTE_DL_FRAME_PARMS *frame_parms=&phy_vars_eNB->frame_parms;

  uint8_t *x0             = dlsch0_harq->eDL;
  uint32_t qpsk_table_offset_re = 0;
  uint32_t qpsk_table_offset_im = 0;

  uint32_t tti_offset;
  uint8_t re;
  uint8_t *x0p;
  uint8_t first_re,last_re;

  last_re=12;
  first_re=0;
  if (skip_half==1)
    last_re=6;
  else if (skip_half==2)
    first_re=6;
  re=first_re+P1_SHIFT[0];

  if (skip_dc == 0) {
    //    printf("pilots: P1_SHIFT[0] %d\n",P1_SHIFT[0]);
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+re;
         re<last_re;
         x0p+=2) {

      qpsk_table_offset_re=x0p[0];
      qpsk_table_offset_im=x0p[1];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qpsk_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qpsk_table_offset_im];
      //      printf("pilots: re %d, tti_offset %d, P1_SHIFT %d\n",re,tti_offset,P1_SHIFT[re+1]);
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }
  }
  else {
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+P1_SHIFT[0],re=P1_SHIFT[0];
         re<6;
         x0p+=2) {

      qpsk_table_offset_re=x0p[0];
      qpsk_table_offset_im=x0p[1];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qpsk_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qpsk_table_offset_im];
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }

    for (tti_offset=symbol_offset+re_offset-frame_parms->ofdm_symbol_size+6+P1_SHIFT[6];
         re<12;
         x0p+=2) {

      qpsk_table_offset_re=x0p[0];
      qpsk_table_offset_im=x0p[1];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qpsk_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qpsk_table_offset_im];
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }
  }
  if(skip_half!=0)
  {
    *re_allocated = *re_allocated + 5;
    *jj=*jj + 10;
  }
  else
  {
    *re_allocated = *re_allocated + 10;
    *jj=*jj + 20;
  }

  return(0);
}

int allocate_REs_in_RB_no_pilots_16QAM_siso(PHY_VARS_eNB* phy_vars_eNB,
                                            int **txdataF,
                                            uint32_t *jj,
                                            uint32_t *jj2,
                                            uint16_t re_offset,
                                            uint32_t symbol_offset,
                                            LTE_DL_eNB_HARQ_t *dlsch0_harq,
                                            LTE_DL_eNB_HARQ_t *dlsch1_harq,
                                            uint8_t pilots,
                                            int16_t amp,
                                            uint8_t precoder_index,
                                            int16_t *qam_table_s0,
                                            int16_t *qam_table_s1,
                                            uint32_t *re_allocated,
                                            uint8_t skip_dc,
                                            uint8_t skip_half,
                                            uint8_t lprime,
                                            uint8_t mprime,
                                            uint8_t Ns,
                                            int *P1_SHIFT,
                                            int *P2_SHIFT)
{

  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNB->frame_parms;
  uint8_t *x0             = dlsch0_harq->eDL;
  uint32_t qam16_table_offset_re = 0;
  uint32_t qam16_table_offset_im = 0;

  uint32_t tti_offset;
  uint8_t re;
  uint8_t *x0p;
  uint8_t first_re,last_re;

  last_re=12;
  first_re=0;
  if (skip_half==1)
    last_re=6;
  else if (skip_half==2)
    first_re=6;
  re=first_re;

  if (skip_dc == 0) {
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+re;
         re<last_re;
         re++,x0p+=4,tti_offset++) {

      qam16_table_offset_re=TWO[x0p[0]];
      qam16_table_offset_im=TWO[x0p[1]];
      qam16_table_offset_re+=x0p[2];
      qam16_table_offset_im+=x0p[3];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam16_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam16_table_offset_im];
    }
  }
  else {
    // 1st half of PRB
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset,re=0;
         re<6;
         re++,x0p+=4,tti_offset++) {

      qam16_table_offset_re=TWO[x0p[0]];
      qam16_table_offset_im=TWO[x0p[1]];
      qam16_table_offset_re+=x0p[2];
      qam16_table_offset_im+=x0p[3];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam16_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam16_table_offset_im];
    }
    // 2nd half of PRB
    for (tti_offset=symbol_offset+re_offset-frame_parms->ofdm_symbol_size+7;
         re<12;
         re++,x0p+=4,tti_offset++) {

      qam16_table_offset_re=TWO[x0p[0]];
      qam16_table_offset_im=TWO[x0p[1]];
      qam16_table_offset_re+=x0p[2];
      qam16_table_offset_im+=x0p[3];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam16_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam16_table_offset_im];
    }
  }
  if(skip_half!=0)
  {
    *re_allocated = *re_allocated + 6;
    *jj=*jj + 24;
  }
  else
  {
    *re_allocated = *re_allocated + 12;
    *jj=*jj + 48;
  }

    return(0);
}

int allocate_REs_in_RB_pilots_16QAM_siso(PHY_VARS_eNB* phy_vars_eNB,
                                         int **txdataF,
                                         uint32_t *jj,
                                         uint32_t *jj2,
                                         uint16_t re_offset,
                                         uint32_t symbol_offset,
                                         LTE_DL_eNB_HARQ_t *dlsch0_harq,
                                         LTE_DL_eNB_HARQ_t *dlsch1_harq,
                                         uint8_t pilots,
                                         int16_t amp,
                                         uint8_t precoder_index,
                                         int16_t *qam_table_s0,
                                         int16_t *qam_table_s1,
                                         uint32_t *re_allocated,
                                         uint8_t skip_dc,
                                         uint8_t skip_half,
                                         uint8_t lprime,
                                         uint8_t mprime,
                                         uint8_t Ns,
                                         int *P1_SHIFT,
                                         int *P2_SHIFT)
{


  LTE_DL_FRAME_PARMS *frame_parms=&phy_vars_eNB->frame_parms;

  uint8_t *x0             = dlsch0_harq->eDL;
  uint32_t qam16_table_offset_re = 0;
  uint32_t qam16_table_offset_im = 0;

  uint32_t tti_offset;
  uint8_t re;
  uint8_t *x0p;
  uint8_t first_re,last_re;

  last_re=12;
  first_re=0;
  if (skip_half==1)
    last_re=6;
  else if (skip_half==2)
    first_re=6;
  re=first_re+P1_SHIFT[0];


  if (skip_dc == 0) {
    //    LOG_I(PHY,"pilots: P1_SHIFT[0] %d\n",P1_SHIFT[0]);
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+re;
         re<last_re;
         x0p+=4) {

      qam16_table_offset_re=TWO[x0p[0]];
      qam16_table_offset_im=TWO[x0p[1]];
      qam16_table_offset_re+=x0p[2];
      qam16_table_offset_im+=x0p[3];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam16_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam16_table_offset_im];
      //      LOG_I(PHY,"pilots: re %d, tti_offset %d, P1_SHIFT %d\n",re,tti_offset,P1_SHIFT[re+1]);
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }
  }
  else {
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+P1_SHIFT[0],re=P1_SHIFT[0];
         re<6;
         x0p+=4) {

      qam16_table_offset_re=TWO[x0p[0]];
      qam16_table_offset_im=TWO[x0p[1]];
      qam16_table_offset_re+=x0p[2];
      qam16_table_offset_im+=x0p[3];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam16_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam16_table_offset_im];
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }

    for (tti_offset=symbol_offset+re_offset-frame_parms->ofdm_symbol_size+6+P1_SHIFT[6];
         re<12;
         x0p+=4) {

      qam16_table_offset_re=TWO[x0p[0]];
      qam16_table_offset_im=TWO[x0p[1]];
      qam16_table_offset_re+=x0p[2];
      qam16_table_offset_im+=x0p[3];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam16_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam16_table_offset_im];
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }
  }
  if(skip_half!=0)
  {
    *re_allocated = *re_allocated + 5;
    *jj=*jj + 20;
  }
  else
  {
    *re_allocated = *re_allocated + 10;
    *jj=*jj + 40;
  }

  return(0);
}

int allocate_REs_in_RB_no_pilots_64QAM_siso(PHY_VARS_eNB* phy_vars_eNB,
                                            int **txdataF,
                                            uint32_t *jj,
                                            uint32_t *jj2,
                                            uint16_t re_offset,
                                            uint32_t symbol_offset,
                                            LTE_DL_eNB_HARQ_t *dlsch0_harq,
                                            LTE_DL_eNB_HARQ_t *dlsch1_harq,
                                            uint8_t pilots,
                                            int16_t amp,
                                            uint8_t precoder_index,
                                            int16_t *qam_table_s0,
                                            int16_t *qam_table_s1,
                                            uint32_t *re_allocated,
                                            uint8_t skip_dc,
                                            uint8_t skip_half,
                                            uint8_t lprime,
                                            uint8_t mprime,
                                            uint8_t Ns,
                                            int *P1_SHIFT,
                                            int *P2_SHIFT)
{

  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNB->frame_parms;

  uint8_t *x0             = dlsch0_harq->eDL;
  uint32_t qam64_table_offset_re = 0;
  uint32_t qam64_table_offset_im = 0;

  uint32_t tti_offset;
  uint8_t re;
  uint8_t *x0p;
  uint8_t first_re;

  first_re=0;
  if (skip_half==2)
    first_re=6;
  re=first_re;

  if (skip_dc == 0) {

    x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+re;

    /*    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset,re=0;
         re<12;
         re+=4,x0p+=24,tti_offset+=4) {*/

      qam64_table_offset_re=(x0p[0]<<2)|(x0p[2]<<1)|x0p[4];
      qam64_table_offset_im=(x0p[1]<<2)|(x0p[3]<<1)|x0p[5];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam64_table_offset_im];

      qam64_table_offset_re=(x0p[6]<<2)|(x0p[8]<<1)|x0p[10];
      qam64_table_offset_im=(x0p[7]<<2)|(x0p[9]<<1)|x0p[11];
      ((int16_t *)&txdataF[0][tti_offset])[2]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[3]=qam_table_s0[qam64_table_offset_im];

      qam64_table_offset_re=(x0p[12]<<2)|(x0p[14]<<1)|x0p[16];
      qam64_table_offset_im=(x0p[13]<<2)|(x0p[15]<<1)|x0p[17];
      ((int16_t *)&txdataF[0][tti_offset])[4]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[5]=qam_table_s0[qam64_table_offset_im];

      qam64_table_offset_re=(x0p[18]<<2)|(x0p[20]<<1)|x0p[22];
      qam64_table_offset_im=(x0p[19]<<2)|(x0p[21]<<1)|x0p[23];
      ((int16_t *)&txdataF[0][tti_offset])[6]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[7]=qam_table_s0[qam64_table_offset_im];

      qam64_table_offset_re=(x0p[24]<<2)|(x0p[26]<<1)|x0p[28];
      qam64_table_offset_im=(x0p[25]<<2)|(x0p[27]<<1)|x0p[29];
      ((int16_t *)&txdataF[0][tti_offset])[8]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[9]=qam_table_s0[qam64_table_offset_im];

      qam64_table_offset_re=(x0p[30]<<2)|(x0p[32]<<1)|x0p[34];
      qam64_table_offset_im=(x0p[31]<<2)|(x0p[33]<<1)|x0p[35];
      ((int16_t *)&txdataF[0][tti_offset])[10]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[11]=qam_table_s0[qam64_table_offset_im];

      if(skip_half==0)
      {
        qam64_table_offset_re=(x0p[36]<<2)|(x0p[38]<<1)|x0p[40];
        qam64_table_offset_im=(x0p[37]<<2)|(x0p[39]<<1)|x0p[41];
        ((int16_t *)&txdataF[0][tti_offset])[12]=qam_table_s0[qam64_table_offset_re];
        ((int16_t *)&txdataF[0][tti_offset])[13]=qam_table_s0[qam64_table_offset_im];

        qam64_table_offset_re=(x0p[42]<<2)|(x0p[44]<<1)|x0p[46];
        qam64_table_offset_im=(x0p[43]<<2)|(x0p[45]<<1)|x0p[47];
        ((int16_t *)&txdataF[0][tti_offset])[14]=qam_table_s0[qam64_table_offset_re];
        ((int16_t *)&txdataF[0][tti_offset])[15]=qam_table_s0[qam64_table_offset_im];

        qam64_table_offset_re=(x0p[48]<<2)|(x0p[50]<<1)|x0p[52];
        qam64_table_offset_im=(x0p[49]<<2)|(x0p[51]<<1)|x0p[53];
        ((int16_t *)&txdataF[0][tti_offset])[16]=qam_table_s0[qam64_table_offset_re];
        ((int16_t *)&txdataF[0][tti_offset])[17]=qam_table_s0[qam64_table_offset_im];

        qam64_table_offset_re=(x0p[54]<<2)|(x0p[56]<<1)|x0p[58];
        qam64_table_offset_im=(x0p[55]<<2)|(x0p[57]<<1)|x0p[59];
        ((int16_t *)&txdataF[0][tti_offset])[18]=qam_table_s0[qam64_table_offset_re];
        ((int16_t *)&txdataF[0][tti_offset])[19]=qam_table_s0[qam64_table_offset_im];

        qam64_table_offset_re=(x0p[60]<<2)|(x0p[62]<<1)|x0p[64];
        qam64_table_offset_im=(x0p[61]<<2)|(x0p[63]<<1)|x0p[65];
        ((int16_t *)&txdataF[0][tti_offset])[20]=qam_table_s0[qam64_table_offset_re];
        ((int16_t *)&txdataF[0][tti_offset])[21]=qam_table_s0[qam64_table_offset_im];

        qam64_table_offset_re=(x0p[66]<<2)|(x0p[68]<<1)|x0p[70];
        qam64_table_offset_im=(x0p[67]<<2)|(x0p[69]<<1)|x0p[71];
        ((int16_t *)&txdataF[0][tti_offset])[22]=qam_table_s0[qam64_table_offset_re];
        ((int16_t *)&txdataF[0][tti_offset])[23]=qam_table_s0[qam64_table_offset_im];
      }

      //    }
  }
  else {
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset,re=0;
         re<6;
         re++,x0p+=6,tti_offset++) {

      qam64_table_offset_re=FOUR[x0p[0]];
      qam64_table_offset_im=FOUR[x0p[1]];
      qam64_table_offset_re+=TWO[x0p[2]];
      qam64_table_offset_im+=TWO[x0p[3]];
      qam64_table_offset_re+=x0p[4];
      qam64_table_offset_im+=x0p[5];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam64_table_offset_im];
    }

    for (tti_offset=symbol_offset+re_offset-frame_parms->ofdm_symbol_size+7;
         re<12;
         re++,x0p+=6,tti_offset++) {

      qam64_table_offset_re=FOUR[x0p[0]];
      qam64_table_offset_im=FOUR[x0p[1]];
      qam64_table_offset_re+=TWO[x0p[2]];
      qam64_table_offset_im+=TWO[x0p[3]];
      qam64_table_offset_re+=x0p[4];
      qam64_table_offset_im+=x0p[5];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam64_table_offset_im];
    }
  }

  if(skip_half!=0)
  {
    *re_allocated = *re_allocated + 6;
    *jj=*jj + 36;
  }
  else
  {
    *re_allocated = *re_allocated + 12;
    *jj=*jj + 72;
  }

  return(0);
}

int allocate_REs_in_RB_pilots_64QAM_siso(PHY_VARS_eNB* phy_vars_eNB,
                                         int **txdataF,
                                         uint32_t *jj,
                                         uint32_t *jj2,
                                         uint16_t re_offset,
                                         uint32_t symbol_offset,
                                         LTE_DL_eNB_HARQ_t *dlsch0_harq,
                                         LTE_DL_eNB_HARQ_t *dlsch1_harq,
                                         uint8_t pilots,
                                         int16_t amp,
                                         uint8_t precoder_index,
                                         int16_t *qam_table_s0,
                                         int16_t *qam_table_s1,
                                         uint32_t *re_allocated,
                                         uint8_t skip_dc,
                                         uint8_t skip_half,
                                         uint8_t lprime,
                                         uint8_t mprime,
                                         uint8_t Ns,
                                         int *P1_SHIFT,
                                         int *P2_SHIFT)
{


  LTE_DL_FRAME_PARMS *frame_parms=&phy_vars_eNB->frame_parms;

  uint8_t *x0             = dlsch0_harq->eDL;
  uint32_t qam64_table_offset_re = 0;
  uint32_t qam64_table_offset_im = 0;

  uint32_t tti_offset;
  uint8_t re;
  uint8_t *x0p;
  uint8_t first_re,last_re;

  last_re=12;
  first_re=0;
  if (skip_half==1)
    last_re=6;
  else if (skip_half==2)
    first_re=6;
  re=first_re+P1_SHIFT[0];


  if (skip_dc == 0) {
    //    LOG_I(PHY,"pilots: P1_SHIFT[0] %d\n",P1_SHIFT[0]);
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+re;
         re<last_re;
         x0p+=6) {

      qam64_table_offset_re=FOUR[x0p[0]];
      qam64_table_offset_im=FOUR[x0p[1]];
      qam64_table_offset_re+=TWO[x0p[2]];
      qam64_table_offset_im+=TWO[x0p[3]];
      qam64_table_offset_re+=x0p[4];
      qam64_table_offset_im+=x0p[5];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam64_table_offset_im];
      //      LOG_I(PHY,"pilots: re %d, tti_offset %d, P1_SHIFT %d\n",re,tti_offset,P1_SHIFT[re+1]);
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }
  }
  else {
    for (x0p=&x0[*jj],tti_offset=symbol_offset+re_offset+P1_SHIFT[0],re=P1_SHIFT[0];
         re<6;
         x0p+=6) {

      qam64_table_offset_re=FOUR[x0p[0]];
      qam64_table_offset_im=FOUR[x0p[1]];
      qam64_table_offset_re+=TWO[x0p[2]];
      qam64_table_offset_im+=TWO[x0p[3]];
      qam64_table_offset_re+=x0p[4];
      qam64_table_offset_im+=x0p[5];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam64_table_offset_im];
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }

    for (tti_offset=symbol_offset+re_offset-frame_parms->ofdm_symbol_size+6+P1_SHIFT[6];
         re<12;
         x0p+=6) {

      qam64_table_offset_re=FOUR[x0p[0]];
      qam64_table_offset_im=FOUR[x0p[1]];
      qam64_table_offset_re+=TWO[x0p[2]];
      qam64_table_offset_im+=TWO[x0p[3]];
      qam64_table_offset_re+=x0p[4];
      qam64_table_offset_im+=x0p[5];
      ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s0[qam64_table_offset_re];
      ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s0[qam64_table_offset_im];
      tti_offset+=P1_SHIFT[re+1];
      re+=P1_SHIFT[re+1];
    }
  }
  if(skip_half!=0)
  {
    *re_allocated = *re_allocated + 5;
    *jj=*jj + 30;
  }
  else
  {
    *re_allocated = *re_allocated + 10;
    *jj=*jj + 60;
  }

  return(0);
}

int allocate_REs_in_RB(PHY_VARS_eNB* phy_vars_eNB,
                       int32_t **txdataF,
                       uint32_t *jj,
                       uint32_t *jj2,
                       uint16_t re_offset,
                       uint32_t symbol_offset,
                       LTE_DL_eNB_HARQ_t *dlsch0_harq,
                       LTE_DL_eNB_HARQ_t *dlsch1_harq,
                       uint8_t pilots,
                       int16_t amp,
                       uint8_t precoder_index,
                       int16_t *qam_table_s0,
                       int16_t *qam_table_s1,
                       uint32_t *re_allocated,
                       uint8_t skip_dc,
                       uint8_t skip_half,
                       uint8_t lprime,
                       uint8_t mprime,
                       uint8_t Ns,
                       int *P1_SHIFT,
                       int *P2_SHIFT)
{


  uint8_t *x0 = NULL;
  MIMO_mode_t mimo_mode = -1;

  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNB->frame_parms;


  int first_layer0 = -1; //= dlsch0_harq->first_layer;
  int Nlayers0 = -1; //  = dlsch0_harq->Nlayers;
  uint8_t mod_order0=0;
  uint8_t mod_order1=0;
  uint8_t precoder_index0,precoder_index1;

  uint8_t *x1=NULL;
  // Fill these in later for TM8-10
  //  int Nlayers1;
  //  int first_layer1;

  int use2ndpilots = (frame_parms->nb_antenna_ports_eNB==1)?1:0;

  uint32_t tti_offset; //,aa;
  uint8_t re;
  uint8_t qam64_table_offset_re = 0;
  uint8_t qam64_table_offset_im = 0;
  uint8_t qam16_table_offset_re = 0;
  uint8_t qam16_table_offset_im = 0;
  uint8_t qam64_table_offset_re0 = 0;
  uint8_t qam64_table_offset_im0 = 0;
  uint8_t qam16_table_offset_re0 = 0;
  uint8_t qam16_table_offset_im0 = 0;
  uint8_t qam64_table_offset_re1 = 0;
  uint8_t qam64_table_offset_im1 = 0;
  uint8_t qam16_table_offset_re1 = 0;
  uint8_t qam16_table_offset_im1 = 0;
  int16_t xx0_re,xx1_re;
  int16_t xx0_im,xx1_im;

  int16_t gain_lin_QPSK;//,gain_lin_16QAM1,gain_lin_16QAM2;
  int16_t re_off=re_offset;

  uint8_t first_re,last_re;
  int32_t tmp_sample1,tmp_sample2;
  int16_t tmp_amp=amp;
  int s=1;
  int mprime2 = mprime,ind,ind_dword,ind_qpsk_symb;

  gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);

  int32_t qpsk[4];
  ((int16_t *)&qpsk[0])[0] = gain_lin_QPSK;
  ((int16_t *)&qpsk[0])[1] = gain_lin_QPSK;
  ((int16_t *)&qpsk[1])[0] = -gain_lin_QPSK;
  ((int16_t *)&qpsk[1])[1] = gain_lin_QPSK;;
  ((int16_t *)&qpsk[2])[0] = gain_lin_QPSK;;
  ((int16_t *)&qpsk[2])[1] = -gain_lin_QPSK;;
  ((int16_t *)&qpsk[3])[0] = -gain_lin_QPSK;;
  ((int16_t *)&qpsk[3])[1] = -gain_lin_QPSK;

  if ((dlsch0_harq != NULL) && (dlsch1_harq != NULL)) { //this is for TM3, TM4

    x0 = dlsch0_harq->eDL;
    mimo_mode = dlsch0_harq->mimo_mode;
    first_layer0 = dlsch0_harq->first_layer;
    Nlayers0 = dlsch0_harq->Nlayers;
    mod_order0 = dlsch0_harq->Qm;
    x1             = dlsch1_harq->eDL;
    // Fill these in later for TM8-10
    //    Nlayers1       = dlsch1_harq->Nlayers;
    //    first_layer1   = dlsch1_harq->first_layer;
    mod_order1     = dlsch1_harq->Qm;

  } else if ((dlsch0_harq != NULL) && (dlsch1_harq == NULL)){ //This is for SIS0 TM1, TM6, etc

    x0 = dlsch0_harq->eDL;
    mimo_mode = dlsch0_harq->mimo_mode;
    first_layer0 = dlsch0_harq->first_layer;
    Nlayers0 = dlsch0_harq->Nlayers;
    mod_order0 = dlsch0_harq->Qm;

  } else if ((dlsch0_harq == NULL) && (dlsch1_harq != NULL)){ // This is for TM4 retransmission

    x0 = dlsch1_harq->eDL;
    mimo_mode = dlsch1_harq->mimo_mode;
    first_layer0 = dlsch1_harq->first_layer;
    Nlayers0 = dlsch1_harq->Nlayers;
    mod_order0 = dlsch1_harq->Qm;

  }

  if (dlsch0_harq != NULL){
    #ifdef DEBUG_DLSCH_MODULATION
      LOG_I(PHY,"allocate_re (mod %d): symbol_offset %d re_offset %d (%d,%d), jj %d -> %d,%d\n",mod_order0,symbol_offset,re_offset,skip_dc,skip_half,*jj, x0[*jj], x0[1+*jj]);
    #endif
  } else{
    #ifdef DEBUG_DLSCH_MODULATION
      LOG_I(PHY,"allocate_re (mod %d): symbol_offset %d re_offset %d (%d,%d), jj %d -> %d,%d\n",mod_order0,symbol_offset,re_offset,skip_dc,skip_half,*jj, x0[*jj], x0[1+*jj]);
    #endif
  }

  first_re=0;
  last_re=12;

  if (skip_half==1)
    last_re=6;
  else if (skip_half==2)
    first_re=6;


  for (re=first_re; re<last_re; re++) {
  // LOG_I(PHY,"element %d precoder_index for allocation %d\n",re, precoder_index );

    if ((skip_dc == 1) && (re==6))
      re_off=re_off - frame_parms->ofdm_symbol_size+1;

    tti_offset = symbol_offset + re_off + re;

      //check that RE is not from Cell-specific RS

    if (is_not_pilot(pilots,re,frame_parms->nushift,use2ndpilots)==1) {
      //LOG_I(PHY,"re %d (jj %d)\n",re,*jj);


      if (mimo_mode == SISO) {  //SISO mapping
        *re_allocated = *re_allocated + 1;

        switch (mod_order0) {
        case 2:  //QPSK


          //LOG_I(PHY,"re %d %d(%d) : %d,%d => ",re,tti_offset,*jj,((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
          ((int16_t*)&txdataF[0][tti_offset])[0] += (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i

          *jj = *jj + 1;

          ((int16_t*)&txdataF[0][tti_offset])[1] += (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}

          *jj = *jj + 1;

          break;

        case 4:  //16QAM

          qam16_table_offset_re = 0;
          qam16_table_offset_im = 0;

          if (x0[*jj] == 1)
            qam16_table_offset_re+=2;

          *jj=*jj+1;

          if (x0[*jj] == 1)
            qam16_table_offset_im+=2;

          *jj=*jj+1;


          if (x0[*jj] == 1)
            qam16_table_offset_re+=1;

          *jj=*jj+1;

          if (x0[*jj] == 1)
            qam16_table_offset_im+=1;

          *jj=*jj+1;

          ((int16_t *)&txdataF[0][tti_offset])[0]+=qam_table_s0[qam16_table_offset_re];
          ((int16_t *)&txdataF[0][tti_offset])[1]+=qam_table_s0[qam16_table_offset_im];

          break;

        case 6:  //64QAM


          qam64_table_offset_re = 0;
          qam64_table_offset_im = 0;

          if (x0[*jj] == 1)
            qam64_table_offset_re+=4;

          *jj=*jj+1;

          if (x0[*jj] == 1)
            qam64_table_offset_im+=4;

          *jj=*jj+1;

          if (x0[*jj] == 1)
            qam64_table_offset_re+=2;

          *jj=*jj+1;

          if (x0[*jj] == 1)
            qam64_table_offset_im+=2;

          *jj=*jj+1;

          if (x0[*jj] == 1)
            qam64_table_offset_re+=1;

          *jj=*jj+1;

          if (x0[*jj] == 1)
            qam64_table_offset_im+=1;

          *jj=*jj+1;

          ((int16_t *)&txdataF[0][tti_offset])[0]+=qam_table_s0[qam64_table_offset_re];
          ((int16_t *)&txdataF[0][tti_offset])[1]+=qam_table_s0[qam64_table_offset_im];

          break;

        }
      }

      else if (mimo_mode == ALAMOUTI) {
        *re_allocated = *re_allocated + 1;

        amp = (int16_t)(((int32_t)tmp_amp*ONE_OVER_SQRT2_Q15)>>15);


        switch (mod_order0) {
        case 2:  //QPSK

          // first antenna position n -> x0

          ((int16_t*)&tmp_sample1)[0] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
          *jj=*jj+1;
          ((int16_t*)&tmp_sample1)[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
          *jj=*jj+1;

          // second antenna position n -> -x1*

          ((int16_t*)&tmp_sample2)[0] = (x0[*jj]==1) ? (gain_lin_QPSK) : -gain_lin_QPSK;
          *jj=*jj+1;
          ((int16_t*)&tmp_sample2)[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
          *jj=*jj+1;

	  //gain_lin_QPSK (=amp/sqrt(2)) is already contains the power offset from rho_a/rho_b, so here we do not need divide by sqrt(2) anymore
          // normalization for 2 tx antennas
          ((int16_t*)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]));
          ((int16_t*)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]));
          ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]));
          ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]));

        break;

        case 4:  //16QAM

          // Antenna 0 position n
          qam16_table_offset_re = 0;
          qam16_table_offset_im = 0;

          if (x0[*jj] == 1)
            qam16_table_offset_re+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam16_table_offset_im+=2;
          *jj=*jj+1;


          if (x0[*jj] == 1)
            qam16_table_offset_re+=1;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam16_table_offset_im+=1;
          *jj=*jj+1;

          //((int16_t *)&txdataF[0][tti_offset])[0]+=(int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_re])>>15);
          //((int16_t *)&txdataF[0][tti_offset])[1]+=(int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_im])>>15);
	  //gain_lin_QPSK (=amp/sqrt(2)) is already contains the power offset from rho_a/rho_b, so here we do not need divide by sqrt(2) anymore
	  ((int16_t *)&txdataF[0][tti_offset])[0]+=(qam_table_s0[qam16_table_offset_re]);
	  ((int16_t *)&txdataF[0][tti_offset])[1]+=(qam_table_s0[qam16_table_offset_im]);

          // Antenna 1 position n Real part -> -x1*

          qam16_table_offset_re = 0;
          qam16_table_offset_im = 0;

          if (x0[*jj] == 1)
            qam16_table_offset_re+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam16_table_offset_im+=2;
          *jj=*jj+1;


          if (x0[*jj] == 1)
            qam16_table_offset_re+=1;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam16_table_offset_im+=1;
          *jj=*jj+1;

          //((int16_t *)&txdataF[1][tti_offset])[0]+=-(int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_re])>>15);
          //((int16_t *)&txdataF[1][tti_offset])[1]+=(int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_im])>>15);
	  //qam_table_s0 already contains the power offset from rho_a/rho_b, so here we do not need divide by sqrt(2) anymore
	  ((int16_t *)&txdataF[1][tti_offset])[0]+=-(qam_table_s0[qam16_table_offset_re]);
	  ((int16_t *)&txdataF[1][tti_offset])[1]+=(qam_table_s0[qam16_table_offset_im]);

         //((int16_t *)&txdataF[1][tti_offset])[0]+=-qam_table_s0[qam16_table_offset_re];
         //((int16_t *)&txdataF[1][tti_offset])[1]+=qam_table_s0[qam16_table_offset_im];

          break;

        case 6:   // 64-QAM

          // Antenna 0
          qam64_table_offset_re = 0;
          qam64_table_offset_im = 0;

          if (x0[*jj] == 1)
            qam64_table_offset_re+=4;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=4;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=1;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=1;
          *jj=*jj+1;

          //((int16_t *)&txdataF[0][tti_offset])[0]+=(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_re])>>15);
          //((int16_t *)&txdataF[0][tti_offset])[1]+=(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_im])>>15);
	  //qam_table_s0 already contains the power offset from rho_a/rho_b, so here we do not need divide by sqrt(2) anymore
	  ((int16_t *)&txdataF[0][tti_offset])[0]+=(qam_table_s0[qam64_table_offset_re]);
	  ((int16_t *)&txdataF[0][tti_offset])[1]+=(qam_table_s0[qam64_table_offset_im]);


          // Antenna 1 => -x1*
          qam64_table_offset_re = 0;
          qam64_table_offset_im = 0;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=4;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=4;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=1;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=1;
          *jj=*jj+1;

          //((int16_t *)&txdataF[1][tti_offset])[0]+=-(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_re])>>15);
          //((int16_t *)&txdataF[1][tti_offset])[1]+=(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_im])>>15);
	  //qam_table_s0 already contains the power offset from rho_a/rho_b, so here we do not need divide by sqrt(2) anymore
	  ((int16_t *)&txdataF[1][tti_offset])[0]+=-(qam_table_s0[qam64_table_offset_re]);
	  ((int16_t *)&txdataF[1][tti_offset])[1]+=(qam_table_s0[qam64_table_offset_im]);


          break;
        }
  // fill in the rest of the ALAMOUTI precoding
        if (is_not_pilot(pilots,re + 1,frame_parms->nushift,use2ndpilots)==1) {
          ((int16_t *)&txdataF[0][tti_offset+1])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
          ((int16_t *)&txdataF[0][tti_offset+1])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
          ((int16_t *)&txdataF[1][tti_offset+1])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
          ((int16_t *)&txdataF[1][tti_offset+1])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
        }
        else {
          ((int16_t *)&txdataF[0][tti_offset+2])[0] += -((int16_t *)&txdataF[1][tti_offset])[0]; //x1
          ((int16_t *)&txdataF[0][tti_offset+2])[1] += ((int16_t *)&txdataF[1][tti_offset])[1];
          ((int16_t *)&txdataF[1][tti_offset+2])[0] += ((int16_t *)&txdataF[0][tti_offset])[0];  //x0*
          ((int16_t *)&txdataF[1][tti_offset+2])[1] += -((int16_t *)&txdataF[0][tti_offset])[1];
        }
      }
      else if (mimo_mode == LARGE_CDD) {

        *re_allocated = *re_allocated + 1;

        if (frame_parms->nb_antenna_ports_eNB == 2) {
          switch (mod_order0) {

          default:
            LOG_E(PHY,"Unknown mod_order0 %d\n",mod_order0);
            xx0_re=xx0_im=0;
            break;

          case 2:  //QPSK
            //LOG_I(PHY,"%d(%d) : %d,%d => ",tti_offset,*jj,((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
            xx0_re = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
            *jj = *jj + 1;
            xx0_im = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
            *jj = *jj + 1;
            //LOG_I(PHY,"%d,%d\n",((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
            break;

          case 4:  //16QAM

            qam16_table_offset_re0 = 0;
            qam16_table_offset_im0 = 0;
            if (x0[*jj] == 1)
              qam16_table_offset_re0+=2;
            *jj=*jj+1;
            if (x0[*jj] == 1)
              qam16_table_offset_im0+=2;
            *jj=*jj+1;

            if (x0[*jj] == 1)
              qam16_table_offset_re0+=1;
            *jj=*jj+1;
            if (x0[*jj] == 1)
              qam16_table_offset_im0+=1;
            *jj=*jj+1;

            xx0_re = qam_table_s0[qam16_table_offset_re0];
            xx0_im = qam_table_s0[qam16_table_offset_im0];

            break;

          case 6:  //64QAM


            qam64_table_offset_re0 = 0;
            qam64_table_offset_im0 = 0;

            if (x0[*jj] == 1)
              qam64_table_offset_re0+=4;
            *jj=*jj+1;
            if (x0[*jj] == 1)
              qam64_table_offset_im0+=4;
            *jj=*jj+1;
            if (x0[*jj] == 1)
              qam64_table_offset_re0+=2;
            *jj=*jj+1;
            if (x0[*jj] == 1)
              qam64_table_offset_im0+=2;
            *jj=*jj+1;
            if (x0[*jj] == 1)
              qam64_table_offset_re0+=1;
            *jj=*jj+1;
            if (x0[*jj] == 1)
              qam64_table_offset_im0+=1;
            *jj=*jj+1;

            xx0_re = qam_table_s0[qam64_table_offset_re0];
            xx0_im = qam_table_s0[qam64_table_offset_im0];

            break;

          }

          switch (mod_order1) {

          default:
            LOG_E(PHY,"Unknown mod_order1 %d\n",mod_order1);
            xx1_re=xx1_im=0;
            break;

          case 2:  //QPSK
            //LOG_I(PHY,"%d(%d) : %d,%d => ",tti_offset,*jj,((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
            xx1_re = (x1[*jj2]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
            *jj2 = *jj2 + 1;
            xx1_im = (x1[*jj2]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
            *jj2 = *jj2 + 1;
            //LOG_I(PHY,"%d,%d\n",((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
            break;

          case 4:  //16QAM

            qam16_table_offset_re1 = 0;
            qam16_table_offset_im1 = 0;
            if (x1[*jj2] == 1)
              qam16_table_offset_re1+=2;
            *jj2 = *jj2 + 1;
            if (x1[*jj2] == 1)
              qam16_table_offset_im1+=2;
            *jj2 = *jj2 + 1;
            if (x1[*jj2] == 1)
              qam16_table_offset_re1+=1;
            *jj2 = *jj2 + 1;
            if (x1[*jj2] == 1)
              qam16_table_offset_im1+=1;
            *jj2 = *jj2 + 1;

            xx1_re = qam_table_s1[qam16_table_offset_re1];
            xx1_im = qam_table_s1[qam16_table_offset_im1];

            break;

          case 6:  //64QAM

            qam64_table_offset_re1 = 0;
            qam64_table_offset_im1 = 0;

            if (x1[*jj2] == 1)
              qam64_table_offset_re1+=4;
            *jj2 = *jj2 + 1;
            if (x1[*jj2] == 1)
              qam64_table_offset_im1+=4;
            *jj2 = *jj2 + 1;
            if (x1[*jj2] == 1)
              qam64_table_offset_re1+=2;
            *jj2 = *jj2 + 1;
            if (x1[*jj2] == 1)
              qam64_table_offset_im1+=2;
            *jj2 = *jj2 + 1;
            if (x1[*jj2] == 1)
              qam64_table_offset_re1+=1;
            *jj2 = *jj2 + 1;
            if (x1[*jj2] == 1)
              qam64_table_offset_im1+=1;
            *jj2 = *jj2 + 1;

            xx1_re = qam_table_s1[qam64_table_offset_re1];
            xx1_im = qam_table_s1[qam64_table_offset_im1];

            break;

          }

          // This implements the Large CDD precoding for 2 TX antennas
          // -  -        -    -  -         -  -     -  -  -       -              -
          //| y0 |      | 1  0 || 1    0    || 1   1 || x0 |     |        x0 + x1 |
          //| y1 | = .5 | 0  1 || 0  (-1)^i || 1  -1 || x1 | = .5| (-1)^i(x0 - x1)|
          // -  -        -    -  -         -  -     -  -  -       -
          // Note: Factor .5 is accounted for in amplitude when calling this function
          ((int16_t *)&txdataF[0][tti_offset])[0]+=((xx0_re+xx1_re)>>1);
          ((int16_t *)&txdataF[1][tti_offset])[0]+=(s*((xx0_re-xx1_re)>>1));
          ((int16_t *)&txdataF[0][tti_offset])[1]+=((xx0_im+xx1_im)>>1);
          ((int16_t *)&txdataF[1][tti_offset])[1]+=(s*((xx0_im-xx1_im)>>1));
          /*
          LOG_I(PHY,"CDD: xx0 (%d,%d), xx1(%d,%d), s(%d), txF[0] (%d,%d), txF[1] (%d,%d)\n",
           xx0_re,xx0_im,xx1_re,xx1_im, s, ((int16_t *)&txdataF[0][tti_offset])[0],((int16_t *)&txdataF[0][tti_offset])[1],
           ((int16_t *)&txdataF[1][tti_offset])[0],((int16_t *)&txdataF[1][tti_offset])[1]);
          */
          // s alternates +1/-1 for each RE
          s = -s;
        }
      }
      else if ((mimo_mode >= UNIFORM_PRECODING11)&&(mimo_mode <= PUSCH_PRECODING1)) {
  // this is for transmission modes 5-6 (1 layer)
        *re_allocated = *re_allocated + 1;
        amp = (int16_t)(((int32_t)tmp_amp*ONE_OVER_SQRT2_Q15)>>15);

        switch (mod_order0) {
        case 2:

          ((int16_t*)&tmp_sample1)[0] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
          *jj = *jj + 1;
          ((int16_t*)&tmp_sample1)[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
          *jj = *jj + 1;

          //normalization for 2 tx antennas
          /*((int16_t*)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
          ((int16_t*)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);

          if (frame_parms->nb_antenna_ports_eNB == 2) {
            layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index);
            ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
            ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
          }*/

          // We remove ONE_OVER_SQRT2_Q15 that was coming from precoder, as now it applied in computation sqrt_rho_a, sqrt_rho_b, same in the receiver in precoder function

          ((int16_t*)&txdataF[0][tti_offset])[0] += ((int16_t*)&tmp_sample1)[0];
          ((int16_t*)&txdataF[0][tti_offset])[1] += ((int16_t*)&tmp_sample1)[1];

          if (frame_parms->nb_antenna_ports_eNB == 2) {
            layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index);
            ((int16_t*)&txdataF[1][tti_offset])[0] += ((int16_t*)&tmp_sample2)[0];
            ((int16_t*)&txdataF[1][tti_offset])[1] += ((int16_t*)&tmp_sample2)[1];
          }

          break;

        case 4:

          qam16_table_offset_re = 0;
          qam16_table_offset_im = 0;

          if (x0[*jj] == 1)
            qam16_table_offset_re+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam16_table_offset_im+=2;
          *jj=*jj+1;


          if (x0[*jj] == 1)
            qam16_table_offset_re+=1;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam16_table_offset_im+=1;
          *jj=*jj+1;


           ((int16_t*)&tmp_sample1)[0] = (int16_t)((qam_table_s0[qam16_table_offset_re]));
           ((int16_t*)&tmp_sample1)[1] = (int16_t)((qam_table_s0[qam16_table_offset_im]));

           ((int16_t *)&txdataF[0][tti_offset])[0] += ((int16_t*)&tmp_sample1)[0];
           ((int16_t *)&txdataF[0][tti_offset])[1] += ((int16_t*)&tmp_sample1)[1];

          if (frame_parms->nb_antennas_tx == 2) {
            layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index);
            ((int16_t*)&txdataF[1][tti_offset])[0] += ((int16_t*)&tmp_sample2)[0];
            ((int16_t*)&txdataF[1][tti_offset])[1] += ((int16_t*)&tmp_sample2)[1];
          }

          break;

        case 6:

          qam64_table_offset_re = 0;
          qam64_table_offset_im = 0;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=4;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=4;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=1;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=1;
          *jj=*jj+1;

          ((int16_t*)&tmp_sample1)[0] = (int16_t)((qam_table_s0[qam64_table_offset_re]));
          ((int16_t*)&tmp_sample1)[1] = (int16_t)((qam_table_s0[qam64_table_offset_im]));

          ((int16_t *)&txdataF[0][tti_offset])[0] += ((int16_t*)&tmp_sample1)[0];
          ((int16_t *)&txdataF[0][tti_offset])[1] += ((int16_t*)&tmp_sample1)[1];

          if (frame_parms->nb_antenna_ports_eNB == 2) {
            layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index);
            ((int16_t*)&txdataF[1][tti_offset])[0] += ((int16_t*)&tmp_sample2)[0];
            ((int16_t*)&txdataF[1][tti_offset])[1] += ((int16_t*)&tmp_sample2)[1];
          }

          break;
        }
      }
      else if ((mimo_mode >= DUALSTREAM_UNIFORM_PRECODING1)&&(mimo_mode <= DUALSTREAM_PUSCH_PRECODING)) {
  // this is for transmission mode 4 (1 layer)
        *re_allocated = *re_allocated + 1;

        if (precoder_index==0) {
          precoder_index0 = 0; //[1 1]
          precoder_index1 = 1; //[1 -1]
        }
        else if (precoder_index==1) {
          precoder_index0 = 2; //[1 j]
          precoder_index1 = 3; //[1 -j]
        }
        else {
         LOG_I(PHY,"problem with precoder in TM4\n");
          return(-1);
        }

        switch (mod_order0) {
        case 2:

          ((int16_t*)&tmp_sample1)[0] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
          *jj = *jj + 1;
          ((int16_t*)&tmp_sample1)[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
          *jj = *jj + 1;

          //normalization for 2 tx antennas
          // We remove ONE_OVER_2_Q15 that was coming from precoder,
          //as now it applied in computation sqrt_rho_a, sqrt_rho_b, same in the receiver in precoder function
          //ONE_OVER_SQRT2_Q15 remains from it
          ((int16_t*)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
          ((int16_t*)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);

          //LOG_I(PHY,"%d,%d\n",((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);

          if (frame_parms->nb_antenna_ports_eNB == 2) {
            layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index0);
            ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
            ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
          }

        break;

        case 4:

          qam16_table_offset_re = 0;
          qam16_table_offset_im = 0;

          if (x0[*jj] == 1)
            qam16_table_offset_re+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam16_table_offset_im+=2;
          *jj=*jj+1;

          if (x0[*jj] == 1)
            qam16_table_offset_re+=1;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam16_table_offset_im+=1;
          *jj=*jj+1;

           ((int16_t*)&tmp_sample1)[0] = (int16_t)((qam_table_s0[qam16_table_offset_re]));
           ((int16_t*)&tmp_sample1)[1] = (int16_t)((qam_table_s0[qam16_table_offset_im]));

           // We remove ONE_OVER_2_Q15 that was coming from precoder,
           //as now it applied in computation sqrt_rho_a, sqrt_rho_b, same in the receiver in precoder function
           //ONE_OVER_SQRT2_Q15 remains from it
           ((int16_t *)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
           ((int16_t *)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);

          if (frame_parms->nb_antenna_ports_eNB == 2) {
            layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index0);
            ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
            ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
          }

          break;

        case 6:

          qam64_table_offset_re = 0;
          qam64_table_offset_im = 0;

          if (x0[*jj] == 1)
            qam64_table_offset_re+=4;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=4;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=2;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_re+=1;
          *jj=*jj+1;
          if (x0[*jj] == 1)
            qam64_table_offset_im+=1;
          *jj=*jj+1;

          ((int16_t*)&tmp_sample1)[0] = (int16_t)((qam_table_s0[qam64_table_offset_re]));
          ((int16_t*)&tmp_sample1)[1] = (int16_t)((qam_table_s0[qam64_table_offset_im]));
          ((int16_t *)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
          ((int16_t *)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);

          if (frame_parms->nb_antennas_tx == 2) {
            layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index0);
            ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
            ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
          }

          break;
        }

        if (dlsch1_harq) {
          switch (mod_order1) {

          case 2:

            ((int16_t*)&tmp_sample1)[0] = (x1[*jj2]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
            *jj2 = *jj2 + 1;
            ((int16_t*)&tmp_sample1)[1] = (x1[*jj2]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
            *jj2 = *jj2 + 1;

            //normalization for 2 tx antennas
            ((int16_t*)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
            ((int16_t*)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);

            if (frame_parms->nb_antenna_ports_eNB== 2) {
              layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index1);
              ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
              ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
            }

            break;

          case 4:

            qam16_table_offset_re = 0;
            qam16_table_offset_im = 0;

            if (x1[*jj2] == 1)
              qam16_table_offset_re+=2;
            *jj2=*jj2+1;
            if (x1[*jj2] == 1)
              qam16_table_offset_im+=2;
            *jj2=*jj2+1;


            if (x1[*jj2] == 1)
              qam16_table_offset_re+=1;
            *jj2=*jj2+1;
            if (x1[*jj2] == 1)
              qam16_table_offset_im+=1;
            *jj2=*jj2+1;

             ((int16_t*)&tmp_sample1)[0] = (int16_t)((qam_table_s1[qam16_table_offset_re]));
             ((int16_t*)&tmp_sample1)[1] = (int16_t)((qam_table_s1[qam16_table_offset_im]));
             ((int16_t *)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
             ((int16_t *)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);

            if (frame_parms->nb_antenna_ports_eNB == 2) {
              layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index1);
              ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
              ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
            }

            break;

          case 6:

            qam64_table_offset_re = 0;
            qam64_table_offset_im = 0;
            if (x1[*jj2] == 1)
              qam64_table_offset_re+=4;
            *jj2=*jj2+1;
            if (x1[*jj2] == 1)
              qam64_table_offset_im+=4;
            *jj2=*jj2+1;
            if (x1[*jj2] == 1)
              qam64_table_offset_re+=2;
            *jj2=*jj2+1;
            if (x1[*jj2] == 1)
              qam64_table_offset_im+=2;
            *jj2=*jj2+1;
            if (x1[*jj2] == 1)
              qam64_table_offset_re+=1;
            *jj2=*jj2+1;
            if (x1[*jj2] == 1)
              qam64_table_offset_im+=1;
            *jj2=*jj2+1;

            ((int16_t*)&tmp_sample1)[0] = (int16_t)((qam_table_s1[qam64_table_offset_re]));
            ((int16_t*)&tmp_sample1)[1] = (int16_t)((qam_table_s1[qam64_table_offset_im]));
            ((int16_t *)&txdataF[0][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample1)[0]*ONE_OVER_SQRT2_Q15)>>15);
            ((int16_t *)&txdataF[0][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample1)[1]*ONE_OVER_SQRT2_Q15)>>15);

            if (frame_parms->nb_antenna_ports_eNB == 2) {
              layer1prec2A(&tmp_sample1,&tmp_sample2,precoder_index1);
              ((int16_t*)&txdataF[1][tti_offset])[0] += (int16_t)((((int16_t*)&tmp_sample2)[0]*ONE_OVER_SQRT2_Q15)>>15);
              ((int16_t*)&txdataF[1][tti_offset])[1] += (int16_t)((((int16_t*)&tmp_sample2)[1]*ONE_OVER_SQRT2_Q15)>>15);
            }

            break;

          }
        }
      }


      if (mimo_mode == ALAMOUTI) {
        re++;  // adjacent carriers are taken care of by precoding
        *re_allocated = *re_allocated + 1;

        if (is_not_pilot(pilots,re,frame_parms->nushift,use2ndpilots)==0) {
          re++;
          *re_allocated = *re_allocated + 1;
        }
      }

      if (mimo_mode == TM7) {
        *re_allocated = *re_allocated + 1;

        if (is_not_UEspecRS(lprime,re,frame_parms->Nid_cell%3,frame_parms->Ncp,7)) {

          switch (mod_order0){
            case 2:  //QPSK

              ((int16_t*)&txdataF[5][tti_offset])[0] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
              *jj = *jj + 1;
              ((int16_t*)&txdataF[5][tti_offset])[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK;
              *jj = *jj + 1;

              //LOG_I(PHY,"%d(%d) : %d,%d =>
              //",tti_offset,*jj,((int16_t*)&tmp_sample1)[0],((int16_t*)&tmp_sample1)[1]);
              break;

            case 4:  //16QAM

              qam16_table_offset_re = 0;
              qam16_table_offset_im = 0;

              if (x0[*jj] == 1)
                qam16_table_offset_re+=2;

              *jj=*jj+1;

              if (x0[*jj] == 1)
                qam16_table_offset_im+=2;

              *jj=*jj+1;


              if (x0[*jj] == 1)
                qam16_table_offset_re+=1;

              *jj=*jj+1;

              if (x0[*jj] == 1)
                qam16_table_offset_im+=1;

              *jj=*jj+1;

              ((int16_t*)&txdataF[5][tti_offset])[0] = (int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_re])>>15);
              ((int16_t*)&txdataF[5][tti_offset])[1] = (int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_im])>>15);

              break;

            case 6:  //64QAM


              qam64_table_offset_re = 0;
              qam64_table_offset_im = 0;

              if (x0[*jj] == 1)
                qam64_table_offset_re+=4;

              *jj=*jj+1;

              if (x0[*jj] == 1)
                qam64_table_offset_im+=4;

              *jj=*jj+1;

              if (x0[*jj] == 1)
                qam64_table_offset_re+=2;

              *jj=*jj+1;

              if (x0[*jj] == 1)
                qam64_table_offset_im+=2;

              *jj=*jj+1;

              if (x0[*jj] == 1)
                qam64_table_offset_re+=1;

              *jj=*jj+1;

              if (x0[*jj] == 1)
                qam64_table_offset_im+=1;

              *jj=*jj+1;

              ((int16_t*)&txdataF[5][tti_offset])[0] = (int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_re])>>15);
              ((int16_t*)&txdataF[5][tti_offset])[1] = (int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_im])>>15);

              break;

          }
        } else {
          //precoding UE spec RS
          //LOG_I(PHY,"precoding UE spec RS\n");

          ind = 3*lprime*dlsch0_harq->nb_rb+mprime2;
          ind_dword = ind>>4;
          ind_qpsk_symb = ind&0xf;

          txdataF[5][tti_offset] = qpsk[(phy_vars_eNB->lte_gold_uespec_port5_table[0][Ns][ind_dword]>>(2*ind_qpsk_symb))&3];
          mprime2++;

        }

      } else if (mimo_mode >= TM8) { //TM8,TM9,TM10
        //uint8_t is_not_UEspecRS(int8_t lprime, uint8_t re, uint8_t nushift, uint8_t Ncp, uint8_t beamforming_mode)

        if (is_not_UEspecRS(lprime,re,frame_parms->nushift,frame_parms->Ncp,8)) {
          switch (mod_order0) {
          case 2:  //QPSK

            //    LOG_I(PHY,"%d : %d,%d => ",tti_offset,((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
            for (int layer=first_layer0; layer<=(first_layer0+Nlayers0); layer++) {
              ((int16_t*)&txdataF[layer][tti_offset])[0] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i
              *jj = *jj + 1;
              ((int16_t*)&txdataF[layer][tti_offset])[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}
              *jj = *jj + 1;
            }

            break;

          case 4:  //16QAM
            if (is_not_UEspecRS(lprime,re,frame_parms->nushift,frame_parms->Ncp,8)) {
              qam16_table_offset_re = 0;
              qam16_table_offset_im = 0;

              if (x0[*jj] == 1)
                qam16_table_offset_re+=2;

              *jj = *jj + 1;

              if (x0[*jj] == 1)
                qam16_table_offset_im+=2;

              *jj = *jj + 1;

              if (x0[*jj] == 1)
                qam16_table_offset_re+=1;

              *jj = *jj + 1;

              if (x0[*jj] == 1)
                qam16_table_offset_im+=1;

              *jj = *jj + 1;

              for (int layer=first_layer0; layer<=(first_layer0+Nlayers0); layer++) {
                ((int16_t*)&txdataF[layer][tti_offset])[0] = qam_table_s0[qam16_table_offset_re];
                ((int16_t*)&txdataF[layer][tti_offset])[1] = qam_table_s0[qam16_table_offset_im];
              }
            }

            break;

          case 6:  //64QAM

            qam64_table_offset_re = 0;
            qam64_table_offset_im = 0;

            if (x0[*jj] == 1)
              qam64_table_offset_re+=4;

            *jj = *jj + 1;

            if (x0[*jj] == 1)
              qam64_table_offset_im+=4;

            *jj = *jj + 1;

            if (x0[*jj] == 1)
              qam64_table_offset_re+=2;

            *jj = *jj + 1;

            if (x0[*jj] == 1)
              qam64_table_offset_im+=2;

            *jj = *jj + 1;

            if (x0[*jj] == 1)
              qam64_table_offset_re+=1;

            *jj = *jj + 1;

            if (x0[*jj] == 1)
              qam64_table_offset_im+=1;

            *jj = *jj + 1;

            for (int layer=first_layer0; layer<=(first_layer0+Nlayers0); layer++) {
              ((int16_t*)&txdataF[layer][tti_offset])[0] = qam_table_s0[qam64_table_offset_re];
              ((int16_t*)&txdataF[layer][tti_offset])[1] = qam_table_s0[qam64_table_offset_im];
            }

            break;

          }
        }
      } else if (mimo_mode>=TM9_10) {
        LOG_I(PHY,"allocate_REs_in_RB() [dlsch.c] : ERROR, unknown mimo_mode %d\n",mimo_mode);
        return(-1);
      }
    }
  }
  return(0);
}


int allocate_REs_in_RB_MCH(int32_t **txdataF,
                           uint32_t *jj,
                            uint16_t re_offset,
                            uint32_t symbol_offset,
                            uint8_t *x0,
                            uint8_t l,
                            uint8_t mod_order,
                            int16_t amp,
                            int16_t *qam_table_s,
                            uint32_t *re_allocated,
                            uint8_t skip_dc,
                            LTE_DL_FRAME_PARMS *frame_parms)
{

  uint32_t tti_offset;
  uint8_t re;
  uint8_t qam64_table_offset_re = 0;
  uint8_t qam64_table_offset_im = 0;
  uint8_t qam16_table_offset_re = 0;
  uint8_t qam16_table_offset_im = 0;
  int16_t gain_lin_QPSK;//,gain_lin_16QAM1,gain_lin_16QAM2;
  int16_t re_off=re_offset;
  gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);
  uint8_t first_re,last_re;
  int inc;
#ifdef DEBUG_DLSCH_MODULATION
  LOG_I(PHY,"allocate_re_MCH (mod %d): symbol_offset %d re_offset %d (%d), jj %d -> %d,%d, gain_lin_QPSK %d,txdataF %p\n",mod_order,symbol_offset,re_offset,skip_dc,*jj, x0[*jj], x0[1+*jj],gain_lin_QPSK,&txdataF[0][symbol_offset]);
#endif

  last_re=12;
  first_re=0;
  inc=1;

  if ((l==2)||(l==10)) {
    inc=2;
    first_re=1;
  } else if (l==6) {
    inc=2;
  }

  for (re=first_re; re<last_re; re+=inc) {

    if ((skip_dc == 1) && (re==(6+first_re)))
      re_off=re_off - frame_parms->ofdm_symbol_size+1;

    tti_offset = symbol_offset + re_off + re;

    //LOG_I(PHY,"re %d (jj %d)\n",re,*jj);
    *re_allocated = *re_allocated + 1;


    switch (mod_order) {

      case 2:  //QPSK

      //            LOG_I(PHY,"%d : %d,%d => ",tti_offset,((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
      ((int16_t*)&txdataF[0][tti_offset])[0] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i

      *jj = *jj + 1;

      ((int16_t*)&txdataF[0][tti_offset])[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}

      *jj = *jj + 1;

      //LOG_I(PHY,"%d,%d\n",((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
      break;


      case 4:  //16QAM

    if (qam_table_s == NULL) {
      LOG_E(PHY,"qam table pointer is NULL\n");
      return -1;
    }
    qam16_table_offset_re = 0;
    qam16_table_offset_im = 0;

    if (x0[*jj] == 1)
      qam16_table_offset_re+=2;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam16_table_offset_im+=2;

    *jj=*jj+1;


    if (x0[*jj] == 1)
      qam16_table_offset_re+=1;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam16_table_offset_im+=1;

    *jj=*jj+1;


    ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s[qam16_table_offset_re];//(int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_re])>>15);
    ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s[qam16_table_offset_im];//(int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_im])>>15);

    break;

      case 6:  //64QAM

    qam64_table_offset_re = 0;
    qam64_table_offset_im = 0;

    if (x0[*jj] == 1)
      qam64_table_offset_re+=4;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_im+=4;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_re+=2;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_im+=2;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_re+=1;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_im+=1;

    *jj=*jj+1;

    ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s[qam64_table_offset_re];//(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_re])>>15);
    ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s[qam64_table_offset_im];//(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_im])>>15);

    break;
    default:
        LOG_E(PHY,"Invalid modulation order %i_n",mod_order);
    break;
    }
  }

  return(0);
}



inline int check_skip(int rb,int subframe_offset,LTE_DL_FRAME_PARMS *frame_parms,int l,int nsymb) __attribute__((always_inline));
inline int check_skip(int rb,int subframe_offset,LTE_DL_FRAME_PARMS *frame_parms,int l,int nsymb) {


  if ((frame_parms->N_RB_DL&1) == 1) { // ODD N_RB_DL
    // PBCH
    if ((subframe_offset==0) &&
        (rb>((frame_parms->N_RB_DL>>1)-3)) &&
        (rb<((frame_parms->N_RB_DL>>1)+3)) &&
        (l>=(nsymb>>1)) &&
        (l<((nsymb>>1) + 4))) {
      return(1);
    }
    if (frame_parms->frame_type == TDD) { // TDD
            //SSS TDD
      if (((subframe_offset==0)||(subframe_offset==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==(nsymb-1)) ) {
        return(1);
      }
      //PSS TDD
      if (((subframe_offset==1) || (subframe_offset==6)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==2) ) {
        return(1);
      }
    } else {
      //PSS FDD
      if (((subframe_offset==0)||(subframe_offset==5)) &&
          (rb>((frame_parms->N_RB_DL>>1)-3)) &&
          (rb<((frame_parms->N_RB_DL>>1)+3)) &&
          (l==((nsymb>>1)-1)) ) {
        return(1);
      }
      //SSS FDD
      if (((subframe_offset==0)||(subframe_offset==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==((nsymb>>1)-2)) ) {
        return(1);
      }
    }
  }
  else { // even N_RB_DL
    //PBCH
    if ((subframe_offset==0) &&
        (rb>=((frame_parms->N_RB_DL>>1)-3)) &&
        (rb<((frame_parms->N_RB_DL>>1)+3)) &&
        (l>=nsymb>>1) && (l<((nsymb>>1) + 4)))
      return(1);

    if (frame_parms->frame_type == TDD) { // TDD
      //SSS
      if (((subframe_offset==0)||
           (subframe_offset==5)) &&
          (rb>=((frame_parms->N_RB_DL>>1)-3)) &&
          (rb<((frame_parms->N_RB_DL>>1)+3)) &&
          (l==nsymb-1) ) {
         return(1);
      }

      //PSS
      if (((subframe_offset==1)||
           (subframe_offset==6)) &&
          (rb>=((frame_parms->N_RB_DL>>1)-3)) &&
          (rb<((frame_parms->N_RB_DL>>1)+3)) &&
          (l==2) ) {
         return(1);
      }
    } else { // FDD
      //SSS
      if (((subframe_offset==0)||(subframe_offset==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==((nsymb>>1)-2)) ) {
         return(1);
      }

      //PSS
      if (((subframe_offset==0)||(subframe_offset==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==((nsymb>>1)-1)) ) {
         return(1);
      }
    }
  }

  return(0);
}

inline int check_skiphalf(int rb,int subframe_offset,LTE_DL_FRAME_PARMS *frame_parms,int l,int nsymb) __attribute__((always_inline));
inline int check_skiphalf(int rb,int subframe_offset,LTE_DL_FRAME_PARMS *frame_parms,int l,int nsymb) {

  //  LOG_I(PHY,"check_skiphalf : rb %d, subframe_offset %d,l %d, nsymb %d\n",rb,subframe_offset,l,nsymb);

  if ((frame_parms->N_RB_DL&1) == 1) { // ODD N_RB_DL

    // PBCH
    if ((subframe_offset==0) &&
        (rb==((frame_parms->N_RB_DL>>1)-3)) &&
        (l>=(nsymb>>1)) &&
        (l<((nsymb>>1) + 4)))
      return(1);
    else if ((subframe_offset==0) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
      return(2);

    if (frame_parms->frame_type == TDD) { // TDD
      //SSS TDD
      if (((subframe_offset==0)||(subframe_offset==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==(nsymb-1)))
        return(1);
      else if (((subframe_offset==0)||(subframe_offset==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==(nsymb-1)))
        return(2);
      //PSS TDD
      if (((subframe_offset==1)||(subframe_offset==6)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==2))
        return(1);
      else if (((subframe_offset==1)||(subframe_offset==6)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==2))
        return(2);
    }
    else { // FDD
      //PSS FDD
      if (((subframe_offset==0)||(subframe_offset==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==((nsymb>>1)-1)))
        return(1);
      else if (((subframe_offset==0)||(subframe_offset==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==(((nsymb>>1)-1))))
        return(2);
      //SSS FDD
      if (((subframe_offset==0)||(subframe_offset==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && ((l==((nsymb>>1)-2))))
        return(1);
      else if (((subframe_offset==0)||(subframe_offset==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && ((l==(nsymb>>1)-2)))
        return(2);
    }
  }
  else { // EVEN N_RB_DL
    return(0);
  }

  return(0);
}

inline int check_skip_dc(int rb,LTE_DL_FRAME_PARMS *frame_parms) __attribute__((always_inline));
inline int check_skip_dc(int rb,LTE_DL_FRAME_PARMS *frame_parms) {

  if (((frame_parms->N_RB_DL&1) == 1) &&  // odd N_RB_DL, rb==N_RB_DL/2 PRB contains DC element
      (rb==(frame_parms->N_RB_DL>>1)))
    return(1);
  else
    return(0);
}


int dlsch_modulation(PHY_VARS_eNB* phy_vars_eNB,
                     int32_t **txdataF,
                     int16_t amp,
                     int frame,
                     uint32_t subframe_offset,
                     uint8_t num_pdcch_symbols,
                     LTE_eNB_DLSCH_t *dlsch0,
                     LTE_eNB_DLSCH_t *dlsch1)
{
  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNB->frame_parms;

  uint8_t nsymb;
  uint8_t harq_pid = -1; //= dlsch0->current_harq_pid;
  LTE_DL_eNB_HARQ_t *dlsch0_harq = NULL;
  LTE_DL_eNB_HARQ_t *dlsch1_harq = NULL; //= dlsch1->harq_processes[harq_pid];
  uint32_t i,i2,jj,jj2,re_allocated,symbol_offset;
  uint16_t l,rb,re_offset;
  uint32_t rb_alloc_ind;
  uint32_t *rb_alloc = NULL; //=dlsch0_harq->rb_alloc;

  uint8_t pilots=0;
  uint8_t skip_dc,skip_half;
  uint8_t mod_order0 = 0;
  uint8_t mod_order1 = 0;
  int16_t amp_rho_a, amp_rho_b;
  int16_t qam16_table_a0[4],qam64_table_a0[8],qam16_table_b0[4],qam64_table_b0[8],qpsk_table_a0[2],qpsk_table_b0[2];
  int16_t qam16_table_a1[4],qam64_table_a1[8],qam16_table_b1[4],qam64_table_b1[8],qpsk_table_a1[2],qpsk_table_b1[2];

  int16_t *qam_table_s0=NULL,*qam_table_s1=NULL;
  int (*allocate_REs)(PHY_VARS_eNB*,
                      int **,
                      uint32_t*,
                      uint32_t*,
                      uint16_t,
                      uint32_t,
                      LTE_DL_eNB_HARQ_t *,
                      LTE_DL_eNB_HARQ_t *,
                      uint8_t,
                      int16_t,
                      uint8_t,
                      int16_t *,
                      int16_t *,
                      uint32_t *,
                      uint8_t,
                      uint8_t,
                      uint8_t,
                      uint8_t,
                      uint8_t,
                      int *,
                      int *);



  int P1_SHIFT[13],P2_SHIFT[13];
  int offset,nushiftmod3;

  uint8_t get_pmi_temp;

  MIMO_mode_t mimo_mode = -1;
  uint8_t mprime=0,Ns;
  int8_t  lprime=-1;


#ifdef DEBUG_DLSCH_MODULATION
  uint8_t Nl0;  //= dlsch0_harq->Nl;
  uint8_t Nl1;
#endif


  if ((dlsch0 != NULL) && (dlsch1 != NULL)){

    harq_pid = dlsch0->harq_ids[frame%2][subframe_offset];
    if((harq_pid < 0) || (harq_pid >= dlsch0->Mdlharq)) {
      LOG_E(PHY,"illegal harq_pid %d %s:%d\n", harq_pid, __FILE__, __LINE__);
      return(-1);
    }
    dlsch0_harq = dlsch0->harq_processes[harq_pid];
    mimo_mode = dlsch0_harq->mimo_mode;
    mod_order0 = dlsch0_harq->Qm;
    rb_alloc = dlsch0_harq->rb_alloc;
#ifdef DEBUG_DLSCH_MODULATION
    Nl0 = dlsch0_harq->Nl;
#endif

    dlsch1_harq = dlsch1->harq_processes[harq_pid];
    mod_order1 = dlsch1_harq->Qm;
#ifdef DEBUG_DLSCH_MODULATION
    Nl1 = dlsch1_harq->Nl;
#endif

  }else if ((dlsch0 != NULL) && (dlsch1 == NULL)){

    harq_pid = dlsch0->harq_ids[frame%2][subframe_offset];
    if((harq_pid < 0) || (harq_pid >= dlsch0->Mdlharq)) {
      LOG_E(PHY,"illegal harq_pid %d %s:%d\n", harq_pid, __FILE__, __LINE__);
      return(-1);
    }
    dlsch0_harq = dlsch0->harq_processes[harq_pid];
    mimo_mode = dlsch0_harq->mimo_mode;
    mod_order0 = dlsch0_harq->Qm;
    rb_alloc = dlsch0_harq->rb_alloc;
#ifdef DEBUG_DLSCH_MODULATION
    Nl0 = dlsch0_harq->Nl;
#endif

    dlsch1_harq = NULL;
    mod_order1 = 0;
#ifdef DEBUG_DLSCH_MODULATION
    Nl1 = 0;
#endif

  }else if ((dlsch0 == NULL) && (dlsch1 != NULL)){

    harq_pid = dlsch1->harq_ids[frame%2][subframe_offset];
    if((harq_pid < 0) || (harq_pid >= dlsch1->Mdlharq)) {
      LOG_E(PHY,"illegal harq_pid %d %s:%d\n", harq_pid, __FILE__, __LINE__);
      return(-1);
    }
    dlsch1_harq = dlsch1->harq_processes[harq_pid];
    mimo_mode = dlsch1_harq->mimo_mode;
    mod_order0 = dlsch1_harq->Qm;
    rb_alloc = dlsch1_harq->rb_alloc;
#ifdef DEBUG_DLSCH_MODULATION
    Nl0 = dlsch1_harq->Nl;
#endif

    dlsch0_harq = NULL;
    mod_order1 = 0;
#ifdef DEBUG_DLSCH_MODULATION
    Nl1 = NULL;
#endif

  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_MODULATION, VCD_FUNCTION_IN);

  nsymb = (frame_parms->Ncp==0) ? 14:12;

  if (dlsch0 != NULL){
  amp_rho_a = (int16_t)(((int32_t)amp*dlsch0->sqrt_rho_a)>>13); //amp=512 in  full scale; dlsch0->sqrt_rho_a=8192in Q2.13, 1 in full scale
  amp_rho_b = (int16_t)(((int32_t)amp*dlsch0->sqrt_rho_b)>>13);
  } else{
  amp_rho_a = (int16_t)(((int32_t)amp*dlsch1->sqrt_rho_a)>>13);
  amp_rho_b = (int16_t)(((int32_t)amp*dlsch1->sqrt_rho_b)>>13);
  }

  if(mod_order0 == 2)
  {
    for(i=0;i<2;i++)
    {
      qpsk_table_a0[i] = (int16_t)(((int32_t)qpsk_table[i]*amp_rho_a)>>15);
      qpsk_table_b0[i] = (int16_t)(((int32_t)qpsk_table[i]*amp_rho_b)>>15);
    }
  }
  else if (mod_order0 == 4)
    for (i=0;i<4; i++) {
      qam16_table_a0[i] = (int16_t)(((int32_t)qam16_table[i]*amp_rho_a)>>15);
      qam16_table_b0[i] = (int16_t)(((int32_t)qam16_table[i]*amp_rho_b)>>15);
    }
  else if (mod_order0 == 6)
    for (i=0; i<8; i++) {
      qam64_table_a0[i] = (int16_t)(((int32_t)qam64_table[i]*amp_rho_a)>>15);
      qam64_table_b0[i] = (int16_t)(((int32_t)qam64_table[i]*amp_rho_b)>>15);
    }

  if (mod_order1 == 2)
  {
    for (i=0; i<2; i++) {
      qpsk_table_a1[i] = (int16_t)(((int32_t)qpsk_table[i]*amp_rho_a)>>15);
      qpsk_table_b1[i] = (int16_t)(((int32_t)qpsk_table[i]*amp_rho_b)>>15);
    }
  }
  else if (mod_order1 == 4)
    for (i=0; i<4; i++) {
      qam16_table_a1[i] = (int16_t)(((int32_t)qam16_table[i]*amp_rho_a)>>15);
      qam16_table_b1[i] = (int16_t)(((int32_t)qam16_table[i]*amp_rho_b)>>15);
    }
  else if (mod_order1 == 6)
    for (i=0; i<8; i++) {
      qam64_table_a1[i] = (int16_t)(((int32_t)qam64_table[i]*amp_rho_a)>>15);
      qam64_table_b1[i] = (int16_t)(((int32_t)qam64_table[i]*amp_rho_b)>>15);
    }

  //Modulation mapping (difference w.r.t. LTE specs)

  jj=0;
  jj2=0;
  re_allocated=0;


  //  LOG_I(PHY,"num_pdcch_symbols %d, nsymb %d\n",num_pdcch_symbols,nsymb);
  for (l=num_pdcch_symbols; l<nsymb; l++) {

  if (dlsch0 != NULL ) {
     LOG_D(PHY,"Generating DLSCH (num_pdcch_symb %d harq_pid %d,mimo %d, pmi_alloc0 %lx, mod0 %d, mod1 %d, rb_alloc[0] %x)\n",
            num_pdcch_symbols,harq_pid,
            dlsch0_harq->mimo_mode,
            pmi2hex_2Ar2(dlsch0_harq->pmi_alloc),
            mod_order0,
            mod_order1,
            rb_alloc[0]
            );
  }

    if (frame_parms->Ncp==0) { // normal prefix
      if ((l==4)||(l==11))
        pilots=2;   // pilots in nushift+3, nushift+9
      else if (l==7)
        pilots=1;   // pilots in nushift, nushift+6
      else
        pilots=0;
    } else {
      if ((l==3)||(l==9))
        pilots=2;
      else if (l==6)
        pilots=1;
      else
        pilots=0;
    }

    if(mimo_mode==TM7){ //36.211 V8.6.0 2009-03
      mprime = 0;
      if (frame_parms->Ncp==0) { // normal prefix
        if (l==12)
          lprime=3;   // pilots in nushift+3, nushift+9
        else if (l==9)
          lprime=2;   // pilots in nushift, nushift+6
        else if (l==6)
          lprime=1;   // pilots in nushift+3, nushift+9
        else if (l==3)
          lprime=0;   // pilots in nushift, nushift+6
        else
          lprime=-1;
      } else {
        if (l==10)
          lprime=2;
        else if (l==7)
          lprime=1;
        else if (l==4)
          lprime=0;
        else
          lprime=-1;
      }

      // mapping ue specific beamforming weights from UE specified DLSCH structure to RU beam weights for the eNB
      /*
      for (int ru_id=0;ru_id<RC.nb_RU;ru_id++) {
        RU_t *ru;
	ru = RC.ru[ru_id];
	for (int eNB_id=0;eNB_id<ru->num_eNB;eNB_id++){
	  if (phy_vars_eNB == ru->eNB_list[eNB_id]) {
	    for (int aa=0;aa<ru->nb_tx;aa++){
              LOG_I(PHY,"ru_id:%d eNB_id:%d aa:%d memcpy(ru->beam_weights, dlsch0->ue_spec_bf_weights[ru_id][0],)\n", ru_id, eNB_id, aa);
	      memcpy(ru->beam_weights[eNB_id][5][aa],
		     dlsch0->ue_spec_bf_weights[ru_id][0],
		     frame_parms->ofdm_symbol_size*sizeof(int32_t));
	    }
	  }
	}
      }
      */

    }

    Ns = 2*subframe_offset+(l>=(nsymb>>1));

    offset = (pilots==2)?3:0;
    nushiftmod3 = frame_parms->nushift%3;

    if (pilots>0) {  // compute pilot arrays, could be done statically if performance suffers
      if (frame_parms->nb_antenna_ports_eNB == 1) {
	//	LOG_I(PHY,"l %d, nushift %d, offset %d\n",l,frame_parms->nushift,offset);
	for (i=0,i2=0;i<12;i++) {
	  if ((i!=(frame_parms->nushift+offset)) && (i!=((frame_parms->nushift+6+offset)%12)))
	    P1_SHIFT[i2++]=1;
	  else
	    P1_SHIFT[i2++]=2;
	}
	P1_SHIFT[0]--;
      }
      else {
        for (i=0,i2=0;i<12;i++) {
          if ((i!=nushiftmod3) && (i!=nushiftmod3+6) && (i!=nushiftmod3+3) && (i!=nushiftmod3+9))
            P2_SHIFT[i2++]=1;
          else
            P2_SHIFT[i2++]=2;
        }
        P2_SHIFT[0]--;
      }
    }
    P1_SHIFT[12]=1;P2_SHIFT[12]=1;

    re_offset = frame_parms->first_carrier_offset;
    symbol_offset = (uint32_t)frame_parms->ofdm_symbol_size*(l+(subframe_offset*nsymb));
    allocate_REs = allocate_REs_in_RB;

    switch (mod_order0) {
    case 2:
      qam_table_s0 = NULL;
      if (pilots) {
        qam_table_s0 = qpsk_table_b0;
        allocate_REs = (dlsch0->harq_processes[harq_pid]->mimo_mode == SISO) ?
          allocate_REs_in_RB_pilots_QPSK_siso :
          allocate_REs_in_RB;
      }
      else {
        qam_table_s0 = qpsk_table_a0;
        allocate_REs = (dlsch0->harq_processes[harq_pid]->mimo_mode == SISO) ?
          allocate_REs_in_RB_no_pilots_QPSK_siso :
          allocate_REs_in_RB;
      }
      break;
    case 4:
      if (pilots) {
        qam_table_s0 = qam16_table_b0;
        allocate_REs = (dlsch0->harq_processes[harq_pid]->mimo_mode == SISO) ?
          allocate_REs_in_RB_pilots_16QAM_siso :
          allocate_REs_in_RB;
      }
      else {
        qam_table_s0 = qam16_table_a0;
        allocate_REs = (dlsch0->harq_processes[harq_pid]->mimo_mode == SISO) ?
          allocate_REs_in_RB_no_pilots_16QAM_siso :
          allocate_REs_in_RB;

      }
      break;

    case 6:
      if (pilots) {
        qam_table_s0 = qam64_table_b0;
        allocate_REs = (dlsch0->harq_processes[harq_pid]->mimo_mode == SISO) ?
          allocate_REs_in_RB_pilots_64QAM_siso :
          allocate_REs_in_RB;
      }
      else {
        qam_table_s0 = qam64_table_a0;
        allocate_REs = (dlsch0->harq_processes[harq_pid]->mimo_mode == SISO) ?
          allocate_REs_in_RB_no_pilots_64QAM_siso :
          allocate_REs_in_RB;
      }
      /* TODO: this is a quick hack to be removed. There is a problem
       * with above code that needs to be analyzed and fixed. In the
       * meantime, let's use the generic function.
       */
      //allocate_REs = allocate_REs_in_RB;
      break;

    }

    /* TODO: hack, to be removed. The power is too different from
     * previous version. Some more work/validation is needed before
     * we switch to the new version.
     */
    //if (frame_parms->N_RB_DL==25)
      //allocate_REs = allocate_REs_in_RB;

    switch (mod_order1) {
    case 2:
      qam_table_s1 = NULL;
      allocate_REs = allocate_REs_in_RB;
      if (pilots) {
        qam_table_s1 = qpsk_table_b1;
      }
      else {
        qam_table_s1 = qpsk_table_a1;
      }
      break;
    case 4:
      if (pilots) {
        qam_table_s1 = qam16_table_b1;
      }
      else {
        qam_table_s1 = qam16_table_a1;
      }
      break;
    case 6:
      if (pilots) {
        qam_table_s1 = qam64_table_b1;
      }
      else {
        qam_table_s1 = qam64_table_a1;
      }
      break;

    }

    //for (aa=0;aa<frame_parms->nb_antennas_tx;aa++)
    //memset(&txdataF[aa][symbol_offset],0,frame_parms->ofdm_symbol_size<<2);
    //LOG_I(PHY,"symbol_offset %d,subframe offset %d : pilots %d\n",symbol_offset,subframe_offset,pilots);
    for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

      if (rb < 32)
        rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
      else if (rb < 64)
        rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
      else if (rb < 96)
        rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
      else if (rb < 100)
        rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
      else
        rb_alloc_ind = 0;

      if (check_skip(rb,subframe_offset,frame_parms,l,nsymb)==1)
        rb_alloc_ind = 0;

      skip_half = check_skiphalf(rb,subframe_offset,frame_parms,l,nsymb);
      skip_dc   = check_skip_dc(rb,frame_parms);

     if (dlsch0) {
        if (dlsch0_harq->Nlayers>1) {
          LOG_E(PHY,"Nlayers %d: re_offset %d, symbol %d offset %d\n",dlsch0_harq->Nlayers,re_offset,l,symbol_offset);
          return(-1);
        }
      }

      if (dlsch1) {
        if (dlsch1_harq->Nlayers>1) {
          LOG_I(PHY,"Nlayers %d: re_offset %d, symbol %d offset %d\n",dlsch0_harq->Nlayers,re_offset,l,symbol_offset);
          return(-1);
        }
      }



      if (rb_alloc_ind > 0) {
        //    LOG_I(PHY,"Allocated rb %d/symbol %d, skip_half %d, subframe_offset %d, symbol_offset %d, re_offset %d, jj %d\n",rb,l,skip_half,subframe_offset,symbol_offset,re_offset,jj);

      if (dlsch0 != NULL) {
        get_pmi_temp = get_pmi(frame_parms->N_RB_DL,
                               dlsch0->harq_processes[harq_pid]->mimo_mode,
                               dlsch0->harq_processes[harq_pid]->pmi_alloc,
                               rb);
      } else
        get_pmi_temp = get_pmi(frame_parms->N_RB_DL,
                               dlsch1->harq_processes[harq_pid]->mimo_mode,
                               dlsch1->harq_processes[harq_pid]->pmi_alloc,
                               rb);


      allocate_REs(phy_vars_eNB,
                         txdataF,
                         &jj,
                         &jj2,
                         re_offset,
                         symbol_offset,
                         (dlsch0 == NULL) ? NULL : dlsch0->harq_processes[harq_pid],
                         (dlsch1 == NULL) ? NULL : dlsch1->harq_processes[harq_pid],
                         pilots,
                         ((pilots) ? amp_rho_b : amp_rho_a),
                         get_pmi_temp,
                         qam_table_s0,
                         qam_table_s1,
                         &re_allocated,
                         skip_dc,
                         skip_half,
                         lprime,
                         mprime,
                         Ns,
                         P1_SHIFT,
                         P2_SHIFT);

          if ((mimo_mode == TM7) && (lprime>=0))
            mprime +=3+frame_parms->Ncp;
      }
      else {
        //      LOG_I(PHY,"Unallocated rb %d/symbol %d, re_offset %d, jj %d\n",rb,l,re_offset,jj);
      }
      re_offset+=12; // go to next RB

      // check if we crossed the symbol boundary and skip DCs

      if (re_offset >= frame_parms->ofdm_symbol_size) {
        if (skip_dc == 0)  //even number of RBs (doesn't straddle DC)
          re_offset=1;
        else
          re_offset=7;  // odd number of RBs
      }
    }
  }

#ifdef DEBUG_DLSCH_MODULATION
  if (dlsch0 != NULL){
    LOG_I(PHY,"generate_dlsch : jj = %d,re_allocated = %d (G %d)\n",jj,re_allocated,get_G(frame_parms,dlsch0_harq->nb_rb,dlsch0_harq->rb_alloc,mod_order0,Nl0,2,0,subframe_offset,1));
  }else{
    LOG_I(PHY,"generate_dlsch : jj = %d,re_allocated = %d (G %d)\n",jj,re_allocated,get_G(frame_parms,dlsch1_harq->nb_rb,dlsch1_harq->rb_alloc,mod_order1,Nl1,2,0,subframe_offset,1));
  }
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_MODULATION, VCD_FUNCTION_OUT);

  return (re_allocated);
}


int dlsch_modulation_SIC(int32_t **sic_buffer,
                         uint32_t subframe_offset,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t num_pdcch_symbols,
                         LTE_eNB_DLSCH_t *dlsch0,
                         int G)
{

  AssertFatal(1==0,"This function needs to be reintegrated ...\n");
  uint8_t harq_pid = -1;//dlsch0->current_harq_pid;
  LTE_DL_eNB_HARQ_t *dlsch0_harq = dlsch0->harq_processes[harq_pid];
  uint32_t i,jj,re_allocated=0;
  uint8_t mod_order0 = dlsch0_harq->Qm;
  uint8_t *x0  = dlsch0_harq->eDL;
  uint8_t qam64_table_offset_re = 0;
  uint8_t qam64_table_offset_im = 0;
  uint8_t qam16_table_offset_re = 0;
  uint8_t qam16_table_offset_im = 0;
  int16_t gain_lin_QPSK;
 #ifdef DEBUG_DLSCH_MODULATION
  uint8_t Nl0 = dlsch0_harq->Nl;
  uint8_t Nl1;
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_MODULATION, VCD_FUNCTION_IN);

  gain_lin_QPSK = (int16_t)((ONE_OVER_SQRT2_Q15));

  jj = 0;
  i = 0;
  while (jj <= G-1) {

    re_allocated = re_allocated + 1;

    switch (mod_order0) {
    case 2:  //QPSK

      ((int16_t*)&sic_buffer[0][i])[0] = (x0[jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i

      jj = jj + 1;

      ((int16_t*)&sic_buffer[0][i])[1] = (x0[jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}

      jj = jj + 1;

      //LOG_I(PHY,"recon %d,%d\n",((int16_t*)&sic_buffer[0][i])[0],((int16_t*)&sic_buffer[0][i])[1]);
      i++;

      break;

    case 4:  //16QAM

      qam16_table_offset_re = 0;
      qam16_table_offset_im = 0;

      if (x0[jj] == 1)
        qam16_table_offset_re+=2;

      jj=jj+1;

      if (x0[jj] == 1)
        qam16_table_offset_im+=2;

      jj=jj+1;


      if (x0[jj] == 1)
        qam16_table_offset_re+=1;

      jj=jj+1;

      if (x0[jj] == 1)
        qam16_table_offset_im+=1;

      jj=jj+1;


      ((int16_t *)&sic_buffer[0][i])[0]+=qam16_table[qam16_table_offset_re];
      ((int16_t *)&sic_buffer[0][i])[1]+=qam16_table[qam16_table_offset_im];

      i++;

      break;

    case 6:

      qam64_table_offset_re = 0;
      qam64_table_offset_im = 0;

      if (x0[jj] == 1)
      qam64_table_offset_re+=4;

      jj=jj+1;

      if (x0[jj] == 1)
        qam64_table_offset_im+=4;

      jj=jj+1;

      if (x0[jj] == 1)
        qam64_table_offset_re+=2;

      jj=jj+1;

      if (x0[jj] == 1)
        qam64_table_offset_im+=2;

      jj=jj+1;

      if (x0[jj] == 1)
        qam64_table_offset_re+=1;

      jj=jj+1;

      if (x0[jj] == 1)
        qam64_table_offset_im+=1;

      jj=jj+1;


      ((int16_t *)&sic_buffer[0][i])[0]+=(qam64_table[qam64_table_offset_re])>>1;//(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_re])>>15);
      ((int16_t *)&sic_buffer[0][i])[1]+=(qam64_table[qam64_table_offset_im])>>1;//(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_im])>>15);

      i++;

      break;
      }

    }


#ifdef DEBUG_DLSCH_MODULATION
  LOG_I(PHY,"generate_dlsch : jj = %d,re_allocated = %d (G %d)\n",jj,re_allocated,get_G(frame_parms,dlsch0_harq->nb_rb,dlsch0_harq->rb_alloc,mod_order0,Nl0,2,0,subframe_offset,1/*transmission mode*/));
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_MODULATION, VCD_FUNCTION_OUT);

  return (re_allocated);
}


int mch_modulation(int32_t **txdataF,
                   int16_t amp,
                   uint32_t subframe_offset,
                   LTE_DL_FRAME_PARMS *frame_parms,
                   LTE_eNB_DLSCH_t *dlsch)
{
  uint8_t nsymb,nsymb_pmch;
  uint32_t i,jj,re_allocated,symbol_offset;
  uint16_t l,rb,re_offset;
  uint8_t skip_dc=0;
  uint8_t mod_order = dlsch->harq_processes[0]->Qm;
  int16_t qam16_table_a[4],qam64_table_a[8];//,qam16_table_b[4],qam64_table_b[8];
  int16_t *qam_table_s;

  nsymb_pmch = 12;
  nsymb = (frame_parms->Ncp == NORMAL) ? 14 : 12;

  if (mod_order == 4)
    for (i=0; i<4; i++) {
      qam16_table_a[i] = (int16_t)(((int32_t)qam16_table[i]*amp)>>15);
    }
  else if (mod_order == 6)
    for (i=0; i<8; i++) {
      qam64_table_a[i] = (int16_t)(((int32_t)qam64_table[i]*amp)>>15);
    }

  jj=0;
  re_allocated=0;

  //  LOG_I(PHY,"num_pdcch_symbols %d, nsymb %d\n",num_pdcch_symbols,nsymb);
  for (l=2; l<nsymb_pmch; l++) {

#ifdef DEBUG_DLSCH_MODULATION
    LOG_I(PHY,"Generating MCH (mod %d) in subframe %d for symbol %d\n",mod_order, subframe_offset,l);
#endif

    re_offset = frame_parms->first_carrier_offset;
    symbol_offset = (uint32_t)frame_parms->ofdm_symbol_size*(l+(subframe_offset*nsymb));

    for (rb=0; rb<frame_parms->N_RB_DL; rb++) {


      if ((frame_parms->N_RB_DL&1) == 1) { // ODD N_RB_DL

    if (rb==(frame_parms->N_RB_DL>>1))
          skip_dc = 1;
        else
          skip_dc = 0;

      }

      if (mod_order == 4)
        qam_table_s = qam16_table_a;
      else if (mod_order == 6)
        qam_table_s = qam64_table_a;
      else
        qam_table_s = NULL;

      //LOG_I(PHY,"Allocated rb %d, subframe_offset %d,amp %d\n",rb,subframe_offset,amp);
      allocate_REs_in_RB_MCH(txdataF,
                             &jj,
                             re_offset,
                             symbol_offset,
                             dlsch->harq_processes[0]->eDL,
                             l,
                             mod_order,
                             amp,
                             qam_table_s,
                             &re_allocated,
                             skip_dc,
                             frame_parms);

      re_offset+=12; // go to next RB

      // check if we crossed the symbol boundary and skip DC
      if (re_offset >= frame_parms->ofdm_symbol_size) {
        if (skip_dc == 0)  //even number of RBs (doesn't straddle DC)
          re_offset=1;
        else
          re_offset=7;  // odd number of RBs
      }
    }
  }

#ifdef DEBUG_DLSCH_MODULATION
  LOG_I(PHY,"generate_dlsch(MCH) : jj = %d,re_allocated = %d (G %d)\n",jj,re_allocated,get_G(frame_parms,dlsch->harq_processes[0]->nb_rb,dlsch->harq_processes[0]->rb_alloc,mod_order,1,2,0,subframe_offset,1/*transmission mode*/));
#endif

  return (re_allocated);
}

int allocate_REs_in_RB_MCH_khz_1dot25(int32_t **txdataF,
                           uint32_t *jj,
                            uint16_t re_offset,
                            uint32_t symbol_offset,
                            uint8_t *x0,
                            uint8_t subframe,
                            uint8_t mod_order,
                            int16_t amp,
                            int16_t *qam_table_s,
                            uint32_t *re_allocated,
                            uint8_t skip_dc,
                            LTE_DL_FRAME_PARMS *frame_parms)
{
  if (!qam_table_s)
    abort();

 uint32_t tti_offset;
  uint8_t re,offset;
  uint8_t qam64_table_offset_re = 0;
  uint8_t qam64_table_offset_im = 0;
  uint8_t qam16_table_offset_re = 0;
  uint8_t qam16_table_offset_im = 0;
  int16_t gain_lin_QPSK;//,gain_lin_16QAM1,gain_lin_16QAM2;
  int16_t re_off=re_offset;
  gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);
  uint8_t first_re,last_re;
  int inc;
#ifdef DEBUG_DLSCH_MODULATION
  LOG_I(PHY,"allocate_re_MCH khz_1dot25 (mod %d): symbol_offset %d re_offset %d (%d), jj %d -> %d,%d, gain_lin_QPSK %d,txdataF %p\n",mod_order,symbol_offset,re_offset,skip_dc,*jj, x0[*jj], x0[1+*jj],gain_lin_QPSK,&txdataF[0][symbol_offset]);
#endif

  /*last_re=12;
  first_re=0;
  inc=1;

  if ((l==2)||(l==10)) {
    inc=2;
    first_re=1;
  } else if (l==6) {
    inc=2;
  }*/
  last_re=144; //12*12
  if ((subframe&0x1)==0){ // n_sf mod 2 == 0: even
        first_re=1;
        offset=0;
  }else{
        first_re=0;
        offset=3;
  }
  inc=1;

  for (re=first_re; re<last_re; re+=inc) {
    if( ((re-offset)%6) == 0  )
                continue; //pilot
    //if ((subframe&0x1)==0) // n_sf mod 2 == 0: even
        //k = 6*m;
    //else
        //k = 6*m + 3;
    //if ((skip_dc == 1) && (re==(6+first_re)))
    if ((skip_dc == 1) && (re==(72+first_re))){
      re_off=re_off - frame_parms->ofdm_symbol_size_khz_1dot25+1;
    }

        tti_offset = symbol_offset + re_off + re;

    //LOG_I(PHY,"re %d (jj %d)\n",re,*jj);
    *re_allocated = *re_allocated + 1;


    switch (mod_order) {

      case 2:  //QPSK

      LOG_D(PHY,"%d : %d,%d => ",tti_offset,((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
      ((int16_t*)&txdataF[0][tti_offset])[0] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //I //b_i

      *jj = *jj + 1;

      ((int16_t*)&txdataF[0][tti_offset])[1] = (x0[*jj]==1) ? (-gain_lin_QPSK) : gain_lin_QPSK; //Q //b_{i+1}

      *jj = *jj + 1;

      LOG_D(PHY,"subframe%d alloc[%d][%d][%d](%d,%d)\n",subframe,tti_offset,symbol_offset,tti_offset-symbol_offset,((int16_t*)&txdataF[0][tti_offset])[0],((int16_t*)&txdataF[0][tti_offset])[1]);
      break;


      case 4:  //16QAM

    qam16_table_offset_re = 0;
    qam16_table_offset_im = 0;

    if (x0[*jj] == 1)
      qam16_table_offset_re+=2;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam16_table_offset_im+=2;

    *jj=*jj+1;


    if (x0[*jj] == 1)
      qam16_table_offset_re+=1;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam16_table_offset_im+=1;

    *jj=*jj+1;


    ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s[qam16_table_offset_re];//(int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_re])>>15);
    ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s[qam16_table_offset_im];//(int16_t)(((int32_t)amp*qam16_table[qam16_table_offset_im])>>15);

    break;

      case 6:  //64QAM

        qam64_table_offset_re = 0;
    qam64_table_offset_im = 0;

    if (x0[*jj] == 1)
      qam64_table_offset_re+=4;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_im+=4;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_re+=2;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_im+=2;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_re+=1;

    *jj=*jj+1;

    if (x0[*jj] == 1)
      qam64_table_offset_im+=1;

    *jj=*jj+1;

    ((int16_t *)&txdataF[0][tti_offset])[0]=qam_table_s[qam64_table_offset_re];//(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_re])>>15);
    ((int16_t *)&txdataF[0][tti_offset])[1]=qam_table_s[qam64_table_offset_im];//(int16_t)(((int32_t)amp*qam64_table[qam64_table_offset_im])>>15);

    break;

    }
  }
  return(0);
}

int mch_modulation_khz_1dot25(int32_t **txdataF,
                   int16_t amp,
                   uint32_t subframe_offset,
                   LTE_DL_FRAME_PARMS *frame_parms,
                   LTE_eNB_DLSCH_t *dlsch)
{
  uint8_t nsymb,nsymb_pmch;
  uint32_t i,jj,re_allocated,symbol_offset;
  uint16_t l,rb,re_offset;
  uint8_t skip_dc=0;
  uint8_t mod_order = dlsch->harq_processes[0]->Qm;
  int16_t qam16_table_a[4],qam64_table_a[8];//,qam16_table_b[4],qam64_table_b[8];
  int16_t *qam_table_s=qam16_table_a;

  nsymb_pmch = 1;
  nsymb = (frame_parms->Ncp == NORMAL) ? 14 : 12;

  if (mod_order == 4)
    for (i=0; i<4; i++) {
      qam16_table_a[i] = (int16_t)(((int32_t)qam16_table[i]*amp)>>15);
    }
  else if (mod_order == 6)
    for (i=0; i<8; i++) {
      qam64_table_a[i] = (int16_t)(((int32_t)qam64_table[i]*amp)>>15);
    }

  jj=0;
  re_allocated=0;





  //  LOG_I(PHY,"num_pdcch_symbols %d, nsymb %d\n",num_pdcch_symbols,nsymb);
  for (l=0; l<nsymb_pmch; l++) {

#ifdef DEBUG_DLSCH_MODULATION
    LOG_I(PHY,"Generating khz_1dot25 MCH (mod %d) in subframe %d for symbol %d\n",mod_order, subframe_offset,l);
#endif

    re_offset = frame_parms->first_carrier_offset_khz_1dot25;
    symbol_offset = (uint32_t)frame_parms->ofdm_symbol_size*(l+(subframe_offset*nsymb));

    for (rb=0; rb<frame_parms->N_RB_DL; rb++) {


      if ((frame_parms->N_RB_DL&1) == 1) { // ODD N_RB_DL

        if (rb==(frame_parms->N_RB_DL>>1))
          skip_dc = 1;
        else
          skip_dc = 0;

      }

      if (mod_order == 6)
        qam_table_s = qam64_table_a;

      //LOG_I(PHY,"Allocated rb %d, subframe_offset %d,amp %d\n",rb,subframe_offset,amp);
      allocate_REs_in_RB_MCH_khz_1dot25(txdataF,
                             &jj,
                             re_offset,
                             symbol_offset,
                             dlsch->harq_processes[0]->eDL,
                             subframe_offset,
                             mod_order,
                             amp,
                             qam_table_s,
                             &re_allocated,
                             skip_dc,
                             frame_parms);

      re_offset+=144; // go to next RB 12*12

      // check if we crossed the symbol boundary and skip DC
      if (re_offset >= frame_parms->ofdm_symbol_size_khz_1dot25) {
        if (skip_dc == 0)  //even number of RBs (doesn't straddle DC)
          re_offset=1;
        else
          re_offset=73;  // odd number of RBs
      }
    }
  }

#ifdef DEBUG_DLSCH_MODULATION
  LOG_I(PHY,"generate_dlsch(MCH) : jj = %d,re_allocated = %d (G %d)\n",jj,re_allocated,get_G(frame_parms,dlsch->harq_processes[0]->nb_rb,dlsch->harq_processes[0]->rb_alloc,mod_order,1,2,0,subframe_offset,1/*transmission mode*/));
  LOG_I(PHY,"generate_dlsch(MCH) : jj = %d,re_allocated = %d\n",jj,re_allocated);
#endif

  return (re_allocated);
}

