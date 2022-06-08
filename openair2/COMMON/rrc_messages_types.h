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
#include "f1ap_messages_types.h"
#include "LTE_SystemInformationBlockType2.h"
#include "LTE_SL-OffsetIndicator-r12.h"
#include "LTE_SubframeBitmapSL-r12.h"
#include "LTE_DRX-Config.h"
#include "LTE_SL-CP-Len-r12.h"
#include "LTE_SL-PeriodComm-r12.h"
#include "LTE_SL-DiscResourcePool-r12.h"
#include "NR_RACH-ConfigCommon.h"
#include "NR_ServingCellConfigCommon.h"
#include "NR_ServingCellConfig.h"
//-------------------------------------------------------------------------------------------//
// Messages for RRC logging
#if defined(DISABLE_ITTI_XER_PRINT)
  #include "LTE_BCCH-DL-SCH-Message.h"
  #include "LTE_DL-CCCH-Message.h"
  #include "LTE_DL-DCCH-Message.h"
  #include "LTE_UE-EUTRA-Capability.h"
  #include "LTE_UL-CCCH-Message.h"
  #include "LTE_UL-DCCH-Message.h"

  typedef LTE_BCCH_DL_SCH_Message_t   RrcDlBcchMessage;
  typedef LTE_DL_CCCH_Message_t       RrcDlCcchMessage;
  typedef LTE_DL_DCCH_Message_t       RrcDlDcchMessage;
  typedef LTE_UE_EUTRA_Capability_t   RrcUeEutraCapability;
  typedef LTE_UL_CCCH_Message_t       RrcUlCcchMessage;
  typedef LTE_UL_DCCH_Message_t       RrcUlDcchMessage;
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

#define RRC_SUBFRAME_PROCESS(mSGpTR)    (mSGpTR)->ittiMsg.rrc_subframe_process

#define RLC_SDU_INDICATION(mSGpTR)      (mSGpTR)->ittiMsg.rlc_sdu_indication
#define NRDuDlReq(mSGpTR)      (mSGpTR)->ittiMsg.nr_du_dl_req

#define NAS_OAI_TUN_NSA(mSGpTR)         (mSGpTR)->ittiMsg.nas_oai_tun_nsa

//-------------------------------------------------------------------------------------------//
typedef struct RrcStateInd_s {
  Rrc_State_t     state;
  Rrc_Sub_State_t sub_state;
} RrcStateInd;

