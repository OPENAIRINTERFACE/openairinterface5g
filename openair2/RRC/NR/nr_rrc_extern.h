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

/*! \file nr_rrc_extern.h
* \brief rrc external vars
* \author Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
* \date 2011, 2018
* \version 1.0
* \company Eurecom, NTUST
* \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
*/

#ifndef __OPENAIR_NR_RRC_EXTERN_H__
#define __OPENAIR_NR_RRC_EXTERN_H__
#include "nr_rrc_defs.h"
#include "COMMON/mac_rrc_primitives.h"
#include "LAYER2/MAC/mac.h"
#include "LAYER2/RLC/rlc.h"
#include "openair2/RRC/LTE/rrc_defs.h"
#include "openair2/RRC/common.h"

#define NR_MAX_SIB_LENGTH 2976 // 3GPP TS 38.331 section 5.2.1 - The physical layer imposes a limit to the maximum size a SIB can take. The maximum SIB1 or SI message size is 2976 bits.

extern UE_RRC_INST *UE_rrc_inst;

extern uint8_t DRB2LCHAN[8];

extern NR_LogicalChannelConfig_t NR_SRB1_logicalChannelConfig_defaultValue;
extern NR_LogicalChannelConfig_t NR_SRB2_logicalChannelConfig_defaultValue;
extern NR_LogicalChannelConfig_t NR_SRB3_logicalChannelConfig_defaultValue;

extern int NB_UE_INST;
extern void* bigphys_malloc(int);


//CONSTANTS
extern rlc_info_t Rlc_info_um,Rlc_info_am_config;
//uint8_t RACH_TIME_ALLOC;
extern uint16_t RACH_FREQ_ALLOC;
//uint8_t NB_RACH;
extern LCHAN_DESC BCCH_LCHAN_DESC,CCCH_LCHAN_DESC,DCCH_LCHAN_DESC,DTCH_DL_LCHAN_DESC,DTCH_UL_LCHAN_DESC;
extern MAC_MEAS_T BCCH_MEAS_TRIGGER,CCCH_MEAS_TRIGGER,DCCH_MEAS_TRIGGER,DTCH_MEAS_TRIGGER;
extern MAC_AVG_T BCCH_MEAS_AVG,CCCH_MEAS_AVG,DCCH_MEAS_AVG, DTCH_MEAS_AVG;

extern UE_PF_PO_t UE_PF_PO[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB];

extern pthread_mutex_t ue_pf_po_mutex;

extern uint16_t reestablish_rnti_map[MAX_MOBILES_PER_ENB][2];
char openair_rrc_gNB_configuration(const module_id_t gnb_mod_idP, gNB_RrcConfigurationReq *configuration);

#endif


