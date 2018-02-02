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
* \brief mac externs
* \author  Navid Nikaein and Raymond Knopp
* \date 2010 - 2014
* \version 1.0
* \email navid.nikaein@eurecom.fr
* @ingroup _mac

*/

#ifndef __MAC_EXTERN_H__
#define __MAC_EXTERN_H__

#include "PHY/defs.h"
#include "defs.h"

#include "RRC/LITE/defs.h"

extern const uint32_t BSR_TABLE[BSR_TABLE_SIZE];
//extern uint32_t EBSR_Level[63];
extern const uint32_t Extended_BSR_TABLE[BSR_TABLE_SIZE];
//extern uint32_t Extended_BSR_TABLE[63];  ----currently not used 

extern const uint8_t cqi2fmt0_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];

extern const uint8_t cqi2fmt1x_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];

extern const uint8_t cqi2fmt2x_agg[MAX_SUPPORTED_BW][CQI_VALUE_RANGE];

extern UE_RRC_INST *UE_rrc_inst;
extern UE_MAC_INST *UE_mac_inst;


extern eNB_ULSCH_INFO eNB_ulsch_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs][NUMBER_OF_UE_MAX];	// eNBxUE = 8x8
extern eNB_DLSCH_INFO eNB_dlsch_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs][NUMBER_OF_UE_MAX];	// eNBxUE = 8x8




#ifndef PHYSIM
#define NB_INST 1
#else
extern unsigned char NB_INST;
#endif
extern unsigned char NB_eNB_INST;
extern unsigned char NB_UE_INST;
extern unsigned char NB_RN_INST;
extern unsigned short NODE_ID[1];


extern int cqi_to_mcs[16];

extern uint32_t RRC_CONNECTION_FLAG;

extern uint8_t rb_table[34];

extern DCI0_5MHz_TDD_1_6_t UL_alloc_pdu;

extern DCI1A_5MHz_TDD_1_6_t RA_alloc_pdu;
extern DCI1A_5MHz_TDD_1_6_t DLSCH_alloc_pdu1A;
extern DCI1A_5MHz_TDD_1_6_t BCCH_alloc_pdu;

extern DCI1A_5MHz_TDD_1_6_t CCCH_alloc_pdu;
extern DCI1_5MHz_TDD_t DLSCH_alloc_pdu;

extern DCI0_5MHz_FDD_t UL_alloc_pdu_fdd;

extern DCI1A_5MHz_FDD_t DLSCH_alloc_pdu1A_fdd;
extern DCI1A_5MHz_FDD_t RA_alloc_pdu_fdd;
extern DCI1A_5MHz_FDD_t BCCH_alloc_pdu_fdd;

extern DCI1A_5MHz_FDD_t CCCH_alloc_pdu_fdd;
extern DCI1_5MHz_FDD_t DLSCH_alloc_pdu_fdd;

extern DCI2_5MHz_2A_TDD_t DLSCH_alloc_pdu1;
extern DCI2_5MHz_2A_TDD_t DLSCH_alloc_pdu2;
extern DCI1E_5MHz_2A_M10PRB_TDD_t DLSCH_alloc_pdu1E;

#endif //DEF_H
