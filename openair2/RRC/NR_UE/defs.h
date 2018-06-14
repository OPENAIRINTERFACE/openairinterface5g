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

/*! \file RRC/LITE/defs.h
* \brief RRC struct definitions and function prototypes
* \author Navid Nikaein and Raymond Knopp
* \date 2010 - 2014
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr
*/

#ifndef __OPENAIR_NR_RRC_DEFS_H__
#define __OPENAIR_NR_RRC_DEFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rrc_list.h"

#include "collection/tree.h"
#include "rrc_types.h"
#include "PHY/defs.h"
#include "LAYER2/RLC/rlc.h"

#include "COMMON/platform_constants.h"
#include "COMMON/platform_types.h"

#include "LAYER2/MAC/defs.h"

#include "SystemInformationBlockType1.h"
#include "SystemInformation.h"
#include "RRCConnectionReconfiguration.h"
#include "RRCConnectionReconfigurationComplete.h"
#include "RRCConnectionSetup.h"
#include "RRCConnectionSetupComplete.h"
#include "RRCConnectionRequest.h"
#include "RRCConnectionReestablishmentRequest.h"
#include "BCCH-DL-SCH-Message.h"
#include "BCCH-BCH-Message.h"
#if defined(Rel10) || defined(Rel14)
#include "MCCH-Message.h"
#include "MBSFNAreaConfiguration-r9.h"
#include "SCellToAddMod-r10.h"
#endif
#include "AS-Config.h"
#include "AS-Context.h"
#include "UE-EUTRA-Capability.h"
#include "MeasResults.h"

/* for ImsiMobileIdentity_t */
#include "MobileIdentity.h"

