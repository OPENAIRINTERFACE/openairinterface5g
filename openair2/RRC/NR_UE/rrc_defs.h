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

#include "NR_MAC_COMMON/nr_mac.h"
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
#include "NR_SIB1.h"
#include "NR_BCCH-BCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "../NR/nr_rrc_defs.h"

#define NB_NR_UE_INST 1
#define NB_CNX_UE 2//MAX_MANAGED_RG_PER_MOBILE
#define NB_SIG_CNX_UE 2 //MAX_MANAGED_RG_PER_MOBILE

#define MAX_MEAS_OBJ 6
#define MAX_MEAS_CONFIG 6
#define MAX_MEAS_ID 6

typedef uint32_t channel_t;

typedef enum {
  nr_SecondaryCellGroupConfig_r15=0,
  nr_RadioBearerConfigX_r15=1
} nsa_message_t;

#define MAX_UE_NR_CAPABILITY_SIZE 2048
typedef struct OAI_NR_UECapability_s {
  uint8_t sdu[MAX_UE_NR_CAPABILITY_SIZE];
  uint8_t sdu_size;
  NR_UE_NR_Capability_t *UE_NR_Capability;
} OAI_NR_UECapability_t;

typedef enum requested_SI_List_e {
  SIB2  = 1,
  SIB3  = 2,
  SIB4  = 4,
  SIB5  = 8,
  SIB6  = 16,
  SIB7  = 32,
  SIB8  = 64,
  SIB9  = 128
} requested_SI_List_t;

// 3GPP TS 38.300 Section 9.2.6
typedef enum RA_trigger_e {
  RA_NOT_RUNNING,
  INITIAL_ACCESS_FROM_RRC_IDLE,
  RRC_CONNECTION_REESTABLISHMENT,
  DURING_HANDOVER,
  NON_SYNCHRONISED,
  TRANSITION_FROM_RRC_INACTIVE,
  TO_ESTABLISH_TA,
  REQUEST_FOR_OTHER_SI,
  BEAM_FAILURE_RECOVERY,
} RA_trigger_t;

typedef struct NR_UE_RRC_INST_s {

    NR_MeasConfig_t        *meas_config;
    NR_CellGroupConfig_t   *cell_group_config;
    NR_ServingCellConfigCommonSIB_t *servingCellConfigCommonSIB;
    NR_CellGroupConfig_t   *scell_group_config;
    NR_RadioBearerConfig_t *radio_bearer_config;

    NR_MeasObjectToAddMod_t        *MeasObj[NB_CNX_UE][MAX_MEAS_OBJ];
    NR_ReportConfigToAddMod_t      *ReportConfig[NB_CNX_UE][MAX_MEAS_CONFIG];
    NR_QuantityConfig_t            *QuantityConfig[NB_CNX_UE];
    NR_MeasIdToAddMod_t            *MeasId[NB_CNX_UE][MAX_MEAS_ID];
    NR_MeasGapConfig_t             *measGapConfig[NB_CNX_UE];
    NR_RSRP_Range_t                s_measure;
    NR_SRB_ToAddMod_t              *SRB1_config[NB_CNX_UE];
    NR_SRB_ToAddMod_t              *SRB2_config[NB_CNX_UE];
    NR_DRB_ToAddMod_t              *DRB_config[NB_CNX_UE][8];
    rb_id_t                        *defaultDRB; // remember the ID of the default DRB

    char                           *uecap_file;

    NR_SRB_INFO                    Srb0[NB_SIG_CNX_UE];
    NR_SRB_INFO_TABLE_ENTRY        Srb1[NB_CNX_UE];
    NR_SRB_INFO_TABLE_ENTRY        Srb2[NB_CNX_UE];

    uint8_t                        MBMS_flag;
    OAI_NR_UECapability_t          *UECap;
    uint8_t 			   *UECapability;
    uint8_t                        UECapability_size;

    RA_trigger_t                   ra_trigger;
    BIT_STRING_t                   requested_SI_List;

    NR_SystemInformation_t         *si[NB_CNX_UE];
    NR_SIB1_t                      *sib1[NB_CNX_UE];
    NR_SIB2_t                      *sib2[NB_CNX_UE];
    NR_SIB3_t                      *sib3[NB_CNX_UE];
    NR_SIB4_t                      *sib4[NB_CNX_UE];
    NR_SIB5_t                      *sib5[NB_CNX_UE];
    NR_SIB6_t                      *sib6[NB_CNX_UE];
    NR_SIB7_t                      *sib7[NB_CNX_UE];
    NR_SIB8_t                      *sib8[NB_CNX_UE];
    NR_SIB9_t                      *sib9[NB_CNX_UE];
    NR_SIB10_r16_t                 *sib10[NB_CNX_UE];
    NR_SIB11_r16_t                 *sib11[NB_CNX_UE];
    NR_SIB12_r16_t                 *sib12[NB_CNX_UE];
    NR_SIB13_r16_t                 *sib13[NB_CNX_UE];
    NR_SIB14_r16_t                 *sib14[NB_CNX_UE];
    plmn_t                         plmnID;

    NR_UE_RRC_INFO                 Info[NB_SIG_CNX_UE];

    NR_MIB_t *mib;

    /* KeNB as computed from parameters within USIM card */
    uint8_t kgnb[32];
    /* Used integrity/ciphering algorithms */
    //RRC_LIST_TYPE(NR_SecurityAlgorithmConfig_t, NR_SecurityAlgorithmConfig) SecurityAlgorithmConfig_list;
    NR_CipheringAlgorithm_t  cipheringAlgorithm;
    e_NR_IntegrityProtAlgorithm  integrityProtAlgorithm;
    
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

    long               selected_plmn_identity;
    Rrc_State_NR_t     nrRrcState;
    Rrc_Sub_State_NR_t nrRrcSubState;
    as_nas_info_t      initialNasMsg;
} NR_UE_RRC_INST_t;

#endif
/** @} */
