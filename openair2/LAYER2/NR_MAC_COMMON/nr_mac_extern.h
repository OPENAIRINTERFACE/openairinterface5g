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

/*! \file extern.h
* \brief NR mac externs
* \author G. Casati
* \date 2019
* \version 1.0
* \email guido.casati@iis.fraunhofer.de
* @ingroup _mac

*/

#ifndef __NR_MAC_EXTERN_H__
#define __NR_MAC_EXTERN_H__

//#include "PHY/defs_common.h"
#include "nr_mac.h"
#include "RRC/LTE/rrc_defs.h"
#include "common/ran_context.h"

/* extern const uint32_t BSR_TABLE[BSR_TABLE_SIZE];
extern const uint32_t Extended_BSR_TABLE[BSR_TABLE_SIZE];
extern const uint8_t cqi2fmt0_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];
extern const uint8_t cqi2fmt1x_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];
extern const uint8_t cqi2fmt2x_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];
extern UE_RRC_INST *UE_rrc_inst;
extern eNB_ULSCH_INFO eNB_ulsch_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs][MAX_MOBILES_PER_ENB];	// eNBxUE = 8x8
extern eNB_DLSCH_INFO eNB_dlsch_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs][MAX_MOBILES_PER_ENB];	// eNBxUE = 8x8
extern const int cqi_to_mcs[16];
extern uint32_t RRC_CONNECTION_FLAG;
extern uint8_t rb_table[34];
extern mac_rlc_am_muilist_t rlc_am_mui;
extern SCHEDULER_MODES global_scheduler_mode;
extern unsigned char NB_UE_INST;*/

extern unsigned char NB_INST;
extern unsigned char NB_eNB_INST;
extern unsigned char NB_RN_INST;
extern unsigned short NODE_ID[1];

/* Scheduler */
extern RAN_CONTEXT_t RC;
extern uint8_t nfapi_mode;

/*#if defined(PRE_SCD_THREAD)
extern uint16_t pre_nb_rbs_required[2][MAX_NUM_CCs][NUMBER_OF_UE_MAX];
extern uint8_t dlsch_ue_select_tbl_in_use;
extern uint8_t new_dlsch_ue_select_tbl_in_use;
extern boolean_t pre_scd_activeUE[NUMBER_OF_UE_MAX];
extern eNB_UE_STATS pre_scd_eNB_UE_stats[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
#endif*/

#endif //DEF_H
