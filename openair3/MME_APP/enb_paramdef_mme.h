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

/*! \file openair2/ENB_APP/enb_paramdef_mce.h
 * \brief definition of configuration parameters for MME modules 
 * \author Javier MORGADE
 * \date 2019
 * \version 0.1
 * \company VICOMTECH Spain
 * \email: javier.morgade@ieee.org
 * \note
 * \warning
 */

#ifndef __MME_APP_ENB_PARAMDEF_MME__H__
#define __MME_APP_ENB_PARAMDEF_MME__H__

#include "common/config/config_paramdesc.h"
#include "RRC_paramsvalues.h"

#define ENB_CONFIG_STRING_MME_PARAMETERS                                  "mme_parameters"
//#define ENB_CONFIG_STRING_SCHEDULING_INFO_BR                               "scheduling_info_br"
//#define ENB_CONFIG_STRING_RSRP_RANGE_LIST                                  "rsrp_range_list"
//#define ENB_CONFIG_STRING_PRACH_CONFIG_COMMON_V1310                        "prach_ConfigCommon_v1310"
//#define ENB_CONFIG_STRING_MPDCCH_START_SF_CSS_RA_R13                       "mpdcch_startSF_CSS_RA_r13"
//#define ENB_CONFIG_STRING_MPDCCH_START_SF_CSS_RA_R13_VAL                   "mpdcch_startSF_CSS_RA_r13_val"
//#define ENB_CONFIG_STRING_PRACH_HOPPING_OFFSET_R13                         "prach_HoppingOffset_r13"
//#define ENB_CONFIG_STRING_SCHEDULING_INFO_SIB1_BR_R13                      "schedulingInfoSIB1_BR_r13"
//#define ENB_CONFIG_STRING_CELL_SELECTION_INFO_CE_R13                       "cellSelectionInfoCE_r13"
//#define ENB_CONFIG_STRING_Q_RX_LEV_MIN_CE_R13                              "q_RxLevMinCE_r13"
//#define ENB_CONFIG_STRING_BANDWIDTH_REDUCED_ACCESS_RELATED_INFO_R13        "bandwidthReducedAccessRelatedInfo_r13"
//#define ENB_CONFIG_STRING_SI_WINDOW_LENGTH_BR_R13                          "si_WindowLength_BR_r13"
//#define ENB_CONFIG_STRING_SI_REPETITION_PATTERN_R13                        "si_RepetitionPattern_r13"
//#define ENB_CONFIG_STRING_FDD_DOWNLINK_OR_TDD_SUBFRAME_BITMAP_BR_R13       "fdd_DownlinkOrTddSubframeBitmapBR_r13"
//#define ENB_CONFIG_STRING_FDD_DOWNLINK_OR_TDD_SUBFRAME_BITMAP_BR_VAL_R13   "fdd_DownlinkOrTddSubframeBitmapBR_val_r13"
//#define ENB_CONFIG_STRING_START_SYMBOL_BR_R13                              "startSymbolBR_r13"
//#define ENB_CONFIG_STRING_SI_HOPPING_CONFIG_COMMON_R13                     "si_HoppingConfigCommon_r13"
//#define ENB_CONFIG_STRING_SI_VALIDITY_TIME_R13                             "si_ValidityTime_r13"
//#define ENB_CONFIG_STRING_FREQ_HOPPING_PARAMETERS_DL_R13                   "freqHoppingParametersDL_r13"
//#define ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_NB_R13                      "mpdcch_pdsch_HoppingNB_r13"
//#define ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13     "interval_DLHoppingConfigCommonModeA_r13"
//#define ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13_VAL "interval_DLHoppingConfigCommonModeA_r13_val"
//#define ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13     "interval_DLHoppingConfigCommonModeB_r13"
//#define ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13_VAL "interval_DLHoppingConfigCommonModeB_r13_val"
//#define ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_OFFSET_R13                  "mpdcch_pdsch_HoppingOffset_r13"
//#define ENB_CONFIG_STRING_PREAMBLE_TRANSMAX_CE_R13                         "preamble_TransMax_ce_r13"
//#define ENB_CONFIG_STRING_PREAMBLE_TRANSMAX_CE_R13_VAL                     "preamble_TransMax_ce_r13_val"
//#define ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_A_R13           "pdsch_maxNumRepetitionCEmodeA_r13"
//#define ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_B_R13           "pdsch_maxNumRepetitionCEmodeB_r13"
//#define ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_A_R13           "pusch_maxNumRepetitionCEmodeA_r13"
//#define ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_B_R13           "pusch_maxNumRepetitionCEmodeB_r13"
//#define ENB_CONFIG_STRING_PUSCH_HOPPING_OFFSET_V1310                       "pusch_HoppingOffset_v1310"
//#define ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_LIST                       "system_info_value_tag_SI"
//#define ENB_CONFIG_STRING_FIRST_PREAMBLE_R13                               "firstPreamble_r13"
//#define ENB_CONFIG_STRING_LAST_PREAMBLE_R13                                "lastPreamble_r13"
//#define ENB_CONFIG_STRING_RA_RESPONSE_WINDOW_SIZE_R13                      "ra_ResponseWindowSize_r13"
//#define ENB_CONFIG_STRING_MAC_CONTENTION_RESOLUTION_TIMER_R13              "mac_ContentionResolutionTimer_r13"
//#define ENB_CONFIG_STRING_RAR_HOPPING_CONFIG_R13                           "rar_HoppingConfig_r13"
//#define ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13                        "rach_CE_LevelInfoList_r13"
//#define ENB_CONFIG_STRING_PRACH_CONFIG_INDEX_BR                            "prach_config_index_br"
//#define ENB_CONFIG_STRING_PRACH_FREQ_OFFSET_BR                             "prach_freq_offset_br"
//#define ENB_CONFIG_STRING_PRACH_STARTING_SUBFRAME_R13                      "prach_StartingSubframe_r13"
//#define ENB_CONFIG_STRING_MAX_NUM_PER_PREAMBLE_ATTEMPT_CE_R13              "maxNumPreambleAttemptCE_r13"
//#define ENB_CONFIG_STRING_NUM_REPETITION_PER_PREAMBLE_ATTEMPT_R13          "numRepetitionPerPreambleAttempt_r13"
//#define ENB_CONFIG_STRING_MPDCCH_NUM_REPETITION_RA_R13                     "mpdcch_NumRepetition_RA_r13"
//#define ENB_CONFIG_STRING_PRACH_HOPPING_CONFIG_R13                         "prach_HoppingConfig_r13"
//#define ENB_CONFIG_SRING_MAX_AVAILABLE_NARROW_BAND                         "max_available_narrow_band"
//#define ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13                          "prach_parameters_ce_r13"							
//#define ENB_CONFIG_STRING_PUCCH_INFO_VALUE                                 "pucch_info_value"
//#define ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13                          "n1PUCCH_AN_InfoList_r13"
//#define ENB_CONFIG_STRING_PCCH_CONFIG_V1310                                "pcch_config_v1310"
//#define ENB_CONFIG_STRING_PAGING_NARROWBANDS_R13                           "paging_narrowbands_r13"
//#define ENB_CONFIG_STRING_MPDCCH_NUMREPETITION_PAGING_R13                  "mpdcch_numrepetition_paging_r13"
//#define ENB_CONFIG_STRING_NB_V1310                                         "nb_v1310"
//#define ENB_CONFIG_STRING_SIB2_FREQ_HOPPINGPARAMETERS_R13                  "sib2_freq_hoppingParameters_r13" 

