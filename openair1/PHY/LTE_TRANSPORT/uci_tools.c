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
* \brief Routines for generation of and computations regarding the uplink control information (UCI) for PUSCH. V8.6 2009-03
* \author R. Knopp, F. Kaltenberger, A. Bhamri
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr, florian.kaltenberger@eurecom.fr, ankit.bhamri@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs.h"
#include "PHY/extern.h"
#ifdef DEBUG_UCI_TOOLS
#include "PHY/vars.h"
#endif

//#define DEBUG_UCI 1

uint64_t pmi2hex_2Ar1(uint32_t pmi)
{

  uint64_t pmil = (uint64_t)pmi;

  return ((pmil&3) + (((pmil>>2)&3)<<4) + (((pmil>>4)&3)<<8) + (((pmil>>6)&3)<<12) +
          (((pmil>>8)&3)<<16) + (((pmil>>10)&3)<<20) + (((pmil>>12)&3)<<24) +
          (((pmil>>14)&3)<<28) + (((pmil>>16)&3)<<32) + (((pmil>>18)&3)<<36) +
          (((pmil>>20)&3)<<40) + (((pmil>>22)&3)<<44) + (((pmil>>24)&3)<<48));
}

uint64_t pmi2hex_2Ar2(uint32_t pmi)
{

  uint64_t pmil = (uint64_t)pmi;
  return ((pmil&1) + (((pmil>>1)&1)<<4) + (((pmil>>2)&1)<<8) + (((pmil>>3)&1)<<12) +
          (((pmil>>4)&1)<<16) + (((pmil>>5)&1)<<20) + (((pmil>>6)&1)<<24) +
          (((pmil>>7)&1)<<28) + (((pmil>>8)&1)<<32) + (((pmil>>9)&1)<<36) +
          (((pmil>>10)&1)<<40) + (((pmil>>11)&1)<<44) + (((pmil>>12)&1)<<48));
}

uint64_t cqi2hex(uint32_t cqi)
{

  uint64_t cqil = (uint64_t)cqi;
  return ((cqil&3) + (((cqil>>2)&3)<<4) + (((cqil>>4)&3)<<8) + (((cqil>>6)&3)<<12) +
          (((cqil>>8)&3)<<16) + (((cqil>>10)&3)<<20) + (((cqil>>12)&3)<<24) +
          (((cqil>>14)&3)<<28) + (((cqil>>16)&3)<<32) + (((cqil>>18)&3)<<36) +
          (((cqil>>20)&3)<<40) + (((cqil>>22)&3)<<44) + (((cqil>>24)&3)<<48));
}

//void do_diff_cqi(uint8_t N_RB_DL,
//     uint8_t *DL_subband_cqi,
//     uint8_t DL_cqi,
//     uint32_t diffcqi1) {
//
//  uint8_t nb_sb,i,offset;
//
//  // This is table 7.2.1-3 from 36.213 (with k replaced by the number of subbands, nb_sb)
//  switch (N_RB_DL) {
//  case 6:
//    nb_sb=0;
//    break;
//  case 15:
//    nb_sb = 4;
//  case 25:
//    nb_sb = 7;
//    break;
//  case 50:
//    nb_sb = 9;
//    break;
//  case 75:
//    nb_sb = 10;
//    break;
//  case 100:
//    nb_sb = 13;
//    break;
//  default:
//    nb_sb=0;
//    break;
//  }
//
//  memset(DL_subband_cqi,0,13);
//
//  for (i=0;i<nb_sb;i++) {
//    offset = (DL_cqi>>(2*i))&3;
//    if (offset == 3)
//      DL_subband_cqi[i] = DL_cqi - 1;
//    else
//      DL_subband_cqi[i] = DL_cqi + offset;
//  }
//}


void do_diff_cqi(uint8_t N_RB_DL,
                 uint8_t *DL_subband_cqi,
                 uint8_t DL_cqi,
                 uint32_t diffcqi1)
{

  uint8_t nb_sb,i,offset;

  // This is table 7.2.1-3 from 36.213 (with k replaced by the number of subbands, nb_sb)
  switch (N_RB_DL) {
  case 6:
    nb_sb=1;
    break;

  case 15:
    nb_sb = 4;
    break;

  case 25:
    nb_sb = 7;
    break;

  case 50:
    nb_sb = 9;
    break;

  case 75:
    nb_sb = 10;
    break;

  case 100:
    nb_sb = 13;
    break;

  default:
    nb_sb=0;
    break;
  }

  memset(DL_subband_cqi,0,13);

  for (i=0; i<nb_sb; i++) {
    offset = (diffcqi1>>(2*i))&3;

    if (offset == 3)
      DL_subband_cqi[i] = DL_cqi - 1;
    else
      DL_subband_cqi[i] = DL_cqi + offset;
  }
}

