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

/*
 * rrc_messages_types.h
 *
 *  Created on: Oct 24, 2013
 *      Author: winckel and Navid Nikaein
 */

#ifndef RRC_MESSAGES_TYPES_H_
#define RRC_MESSAGES_TYPES_H_

#include "as_message.h"
#include "rrc_types.h"
#include "s1ap_messages_types.h"
#ifdef CMAKER
#include "SystemInformationBlockType2.h"
#else
#include "RRC/LTE/MESSAGES/SystemInformationBlockType2.h"
#endif
#include "SL-OffsetIndicator-r12.h"
#include "SubframeBitmapSL-r12.h"
#include "SL-CP-Len-r12.h"
#include "SL-PeriodComm-r12.h"
#include "SL-DiscResourcePool-r12.h"


//-------------------------------------------------------------------------------------------//
// Messages for RRC logging
#if defined(DISABLE_ITTI_XER_PRINT)
#include "BCCH-DL-SCH-Message.h"
#include "DL-CCCH-Message.h"
#include "DL-DCCH-Message.h"
#include "UE-EUTRA-Capability.h"
#include "UL-CCCH-Message.h"
#include "UL-DCCH-Message.h"

typedef BCCH_DL_SCH_Message_t   RrcDlBcchMessage;
typedef DL_CCCH_Message_t       RrcDlCcchMessage;
typedef DL_DCCH_Message_t       RrcDlDcchMessage;
typedef UE_EUTRA_Capability_t   RrcUeEutraCapability;
typedef UL_CCCH_Message_t       RrcUlCcchMessage;
typedef UL_DCCH_Message_t       RrcUlDcchMessage;
#endif

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.
#define RRC_STATE_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_state_ind

#define RRC_CONFIGURATION_REQ(mSGpTR)   (mSGpTR)->ittiMsg.rrc_configuration_req

#define NBIOTRRC_CONFIGURATION_REQ(mSGpTR)   (mSGpTR)->ittiMsg.nbiotrrc_configuration_req

#define NRRRC_CONFIGURATION_REQ(mSGpTR)   (mSGpTR)->ittiMsg.nrrrc_configuration_req

#define NAS_KENB_REFRESH_REQ(mSGpTR)    (mSGpTR)->ittiMsg.nas_kenb_refresh_req
#define NAS_CELL_SELECTION_REQ(mSGpTR)  (mSGpTR)->ittiMsg.nas_cell_selection_req
#define NAS_CONN_ESTABLI_REQ(mSGpTR)    (mSGpTR)->ittiMsg.nas_conn_establi_req
#define NAS_UPLINK_DATA_REQ(mSGpTR)     (mSGpTR)->ittiMsg.nas_ul_data_req

#define NAS_RAB_ESTABLI_RSP(mSGpTR)     (mSGpTR)->ittiMsg.nas_rab_est_rsp

#define NAS_CELL_SELECTION_CNF(mSGpTR)  (mSGpTR)->ittiMsg.nas_cell_selection_cnf
#define NAS_CELL_SELECTION_IND(mSGpTR)  (mSGpTR)->ittiMsg.nas_cell_selection_ind
#define NAS_PAGING_IND(mSGpTR)          (mSGpTR)->ittiMsg.nas_paging_ind
#define NAS_CONN_ESTABLI_CNF(mSGpTR)    (mSGpTR)->ittiMsg.nas_conn_establi_cnf
#define NAS_CONN_RELEASE_IND(mSGpTR)    (mSGpTR)->ittiMsg.nas_conn_release_ind
#define NAS_UPLINK_DATA_CNF(mSGpTR)     (mSGpTR)->ittiMsg.nas_ul_data_cnf
#define NAS_DOWNLINK_DATA_IND(mSGpTR)   (mSGpTR)->ittiMsg.nas_dl_data_ind

//-------------------------------------------------------------------------------------------//
typedef struct RrcStateInd_s {
  Rrc_State_t     state;
  Rrc_Sub_State_t sub_state;
} RrcStateInd;