typedef struct ccparams_MME_s {
  /// indicator that eMTC is configured for this cell
  int32_t        MME_configured;
 // /// the SIB2 parameters for eMTC SIB2
 // ccparams_lte_t ccparams;
 // int            si_Narrowband_r13;
 // int            si_TBS_r13;
 // int            systemInfoValueTagSi_r13;
 // int            firstPreamble_r13;
 // int            lastPreamble_r13;
 // int            ra_ResponseWindowSize_r13;
 // int            mac_ContentionResolutionTimer_r13;
 // int            rar_HoppingConfig_r13;
 // int            rsrp_range_br;
 // int            prach_config_index_br;
 // int            prach_freq_offset_br;
 // int            prach_StartingSubframe_r13;
 // int            maxNumPreambleAttemptCE_r13;
 // int            numRepetitionPerPreambleAttempt_r13;
 // int            mpdcch_NumRepetition_RA_r13;
 // int            prach_HoppingConfig_r13;
 // int           *maxavailablenarrowband;
 // int            pucch_info_value;
 // int            paging_narrowbands_r13;
 // int            mpdcch_numrepetition_paging_r13;
 // int            nb_v1310;
 // char          *pucch_NumRepetitionCE_Msg4_Level0_r13;
 // char          *pucch_NumRepetitionCE_Msg4_Level1_r13;
 // char          *pucch_NumRepetitionCE_Msg4_Level2_r13;
 // char          *pucch_NumRepetitionCE_Msg4_Level3_r13;
 // int            sib2_mpdcch_pdsch_hoppingNB_r13;
 // char          *sib2_interval_DLHoppingConfigCommonModeA_r13;
 // int            sib2_interval_DLHoppingConfigCommonModeA_r13_val;
 // char          *sib2_interval_DLHoppingConfigCommonModeB_r13;
 // int            sib2_interval_DLHoppingConfigCommonModeB_r13_val;
 // char          *sib2_interval_ULHoppingConfigCommonModeA_r13;
 // int            sib2_interval_ULHoppingConfigCommonModeA_r13_val;
 // char          *sib2_interval_ULHoppingConfigCommonModeB_r13;
 // int            sib2_interval_ULHoppingConfigCommonModeB_r13_val;
 // int            sib2_mpdcch_pdsch_hoppingOffset_r13;
 // int            pusch_HoppingOffset_v1310;
 // int            hyperSFN_r13;
 // int            eDRX_Allowed_r13;
 // int            q_RxLevMinCE_r13;
 // int            q_QualMinRSRQ_CE_r13;
 // char          *si_WindowLength_BR_r13;
 // char          *si_RepetitionPattern_r13;
 // int            startSymbolBR_r13;
 // char          *si_HoppingConfigCommon_r13;
 // char          *si_ValidityTime_r13;
 // char          *mpdcch_pdsch_HoppingNB_r13;
 // int            interval_DLHoppingConfigCommonModeA_r13_val;
 // int            interval_DLHoppingConfigCommonModeB_r13_val;
 // int            mpdcch_pdsch_HoppingOffset_r13;
 // char          *preambleTransMax_CE_r13;
 // int            prach_HoppingOffset_r13;
 // int            schedulingInfoSIB1_BR_r13;
 // int64_t        fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
 // char          *cellSelectionInfoCE_r13;
 // char          *bandwidthReducedAccessRelatedInfo_r13;
 // char          *fdd_DownlinkOrTddSubframeBitmapBR_r13;
 // char          *fdd_UplinkSubframeBitmapBR_r13;
 // char          *freqHoppingParametersDL_r13;
 // char          *interval_DLHoppingConfigCommonModeA_r13;
 // char          *interval_DLHoppingConfigCommonModeB_r13;
 // char          *prach_ConfigCommon_v1310;
 // char          *mpdcch_startSF_CSS_RA_r13;
 // char          *mpdcch_startSF_CSS_RA_r13_val;
 // char          *pdsch_maxNumRepetitionCEmodeA_r13;
 // char          *pdsch_maxNumRepetitionCEmodeB_r13;
 // char          *pusch_maxNumRepetitionCEmodeA_r13;
 // char          *pusch_maxNumRepetitionCEmodeB_r13;
} ccparams_MME_t;


#define MMEPARAMS_DESC(MMEconfig) {					\
  {"MME_configured",                                              NULL,   0,           iptr:&MMEconfig->MME_configured,                             defintval:0,                       TYPE_UINT,         0} \
}

#define MMEPARAMS_CHECK                 {                                     \
             { .s5= {NULL }}						     \
}

#endif
