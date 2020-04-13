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

/* \file rrc_defs.h
 * \brief RRC structures/types definition
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __OPENAIR_NR_RRC_DEFS_H__
#define __OPENAIR_NR_RRC_DEFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform_types.h"

#include "LAYER2/NR_MAC_UE/mac.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "rrc_list.h"
#include "NR_asn_constant.h"
#include "NR_MeasConfig.h"
#include "NR_CellGroupConfig.h"
#include "NR_RadioBearerConfig.h"
#include "NR_RLC-BearerConfig.h"
#include "NR_TAG.h"
#include "NR_asn_constant.h"
#include "NR_SchedulingRequestToAddMod.h"
#include "NR_MIB.h"
#include "NR_BCCH-BCH-Message.h"
#include "NR_DL-DCCH-Message.h"

#define NB_NR_UE_INST 1

typedef uint32_t channel_t;

typedef enum {
  nr_SecondaryCellGroupConfig_r15=0,
  nr_RadioBearerConfigX_r15=1
} nsa_message_t;

typedef struct NR_UE_RRC_INST_s {

    NR_MeasConfig_t        *meas_config;
    NR_CellGroupConfig_t   *cell_group_config;
    NR_RadioBearerConfig_t *radio_bearer_config;
  
    NR_MIB_t *mib;
  
    //  lists
    //  CellGroupConfig.rlc-BearerToAddModList
    RRC_LIST_TYPE(NR_RLC_BearerConfig_t, NR_maxLC_ID) RLC_Bearer_Config_list;
    //  CellGroupConfig.mac-CellGroupConfig.schedulingrequest
    RRC_LIST_TYPE(NR_SchedulingRequestToAddMod_t, NR_maxNrofSR_ConfigPerCellGroup) SchedulingRequest_list;
    //  CellGroupConfig.mac-CellGroupConfig.TAG
    RRC_LIST_TYPE(NR_TAG_t, NR_maxNrofTAGs) TAG_list;
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.tdduldlslotconfig
    RRC_LIST_TYPE(NR_TDD_UL_DL_SlotConfig_t, NR_maxNrofSlots) TDD_UL_DL_SlotConfig_list;
   
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.bwps 
    RRC_LIST_TYPE(NR_BWP_Downlink_t, NR_maxNrofBWPs) BWP_Downlink_list;
    //BWP-DownlinkDedicated 0=INIT-DL-BWP, 1..4 for DL-BWPs
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdcchconfig.controlresourceset
    RRC_LIST_TYPE(NR_ControlResourceSet_t, 3) ControlResourceSet_list[5];
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdcchconfig.searchspace
    RRC_LIST_TYPE(NR_SearchSpace_t, 10) SearchSpace_list[5];
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdcchconfig.slotformatindicator
    RRC_LIST_TYPE(NR_SlotFormatCombinationsPerCell_t, NR_maxNrofAggregatedCellsPerCellGroup) SlotFormatCombinationsPerCell_list[5];
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdschconfig
    RRC_LIST_TYPE(NR_TCI_State_t, NR_maxNrofTCI_States) TCI_State_list[5];
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdschconfig
    RRC_LIST_TYPE(NR_RateMatchPattern_t, NR_maxNrofRateMatchPatterns) RateMatchPattern_list[5];
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdschconfig
    RRC_LIST_TYPE(NR_ZP_CSI_RS_Resource_t, NR_maxNrofZP_CSI_RS_Resources) ZP_CSI_RS_Resource_list[5];
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdschconfig
    RRC_LIST_TYPE(NR_ZP_CSI_RS_ResourceSet_t, NR_maxNrofZP_CSI_RS_ResourceSets) Aperidic_ZP_CSI_RS_ResourceSet_list[5];
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdschconfig
    RRC_LIST_TYPE(NR_ZP_CSI_RS_ResourceSet_t, NR_maxNrofZP_CSI_RS_ResourceSets) SP_ZP_CSI_RS_ResourceSet_list[5];

    //  TODO check the way to implement mutiple list inside bwps
    //  uplink bwp also

    //  CellGroupConfig.spCellConfig.spCellConfigDedicated
    RRC_LIST_TYPE(NR_NZP_CSI_RS_Resource_t, NR_maxNrofNZP_CSI_RS_Resources) NZP_CSI_RS_Resource_list;
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated
    RRC_LIST_TYPE(NR_NZP_CSI_RS_ResourceSet_t, NR_maxNrofNZP_CSI_RS_ResourceSets) NZP_CSI_RS_ResourceSet_list;
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated
    RRC_LIST_TYPE(NR_CSI_IM_Resource_t, NR_maxNrofCSI_IM_Resources) CSI_IM_Resource_list;
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated
    RRC_LIST_TYPE(NR_CSI_IM_ResourceSet_t, NR_maxNrofCSI_IM_ResourceSets) CSI_IM_ResourceSet_list;
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated
    RRC_LIST_TYPE(NR_CSI_SSB_ResourceSet_t, NR_maxNrofCSI_SSB_ResourceSets) CSI_SSB_ResourceSet_list;
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated
    RRC_LIST_TYPE(NR_CSI_ResourceConfig_t, NR_maxNrofCSI_ResourceConfigurations) CSI_ResourceConfig_list;
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated
    RRC_LIST_TYPE(NR_CSI_ReportConfig_t, NR_maxNrofCSI_ReportConfigurations) CSI_ReportConfig_list;
  
} NR_UE_RRC_INST_t;

#endif
/** @} */