void extract_CQI(void *o,UCI_format_t uci_format,LTE_eNB_UE_stats *stats, uint8_t N_RB_DL, uint16_t * crnti, uint8_t * access_mode)
{

  //unsigned char rank;
  //UCI_format fmt;
  //uint8_t N_RB_DL = 25;
  uint8_t i;
  LOG_D(PHY,"[eNB][UCI] N_RB_DL %d uci format %d\n", N_RB_DL,uci_format);

  switch(N_RB_DL) {
  case 6:
    switch(uci_format) {
    case wideband_cqi_rank1_2A:
      stats->DL_cqi[0]     = (((wideband_cqi_rank1_2A_1_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_pmi_single = ((wideband_cqi_rank1_2A_1_5MHz *)o)->pmi;
      break;

    case wideband_cqi_rank2_2A:
      stats->DL_cqi[0]     = (((wideband_cqi_rank2_2A_1_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_cqi[1]     = (((wideband_cqi_rank2_2A_1_5MHz *)o)->cqi2);

      if (stats->DL_cqi[1] > 24)
        stats->DL_cqi[1] = 24;

      stats->DL_pmi_dual   = ((wideband_cqi_rank2_2A_1_5MHz *)o)->pmi;
      break;

    case HLC_subband_cqi_nopmi:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_nopmi_1_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],((HLC_subband_cqi_nopmi_1_5MHz *)o)->diffcqi1);
      break;

    case HLC_subband_cqi_rank1_2A:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_rank1_2A_1_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank1_2A_1_5MHz *)o)->diffcqi1));
      stats->DL_pmi_single = ((HLC_subband_cqi_rank1_2A_1_5MHz *)o)->pmi;
      break;

    case HLC_subband_cqi_rank2_2A:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_cqi[1]     = (((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->cqi2);

      if (stats->DL_cqi[1] > 24)
        stats->DL_cqi[1] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->diffcqi1));
      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[1],stats->DL_cqi[1],(((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->diffcqi2));
      stats->DL_pmi_dual   = ((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->pmi;
      break;

    case unknown_cqi:
    default:
      LOG_N(PHY,"[eNB][UCI] received unknown uci (rb %d)\n",N_RB_DL);
      break;
    }

    break;

  case 25:

    switch(uci_format) {
    case wideband_cqi_rank1_2A:
      stats->DL_cqi[0]     = (((wideband_cqi_rank1_2A_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_pmi_single = ((wideband_cqi_rank1_2A_5MHz *)o)->pmi;
      break;

    case wideband_cqi_rank2_2A:
      stats->DL_cqi[0]     = (((wideband_cqi_rank2_2A_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_cqi[1]     = (((wideband_cqi_rank2_2A_5MHz *)o)->cqi2);

      if (stats->DL_cqi[1] > 24)
        stats->DL_cqi[1] = 24;

      stats->DL_pmi_dual   = ((wideband_cqi_rank2_2A_5MHz *)o)->pmi;
      //this translates the 2-layer PMI into a single layer PMI for the first codeword
      //the PMI for the second codeword will be stats->DL_pmi_single^0x1555
      stats->DL_pmi_single = 0;
      for (i=0;i<7;i++)
	stats->DL_pmi_single = stats->DL_pmi_single | (((stats->DL_pmi_dual&(1<i))>>i)*2)<<2*i;  
      break;

    case HLC_subband_cqi_nopmi:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_nopmi_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],((HLC_subband_cqi_nopmi_5MHz *)o)->diffcqi1);
      break;

    case HLC_subband_cqi_rank1_2A:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
      stats->DL_pmi_single = ((HLC_subband_cqi_rank1_2A_5MHz *)o)->pmi;
      break;

    case HLC_subband_cqi_rank2_2A:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_rank2_2A_5MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_cqi[1]     = (((HLC_subband_cqi_rank2_2A_5MHz *)o)->cqi2);

      if (stats->DL_cqi[1] > 24)
        stats->DL_cqi[1] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank2_2A_5MHz *)o)->diffcqi1));
      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[1],stats->DL_cqi[1],(((HLC_subband_cqi_rank2_2A_5MHz *)o)->diffcqi2));
      stats->DL_pmi_dual   = ((HLC_subband_cqi_rank2_2A_5MHz *)o)->pmi;
      break;

    case unknown_cqi:
    default:
      LOG_N(PHY,"[eNB][UCI] received unknown uci (rb %d)\n",N_RB_DL);
      break;
    }

    break;

  case 50:
    switch(uci_format) {
    case wideband_cqi_rank1_2A:
      stats->DL_cqi[0]     = (((wideband_cqi_rank1_2A_10MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_pmi_single = ((wideband_cqi_rank1_2A_10MHz *)o)->pmi;
      break;

    case wideband_cqi_rank2_2A:
      stats->DL_cqi[0]     = (((wideband_cqi_rank2_2A_10MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_cqi[1]     = (((wideband_cqi_rank2_2A_10MHz *)o)->cqi2);

      if (stats->DL_cqi[1] > 24)
        stats->DL_cqi[1] = 24;

      stats->DL_pmi_dual   = ((wideband_cqi_rank2_2A_10MHz *)o)->pmi;
      break;

    case HLC_subband_cqi_nopmi:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_nopmi_10MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],((HLC_subband_cqi_nopmi_10MHz *)o)->diffcqi1);
      break;

    case HLC_subband_cqi_rank1_2A:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_rank1_2A_10MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank1_2A_10MHz *)o)->diffcqi1));
      stats->DL_pmi_single = ((HLC_subband_cqi_rank1_2A_10MHz *)o)->pmi;
      break;

    case HLC_subband_cqi_rank2_2A:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_rank2_2A_10MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_cqi[1]     = (((HLC_subband_cqi_rank2_2A_10MHz *)o)->cqi2);

      if (stats->DL_cqi[1] > 24)
        stats->DL_cqi[1] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank2_2A_10MHz *)o)->diffcqi1));
      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[1],stats->DL_cqi[1],(((HLC_subband_cqi_rank2_2A_10MHz *)o)->diffcqi2));
      stats->DL_pmi_dual   = ((HLC_subband_cqi_rank2_2A_10MHz *)o)->pmi;
      break;

    case unknown_cqi:
    default:
      LOG_N(PHY,"[eNB][UCI] received unknown uci (RB %d)\n",N_RB_DL);
      break;
    }

    break;

  case 100:
    switch(uci_format) {
    case wideband_cqi_rank1_2A:
      stats->DL_cqi[0]     = (((wideband_cqi_rank1_2A_20MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_pmi_single = ((wideband_cqi_rank1_2A_20MHz *)o)->pmi;
      break;

    case wideband_cqi_rank2_2A:
      stats->DL_cqi[0]     = (((wideband_cqi_rank2_2A_20MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_cqi[1]     = (((wideband_cqi_rank2_2A_20MHz *)o)->cqi2);

      if (stats->DL_cqi[1] > 24)
        stats->DL_cqi[1] = 24;

      stats->DL_pmi_dual   = ((wideband_cqi_rank2_2A_20MHz *)o)->pmi;
      break;

    case HLC_subband_cqi_nopmi:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_nopmi_20MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],((HLC_subband_cqi_nopmi_20MHz *)o)->diffcqi1);
      break;

    case HLC_subband_cqi_rank1_2A:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_rank1_2A_20MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank1_2A_20MHz *)o)->diffcqi1));
      stats->DL_pmi_single = ((HLC_subband_cqi_rank1_2A_20MHz *)o)->pmi;
      break;

    case HLC_subband_cqi_rank2_2A:
      stats->DL_cqi[0]     = (((HLC_subband_cqi_rank2_2A_20MHz *)o)->cqi1);

      if (stats->DL_cqi[0] > 24)
        stats->DL_cqi[0] = 24;

      stats->DL_cqi[1]     = (((HLC_subband_cqi_rank2_2A_20MHz *)o)->cqi2);

      if (stats->DL_cqi[1] > 24)
        stats->DL_cqi[1] = 24;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank2_2A_20MHz *)o)->diffcqi1));
      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[1],stats->DL_cqi[1],(((HLC_subband_cqi_rank2_2A_20MHz *)o)->diffcqi2));
      stats->DL_pmi_dual   = ((HLC_subband_cqi_rank2_2A_20MHz *)o)->pmi;
      break;

    case unknown_cqi:
    default:
      LOG_N(PHY,"[eNB][UCI] received unknown uci (RB %d)\n",N_RB_DL);
      break;
    }

    break;

  default:
    LOG_N(PHY,"[eNB][UCI] unknown RB %d\n",N_RB_DL);
    break;
  }

  /*
  switch (tmode) {

  case 1:
  case 2:
  case 3:
  case 5:
  case 6:
  case 7:
  default:
    fmt = hlc_cqi;
    break;
  case 4:
    fmt = wideband_cqi;
    break;
  }

  rank = o_RI[0];
  //printf("extract_CQI: rank = %d\n",rank);

  switch (fmt) {

  case wideband_cqi: //and subband pmi
    if (rank == 0) {
      stats->DL_cqi[0]     = (((wideband_cqi_rank1_2A_5MHz *)o)->cqi1);
      if (stats->DL_cqi[0] > 15)
  stats->DL_cqi[0] = 15;
      stats->DL_pmi_single = ((wideband_cqi_rank1_2A_5MHz *)o)->pmi;
    }
    else {
      stats->DL_cqi[0]     = (((wideband_cqi_rank2_2A_5MHz *)o)->cqi1);
      if (stats->DL_cqi[0] > 15)
  stats->DL_cqi[0] = 15;
      stats->DL_cqi[1]     = (((wideband_cqi_rank2_2A_5MHz *)o)->cqi2);
      if (stats->DL_cqi[1] > 15)
  stats->DL_cqi[1] = 15;
      stats->DL_pmi_dual   = ((wideband_cqi_rank2_2A_5MHz *)o)->pmi;
    }
    break;
  case hlc_cqi:
    if (tmode > 2) {
      if (rank == 0) {
  stats->DL_cqi[0]     = (((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);
  if (stats->DL_cqi[0] > 15)
    stats->DL_cqi[0] = 15;
  do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
  stats->DL_pmi_single = ((HLC_subband_cqi_rank1_2A_5MHz *)o)->pmi;
      }
      else {
  stats->DL_cqi[0]     = (((HLC_subband_cqi_rank2_2A_5MHz *)o)->cqi1);
  if (stats->DL_cqi[0] > 15)
    stats->DL_cqi[0] = 15;
  stats->DL_cqi[1]     = (((HLC_subband_cqi_rank2_2A_5MHz *)o)->cqi2);
  if (stats->DL_cqi[1] > 15)
    stats->DL_cqi[1] = 15;
  do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],(((HLC_subband_cqi_rank2_2A_5MHz *)o)->diffcqi1));
  do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[1],stats->DL_cqi[1],(((HLC_subband_cqi_rank2_2A_5MHz *)o)->diffcqi2));
  stats->DL_pmi_dual   = ((HLC_subband_cqi_rank2_2A_5MHz *)o)->pmi;
      }
    }
    else {
      stats->DL_cqi[0]     = (((HLC_subband_cqi_nopmi_5MHz *)o)->cqi1);
      if (stats->DL_cqi[0] > 15)
  stats->DL_cqi[0] = 15;

      do_diff_cqi(N_RB_DL,stats->DL_subband_cqi[0],stats->DL_cqi[0],((HLC_subband_cqi_nopmi_5MHz *)o)->diffcqi1);

    }
    break;
  default:
    break;
  }
  */

}