typedef struct RadioResourceConfig_s {
  long                    prach_root;
  long                    prach_config_index;
  BOOLEAN_t               prach_high_speed;
  long                    prach_zero_correlation;
  long                    prach_freq_offset;
  long                    pucch_delta_shift;
  long                    pucch_nRB_CQI;
  long                    pucch_nCS_AN;
  long                    pucch_n1_AN;
  long                    pdsch_referenceSignalPower;
  long                    pdsch_p_b;
  long                    pusch_n_SB;
  long                    pusch_hoppingMode;
  long                    pusch_hoppingOffset;
  BOOLEAN_t               pusch_enable64QAM;
  BOOLEAN_t               pusch_groupHoppingEnabled;
  long                    pusch_groupAssignment;
  BOOLEAN_t               pusch_sequenceHoppingEnabled;
  long                    pusch_nDMRS1;
  long                    phich_duration;
  long                    phich_resource;
  BOOLEAN_t               srs_enable;
  long                    srs_BandwidthConfig;
  long                    srs_SubframeConfig;
  BOOLEAN_t               srs_ackNackST;
  BOOLEAN_t               srs_MaxUpPts;
  long                    pusch_p0_Nominal;
  long                    pusch_alpha;
  long                    pucch_p0_Nominal;
  long                    msg3_delta_Preamble;
  long                    ul_CyclicPrefixLength;
  e_LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1                    pucch_deltaF_Format1;
  e_LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b                   pucch_deltaF_Format1b;
  e_LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2                    pucch_deltaF_Format2;
  e_LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a                   pucch_deltaF_Format2a;
  e_LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b                   pucch_deltaF_Format2b;
  long                    rach_numberOfRA_Preambles;
  BOOLEAN_t               rach_preamblesGroupAConfig;
  long                    rach_sizeOfRA_PreamblesGroupA;
  long                    rach_messageSizeGroupA;
  e_LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB                    rach_messagePowerOffsetGroupB;
  long                    rach_powerRampingStep;
  long                    rach_preambleInitialReceivedTargetPower;
  long                    rach_preambleTransMax;
  long                    rach_raResponseWindowSize;
  long                    rach_macContentionResolutionTimer;
  long                    rach_maxHARQ_Msg3Tx;
  long                    bcch_modificationPeriodCoeff;
  long                    pcch_defaultPagingCycle;
  long                    pcch_nB;
  LTE_DRX_Config_PR                                   drx_Config_present;
  long                                                drx_onDurationTimer;
  long                                                drx_InactivityTimer;
  long                                                drx_RetransmissionTimer;
  LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR  drx_longDrx_CycleStartOffset_present;
  long                                                drx_longDrx_CycleStartOffset;
  long                                                drx_shortDrx_Cycle;
  long                                                drx_shortDrx_ShortCycleTimer;
  long                    ue_TimersAndConstants_t300;
  long                    ue_TimersAndConstants_t301;
  long                    ue_TimersAndConstants_t310;
  long                    ue_TimersAndConstants_t311;
  long                    ue_TimersAndConstants_n310;
  long                    ue_TimersAndConstants_n311;
  long                    ue_TransmissionMode;
  long                    ue_multiple_max;
  //SIB2 BR Options
  long       preambleTransMax_CE_r13;
  BOOLEAN_t     prach_ConfigCommon_v1310;
  BOOLEAN_t            *mpdcch_startSF_CSS_RA_r13;
  long        mpdcch_startSF_CSS_RA_r13_val;
  long       *prach_HoppingOffset_r13;
  BOOLEAN_t     mbms_dedicated_serving_cell;
} RadioResourceConfig;

