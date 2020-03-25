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

/* \file config_ue.c
 * \brief common utility functions for NR (gNB and UE)
 * \author R. Knopp,
 * \date 2019
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include <stdint.h>
#include "assertions.h"

int NRRIV2BW(int locationAndBandwidth,int N_RB) {
  int tmp = locationAndBandwidth/N_RB;
  int tmp2 = locationAndBandwidth%N_RB;


  if (tmp <= ((N_RB>>1)+1) && (tmp+tmp2)<N_RB) return(tmp+1);
  else                      return(N_RB+1-tmp);

}

int NRRIV2PRBOFFSET(int locationAndBandwidth,int N_RB) {
  int tmp = locationAndBandwidth/N_RB;
  int tmp2 = locationAndBandwidth%N_RB;


  if (tmp <= ((N_RB>>1)+1) && (tmp+tmp2)<N_RB) return(tmp2);
  else                      return(N_RB-1-tmp2);
}

int PRBalloc_to_locationandbandwidth0(int NPRB,int RBstart,int BWPsize) {
  AssertFatal(NPRB>0 && (NPRB + RBstart <= BWPsize),"Illegal NPRB/RBstart Configuration (%d,%d) for BWPsize %d\n",NPRB,RBstart,BWPsize);

  if (NPRB <= 1+(BWPsize>>1)) return(BWPsize*(NPRB-1)+RBstart);
  else                        return(BWPsize*(BWPsize+1-NPRB) + (BWPsize-1-RBstart));
}

int PRBalloc_to_locationandbandwidth(int NPRB,int RBstart) {
  return(PRBalloc_to_locationandbandwidth0(NPRB,RBstart,275));
}
/// Target code rate tables indexed by Imcs
uint16_t nr_target_code_rate_table1[29] = {120, 157, 193, 251, 308, 379, 449, 526, 602, 679, 340, 378, 434, 490, 553, \
                                            616, 658, 438, 466, 517, 567, 616, 666, 719, 772, 822, 873, 910, 948};
  // Imcs values 20 and 26 have been multiplied by 2 to avoid the floating point
uint16_t nr_target_code_rate_table2[28] = {120, 193, 308, 449, 602, 378, 434, 490, 553, 616, 658, 466, 517, 567, \
                                            616, 666, 719, 772, 822, 873, 1365, 711, 754, 797, 841, 885, 1833, 948};
uint16_t nr_target_code_rate_table3[29] = {30, 40, 50, 64, 78, 99, 120, 157, 193, 251, 308, 379, 449, 526, 602, 340, \
                                            378, 434, 490, 553, 616, 438, 466, 517, 567, 616, 666, 719, 772};
uint16_t nr_tbs_table[93] = {24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 208, 224, 240, 256, 272, 288, 304, 320, \
                              336, 352, 368, 384, 408, 432, 456, 480, 504, 528, 552, 576, 608, 640, 672, 704, 736, 768, 808, 848, 888, 928, 984, 1032, 1064, 1128, 1160, 1192, 1224, 1256, \
                              1288, 1320, 1352, 1416, 1480, 1544, 1608, 1672, 1736, 1800, 1864, 1928, 2024, 2088, 2152, 2216, 2280, 2408, 2472, 2536, 2600, 2664, 2728, 2792, 2856, 2976, \
                              3104, 3240, 3368, 3496, 3624, 3752, 3824};

uint8_t nr_get_Qm(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 1:
      return (((Imcs<10)||(Imcs==29))?2:((Imcs<17)||(Imcs==30))?4:((Imcs<29)||(Imcs==31))?6:-1);
    break;

    case 2:
      return (((Imcs<5)||(Imcs==28))?2:((Imcs<11)||(Imcs==29))?4:((Imcs<20)||(Imcs==30))?6:((Imcs<28)||(Imcs==31))?8:-1);
    break;

    case 3:
      return (((Imcs<15)||(Imcs==29))?2:((Imcs<21)||(Imcs==30))?4:((Imcs<29)||(Imcs==31))?6:-1);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
      return(0);
      break;
  }
}

uint32_t nr_get_code_rate(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 1:
      return (nr_target_code_rate_table1[Imcs]);
    break;

    case 2:
      return (nr_target_code_rate_table2[Imcs]);
    break;

    case 3:
      return (nr_target_code_rate_table3[Imcs]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
      return(0);
      break;
  }
}

int get_subband_size(int NPRB,int size) {
  // implements table  5.2.1.4-2 from 36.214
  //
  //Bandwidth part (PRBs)	Subband size (PRBs)
  // < 24	                   N/A
  //24 – 72	                   4, 8
  //73 – 144	                   8, 16
  //145 – 275	                  16, 32

  if (NPRB<24) return(1);
  if (NPRB<72) return (size==0 ? 4 : 8);
  if (NPRB<144) return (size==0 ? 8 : 16);
  if (NPRB<275) return (size==0 ? 16 : 32);
  AssertFatal(1==0,"Shouldn't get here, NPRB %d\n",NPRB);
 
}

void SLIV2SL(int SLIV,int *S,int *L) {

  int SLIVdiv14 = SLIV/14;
  int SLIVmod14 = SLIV%14;
  // Either SLIV = 14*(L-1) + S, or SLIV = 14*(14-L+1) + (14-1-S). Condition is 0 <= L <= 14-S
  if ((SLIVdiv14 + 1) >= 0 && (SLIVdiv14 <= 13-SLIVmod14)) {
    *L=SLIVdiv14+1;
    *S=SLIVmod14;
  } else  {
    *L=15-SLIVdiv14;
    *S=13-SLIVmod14;
  }

}
