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

/*! \file vars.h
* \brief rrc external vars
* \author Navid Nikaein and Raymond Knopp
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
*/

#ifndef __OPENAIR_RRC_EXTERN_H__
#define __OPENAIR_RRC_EXTERN_H__
#include "rrc_defs.h"
#include "COMMON/mac_rrc_primitives.h"
#include "LAYER2/MAC/mac.h"
#include "LAYER2/RLC/rlc.h"

extern UE_RRC_INST *UE_rrc_inst;


extern uint8_t DRB2LCHAN[8];

extern LTE_LogicalChannelConfig_t SRB1_logicalChannelConfig_defaultValue;
extern LTE_LogicalChannelConfig_t SRB2_logicalChannelConfig_defaultValue;


extern unsigned char NB_INST;
extern unsigned char NB_eNB_INST;
extern uint16_t NB_UE_INST;
extern void* bigphys_malloc(int);


//CONSTANTS
extern rlc_info_t Rlc_info_um,Rlc_info_am_config;
//uint8_t RACH_TIME_ALLOC;
extern uint16_t RACH_FREQ_ALLOC;
//uint8_t NB_RACH;
extern LCHAN_DESC BCCH_LCHAN_DESC,CCCH_LCHAN_DESC,DCCH_LCHAN_DESC,DTCH_DL_LCHAN_DESC,DTCH_UL_LCHAN_DESC;
extern MAC_MEAS_T BCCH_MEAS_TRIGGER,CCCH_MEAS_TRIGGER,DCCH_MEAS_TRIGGER,DTCH_MEAS_TRIGGER;
extern MAC_AVG_T BCCH_MEAS_AVG,CCCH_MEAS_AVG,DCCH_MEAS_AVG, DTCH_MEAS_AVG;

extern uint16_t T300[8];
extern uint16_t T310[8];
extern uint16_t N310[8];
extern uint16_t N311[8];
extern uint32_t T304[8];
extern uint32_t timeToTrigger_ms[16];
extern float RSRP_meas_mapping[98];
extern float RSRQ_meas_mapping[35];

extern UE_PF_PO_t UE_PF_PO[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB];
extern pthread_mutex_t ue_pf_po_mutex;

extern uint16_t reestablish_rnti_map[MAX_MOBILES_PER_ENB][2];

#endif


