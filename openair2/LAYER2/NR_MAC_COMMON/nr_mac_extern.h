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

/*! \file nr_mac_extern.h
* \brief NR mac externs
* \author  Navid Nikaein, Raymond Knopp, Guido Casati
* \date 2019
* \version 1.0
* \email navid.nikaein@eurecom.fr, guido.casati@iis.fraunhofer.de
* @ingroup _mac

*/

#ifndef __NR_MAC_EXTERN_H__
#define __NR_MAC_EXTERN_H__

#include "common/ran_context.h"
#include "nr_mac.h"

/*#include "PHY/defs_common.h"*/

extern const uint8_t nr_slots_per_frame[5];

extern dci_pdu_rel15_t *def_dci_pdu_rel15;

/* extern const uint32_t BSR_TABLE[BSR_TABLE_SIZE];
extern const uint32_t Extended_BSR_TABLE[BSR_TABLE_SIZE];
extern const uint8_t cqi2fmt0_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];
extern const uint8_t cqi2fmt1x_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];
extern const uint8_t cqi2fmt2x_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];
extern UE_RRC_INST *UE_rrc_inst;
extern const int cqi_to_mcs[16];
extern uint32_t RRC_CONNECTION_FLAG;
extern uint8_t rb_table[34];
extern mac_rlc_am_muilist_t rlc_am_mui;
extern SCHEDULER_MODES global_scheduler_mode;
extern unsigned char NB_UE_INST;*/


extern unsigned char NB_INST;
extern unsigned char NB_eNB_INST;
extern unsigned char NB_RN_INST;

/* Scheduler */
extern RAN_CONTEXT_t RC;
extern uint8_t nfapi_mode;
extern mac_rlc_am_muilist_t rlc_am_mui;
extern SCHEDULER_MODES global_scheduler_mode;

/*#if defined(PRE_SCD_THREAD)
extern const int cqi_to_mcs[16];
extern uint32_t RRC_CONNECTION_FLAG;
extern uint8_t rb_table[34];

#if defined(PRE_SCD_THREAD)
extern uint16_t pre_nb_rbs_required[2][MAX_NUM_CCs][NUMBER_OF_UE_MAX];
extern uint8_t dlsch_ue_select_tbl_in_use;
extern uint8_t new_dlsch_ue_select_tbl_in_use;
extern bool pre_scd_activeUE[NUMBER_OF_UE_MAX];
extern eNB_UE_STATS pre_scd_eNB_UE_stats[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
#endif*/

extern const uint32_t NR_SHORT_BSR_TABLE[NR_SHORT_BSR_TABLE_SIZE];
extern const uint32_t NR_LONG_BSR_TABLE[NR_LONG_BSR_TABLE_SIZE];

//	Type0-PDCCH search space
extern const int32_t table_38213_13_1_c1[16];
extern const int32_t table_38213_13_1_c2[16];
extern const int32_t table_38213_13_1_c3[16];
extern const int32_t table_38213_13_1_c4[16];

extern const int32_t table_38213_13_2_c1[16];
extern const int32_t table_38213_13_2_c2[16];
extern const int32_t table_38213_13_2_c3[16];
extern const int32_t table_38213_13_2_c4[16];

extern const int32_t table_38213_13_3_c1[16];
extern const int32_t table_38213_13_3_c2[16];
extern const int32_t table_38213_13_3_c3[16];
extern const int32_t table_38213_13_3_c4[16];

extern const int32_t table_38213_13_4_c1[16];
extern const int32_t table_38213_13_4_c2[16];
extern const int32_t table_38213_13_4_c3[16];
extern const int32_t table_38213_13_4_c4[16];

extern const int32_t table_38213_13_5_c1[16];
extern const int32_t table_38213_13_5_c2[16];
extern const int32_t table_38213_13_5_c3[16];
extern const int32_t table_38213_13_5_c4[16];

extern const int32_t table_38213_13_6_c1[16];
extern const int32_t table_38213_13_6_c2[16];
extern const int32_t table_38213_13_6_c3[16];
extern const int32_t table_38213_13_6_c4[16];

extern const int32_t table_38213_13_7_c1[16];
extern const int32_t table_38213_13_7_c2[16];
extern const int32_t table_38213_13_7_c3[16];
extern const int32_t table_38213_13_7_c4[16];

extern const int32_t table_38213_13_8_c1[16];
extern const int32_t table_38213_13_8_c2[16];
extern const int32_t table_38213_13_8_c3[16];
extern const int32_t table_38213_13_8_c4[16];

extern const int32_t table_38213_13_9_c1[16];
extern const int32_t table_38213_13_9_c2[16];
extern const int32_t table_38213_13_9_c3[16];
extern const int32_t table_38213_13_9_c4[16];

extern const int32_t table_38213_13_10_c1[16];
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

#endif //DEF_H