void print_CQI(void *o,UCI_format_t uci_format,unsigned char eNB_id,int N_RB_DL)
{


  switch(uci_format) {
  case wideband_cqi_rank1_2A:
#ifdef DEBUG_UCI
    LOG_D(PHY,"[PRINT CQI] flat_LA %d\n", flag_LA);
    switch(N_RB_DL) {
    case 6:
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 1: eNB %d, cqi %d\n",eNB_id,
            ((wideband_cqi_rank1_2A_1_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 1: eNB %d, pmi (%x) %8x\n",eNB_id,
            ((wideband_cqi_rank1_2A_1_5MHz *)o)->pmi,
            pmi2hex_2Ar1(((wideband_cqi_rank1_2A_1_5MHz *)o)->pmi));
      break;

    case 25:
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 1: eNB %d, cqi %d\n",eNB_id,
            ((wideband_cqi_rank1_2A_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 1: eNB %d, pmi (%x) %8x\n",eNB_id,
            ((wideband_cqi_rank1_2A_5MHz *)o)->pmi,
            pmi2hex_2Ar1(((wideband_cqi_rank1_2A_5MHz *)o)->pmi));
      break;

    case 50:
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 1: eNB %d, cqi %d\n",eNB_id,
            ((wideband_cqi_rank1_2A_10MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 1: eNB %d, pmi (%x) %8x\n",eNB_id,
            ((wideband_cqi_rank1_2A_10MHz *)o)->pmi,
            pmi2hex_2Ar1(((wideband_cqi_rank1_2A_10MHz *)o)->pmi));
      break;

    case 100:
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 1: eNB %d, cqi %d\n",eNB_id,
            ((wideband_cqi_rank1_2A_20MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 1: eNB %d, pmi (%x) %8x\n",eNB_id,
            ((wideband_cqi_rank1_2A_20MHz *)o)->pmi,
            pmi2hex_2Ar1(((wideband_cqi_rank1_2A_20MHz *)o)->pmi));
      break;
    }

#endif //DEBUG_UCI
    break;

  case wideband_cqi_rank2_2A:
#ifdef DEBUG_UCI
    switch(N_RB_DL) {
    case 6:
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((wideband_cqi_rank2_2A_1_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((wideband_cqi_rank2_2A_1_5MHz *)o)->cqi2);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, pmi %8x\n",eNB_id,pmi2hex_2Ar2(((wideband_cqi_rank2_2A_1_5MHz *)o)->pmi));
      break;

    case 25:
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((wideband_cqi_rank2_2A_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((wideband_cqi_rank2_2A_5MHz *)o)->cqi2);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, pmi %8x\n",eNB_id,pmi2hex_2Ar2(((wideband_cqi_rank2_2A_5MHz *)o)->pmi));
      break;

    case 50:
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((wideband_cqi_rank2_2A_10MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((wideband_cqi_rank2_2A_10MHz *)o)->cqi2);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, pmi %8x\n",eNB_id,pmi2hex_2Ar2(((wideband_cqi_rank2_2A_10MHz *)o)->pmi));
      break;

    case 100:
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((wideband_cqi_rank2_2A_20MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((wideband_cqi_rank2_2A_20MHz *)o)->cqi2);
      LOG_I(PHY,"[PRINT CQI] wideband_cqi rank 2: eNB %d, pmi %8x\n",eNB_id,pmi2hex_2Ar2(((wideband_cqi_rank2_2A_20MHz *)o)->pmi));
      break;
    }

#endif //DEBUG_UCI
    break;

  case HLC_subband_cqi_nopmi:
#ifdef DEBUG_UCI
    switch(N_RB_DL) {
    case 6:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi (no pmi) : eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_1_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi (no pmi) : eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_1_5MHz *)o)->diffcqi1));
      break;

    case 25:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi (no pmi) : eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi (no pmi) : eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
      break;

    case 50:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi (no pmi) : eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_10MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi (no pmi) : eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_10MHz *)o)->diffcqi1));
      break;

    case 100:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi (no pmi) : eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_20MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi (no pmi) : eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_20MHz *)o)->diffcqi1));
      break;
    }

