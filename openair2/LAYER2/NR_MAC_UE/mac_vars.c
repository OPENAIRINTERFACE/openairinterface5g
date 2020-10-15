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

/* \file vars.h
 * \brief MAC Layer variables
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include "mac_defs.h"

/*
 #define mu_pusch 1
// definition table j Table 6.1.2.1.1-4
 #define j ((mu_pusch==3)?3:(mu_pusch==2)?2:1)
 uint8_t table_6_1_2_1_1_2_time_dom_res_alloc_A[16][3]={ // for PUSCH from TS 38.214 subclause 6.1.2.1.1
  {j,  0,14}, // row index 1
  {j,  0,12}, // row index 2
  {j,  0,10}, // row index 3
  {j,  2,10}, // row index 4
  {j,  4,10}, // row index 5
  {j,  4,8},  // row index 6
  {j,  4,6},  // row index 7
  {j+1,0,14}, // row index 8
  {j+1,0,12}, // row index 9
  {j+1,0,10}, // row index 10
  {j+2,0,14}, // row index 11
  {j+2,0,12}, // row index 12
  {j+2,0,10}, // row index 13
  {j,  8,6},  // row index 14
  {j+3,0,14}, // row index 15
  {j+3,0,10}  // row index 16
  };

 #define dmrs_typeA_pos 2
 uint8_t table_5_1_2_1_1_2_time_dom_res_alloc_A[16][3]={ // for PDSCH from TS 38.214 subclause 5.1.2.1.1
  {0,(dmrs_typeA_pos == 2)?2:3, (dmrs_typeA_pos == 2)?12:11}, // row index 1
  {0,(dmrs_typeA_pos == 2)?2:3, (dmrs_typeA_pos == 2)?10:9},  // row index 2
  {0,(dmrs_typeA_pos == 2)?2:3, (dmrs_typeA_pos == 2)?9:8},   // row index 3
  {0,(dmrs_typeA_pos == 2)?2:3, (dmrs_typeA_pos == 2)?7:6},   // row index 4
  {0,(dmrs_typeA_pos == 2)?2:3, (dmrs_typeA_pos == 2)?5:4},   // row index 5
  {0,(dmrs_typeA_pos == 2)?9:10,(dmrs_typeA_pos == 2)?4:4},   // row index 6
  {0,(dmrs_typeA_pos == 2)?4:6, (dmrs_typeA_pos == 2)?4:4},   // row index 7
  {0,5,7},  // row index 8
  {0,5,2},  // row index 9
  {0,9,2},  // row index 10
  {0,12,2}, // row index 11
  {0,1,13}, // row index 12
  {0,1,6},  // row index 13
  {0,2,4},  // row index 14
  {0,4,7},  // row index 15
  {0,8,4}   // row index 16
  };
*/