// eNB: ENB_APP -> RRC messages
typedef struct RrcConfigurationReq_s {
  uint32_t                cell_identity;

  uint16_t                tac;

  uint16_t                mcc;
  uint16_t                mnc;
  uint8_t                 mnc_digit_length;

  
  paging_drx_t            default_drx;
  int16_t                 nb_cc;
  lte_frame_type_t        frame_type[MAX_NUM_CCs];
  uint8_t                 tdd_config[MAX_NUM_CCs];
  uint8_t                 tdd_config_s[MAX_NUM_CCs];
  lte_prefix_type_t       prefix_type[MAX_NUM_CCs];
  uint8_t                 pbch_repetition[MAX_NUM_CCs];
  int16_t                 eutra_band[MAX_NUM_CCs];
  uint32_t                downlink_frequency[MAX_NUM_CCs];
  int32_t                 uplink_frequency_offset[MAX_NUM_CCs];
  int16_t                 Nid_cell[MAX_NUM_CCs];// for testing, change later
  int16_t                 N_RB_DL[MAX_NUM_CCs];// for testing, change later
  int                     nb_antenna_ports[MAX_NUM_CCs];
  long                    prach_root[MAX_NUM_CCs];
  long                    prach_config_index[MAX_NUM_CCs];
  BOOLEAN_t               prach_high_speed[MAX_NUM_CCs];
  long                    prach_zero_correlation[MAX_NUM_CCs];
  long                    prach_freq_offset[MAX_NUM_CCs];
  long                    pucch_delta_shift[MAX_NUM_CCs];
  long                    pucch_nRB_CQI[MAX_NUM_CCs];
  long                    pucch_nCS_AN[MAX_NUM_CCs];
//#if (RRC_VERSION < MAKE_VERSION(10, 0, 0))
  long                    pucch_n1_AN[MAX_NUM_CCs];
//#endif
  long                    pdsch_referenceSignalPower[MAX_NUM_CCs];
  long                    pdsch_p_b[MAX_NUM_CCs];
  long                    pusch_n_SB[MAX_NUM_CCs];
  long                    pusch_hoppingMode[MAX_NUM_CCs];
  long                    pusch_hoppingOffset[MAX_NUM_CCs];
  BOOLEAN_t               pusch_enable64QAM[MAX_NUM_CCs];
  BOOLEAN_t               pusch_groupHoppingEnabled[MAX_NUM_CCs];
  long                    pusch_groupAssignment[MAX_NUM_CCs];
  BOOLEAN_t               pusch_sequenceHoppingEnabled[MAX_NUM_CCs];
  long                    pusch_nDMRS1[MAX_NUM_CCs];
  long                    phich_duration[MAX_NUM_CCs];
  long                    phich_resource[MAX_NUM_CCs];
  BOOLEAN_t               srs_enable[MAX_NUM_CCs];
  long                    srs_BandwidthConfig[MAX_NUM_CCs];
  long                    srs_SubframeConfig[MAX_NUM_CCs];
  BOOLEAN_t               srs_ackNackST[MAX_NUM_CCs];
  BOOLEAN_t               srs_MaxUpPts[MAX_NUM_CCs];
  long                    pusch_p0_Nominal[MAX_NUM_CCs];
  long                    pusch_alpha[MAX_NUM_CCs];
  long                    pucch_p0_Nominal[MAX_NUM_CCs];
  long                    msg3_delta_Preamble[MAX_NUM_CCs];
  long                    ul_CyclicPrefixLength[MAX_NUM_CCs];
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format1                    pucch_deltaF_Format1[MAX_NUM_CCs];
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format1b                   pucch_deltaF_Format1b[MAX_NUM_CCs];
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format2                    pucch_deltaF_Format2[MAX_NUM_CCs];
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format2a                   pucch_deltaF_Format2a[MAX_NUM_CCs];
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format2b                   pucch_deltaF_Format2b[MAX_NUM_CCs];
  long                    rach_numberOfRA_Preambles[MAX_NUM_CCs];
  BOOLEAN_t               rach_preamblesGroupAConfig[MAX_NUM_CCs];
  long                    rach_sizeOfRA_PreamblesGroupA[MAX_NUM_CCs];
  long                    rach_messageSizeGroupA[MAX_NUM_CCs];
  e_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB                    rach_messagePowerOffsetGroupB[MAX_NUM_CCs];
  long                    rach_powerRampingStep[MAX_NUM_CCs];
  long                    rach_preambleInitialReceivedTargetPower[MAX_NUM_CCs];
  long                    rach_preambleTransMax[MAX_NUM_CCs];
  long                    rach_raResponseWindowSize[MAX_NUM_CCs];
  long                    rach_macContentionResolutionTimer[MAX_NUM_CCs];
  long                    rach_maxHARQ_Msg3Tx[MAX_NUM_CCs];
  long                    bcch_modificationPeriodCoeff[MAX_NUM_CCs];
  long                    pcch_defaultPagingCycle[MAX_NUM_CCs];
  long                    pcch_nB[MAX_NUM_CCs];
  long                    ue_TimersAndConstants_t300[MAX_NUM_CCs];
  long                    ue_TimersAndConstants_t301[MAX_NUM_CCs];
  long                    ue_TimersAndConstants_t310[MAX_NUM_CCs];
  long                    ue_TimersAndConstants_t311[MAX_NUM_CCs];
  long                    ue_TimersAndConstants_n310[MAX_NUM_CCs];
  long                    ue_TimersAndConstants_n311[MAX_NUM_CCs];
  long                    ue_TransmissionMode[MAX_NUM_CCs];
  long                    ue_multiple_max[MAX_NUM_CCs];

  //TTN - for D2D
  //SIB18
  e_SL_CP_Len_r12                rxPool_sc_CP_Len[MAX_NUM_CCs];
  e_SL_PeriodComm_r12            rxPool_sc_Period[MAX_NUM_CCs];
  e_SL_CP_Len_r12                rxPool_data_CP_Len[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_prb_Num[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_prb_Start[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_prb_End[MAX_NUM_CCs];
  SL_OffsetIndicator_r12_PR      rxPool_ResourceConfig_offsetIndicator_present[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_offsetIndicator_choice[MAX_NUM_CCs];
  SubframeBitmapSL_r12_PR        rxPool_ResourceConfig_subframeBitmap_present[MAX_NUM_CCs];
  char*                          rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_subframeBitmap_choice_bs_size[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[MAX_NUM_CCs];

  //SIB19
  //for discRxPool
  SL_CP_Len_r12_t                discRxPool_cp_Len[MAX_NUM_CCs];
  e_SL_DiscResourcePool_r12__discPeriod_r12                   discRxPool_discPeriod[MAX_NUM_CCs];
  long                           discRxPool_numRetx[MAX_NUM_CCs];
  long                           discRxPool_numRepetition[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_prb_Num[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_prb_Start[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_prb_End[MAX_NUM_CCs];
  SL_OffsetIndicator_r12_PR      discRxPool_ResourceConfig_offsetIndicator_present[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_offsetIndicator_choice[MAX_NUM_CCs];
  SubframeBitmapSL_r12_PR        discRxPool_ResourceConfig_subframeBitmap_present[MAX_NUM_CCs];
  char*                          discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[MAX_NUM_CCs];
  //for discRxPoolPS
  SL_CP_Len_r12_t                discRxPoolPS_cp_Len[MAX_NUM_CCs];
  e_SL_DiscResourcePool_r12__discPeriod_r12                   discRxPoolPS_discPeriod[MAX_NUM_CCs];
  long                           discRxPoolPS_numRetx[MAX_NUM_CCs];
  long                           discRxPoolPS_numRepetition[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_prb_Num[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_prb_Start[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_prb_End[MAX_NUM_CCs];
  SL_OffsetIndicator_r12_PR      discRxPoolPS_ResourceConfig_offsetIndicator_present[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_offsetIndicator_choice[MAX_NUM_CCs];
  SubframeBitmapSL_r12_PR        discRxPoolPS_ResourceConfig_subframeBitmap_present[MAX_NUM_CCs];
  char*                          discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[MAX_NUM_CCs];
} RrcConfigurationReq;

#define MAX_NUM_NBIOT_CELEVELS    3

typedef struct NbIoTRrcConfigurationReq_s {
  uint32_t                cell_identity;

  uint16_t                tac;

  uint16_t                mcc;
  uint16_t                mnc;
  uint8_t                 mnc_digit_length;
  lte_frame_type_t        frame_type;
  uint8_t                 tdd_config;
  uint8_t                 tdd_config_s;
  lte_prefix_type_t       prefix_type;
  lte_prefix_type_t       prefix_type_UL;
  int16_t                 eutra_band;
  uint32_t                downlink_frequency;
  int32_t                 uplink_frequency_offset;
  int16_t                 Nid_cell;// for testing, change later
  int16_t                 N_RB_DL;// for testing, change later
  //RACH
  long                    rach_raResponseWindowSize_NB;
  long                    rach_macContentionResolutionTimer_NB;
  long                    rach_powerRampingStep_NB;
  long                    rach_preambleInitialReceivedTargetPower_NB;
  long                    rach_preambleTransMax_CE_NB;
  //BCCH
  long                    bcch_modificationPeriodCoeff_NB;
  //PCCH
  long                    pcch_defaultPagingCycle_NB;
  long                    pcch_nB_NB;
  long                    pcch_npdcch_NumRepetitionPaging_NB;
  //NPRACH
  long                    nprach_CP_Length;
  long                    nprach_rsrp_range;
  long                    nprach_Periodicity[MAX_NUM_NBIOT_CELEVELS];
  long                    nprach_StartTime[MAX_NUM_NBIOT_CELEVELS];
  long                    nprach_SubcarrierOffset[MAX_NUM_NBIOT_CELEVELS];
  long                    nprach_NumSubcarriers[MAX_NUM_NBIOT_CELEVELS];
  long                    numRepetitionsPerPreambleAttempt_NB[MAX_NUM_NBIOT_CELEVELS];
  long                    nprach_SubcarrierMSG3_RangeStart;
  long                    maxNumPreambleAttemptCE_NB;
  long                    npdcch_NumRepetitions_RA[MAX_NUM_NBIOT_CELEVELS];
  long                    npdcch_StartSF_CSS_RA[MAX_NUM_NBIOT_CELEVELS];
  long                    npdcch_Offset_RA[MAX_NUM_NBIOT_CELEVELS];
  //NPDSCH
  long                    npdsch_nrs_Power;
  //NPUSCH
  long                    npusch_ack_nack_numRepetitions_NB;
  long                    npusch_srs_SubframeConfig_NB;
  long                    npusch_threeTone_CyclicShift_r13;
  long                    npusch_sixTone_CyclicShift_r13;
  BOOLEAN_t               npusch_groupHoppingEnabled;
  long                    npusch_groupAssignmentNPUSCH_r13;

  //DL_GapConfig
  long                    dl_GapThreshold_NB;
  long                    dl_GapPeriodicity_NB;
  long                    dl_GapDurationCoeff_NB;
  //Uplink power control Common
  long                    npusch_p0_NominalNPUSCH;
  long                    npusch_alpha;
  long                    deltaPreambleMsg3;
  //UE timers and constants
  long                    ue_TimersAndConstants_t300_NB;
  long                    ue_TimersAndConstants_t301_NB;
  long                    ue_TimersAndConstants_t310_NB;
  long                    ue_TimersAndConstants_t311_NB;
  long                    ue_TimersAndConstants_n310_NB;
  long                    ue_TimersAndConstants_n311_NB;
} NbIoTRrcConfigurationReq;

// gNB: GNB_APP -> RRC messages
typedef struct NRRrcConfigurationReq_s {
  uint32_t                cell_identity;
  uint16_t                tac;
  uint16_t                mcc;
  uint16_t                mnc;
  uint8_t                 mnc_digit_length;
  int16_t                 nb_cc;
  lte_frame_type_t        frame_type[MAX_NUM_CCs];
  uint8_t                 tdd_config[MAX_NUM_CCs];
  uint8_t                 tdd_config_s[MAX_NUM_CCs];
  lte_prefix_type_t       DL_prefix_type[MAX_NUM_CCs];
  lte_prefix_type_t       UL_prefix_type[MAX_NUM_CCs];
  int16_t                 eutra_band[MAX_NUM_CCs];
  uint32_t                downlink_frequency[MAX_NUM_CCs];
  int32_t                 uplink_frequency_offset[MAX_NUM_CCs];
  int16_t                 Nid_cell[MAX_NUM_CCs];// for testing, change later
  int16_t                 N_RB_DL[MAX_NUM_CCs];// for testing, change later
  int                     nb_antenna_ports[MAX_NUM_CCs];

  ///NR
  //MIB
  long                    MIB_subCarrierSpacingCommon[MAX_NUM_CCs]; 
  uint32_t                MIB_ssb_SubcarrierOffset[MAX_NUM_CCs]; 
  long                    MIB_dmrs_TypeA_Position[MAX_NUM_CCs];
  uint32_t                pdcch_ConfigSIB1[MAX_NUM_CCs];

  //SIB1
  long                    SIB1_frequencyOffsetSSB[MAX_NUM_CCs]; 
  long                    SIB1_ssb_PeriodicityServingCell[MAX_NUM_CCs];
  long                    SIB1_ss_PBCH_BlockPower[MAX_NUM_CCs];
  //NR FrequencyInfoDL
  long                    absoluteFrequencySSB[MAX_NUM_CCs];
  long                    DL_FreqBandIndicatorNR[MAX_NUM_CCs];
  long                    DL_absoluteFrequencyPointA[MAX_NUM_CCs];

  //NR DL SCS-SpecificCarrier
  uint32_t                DL_offsetToCarrier[MAX_NUM_CCs];
  long                    DL_SCS_SubcarrierSpacing[MAX_NUM_CCs];
  uint32_t                DL_carrierBandwidth[MAX_NUM_CCs];

  //NR BWP-DownlinkCommon
  uint32_t                DL_locationAndBandwidth[MAX_NUM_CCs]; 
  long                    DL_BWP_SubcarrierSpacing[MAX_NUM_CCs];
  lte_prefix_type_t       DL_BWP_prefix_type[MAX_NUM_CCs];   

  //NR FrequencyInfoUL
  long                    UL_FreqBandIndicatorNR[MAX_NUM_CCs];
  long                    UL_absoluteFrequencyPointA[MAX_NUM_CCs];
  long                    UL_additionalSpectrumEmission[MAX_NUM_CCs];
  long                    UL_p_Max[MAX_NUM_CCs];
  long                    UL_frequencyShift7p5khz[MAX_NUM_CCs];

  //NR UL SCS-SpecificCarrier
  uint32_t                UL_offsetToCarrier[MAX_NUM_CCs];
  long                    UL_SCS_SubcarrierSpacing[MAX_NUM_CCs];
  uint32_t                UL_carrierBandwidth[MAX_NUM_CCs];

  // NR BWP-UplinkCommon
  uint32_t                UL_locationAndBandwidth[MAX_NUM_CCs];
  long                    UL_BWP_SubcarrierSpacing[MAX_NUM_CCs];
  lte_prefix_type_t       UL_BWP_prefix_type[MAX_NUM_CCs];
  long                    UL_timeAlignmentTimerCommon[MAX_NUM_CCs];
  long                    ServingCellConfigCommon_n_TimingAdvanceOffset[MAX_NUM_CCs];
  long                    ServingCellConfigCommon_ssb_PositionsInBurst_PR[MAX_NUM_CCs];
  long                    ServingCellConfigCommon_ssb_periodicityServingCell[MAX_NUM_CCs]; //ServingCellConfigCommon
  long                    ServingCellConfigCommon_dmrs_TypeA_Position[MAX_NUM_CCs];        //ServingCellConfigCommon
  long                    NIA_SubcarrierSpacing[MAX_NUM_CCs];      //ServingCellConfigCommon Used only for non-initial access
  long                    ServingCellConfigCommon_ss_PBCH_BlockPower[MAX_NUM_CCs];         //ServingCellConfigCommon


  //NR TDD-UL-DL-ConfigCommon
  long                    referenceSubcarrierSpacing[MAX_NUM_CCs];
  long                    dl_UL_TransmissionPeriodicity[MAX_NUM_CCs];
  long                    nrofDownlinkSlots[MAX_NUM_CCs];
  long                    nrofDownlinkSymbols[MAX_NUM_CCs];
  long                    nrofUplinkSlots[MAX_NUM_CCs];
  long                    nrofUplinkSymbols[MAX_NUM_CCs];

  //NR RACH-ConfigCommon
  long                    rach_totalNumberOfRA_Preambles[MAX_NUM_CCs];
  long                    rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_choice[MAX_NUM_CCs];
  long                    rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneEighth[MAX_NUM_CCs];
  long                    rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneFourth[MAX_NUM_CCs];
  long                    rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneHalf[MAX_NUM_CCs];
  long                    rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_one[MAX_NUM_CCs];
  long                    rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_two[MAX_NUM_CCs];
  uint32_t                rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_four[MAX_NUM_CCs];
  uint32_t                rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_eight[MAX_NUM_CCs];
  uint32_t                rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_sixteen[MAX_NUM_CCs];
  BOOLEAN_t               rach_groupBconfigured[MAX_NUM_CCs];
  long                    rach_ra_Msg3SizeGroupA[MAX_NUM_CCs];
  long                    rach_messagePowerOffsetGroupB[MAX_NUM_CCs];
  long                    rach_numberOfRA_PreamblesGroupA[MAX_NUM_CCs];
  long                    rach_ra_ContentionResolutionTimer[MAX_NUM_CCs];
  long                    rsrp_ThresholdSSB[MAX_NUM_CCs];
  long                    rsrp_ThresholdSSB_SUL[MAX_NUM_CCs];
  long                    prach_RootSequenceIndex_choice[MAX_NUM_CCs];
  uint32_t                prach_RootSequenceIndex_l839[MAX_NUM_CCs];
  uint32_t                prach_RootSequenceIndex_l139[MAX_NUM_CCs];
  long                    prach_msg1_SubcarrierSpacing[MAX_NUM_CCs];
  long                    restrictedSetConfig[MAX_NUM_CCs];
  long                    msg3_transformPrecoding[MAX_NUM_CCs];
  //ssb-perRACH-OccasionAndCB-PreamblesPerSSB not sure

  //NR RACH-ConfigGeneric
  uint32_t                prach_ConfigurationIndex[MAX_NUM_CCs];
  long                    prach_msg1_FDM[MAX_NUM_CCs];
  long                    prach_msg1_FrequencyStart[MAX_NUM_CCs];
  uint32_t                zeroCorrelationZoneConfig[MAX_NUM_CCs];
  long                    preambleReceivedTargetPower[MAX_NUM_CCs];
  long                    preambleTransMax[MAX_NUM_CCs];
  long                    powerRampingStep[MAX_NUM_CCs];
  long                    ra_ResponseWindow[MAX_NUM_CCs];

  //NR PUSCH-ConfigCommon
  BOOLEAN_t               groupHoppingEnabledTransformPrecoding[MAX_NUM_CCs];
  long                    msg3_DeltaPreamble[MAX_NUM_CCs];
  long                    p0_NominalWithGrant[MAX_NUM_CCs];

  ///NR PUSCH-TimeDomainResourceAllocation
  uint32_t                PUSCH_TimeDomainResourceAllocation_k2[MAX_NUM_CCs];
  long                    PUSCH_TimeDomainResourceAllocation_mappingType[MAX_NUM_CCs];
  uint32_t                PUSCH_TimeDomainResourceAllocation_startSymbolAndLength[MAX_NUM_CCs];

  //NR PUCCH-ConfigCommon
  uint32_t                pucch_ResourceCommon[MAX_NUM_CCs];
  long                    pucch_GroupHopping[MAX_NUM_CCs];
  uint32_t                hoppingId[MAX_NUM_CCs];
  long                    p0_nominal[MAX_NUM_CCs];

  //NR PDSCH-ConfigCOmmon
  //NR PDSCH-TimeDomainResourceAllocation
  uint32_t                PDSCH_TimeDomainResourceAllocation_k0[MAX_NUM_CCs];
  long                    PDSCH_TimeDomainResourceAllocation_mappingType[MAX_NUM_CCs];
  long                    PDSCH_TimeDomainResourceAllocation_startSymbolAndLength[MAX_NUM_CCs];

  //NR RateMatchPattern  is used to configure one rate matching pattern for PDSCH
  long                    rateMatchPatternId[MAX_NUM_CCs];
  long                    RateMatchPattern_patternType[MAX_NUM_CCs];
  long                    symbolsInResourceBlock[MAX_NUM_CCs];
  long                    periodicityAndPattern[MAX_NUM_CCs];
  long                    RateMatchPattern_controlResourceSet[MAX_NUM_CCs]; ///ControlResourceSetId
  long                    RateMatchPattern_subcarrierSpacing[MAX_NUM_CCs];
  long                    RateMatchPattern_mode[MAX_NUM_CCs];

  //NR PDCCH-ConfigCommon
  uint32_t                controlResourceSetZero[MAX_NUM_CCs];
  uint32_t                searchSpaceZero[MAX_NUM_CCs];
  long                    searchSpaceSIB1[MAX_NUM_CCs];
  long                    searchSpaceOtherSystemInformation[MAX_NUM_CCs];
  long                    pagingSearchSpace[MAX_NUM_CCs];
  long                    ra_SearchSpace[MAX_NUM_CCs];
  //NR PDCCH-ConfigCommon commonControlResourcesSets
  long                    PDCCH_common_controlResourceSetId[MAX_NUM_CCs];
  long                    PDCCH_common_ControlResourceSet_duration[MAX_NUM_CCs];
  long                    PDCCH_cce_REG_MappingType[MAX_NUM_CCs];
  long                    PDCCH_reg_BundleSize[MAX_NUM_CCs];
  long                    PDCCH_interleaverSize[MAX_NUM_CCs];
  long                    PDCCH_shiftIndex[MAX_NUM_CCs];  
  long                    PDCCH_precoderGranularity[MAX_NUM_CCs]; //Corresponds to L1 parameter 'CORESET-precoder-granuality'
  long                    PDCCH_TCI_StateId[MAX_NUM_CCs];
  BOOLEAN_t               tci_PresentInDCI[MAX_NUM_CCs];
  uint32_t                PDCCH_DMRS_ScramblingID[MAX_NUM_CCs];

  //NR PDCCH-ConfigCommon commonSearchSpaces
  long                    SearchSpaceId[MAX_NUM_CCs];
  long                    commonSearchSpaces_controlResourceSetId[MAX_NUM_CCs];
  long                    SearchSpace_monitoringSlotPeriodicityAndOffset_choice[MAX_NUM_CCs];
  uint32_t                SearchSpace_monitoringSlotPeriodicityAndOffset_value[MAX_NUM_CCs];
  uint32_t                SearchSpace_duration[MAX_NUM_CCs];
  long                    SearchSpace_nrofCandidates_aggregationLevel1[MAX_NUM_CCs];
  long                    SearchSpace_nrofCandidates_aggregationLevel2[MAX_NUM_CCs];
  long                    SearchSpace_nrofCandidates_aggregationLevel4[MAX_NUM_CCs];
  long                    SearchSpace_nrofCandidates_aggregationLevel8[MAX_NUM_CCs];
  long                    SearchSpace_nrofCandidates_aggregationLevel16[MAX_NUM_CCs];
  long                    SearchSpace_searchSpaceType[MAX_NUM_CCs];
  long                    Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel1[MAX_NUM_CCs];
  long                    Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel2[MAX_NUM_CCs];
  long                    Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel4[MAX_NUM_CCs];
  long                    Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel8[MAX_NUM_CCs];
  long                    Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel16[MAX_NUM_CCs];  
  long                    Common_dci_Format2_3_monitoringPeriodicity[MAX_NUM_CCs];
  long                    Common_dci_Format2_3_nrofPDCCH_Candidates[MAX_NUM_CCs];
  long                    ue_Specific__dci_Formats[MAX_NUM_CCs];

  //RateMatchPatternLTE-CRS
  uint32_t                RateMatchPatternLTE_CRS_carrierFreqDL[MAX_NUM_CCs];
  long                    RateMatchPatternLTE_CRS_carrierBandwidthDL[MAX_NUM_CCs];
  long                    RateMatchPatternLTE_CRS_nrofCRS_Ports[MAX_NUM_CCs];
  long                    RateMatchPatternLTE_CRS_v_Shift[MAX_NUM_CCs];
  long                    RateMatchPatternLTE_CRS_radioframeAllocationPeriod[MAX_NUM_CCs];
  uint32_t                RateMatchPatternLTE_CRS_radioframeAllocationOffset[MAX_NUM_CCs];
  long                    RateMatchPatternLTE_CRS_subframeAllocation_choice[MAX_NUM_CCs];

} gNB_RrcConfigurationReq;


// UE: NAS -> RRC messages
typedef kenb_refresh_req_t      NasKenbRefreshReq;
typedef cell_info_req_t         NasCellSelectionReq;
typedef nas_establish_req_t     NasConnEstabliReq;
typedef ul_info_transfer_req_t  NasUlDataReq;

typedef rab_establish_rsp_t     NasRabEstRsp;

// UE: RRC -> NAS messages
typedef cell_info_cnf_t         NasCellSelectionCnf;
typedef cell_info_ind_t         NasCellSelectionInd;
typedef paging_ind_t            NasPagingInd;
typedef nas_establish_cnf_t     NasConnEstabCnf;
typedef nas_release_ind_t       NasConnReleaseInd;
typedef ul_info_transfer_cnf_t  NasUlDataCnf;
typedef dl_info_transfer_ind_t  NasDlDataInd;

#endif /* RRC_MESSAGES_TYPES_H_ */
