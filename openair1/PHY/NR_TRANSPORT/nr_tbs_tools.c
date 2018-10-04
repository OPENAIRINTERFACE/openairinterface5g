/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file PHY/LTE_TRANSPORT/dlsch_coding.c
* \brief Top-level routines for implementing LDPC-coded (DLSCH) transport channels from 38-212, 15.2
* \author H.Wang
* \date 2018
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#include "nr_dlsch.h"

/// Target code rate tables indexed by Imcs
uint16_t nr_target_code_rate_table1[29] = {120, 157, 193, 251, 308, 379, 449, 526, 602, 679, 340, 378, 434, 490, 553, \
                                            616, 658, 438, 466, 517, 567, 616, 666, 719, 772, 822, 873, 910, 948};
  // The -1 values correspond to non integer factors which are handled differently for indexes 20(682.5) and 26(916.5)
uint16_t nr_target_code_rate_table2[28] = {120, 193, 308, 449, 602, 378, 434, 490, 553, 616, 658, 466, 517, 567, \
                                            616, 666, 719, 772, 822, 873, -1, 711, 754, 797, 841, 885, -1, 948};
uint16_t nr_target_code_rate_table3[29] = {30, 40, 50, 64, 78, 99, 120, 157, 193, 251, 308, 379, 449, 526, 602, 340, \
                                            378, 434, 490, 553, 616, 438, 466, 517, 567, 616, 666, 719, 772};


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
  }
}

uint32_t nr_get_code_rate(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 1:
      return (nr_target_code_rate_table1[Imcs]<<10);
    break;

    case 2:
      return ((Imcs==20)?0xaaa00 : (Imcs==26)?0xe5200 : nr_target_code_rate_table3[Imcs]<<10);
    break;

    case 3:
      return (nr_target_code_rate_table3[Imcs]<<10);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
  }
}

uint32_t nr_get_G(uint16_t nb_rb, uint16_t nb_symb_sch,uint8_t nb_re_dmrs,uint16_t length_dmrs) {
	uint32_t G;
	G = ((12*nb_symb_sch)-(nb_re_dmrs*length_dmrs))*nb_rb;
	return(G);
}
