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

/* \file extern.h
 * \brief extern variables for MAC layer
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include "mac_defs.h"

//extern NR_UE_MAC_INST_t *UE_mac_inst;

//	Type0-PDCCH search space
extern const int32_t table_38213_13_1_c2[16];
extern const int32_t table_38213_13_1_c3[16];
extern const int32_t table_38213_13_1_c4[16];

extern const int32_t table_38213_13_2_c2[16];
extern const int32_t table_38213_13_2_c3[16];
extern const int32_t table_38213_13_2_c4[16];

extern const int32_t table_38213_13_3_c2[16];
extern const int32_t table_38213_13_3_c3[16];
extern const int32_t table_38213_13_3_c4[16];

extern const int32_t table_38213_13_4_c2[16];
extern const int32_t table_38213_13_4_c3[16];
extern const int32_t table_38213_13_4_c4[16];

extern const int32_t table_38213_13_5_c2[16];
extern const int32_t table_38213_13_5_c3[16];
extern const int32_t table_38213_13_5_c4[16];

extern const int32_t table_38213_13_6_c2[16];
extern const int32_t table_38213_13_6_c3[16];
extern const int32_t table_38213_13_6_c4[16];

extern const int32_t table_38213_13_7_c2[16];
extern const int32_t table_38213_13_7_c3[16];
extern const int32_t table_38213_13_7_c4[16];

extern const int32_t table_38213_13_8_c2[16];
extern const int32_t table_38213_13_8_c3[16];
extern const int32_t table_38213_13_8_c4[16];

extern const int32_t table_38213_13_9_c2[16];
extern const int32_t table_38213_13_9_c3[16];
extern const int32_t table_38213_13_9_c4[16];

extern const int32_t table_38213_13_10_c2[16];
extern const int32_t table_38213_13_10_c3[16];
extern const int32_t table_38213_13_10_c4[16];

extern const float   table_38213_13_11_c1[16];
extern const int32_t table_38213_13_11_c2[16];
extern const float   table_38213_13_11_c3[16];
extern const int32_t table_38213_13_11_c4[16];

extern const float   table_38213_13_12_c1[16];
extern const int32_t table_38213_13_12_c2[16];
extern const float   table_38213_13_12_c3[16];

extern const int32_t table_38213_10_1_1_c2[5];

//  DCI extraction
//  for PUSCH from TS 38.214 subclause 6.1.2.1.1
extern uint8_t table_6_1_2_1_1_2_time_dom_res_alloc_A[16][3];
//  for PDSCH from TS 38.214 subclause 5.1.2.1.1
extern uint8_t table_5_1_2_1_1_2_time_dom_res_alloc_A[16][3];