#endif //DEBUG_UCI
    break;

  case HLC_subband_cqi_rank1_2A:
#ifdef DEBUG_UCI
    switch(N_RB_DL) {
    case 6:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->pmi);
      break;

    case 25:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->pmi);
      break;

    case 50:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->pmi);
      break;

    case 100:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 1: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->pmi);
      break;
    }

#endif //DEBUG_UCI
    break;

  case HLC_subband_cqi_rank2_2A:
#ifdef DEBUG_UCI
    switch(N_RB_DL) {
    case 6:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->cqi2);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->diffcqi1));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi2 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->diffcqi2));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_1_5MHz *)o)->pmi);
      break;

    case 25:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_5MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_5MHz *)o)->cqi2);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_5MHz *)o)->diffcqi1));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi2 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_5MHz *)o)->diffcqi2));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_5MHz *)o)->pmi);
      break;

    case 50:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_10MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_10MHz *)o)->cqi2);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_10MHz *)o)->diffcqi1));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi2 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_10MHz *)o)->diffcqi2));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_10MHz *)o)->pmi);
      break;

    case 100:
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_20MHz *)o)->cqi1);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_20MHz *)o)->cqi2);
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_20MHz *)o)->diffcqi1));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi2 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_20MHz *)o)->diffcqi2));
      LOG_I(PHY,"[PRINT CQI] hlc_cqi rank 2: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_20MHz *)o)->pmi);
      break;
    }

