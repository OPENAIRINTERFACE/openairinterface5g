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

/*! \file rrc_vars.h
* \brief rrc variables
* \author Raymond Knopp and Navid Nikaein
* \date 2013
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
*/


#ifndef __OPENAIR_RRC_VARS_H__
#define __OPENAIR_RRC_VARS_H__
#include "rrc_defs.h"
#include "LAYER2/RLC/rlc.h"
#include "COMMON/mac_rrc_primitives.h"
#include "LAYER2/MAC/mac.h"
#include "openair2/RRC/NR/nr_rrc_extern.h"

UE_PF_PO_t UE_PF_PO[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB];
pthread_mutex_t ue_pf_po_mutex;
UE_RRC_INST *UE_rrc_inst = NULL;
#include "LAYER2/MAC/mac_extern.h"
extern uint16_t ue_id_g;

uint8_t DRB2LCHAN[8];

long logicalChannelGroup0 = 0;
long logicalChannelSR_Mask_r9=0;

struct LTE_LogicalChannelConfig__ul_SpecificParameters LCSRB1 =  {1,
         LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity,
         0,
         &logicalChannelGroup0
};
struct LTE_LogicalChannelConfig__ul_SpecificParameters LCSRB2 =  {3,
         LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity,
         0,
         &logicalChannelGroup0
};

struct LTE_LogicalChannelConfig__ext1 logicalChannelSR_Mask_r9_ext1 = {
.logicalChannelSR_Mask_r9=
  &logicalChannelSR_Mask_r9
};

// These are the default SRB configurations from 36.331 (Chapter 9, p. 176-179 in v8.6)
LTE_LogicalChannelConfig_t  SRB1_logicalChannelConfig_defaultValue = {.ul_SpecificParameters=
                                                                      &LCSRB1,
                                                                      .ext1=
                                                                      &logicalChannelSR_Mask_r9_ext1
                                                                     };

const LTE_LogicalChannelConfig_t SRB2_logicalChannelConfig_defaultValue = {.ul_SpecificParameters = &LCSRB2,
                                                                           .ext1 = &logicalChannelSR_Mask_r9_ext1};

//CONSTANTS
uint16_t RACH_FREQ_ALLOC;
//uint8_t NB_RACH;
LCHAN_DESC BCCH_LCHAN_DESC,CCCH_LCHAN_DESC,DCCH_LCHAN_DESC,DTCH_DL_LCHAN_DESC,DTCH_UL_LCHAN_DESC;
MAC_MEAS_T BCCH_MEAS_TRIGGER,CCCH_MEAS_TRIGGER,DCCH_MEAS_TRIGGER,DTCH_MEAS_TRIGGER;
MAC_AVG_T BCCH_MEAS_AVG, CCCH_MEAS_AVG,DCCH_MEAS_AVG, DTCH_MEAS_AVG;

// only used for RRC connection re-establishment procedure TS36.331 5.3.7
// [0]: current C-RNTI, [1]: prior C-RNTI
// insert one when eNB received RRCConnectionReestablishmentRequest message
// delete one when eNB received RRCConnectionReestablishmentComplete message
uint16_t reestablish_rnti_map[MAX_MOBILES_PER_ENB][2] = {{0}};

#endif