// eNB: ENB_APP -> RRC messages
typedef struct RrcConfigurationReq_s {
  uint32_t                cell_identity;
  uint16_t                tac;
  uint16_t                mcc[PLMN_LIST_MAX_SIZE];
  uint16_t                mnc[PLMN_LIST_MAX_SIZE];
  uint8_t                 mnc_digit_length[PLMN_LIST_MAX_SIZE];
  uint8_t                 num_plmn;
  int                     enable_measurement_reports;
  int                     enable_x2;
  uint32_t                rrc_inactivity_timer_thres; // for testing, maybe change later
  paging_drx_t            default_drx;
  int16_t                 nb_cc;
  frame_type_t            frame_type[MAX_NUM_CCs];
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
  int                     eMBMS_configured;
  int                     eMBMS_M2_configured;
  int                     eMTC_configured;
  int                     SL_configured;

  RadioResourceConfig     radioresourceconfig[MAX_NUM_CCs];
  RadioResourceConfig     radioresourceconfig_BR[MAX_NUM_CCs];


  //MIB
  long        schedulingInfoSIB1_BR_r13[MAX_NUM_CCs];
  //SIB1 BR options
  uint16_t     *hyperSFN_r13                           [MAX_NUM_CCs];
  long       *eDRX_Allowed_r13                       [MAX_NUM_CCs];
  BOOLEAN_t     cellSelectionInfoCE_r13                [MAX_NUM_CCs];
  long        q_RxLevMinCE_r13                       [MAX_NUM_CCs];
  long       *q_QualMinRSRQ_CE_r13                   [MAX_NUM_CCs];
  BOOLEAN_t     bandwidthReducedAccessRelatedInfo_r13  [MAX_NUM_CCs];
  long            si_Narrowband_r13         [MAX_NUM_CCs][32];
  long            si_TBS_r13                [MAX_NUM_CCs][32];
  int             scheduling_info_br_size   [MAX_NUM_CCs];
  long        si_WindowLength_BR_r13                       [MAX_NUM_CCs];
  long        si_RepetitionPattern_r13                     [MAX_NUM_CCs];
  BOOLEAN_t     *fdd_DownlinkOrTddSubframeBitmapBR_r13       [MAX_NUM_CCs];
  uint64_t      fdd_DownlinkOrTddSubframeBitmapBR_val_r13    [MAX_NUM_CCs];
  uint16_t      *fdd_UplinkSubframeBitmapBR_r13              [MAX_NUM_CCs];
  long        startSymbolBR_r13                            [MAX_NUM_CCs];
  long        si_HoppingConfigCommon_r13                   [MAX_NUM_CCs];
  long       *si_ValidityTime_r13                          [MAX_NUM_CCs];
  long            systemInfoValueTagSi_r13      [MAX_NUM_CCs][10];
  int             system_info_value_tag_SI_size [MAX_NUM_CCs];
  BOOLEAN_t     freqHoppingParametersDL_r13                   [MAX_NUM_CCs];
  long       *mpdcch_pdsch_HoppingNB_r13                    [MAX_NUM_CCs];
  BOOLEAN_t     interval_DLHoppingConfigCommonModeA_r13       [MAX_NUM_CCs];
  long        interval_DLHoppingConfigCommonModeA_r13_val   [MAX_NUM_CCs];
  BOOLEAN_t     interval_DLHoppingConfigCommonModeB_r13       [MAX_NUM_CCs];
  long        interval_DLHoppingConfigCommonModeB_r13_val   [MAX_NUM_CCs];
  long       *mpdcch_pdsch_HoppingOffset_r13                [MAX_NUM_CCs];
  long firstPreamble_r13                 [MAX_NUM_CCs][4];
  long lastPreamble_r13                  [MAX_NUM_CCs][4];
  long ra_ResponseWindowSize_r13         [MAX_NUM_CCs][4];
  long mac_ContentionResolutionTimer_r13 [MAX_NUM_CCs][4];
  long rar_HoppingConfig_r13             [MAX_NUM_CCs][4];
  int  rach_CE_LevelInfoList_r13_size    [MAX_NUM_CCs];
  //  long pcch_defaultPagingCycle_br;
  long rsrp_range           [MAX_NUM_CCs][3];
  int rsrp_range_list_size  [MAX_NUM_CCs];
  long prach_config_index                        [MAX_NUM_CCs][4];
  long prach_freq_offset                         [MAX_NUM_CCs][4];
  long *prach_StartingSubframe_r13               [MAX_NUM_CCs][4];
  long *maxNumPreambleAttemptCE_r13              [MAX_NUM_CCs][4];
  long numRepetitionPerPreambleAttempt_r13       [MAX_NUM_CCs][4];
  long mpdcch_NumRepetition_RA_r13               [MAX_NUM_CCs][4];
  long prach_HoppingConfig_r13                   [MAX_NUM_CCs][4];
  int  prach_parameters_list_size                [MAX_NUM_CCs];
  long max_available_narrow_band                 [MAX_NUM_CCs][4][2];
  int  max_available_narrow_band_size            [MAX_NUM_CCs][4];
  long pucch_info_value       [MAX_NUM_CCs][4];
  int  pucch_info_value_size  [MAX_NUM_CCs];
  bool  pcch_config_v1310               [MAX_NUM_CCs];
  long  paging_narrowbands_r13          [MAX_NUM_CCs];
  long  mpdcch_numrepetition_paging_r13 [MAX_NUM_CCs];
  long  *nb_v1310                        [MAX_NUM_CCs];
  long  *pucch_NumRepetitionCE_Msg4_Level0_r13  [MAX_NUM_CCs];
  long  *pucch_NumRepetitionCE_Msg4_Level1_r13  [MAX_NUM_CCs];
  long  *pucch_NumRepetitionCE_Msg4_Level2_r13  [MAX_NUM_CCs];
  long  *pucch_NumRepetitionCE_Msg4_Level3_r13  [MAX_NUM_CCs];
  bool  sib2_freq_hoppingParameters_r13_exists             [MAX_NUM_CCs];
  long  *sib2_mpdcch_pdsch_hoppingNB_r13                   [MAX_NUM_CCs];
  long  *sib2_interval_DLHoppingConfigCommonModeA_r13      [MAX_NUM_CCs];
  long  sib2_interval_DLHoppingConfigCommonModeA_r13_val  [MAX_NUM_CCs];
  long  *sib2_interval_DLHoppingConfigCommonModeB_r13      [MAX_NUM_CCs];
  long  sib2_interval_DLHoppingConfigCommonModeB_r13_val  [MAX_NUM_CCs];
  long  *sib2_interval_ULHoppingConfigCommonModeA_r13      [MAX_NUM_CCs];
  long  sib2_interval_ULHoppingConfigCommonModeA_r13_val  [MAX_NUM_CCs];
  long  *sib2_interval_ULHoppingConfigCommonModeB_r13      [MAX_NUM_CCs];
  long  sib2_interval_ULHoppingConfigCommonModeB_r13_val  [MAX_NUM_CCs];
  long  *sib2_mpdcch_pdsch_hoppingOffset_r13               [MAX_NUM_CCs];
  long  *pdsch_maxNumRepetitionCEmodeA_r13                 [MAX_NUM_CCs];
  long  *pdsch_maxNumRepetitionCEmodeB_r13                 [MAX_NUM_CCs];
  long  *pusch_maxNumRepetitionCEmodeA_r13                 [MAX_NUM_CCs];
  long  *pusch_maxNumRepetitionCEmodeB_r13                 [MAX_NUM_CCs];
  long  *pusch_repetitionLevelCEmodeA_r13				   [MAX_NUM_CCs];
  long  *pusch_HoppingOffset_v1310                         [MAX_NUM_CCs];

  //SIB18
  e_LTE_SL_CP_Len_r12            rxPool_sc_CP_Len[MAX_NUM_CCs];
  e_LTE_SL_PeriodComm_r12        rxPool_sc_Period[MAX_NUM_CCs];
  e_LTE_SL_CP_Len_r12            rxPool_data_CP_Len[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_prb_Num[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_prb_Start[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_prb_End[MAX_NUM_CCs];
  LTE_SL_OffsetIndicator_r12_PR  rxPool_ResourceConfig_offsetIndicator_present[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_offsetIndicator_choice[MAX_NUM_CCs];
  LTE_SubframeBitmapSL_r12_PR    rxPool_ResourceConfig_subframeBitmap_present[MAX_NUM_CCs];
  char                          *rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_subframeBitmap_choice_bs_size[MAX_NUM_CCs];
  long                           rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[MAX_NUM_CCs];

  //SIB19
  //for discRxPool
  LTE_SL_CP_Len_r12_t            discRxPool_cp_Len[MAX_NUM_CCs];
  e_LTE_SL_DiscResourcePool_r12__discPeriod_r12               discRxPool_discPeriod[MAX_NUM_CCs];
  long                           discRxPool_numRetx[MAX_NUM_CCs];
  long                           discRxPool_numRepetition[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_prb_Num[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_prb_Start[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_prb_End[MAX_NUM_CCs];
  LTE_SL_OffsetIndicator_r12_PR  discRxPool_ResourceConfig_offsetIndicator_present[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_offsetIndicator_choice[MAX_NUM_CCs];
  LTE_SubframeBitmapSL_r12_PR    discRxPool_ResourceConfig_subframeBitmap_present[MAX_NUM_CCs];
  char                          *discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[MAX_NUM_CCs];
  long                           discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[MAX_NUM_CCs];
  //for discRxPoolPS
  LTE_SL_CP_Len_r12_t            discRxPoolPS_cp_Len[MAX_NUM_CCs];
  e_LTE_SL_DiscResourcePool_r12__discPeriod_r12                   discRxPoolPS_discPeriod[MAX_NUM_CCs];
  long                           discRxPoolPS_numRetx[MAX_NUM_CCs];
  long                           discRxPoolPS_numRepetition[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_prb_Num[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_prb_Start[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_prb_End[MAX_NUM_CCs];
  LTE_SL_OffsetIndicator_r12_PR  discRxPoolPS_ResourceConfig_offsetIndicator_present[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_offsetIndicator_choice[MAX_NUM_CCs];
  LTE_SubframeBitmapSL_r12_PR    discRxPoolPS_ResourceConfig_subframeBitmap_present[MAX_NUM_CCs];
  char                          *discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[MAX_NUM_CCs];
  long                           discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[MAX_NUM_CCs];
  //Nr secondary cell group SSB central frequency (for ENDC NSA)
  int                            nr_scg_ssb_freq;
} RrcConfigurationReq;

#define MAX_NUM_NBIOT_CELEVELS    3

typedef struct NbIoTRrcConfigurationReq_s {
  uint32_t                cell_identity;
  uint16_t                tac;
  uint16_t                mcc;
  uint16_t                mnc;
  uint8_t                 mnc_digit_length;
  frame_type_t            frame_type;
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

typedef struct {
  int N1;
  int N2;
  int XP;
} rrc_pdsch_AntennaPorts_t;

// gNB: GNB_APP -> RRC messages
typedef struct NRRrcConfigurationReq_s {
  uint64_t                cell_identity;
  uint32_t                tac;
  uint16_t                mcc[PLMN_LIST_MAX_SIZE];
  uint16_t                mnc[PLMN_LIST_MAX_SIZE];
  uint8_t                 mnc_digit_length[PLMN_LIST_MAX_SIZE];
  uint8_t                 num_plmn;
  NR_ServingCellConfigCommon_t *scc;
  NR_ServingCellConfig_t  *scd;
  int                     ssb_SubcarrierOffset;
  int                     sib1_tda;
  rrc_pdsch_AntennaPorts_t pdsch_AntennaPorts;
  int                     pusch_AntennaPorts;
  int                     minRXTXTIME;
  int                     do_CSIRS;
  int                     do_SRS;
  bool                    force_256qam_off;
  int                     pusch_TargetSNRx10;
  int                     pucch_TargetSNRx10;
  bool                    enable_sdap;
} gNB_RrcConfigurationReq;

typedef struct NRDuDlReq_s {
  rnti_t rnti;
  mem_block_t * buf;
  uint64_t srb_id;
}  NRDuDlReq_t; 

// UE: NAS -> RRC messages
typedef kenb_refresh_req_t      NasKenbRefreshReq;
typedef cell_info_req_t         NasCellSelectionReq;
typedef nas_establish_req_t     NasConnEstabliReq;
typedef ul_info_transfer_req_t  NasUlDataReq;

typedef rab_establish_rsp_t     NasRabEstRsp;

typedef nas_oai_tun_nsa_t       NasOaiTunNsa;

// UE: RRC -> NAS messages
typedef cell_info_cnf_t         NasCellSelectionCnf;
typedef cell_info_ind_t         NasCellSelectionInd;
typedef paging_ind_t            NasPagingInd;
typedef nas_establish_cnf_t     NasConnEstabCnf;
typedef nas_release_ind_t       NasConnReleaseInd;
typedef ul_info_transfer_cnf_t  NasUlDataCnf;
typedef dl_info_transfer_ind_t  NasDlDataInd;

// eNB: realtime -> RRC messages
typedef struct rrc_subframe_process_s {
  protocol_ctxt_t ctxt;
  int             CC_id;
} RrcSubframeProcess;

// eNB: RLC -> RRC messages
typedef struct rlc_sdu_indication_s {
  int rnti;
  int is_successful;
  int srb_id;
  int message_id;
} RlcSduIndication;

#endif /* RRC_MESSAGES_TYPES_H_ */
