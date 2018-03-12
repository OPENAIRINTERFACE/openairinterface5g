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
#include "RRC/LITE/MESSAGES/SystemInformationBlockType2.h"
#endif

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
  uint32_t            cell_identity;

  uint16_t            tac;

  uint16_t            mcc;
  uint16_t            mnc;
  uint8_t             mnc_digit_length;

  
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
#if !defined(Rel10) && !defined(Rel14)
  long                    pucch_n1_AN[MAX_NUM_CCs];
#endif
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
} RrcConfigurationReq;
#define MAX_NUM_NBIOT_CELEVELS    3
typedef struct NbIoTRrcConfigurationReq_s {
  uint32_t            cell_identity;

  uint16_t            tac;

  uint16_t	      mcc;
  uint16_t	      mnc;
  uint8_t	      mnc_digit_length;
  lte_frame_type_t	  frame_type;
  uint8_t                 tdd_config;
  uint8_t                 tdd_config_s;
  lte_prefix_type_t       prefix_type;
  lte_prefix_type_t	  prefix_type_UL;
  int16_t                 eutra_band;
  uint32_t                downlink_frequency;
  int32_t                 uplink_frequency_offset;
  int16_t                 Nid_cell;// for testing, change later
  int16_t                 N_RB_DL;// for testing, change later
  //RACH
  long					  rach_raResponseWindowSize_NB;
  long					  rach_macContentionResolutionTimer_NB;
  long					  rach_powerRampingStep_NB;
  long					  rach_preambleInitialReceivedTargetPower_NB;
  long					  rach_preambleTransMax_CE_NB;
  //BCCH
  long					  bcch_modificationPeriodCoeff_NB;
  //PCCH
  long					  pcch_defaultPagingCycle_NB;
  long					  pcch_nB_NB;
  long					  pcch_npdcch_NumRepetitionPaging_NB;
  //NPRACH
  long					  nprach_CP_Length;
  long					  nprach_rsrp_range;
  long					  nprach_Periodicity[MAX_NUM_NBIOT_CELEVELS];
  long					  nprach_StartTime[MAX_NUM_NBIOT_CELEVELS];
  long					  nprach_SubcarrierOffset[MAX_NUM_NBIOT_CELEVELS];
  long					  nprach_NumSubcarriers[MAX_NUM_NBIOT_CELEVELS];
  long					  numRepetitionsPerPreambleAttempt_NB[MAX_NUM_NBIOT_CELEVELS];
  long					  nprach_SubcarrierMSG3_RangeStart;
  long					  maxNumPreambleAttemptCE_NB;
  long					  npdcch_NumRepetitions_RA[MAX_NUM_NBIOT_CELEVELS];
  long					  npdcch_StartSF_CSS_RA[MAX_NUM_NBIOT_CELEVELS];
  long					  npdcch_Offset_RA[MAX_NUM_NBIOT_CELEVELS];
  //NPDSCH
  long					  npdsch_nrs_Power;
  //NPUSCH
  long					  npusch_ack_nack_numRepetitions_NB;
  long					  npusch_srs_SubframeConfig_NB;
  long					  npusch_threeTone_CyclicShift_r13;
  long					  npusch_sixTone_CyclicShift_r13;
  BOOLEAN_t				  npusch_groupHoppingEnabled;
  long					  npusch_groupAssignmentNPUSCH_r13;

  //DL_GapConfig
  long					  dl_GapThreshold_NB;
  long	 				  dl_GapPeriodicity_NB;
  long	 				  dl_GapDurationCoeff_NB;
  //Uplink power control Common
  long					  npusch_p0_NominalNPUSCH;
  long					  npusch_alpha;
  long					  deltaPreambleMsg3;
  //UE timers and constants
  long					  ue_TimersAndConstants_t300_NB;
  long					  ue_TimersAndConstants_t301_NB;
  long					  ue_TimersAndConstants_t310_NB;
  long					  ue_TimersAndConstants_t311_NB;
  long					  ue_TimersAndConstants_n310_NB;
  long					  ue_TimersAndConstants_n311_NB;
} NbIoTRrcConfigurationReq;


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