typedef struct NR_UE_RRC_INST_s {
  nr_rrc_state_t     rrc_state;
  nr_rrc_sub_state_t rrc_sub_state;
#if 0
  OAI_UECapability_t *UECap;
  uint8_t *UECapability;
  uint8_t UECapability_size;
  UE_RRC_INFO Info[NB_SIG_CNX_UE];
  SRB_INFO Srb0[NB_SIG_CNX_UE];
  SRB_INFO_TABLE_ENTRY Srb1[NB_CNX_UE];
  SRB_INFO_TABLE_ENTRY Srb2[NB_CNX_UE];
  HANDOVER_INFO_UE HandoverInfoUe;
  uint8_t *SIB1[NB_CNX_UE];
  uint8_t sizeof_SIB1[NB_CNX_UE];
  uint8_t *SI[NB_CNX_UE];
  uint8_t sizeof_SI[NB_CNX_UE];
  uint8_t SIB1Status[NB_CNX_UE];
  uint8_t SIStatus[NB_CNX_UE];
  SystemInformationBlockType1_t *sib1[NB_CNX_UE];
  SystemInformation_t *si[NB_CNX_UE]; //!< Temporary storage for an SI message. Decoding happens in decode_SI().
  SystemInformationBlockType2_t *sib2[NB_CNX_UE];
  SystemInformationBlockType3_t *sib3[NB_CNX_UE];
  SystemInformationBlockType4_t *sib4[NB_CNX_UE];
  SystemInformationBlockType5_t *sib5[NB_CNX_UE];
  SystemInformationBlockType6_t *sib6[NB_CNX_UE];
  SystemInformationBlockType7_t *sib7[NB_CNX_UE];
  SystemInformationBlockType8_t *sib8[NB_CNX_UE];
  SystemInformationBlockType9_t *sib9[NB_CNX_UE];
  SystemInformationBlockType10_t *sib10[NB_CNX_UE];
  SystemInformationBlockType11_t *sib11[NB_CNX_UE];

#if defined(Rel10) || defined(Rel14)
  uint8_t                           MBMS_flag;
  uint8_t *MCCH_MESSAGE[NB_CNX_UE];
  uint8_t sizeof_MCCH_MESSAGE[NB_CNX_UE];
  uint8_t MCCH_MESSAGEStatus[NB_CNX_UE];
  MBSFNAreaConfiguration_r9_t       *mcch_message[NB_CNX_UE];
  SystemInformationBlockType12_r9_t *sib12[NB_CNX_UE];
  SystemInformationBlockType13_r9_t *sib13[NB_CNX_UE];
#endif
#ifdef CBA
  uint8_t                         num_active_cba_groups;
  uint16_t                        cba_rnti[NUM_MAX_CBA_GROUP];
#endif
  uint8_t                         num_srb;
  struct SRB_ToAddMod             *SRB1_config[NB_CNX_UE];
  struct SRB_ToAddMod             *SRB2_config[NB_CNX_UE];
  struct DRB_ToAddMod             *DRB_config[NB_CNX_UE][8];
  rb_id_t                         *defaultDRB; // remember the ID of the default DRB
  MeasObjectToAddMod_t            *MeasObj[NB_CNX_UE][MAX_MEAS_OBJ];
  struct ReportConfigToAddMod     *ReportConfig[NB_CNX_UE][MAX_MEAS_CONFIG];
  struct QuantityConfig           *QuantityConfig[NB_CNX_UE];
  struct MeasIdToAddMod           *MeasId[NB_CNX_UE][MAX_MEAS_ID];
  MEAS_REPORT_LIST      *measReportList[NB_CNX_UE][MAX_MEAS_ID];
  uint32_t           measTimer[NB_CNX_UE][MAX_MEAS_ID][6]; // 6 neighboring cells
  RSRP_Range_t                    s_measure;
  struct MeasConfig__speedStatePars *speedStatePars;
  struct PhysicalConfigDedicated  *physicalConfigDedicated[NB_CNX_UE];
  struct SPS_Config               *sps_Config[NB_CNX_UE];
  MAC_MainConfig_t                *mac_MainConfig[NB_CNX_UE];
  MeasGapConfig_t                 *measGapConfig[NB_CNX_UE];
  double                          filter_coeff_rsrp; // [7] ???
  double                          filter_coeff_rsrq; // [7] ???
  float                           rsrp_db[7];
  float                           rsrq_db[7];
  float                           rsrp_db_filtered[7];
  float                           rsrq_db_filtered[7];
#if defined(ENABLE_SECURITY)
  /* KeNB as computed from parameters within USIM card */
  uint8_t kenb[32];
  uint8_t nh[32];
  int8_t  nh_ncc;
#endif

  /* Used integrity/ciphering algorithms */
  CipheringAlgorithm_r12_t                          ciphering_algorithm;
  e_SecurityAlgorithmConfig__integrityProtAlgorithm integrity_algorithm;
#endif

	NR_MeasConfig_t 		*meas_config;
	NR_CellGroupConfig_t 	*cell_group_config;
	NR_RadioBearerConfig_t *radio_bearer_config;
  
    //  lists
    //  CellGroupConfig.rlc-BearerToAddModList
    RRC_LIST_TYPE(NR_RLC_Bearer_Config_t, NR_maxLC_ID) RLC_Bearer_Config_list;
    //  CellGroupConfig.mac-CellGroupConfig.schedulingrequest
    RRC_LIST_TYPE(NR_SchedulingRequestToAddMod_t, NR_maxNrofSR_ConfigPerCellGroup) SchedulingRequest_list;
    //  CellGroupConfig.mac-CellGroupConfig.TAG
    RRC_LIST_TYPE(NR_TAG_ToAddMod_t, NR_maxNrofTAGs) TAG_list;
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
    RRC_LIST_TYPE(NR_ZP_CSI_RS_ResourceSet_t, NR_maxNrofZP_CSI_RS_Sets) Aperidic_ZP_CSI_RS_ResourceSet_list[5];
    //  CellGroupConfig.spCellConfig.spCellConfigDedicated.initialdlbwp.pdschconfig
    RRC_LIST_TYPE(NR_ZP_CSI_RS_ResourceSet_t, NR_maxNrofZP_CSI_RS_Sets) SP_ZP_CSI_RS_ResourceSet_list[5];

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

#include "proto.h"

#endif
/** @} */
