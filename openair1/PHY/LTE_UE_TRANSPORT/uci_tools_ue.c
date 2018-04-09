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
#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
#ifdef DEBUG_UCI_TOOLS
#include "PHY/phy_vars.h"
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

}