#endif //DEBUG_UCI
    break;

  case ue_selected:
#ifdef DEBUG_UCI
    LOG_W(PHY,"[PRINT CQI] ue_selected CQI not supported yet!!!\n");
#endif //DEBUG_UCI
    break;

  default:
#ifdef DEBUG_UCI
    LOG_E(PHY,"[PRINT CQI] unsupported CQI mode (%d)!!!\n",uci_format);
#endif //DEBUG_UCI
    break;
  }

  /*
  switch (tmode) {

  case 1:
  case 2:
  case 3:
  case 5:
  case 6:
  case 7:
  default:
    fmt = hlc_cqi;
    break;
  case 4:
    fmt = wideband_cqi;
    break;
  }

  switch (fmt) {

  case wideband_cqi:
    if (rank == 0) {
  #ifdef DEBUG_UCI
      msg("[PRINT CQI] wideband_cqi rank 1: eNB %d, cqi %d\n",eNB_id,((wideband_cqi_rank1_2A_5MHz *)o)->cqi1);
      msg("[PRINT CQI] wideband_cqi rank 1: eNB %d, pmi (%x) %8x\n",eNB_id,((wideband_cqi_rank1_2A_5MHz *)o)->pmi,pmi2hex_2Ar1(((wideband_cqi_rank1_2A_5MHz *)o)->pmi));
  #endif //DEBUG_UCI
    }
    else {
  #ifdef DEBUG_UCI
      msg("[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((wideband_cqi_rank2_2A_5MHz *)o)->cqi1);
      msg("[PRINT CQI] wideband_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((wideband_cqi_rank2_2A_5MHz *)o)->cqi2);
      msg("[PRINT CQI] wideband_cqi rank 2: eNB %d, pmi %8x\n",eNB_id,pmi2hex_2Ar2(((wideband_cqi_rank2_2A_5MHz *)o)->pmi));
  #endif //DEBUG_UCI
    }
    break;
  case hlc_cqi:
    if (tmode > 2) {
      if (rank == 0) {
  #ifdef DEBUG_UCI
  msg("[PRINT CQI] hlc_cqi rank 1: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);
  msg("[PRINT CQI] hlc_cqi rank 1: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
  msg("[PRINT CQI] hlc_cqi rank 1: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->pmi);
  #endif //DEBUG_UCI
      }
      else {
  #ifdef DEBUG_UCI
  msg("[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_5MHz *)o)->cqi1);
  msg("[PRINT CQI] hlc_cqi rank 2: eNB %d, cqi2 %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_5MHz *)o)->cqi2);
  msg("[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_5MHz *)o)->diffcqi1));
  msg("[PRINT CQI] hlc_cqi rank 2: eNB %d, diffcqi2 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank2_2A_5MHz *)o)->diffcqi2));
  msg("[PRINT CQI] hlc_cqi rank 2: eNB %d, pmi %d\n",eNB_id,((HLC_subband_cqi_rank2_2A_5MHz *)o)->pmi);
  #endif //DEBUG_UCI
      }
    }
    else {
  #ifdef DEBUG_UCI
      msg("[PRINT CQI] hlc_cqi (no pmi) : eNB %d, cqi1 %d\n",eNB_id,((HLC_subband_cqi_rank1_2A_5MHz *)o)->cqi1);
      msg("[PRINT CQI] hlc_cqi (no pmi) : eNB %d, diffcqi1 %8x\n",eNB_id,cqi2hex(((HLC_subband_cqi_rank1_2A_5MHz *)o)->diffcqi1));
  #endif //DEBUG_UCI
    }
    break;
  case ue_selected:
  #ifdef DEBUG_UCI
    msg("dci_tools.c: print_CQI ue_selected CQI not supported yet!!!\n");
  #endif //DEBUG_UCI
    break;
  }
  */

}


int8_t find_uci(uint16_t rnti, int frame, int subframe, PHY_VARS_eNB *eNB,find_type_t type) {
  uint8_t i;
  int8_t first_free_index=-1;

  AssertFatal(eNB!=NULL,"eNB is null\n");
  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    if ((eNB->uci_vars[i].active >0) &&
	(eNB->uci_vars[i].rnti==rnti) &&
	(eNB->uci_vars[i].frame==frame) &&
	(eNB->uci_vars[i].subframe==subframe)) return(i); 
    else if ((eNB->uci_vars[i].active == 0) && (first_free_index==-1)) first_free_index=i;
  }
  if (type == SEARCH_EXIST) return(-1);
  else return(first_free_index);
}



