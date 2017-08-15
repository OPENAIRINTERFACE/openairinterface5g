/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

typedef struct RadioResourceConfig_s {
  long                    prach_root;
  long                    prach_config_index;
  BOOLEAN_t               prach_high_speed;
  long                    prach_zero_correlation;
  long                    prach_freq_offset;
  long                    pucch_delta_shift;
  long                    pucch_nRB_CQI;
  long                    pucch_nCS_AN;
#if !defined(Rel10) && !defined(Rel14)
  long                    pucch_n1_AN;
#endif
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
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format1                    pucch_deltaF_Format1;
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format1b                   pucch_deltaF_Format1b;
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format2                    pucch_deltaF_Format2;
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format2a                   pucch_deltaF_Format2a;
  e_DeltaFList_PUCCH__deltaF_PUCCH_Format2b                   pucch_deltaF_Format2b;
  long                    rach_numberOfRA_Preambles;
  BOOLEAN_t               rach_preamblesGroupAConfig;
  long                    rach_sizeOfRA_PreamblesGroupA;
  long                    rach_messageSizeGroupA;
  e_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB                    rach_messagePowerOffsetGroupB;
  long                    rach_powerRampingStep;
  long                    rach_preambleInitialReceivedTargetPower;
  long                    rach_preambleTransMax;
  long                    rach_raResponseWindowSize;
  long                    rach_macContentionResolutionTimer;
  long                    rach_maxHARQ_Msg3Tx;
  long                    bcch_modificationPeriodCoeff;
  long                    pcch_defaultPagingCycle;
  long                    pcch_nB;
  long                    ue_TimersAndConstants_t300;
  long                    ue_TimersAndConstants_t301;
  long                    ue_TimersAndConstants_t310;
  long                    ue_TimersAndConstants_t311;
  long                    ue_TimersAndConstants_n310;
  long                    ue_TimersAndConstants_n311;
  long                    ue_TransmissionMode;
#ifdef Rel14
  //SIB2 BR Options
  long*			  preambleTransMax_CE_r13;
  BOOLEAN_t		  prach_ConfigCommon_v1310;
  BOOLEAN_t*	          mpdcch_startSF_CSS_RA_r13;
  long			  mpdcch_startSF_CSS_RA_r13_val;
  long*			  prach_HoppingOffset_r13;
#endif
} RadioResourceConfig;

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


  RadioResourceConfig     radioresourceconfig[MAX_NUM_CCs];
  RadioResourceConfig     radioresourceconfig_BR[MAX_NUM_CCs];
   
#ifdef Rel14


  //MIB
  long	 		  schedulingInfoSIB1_BR_r13[MAX_NUM_CCs];
  //SIB1 BR options
  

  uint16_t*		  hyperSFN_r13                           [MAX_NUM_CCs];
  long*			  eDRX_Allowed_r13                       [MAX_NUM_CCs];
  BOOLEAN_t		  cellSelectionInfoCE_r13                [MAX_NUM_CCs];
  long			  q_RxLevMinCE_r13                       [MAX_NUM_CCs];
  long*			  q_QualMinRSRQ_CE_r13                   [MAX_NUM_CCs];
  BOOLEAN_t		  bandwidthReducedAccessRelatedInfo_r13  [MAX_NUM_CCs];

//  +kogo -- FIXME -- size 10 is temporary
  long            si_Narrowband_r13         [MAX_NUM_CCs][32];
  long            si_TBS_r13                [MAX_NUM_CCs][32];
  int             scheduling_info_br_size   [MAX_NUM_CCs];
//  end +kogo

  long			  si_WindowLength_BR_r13                       [MAX_NUM_CCs];
  long			  si_RepetitionPattern_r13                     [MAX_NUM_CCs];
  BOOLEAN_t		 * fdd_DownlinkOrTddSubframeBitmapBR_r13     [MAX_NUM_CCs];
  uint64_t		  fdd_DownlinkOrTddSubframeBitmapBR_val_r13  [MAX_NUM_CCs];
  uint16_t		  *fdd_UplinkSubframeBitmapBR_r13            [MAX_NUM_CCs];
  long			  startSymbolBR_r13                            [MAX_NUM_CCs];
  long			  si_HoppingConfigCommon_r13                   [MAX_NUM_CCs];
  long*			  si_ValidityTime_r13                          [MAX_NUM_CCs];

//  +kogo
  long            systemInfoValueTagSi_r13      [MAX_NUM_CCs][10];
  int             system_info_value_tag_SI_size [MAX_NUM_CCs];
//  end +kogo

  BOOLEAN_t		  freqHoppingParametersDL_r13                   [MAX_NUM_CCs];
  long*			  mpdcch_pdsch_HoppingNB_r13                    [MAX_NUM_CCs];
  BOOLEAN_t		  interval_DLHoppingConfigCommonModeA_r13       [MAX_NUM_CCs];
  long			  interval_DLHoppingConfigCommonModeA_r13_val   [MAX_NUM_CCs];
  BOOLEAN_t		  interval_DLHoppingConfigCommonModeB_r13       [MAX_NUM_CCs];
  long			  interval_DLHoppingConfigCommonModeB_r13_val   [MAX_NUM_CCs];
  long*			  mpdcch_pdsch_HoppingOffset_r13                [MAX_NUM_CCs];

//  +kogo -- rach_CE_LevelInfoList_r13
  long firstPreamble_r13                 [MAX_NUM_CCs][4];
  long lastPreamble_r13                  [MAX_NUM_CCs][4];
  long ra_ResponseWindowSize_r13         [MAX_NUM_CCs][4];
  long mac_ContentionResolutionTimer_r13 [MAX_NUM_CCs][4];
  long rar_HoppingConfig_r13             [MAX_NUM_CCs][4];
  int  rach_CE_LevelInfoList_r13_size    [MAX_NUM_CCs];
//  end kogo


// +kogo -- rsrp_range_list
  long rsrp_range           [MAX_NUM_CCs][3];
  int rsrp_range_list_size  [MAX_NUM_CCs];
//  end kogo

//   +kogo -- prach parameters ce list
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
//    end kogo


// +kogo n1PUCCH_AN_InfoList_r13 list
    long pucch_info_value       [MAX_NUM_CCs][4];
    int  pucch_info_value_size  [MAX_NUM_CCs];
//  end kogo

    // +kogo
    bool  pcch_config_v1310               [MAX_NUM_CCs];
    long  paging_narrowbands_r13          [MAX_NUM_CCs];
    long  mpdcch_numrepetition_paging_r13 [MAX_NUM_CCs];
    long  *nb_v1310                        [MAX_NUM_CCs];

#endif
} RrcConfigurationReq;

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
