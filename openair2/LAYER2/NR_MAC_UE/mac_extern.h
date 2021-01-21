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

//  DCI extraction
//  for PUSCH from TS 38.214 subclause 6.1.2.1.1
extern uint8_t table_6_1_2_1_1_2_time_dom_res_alloc_A[16][3];
//  for PDSCH from TS 38.214 subclause 5.1.2.1.1
extern uint8_t table_5_1_2_1_1_2_time_dom_res_alloc_A[16][3];

extern int64_t table_6_3_3_2_3_prachConfig_Index [256][9];

// DCI
extern const uint8_t table_7_3_1_1_2_2_3_4_5[64][20];
extern const uint8_t table_7_3_1_1_2_12[14][3];
extern const uint8_t table_7_3_1_1_2_13[10][4];
extern const uint8_t table_7_3_1_1_2_14[3][5];
extern const uint8_t table_7_3_1_1_2_15[4][6];
extern const uint8_t table_7_3_1_1_2_16[12][2];
extern const uint8_t table_7_3_1_1_2_17[7][3];
extern const uint8_t table_7_3_1_1_2_18[3][4];
extern const uint8_t table_7_3_1_1_2_19[2][5];
extern const uint8_t table_7_3_1_1_2_20[28][3];
extern const uint8_t table_7_3_1_1_2_21[19][4];
extern const uint8_t table_7_3_1_1_2_22[6][5];
extern const uint8_t table_7_3_1_1_2_23[5][6];
extern const uint8_t table_7_3_2_3_3_1[12][5];
extern const uint8_t table_7_3_2_3_3_2_oneCodeword[31][6];
extern const uint8_t table_7_3_2_3_3_2_twoCodeword[4][10];
extern const uint8_t table_7_3_2_3_3_3_oneCodeword[24][5];
extern const uint8_t table_7_3_2_3_3_3_twoCodeword[2][7];
extern const uint8_t table_7_3_2_3_3_4_oneCodeword[58][6];
extern const uint8_t table_7_3_2_3_3_4_twoCodeword[6][10];

extern const uint16_t table_7_2_1[16];

extern dci_pdu_rel15_t *def_dci_pdu_rel15;

extern dci_pdu_rel15_t *def_dci_pdu_rel15;

extern void mac_rlc_data_ind(const module_id_t         module_idP,
                             const rnti_t              rntiP,
                             const eNB_index_t         eNB_index,
                             const frame_t             frameP,
                             const eNB_flag_t          enb_flagP,
                             const MBMS_flag_t         MBMS_flagP,
                             const logical_chan_id_t   channel_idP,
                             char                     *buffer_pP,
                             const tb_size_t           tb_sizeP,
                             num_tb_t                  num_tbP,
                             crc_t                    *crcs_pP);

extern const char *rnti_types[];
extern const char *dci_formats[];
