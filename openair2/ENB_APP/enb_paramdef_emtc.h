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

/*! \file openair2/ENB_APP/enb_paramdef_emtc.h
 * \brief definition of configuration parameters for emtc eNodeB modules 
 * \author Raymond KNOPP
 * \date 2018
 * \version 0.1
 * \company EURECOM France
 * \email: raymond.knopp@eurecom.fr
 * \note
 * \warning
 */

#ifndef __ENB_APP_ENB_PARAMDEF_EMTC__H__
#define __ENB_APP_ENB_PARAMDEF_EMTC__H__

#include "common/config/config_paramdesc.h"
#include "RRC_paramsvalues.h"

#define ENB_CONFIG_STRING_EMTC_PARAMETERS                                  "emtc_parameters"
#define ENB_CONFIG_STRING_SCHEDULING_INFO_BR                               "scheduling_info_br"
#define ENB_CONFIG_STRING_RSRP_RANGE_LIST                                  "rsrp_range_list"
#define ENB_CONFIG_STRING_PRACH_CONFIG_COMMON_V1310                        "prach_ConfigCommon_v1310"
#define ENB_CONFIG_STRING_MPDCCH_START_SF_CSS_RA_R13                       "mpdcch_startSF_CSS_RA_r13"
#define ENB_CONFIG_STRING_MPDCCH_START_SF_CSS_RA_R13_VAL                   "mpdcch_startSF_CSS_RA_r13_val"
#define ENB_CONFIG_STRING_PRACH_HOPPING_OFFSET_R13                         "prach_HoppingOffset_r13"
#define ENB_CONFIG_STRING_SCHEDULING_INFO_SIB1_BR_R13                      "schedulingInfoSIB1_BR_r13"
#define ENB_CONFIG_STRING_CELL_SELECTION_INFO_CE_R13                       "cellSelectionInfoCE_r13"
#define ENB_CONFIG_STRING_Q_RX_LEV_MIN_CE_R13                              "q_RxLevMinCE_r13"
#define ENB_CONFIG_STRING_BANDWIDTH_REDUCED_ACCESS_RELATED_INFO_R13        "bandwidthReducedAccessRelatedInfo_r13"
#define ENB_CONFIG_STRING_SI_WINDOW_LENGTH_BR_R13                          "si_WindowLength_BR_r13"
#define ENB_CONFIG_STRING_SI_REPETITION_PATTERN_R13                        "si_RepetitionPattern_r13"
#define ENB_CONFIG_STRING_FDD_DOWNLINK_OR_TDD_SUBFRAME_BITMAP_BR_R13       "fdd_DownlinkOrTddSubframeBitmapBR_r13"
#define ENB_CONFIG_STRING_FDD_DOWNLINK_OR_TDD_SUBFRAME_BITMAP_BR_VAL_R13   "fdd_DownlinkOrTddSubframeBitmapBR_val_r13"
#define ENB_CONFIG_STRING_START_SYMBOL_BR_R13                              "startSymbolBR_r13"
#define ENB_CONFIG_STRING_SI_HOPPING_CONFIG_COMMON_R13                     "si_HoppingConfigCommon_r13"
#define ENB_CONFIG_STRING_SI_VALIDITY_TIME_R13                             "si_ValidityTime_r13"
#define ENB_CONFIG_STRING_FREQ_HOPPING_PARAMETERS_DL_R13                   "freqHoppingParametersDL_r13"
#define ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_NB_R13                      "mpdcch_pdsch_HoppingNB_r13"
#define ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13     "interval_DLHoppingConfigCommonModeA_r13"
#define ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13_VAL "interval_DLHoppingConfigCommonModeA_r13_val"
#define ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13     "interval_DLHoppingConfigCommonModeB_r13"
#define ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13_VAL "interval_DLHoppingConfigCommonModeB_r13_val"
#define ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_OFFSET_R13                  "mpdcch_pdsch_HoppingOffset_r13"
#define ENB_CONFIG_STRING_PREAMBLE_TRANSMAX_CE_R13                         "preamble_TransMax_ce_r13"
#define ENB_CONFIG_STRING_PREAMBLE_TRANSMAX_CE_R13_VAL                     "preamble_TransMax_ce_r13_val"
#define ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_A_R13           "pdsch_maxNumRepetitionCEmodeA_r13"
#define ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_B_R13           "pdsch_maxNumRepetitionCEmodeB_r13"
#define ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_A_R13           "pusch_maxNumRepetitionCEmodeA_r13"
#define ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_B_R13           "pusch_maxNumRepetitionCEmodeB_r13"
#define ENB_CONFIG_STRING_PUSCH_REPETITION_LEVEL_CE_MODE_A_R13			   "pusch_repetitionLevelCEmodeA_r13"
#define ENB_CONFIG_STRING_PUSCH_HOPPING_OFFSET_V1310                       "pusch_HoppingOffset_v1310"
#define ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_LIST                       "system_info_value_tag_SI"
#define ENB_CONFIG_STRING_FIRST_PREAMBLE_R13                               "firstPreamble_r13"
#define ENB_CONFIG_STRING_LAST_PREAMBLE_R13                                "lastPreamble_r13"
#define ENB_CONFIG_STRING_RA_RESPONSE_WINDOW_SIZE_R13                      "ra_ResponseWindowSize_r13"
#define ENB_CONFIG_STRING_MAC_CONTENTION_RESOLUTION_TIMER_R13              "mac_ContentionResolutionTimer_r13"
#define ENB_CONFIG_STRING_RAR_HOPPING_CONFIG_R13                           "rar_HoppingConfig_r13"
#define ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13                        "rach_CE_LevelInfoList_r13"
#define ENB_CONFIG_STRING_PRACH_CONFIG_INDEX_BR                            "prach_config_index_br"
#define ENB_CONFIG_STRING_PRACH_FREQ_OFFSET_BR                             "prach_freq_offset_br"
#define ENB_CONFIG_STRING_PRACH_STARTING_SUBFRAME_R13                      "prach_StartingSubframe_r13"
#define ENB_CONFIG_STRING_MAX_NUM_PER_PREAMBLE_ATTEMPT_CE_R13              "maxNumPreambleAttemptCE_r13"
#define ENB_CONFIG_STRING_NUM_REPETITION_PER_PREAMBLE_ATTEMPT_R13          "numRepetitionPerPreambleAttempt_r13"
#define ENB_CONFIG_STRING_MPDCCH_NUM_REPETITION_RA_R13                     "mpdcch_NumRepetition_RA_r13"
#define ENB_CONFIG_STRING_PRACH_HOPPING_CONFIG_R13                         "prach_HoppingConfig_r13"
#define ENB_CONFIG_SRING_MAX_AVAILABLE_NARROW_BAND                         "max_available_narrow_band"
#define ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13                          "prach_parameters_ce_r13"							
#define ENB_CONFIG_STRING_PUCCH_INFO_VALUE                                 "pucch_info_value"
#define ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13                          "n1PUCCH_AN_InfoList_r13"
#define ENB_CONFIG_STRING_PCCH_CONFIG_V1310                                "pcch_config_v1310"
#define ENB_CONFIG_STRING_PAGING_NARROWBANDS_R13                           "paging_narrowbands_r13"
#define ENB_CONFIG_STRING_MPDCCH_NUMREPETITION_PAGING_R13                  "mpdcch_numrepetition_paging_r13"
#define ENB_CONFIG_STRING_NB_V1310                                         "nb_v1310"
#define ENB_CONFIG_STRING_SIB2_FREQ_HOPPINGPARAMETERS_R13                  "sib2_freq_hoppingParameters_r13" 

typedef struct ccparams_eMTC_s {
  /// indicator that eMTC is configured for this cell
  int32_t        eMTC_configured;
  /// the SIB2 parameters for eMTC SIB2
  ccparams_lte_t ccparams;
  int            si_Narrowband_r13;
  int            si_TBS_r13;
  int            systemInfoValueTagSi_r13;
  int            firstPreamble_r13;
  int            lastPreamble_r13;
  int            ra_ResponseWindowSize_r13;
  int            mac_ContentionResolutionTimer_r13;
  int            rar_HoppingConfig_r13;
  int            rsrp_range_br;
  int            prach_config_index_br;
  int            prach_freq_offset_br;
  int            prach_StartingSubframe_r13;
  int            maxNumPreambleAttemptCE_r13;
  int            numRepetitionPerPreambleAttempt_r13;
  int            mpdcch_NumRepetition_RA_r13;
  int            prach_HoppingConfig_r13;
  int           *maxavailablenarrowband;
  int            pucch_info_value;
  int            paging_narrowbands_r13;
  int            mpdcch_numrepetition_paging_r13;
  int            nb_v1310;
  char          *pucch_NumRepetitionCE_Msg4_Level0_r13;
  char          *pucch_NumRepetitionCE_Msg4_Level1_r13;
  char          *pucch_NumRepetitionCE_Msg4_Level2_r13;
  char          *pucch_NumRepetitionCE_Msg4_Level3_r13;
  int            sib2_mpdcch_pdsch_hoppingNB_r13;
  char          *sib2_interval_DLHoppingConfigCommonModeA_r13;
  int            sib2_interval_DLHoppingConfigCommonModeA_r13_val;
  char          *sib2_interval_DLHoppingConfigCommonModeB_r13;
  int            sib2_interval_DLHoppingConfigCommonModeB_r13_val;
  char          *sib2_interval_ULHoppingConfigCommonModeA_r13;
  int            sib2_interval_ULHoppingConfigCommonModeA_r13_val;
  char          *sib2_interval_ULHoppingConfigCommonModeB_r13;
  int            sib2_interval_ULHoppingConfigCommonModeB_r13_val;
  int            sib2_mpdcch_pdsch_hoppingOffset_r13;
  int            pusch_HoppingOffset_v1310;
  int            hyperSFN_r13;
  int            eDRX_Allowed_r13;
  int            q_RxLevMinCE_r13;
  int            q_QualMinRSRQ_CE_r13;
  char          *si_WindowLength_BR_r13;
  char          *si_RepetitionPattern_r13;
  int            startSymbolBR_r13;
  char          *si_HoppingConfigCommon_r13;
  char          *si_ValidityTime_r13;
  char          *mpdcch_pdsch_HoppingNB_r13;
  int            interval_DLHoppingConfigCommonModeA_r13_val;
  int            interval_DLHoppingConfigCommonModeB_r13_val;
  int            mpdcch_pdsch_HoppingOffset_r13;
  int            preambleTransMax_CE_r13;
  int            prach_HoppingOffset_r13;
  int            schedulingInfoSIB1_BR_r13;
  int64_t        fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
  char          *cellSelectionInfoCE_r13;
  char          *bandwidthReducedAccessRelatedInfo_r13;
  char          *fdd_DownlinkOrTddSubframeBitmapBR_r13;
  char          *fdd_UplinkSubframeBitmapBR_r13;
  char          *freqHoppingParametersDL_r13;
  char          *interval_DLHoppingConfigCommonModeA_r13;
  char          *interval_DLHoppingConfigCommonModeB_r13;
  char          *prach_ConfigCommon_v1310;
  char          *mpdcch_startSF_CSS_RA_r13;
  char          *mpdcch_startSF_CSS_RA_r13_val;
  char          *pdsch_maxNumRepetitionCEmodeA_r13;
  char          *pdsch_maxNumRepetitionCEmodeB_r13;
  char          *pusch_maxNumRepetitionCEmodeA_r13;
  char          *pusch_maxNumRepetitionCEmodeB_r13;
  char			*pusch_repetitionLevelCEmodeA_r13;
} ccparams_eMTC_t;


#define EMTCPARAMS_DESC(eMTCconfig) {					\
  {"eMTC_configured",                                              NULL,   0,           iptr:&eMTCconfig->eMTC_configured,                             defintval:0,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_PRACH_ROOT,                                           NULL,   0,           iptr:&eMTCconfig->ccparams.prach_root,                                  defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PRACH_CONFIG_INDEX,                                   NULL,   0,           iptr:&eMTCconfig->ccparams.prach_config_index,                          defintval:0,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PRACH_HIGH_SPEED,                                     NULL,   0,           strptr:&eMTCconfig->ccparams.prach_high_speed,                          defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION,                               NULL,   0,           iptr:&eMTCconfig->ccparams.prach_zero_correlation,                      defintval:1,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PRACH_FREQ_OFFSET,                                    NULL,   0,           iptr:&eMTCconfig->ccparams.prach_freq_offset,                           defintval:2,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT,                                    NULL,   0,           iptr:&eMTCconfig->ccparams.pucch_delta_shift,                           defintval:1,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_NRB_CQI,                                        NULL,   0,           iptr:&eMTCconfig->ccparams.pucch_nRB_CQI,                               defintval:1,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_NCS_AN,                                         NULL,   0,           iptr:&eMTCconfig->ccparams.pucch_nCS_AN,                                defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_N1_AN,                                          NULL,   0,           iptr:&eMTCconfig->ccparams.pucch_n1_AN,                                 defintval:32,                      TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PDSCH_RS_EPRE,                                        NULL,   0,           iptr:&eMTCconfig->ccparams.pdsch_referenceSignalPower,                  defintval:-29,                     TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PDSCH_PB,                                             NULL,   0,           iptr:&eMTCconfig->ccparams.pdsch_p_b,                                   defintval:0,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_N_SB,                                           NULL,   0,           iptr:&eMTCconfig->ccparams.pusch_n_SB,                                  defintval:1,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_HOPPINGMODE,                                    NULL,   0,           strptr:&eMTCconfig->ccparams.pusch_hoppingMode,                         defstrval:"interSubFrame",         TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET,                                  NULL,   0,           iptr:&eMTCconfig->ccparams.pusch_hoppingOffset,                         defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUSCH_ENABLE64QAM,                                    NULL,   0,           strptr:&eMTCconfig->ccparams.pusch_enable64QAM,                         defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN,                               NULL,   0,           strptr:&eMTCconfig->ccparams.pusch_groupHoppingEnabled,                 defstrval:"ENABLE",                TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT,                               NULL,   0,           iptr:&eMTCconfig->ccparams.pusch_groupAssignment,                       defintval:0,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN,                            NULL,   0,           strptr:&eMTCconfig->ccparams.pusch_sequenceHoppingEnabled,              defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_NDMRS1,                                         NULL,   0,           iptr:&eMTCconfig->ccparams.pusch_nDMRS1,                                defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PHICH_DURATION,                                       NULL,   0,           strptr:&eMTCconfig->ccparams.phich_duration,                            defstrval:"NORMAL",                TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PHICH_RESOURCE,                                       NULL,   0,           strptr:&eMTCconfig->ccparams.phich_resource,                            defstrval:"ONESIXTH",              TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_ENABLE,                                           NULL,   0,           strptr:&eMTCconfig->ccparams.srs_enable,                                defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG,                                 NULL,   0,           iptr:&eMTCconfig->ccparams.srs_BandwidthConfig,                         defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG,                                  NULL,   0,           iptr:&eMTCconfig->ccparams.srs_SubframeConfig,                          defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG,                                 NULL,   0,           strptr:&eMTCconfig->ccparams.srs_ackNackST,                             defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_MAXUPPTS,                                         NULL,   0,           strptr:&eMTCconfig->ccparams.srs_MaxUpPts,                              defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_PO_NOMINAL,                                     NULL,   0,           iptr:&eMTCconfig->ccparams.pusch_p0_Nominal,                            defintval:-90,                     TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_ALPHA,                                          NULL,   0,           strptr:&eMTCconfig->ccparams.pusch_alpha,                               defstrval:"AL1",                   TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_PO_NOMINAL,                                     NULL,   0,           iptr:&eMTCconfig->ccparams.pucch_p0_Nominal,                            defintval:-96,                     TYPE_INT,        0},  \
{ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE,                                  NULL,   0,           iptr:&eMTCconfig->ccparams.msg3_delta_Preamble,                         defintval:6,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1,                                 NULL,   0,           strptr:&eMTCconfig->ccparams.pucch_deltaF_Format1,                      defstrval:"DELTAF2",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b,                                NULL,   0,           strptr:&eMTCconfig->ccparams.pucch_deltaF_Format1b,                     defstrval:"deltaF3",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2,                                 NULL,   0,           strptr:&eMTCconfig->ccparams.pucch_deltaF_Format2,                      defstrval:"deltaF0",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A,                                NULL,   0,           strptr:&eMTCconfig->ccparams.pucch_deltaF_Format2a,                     defstrval:"deltaF0",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B,                                NULL,   0,           strptr:&eMTCconfig->ccparams.pucch_deltaF_Format2b,                     defstrval:"deltaF0",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES,                                NULL,   0,           iptr:&eMTCconfig->ccparams.rach_numberOfRA_Preambles,                   defintval:4,                       TYPE_UINT,     0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG,                           NULL,   0,           strptr:&eMTCconfig->ccparams.rach_preamblesGroupAConfig,                defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA,                        NULL,   0,           iptr:&eMTCconfig->ccparams.rach_sizeOfRA_PreamblesGroupA,               defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA,                               NULL,   0,           iptr:&eMTCconfig->ccparams.rach_messageSizeGroupA,                      defintval:56,                      TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB,                        NULL,   0,           strptr:&eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,             defstrval:"minusinfinity",         TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP,                                NULL,   0,           iptr:&eMTCconfig->ccparams.rach_powerRampingStep,                       defintval:4,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER,              NULL,   0,           iptr:&eMTCconfig->ccparams.rach_preambleInitialReceivedTargetPower,     defintval:-100,                    TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX,                                NULL,   0,           iptr:&eMTCconfig->ccparams.rach_preambleTransMax,                       defintval:10,                      TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE,                            NULL,   0,           iptr:&eMTCconfig->ccparams.rach_raResponseWindowSize,                   defintval:10,                      TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER,                    NULL,   0,           iptr:&eMTCconfig->ccparams.rach_macContentionResolutionTimer,           defintval:48,                      TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX,                                   NULL,   0,           iptr:&eMTCconfig->ccparams.rach_maxHARQ_Msg3Tx,                         defintval:4,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE,                            NULL,   0,           iptr:&eMTCconfig->ccparams.pcch_defaultPagingCycle,                     defintval:128,                 TYPE_INT,     0},  \
{ENB_CONFIG_STRING_PCCH_NB,                                              NULL,   0,           strptr:&eMTCconfig->ccparams.pcch_nB,                                   defstrval:"oneT",                  TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF,                         NULL,   0,           iptr:&eMTCconfig->ccparams.bcch_modificationPeriodCoeff,                defintval:2,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG,                           NULL,   0,           strptr:&eMTCconfig->ccparams.rach_preamblesGroupAConfig,                defstrval:"",                      TYPE_STRING,     0}, \
{ENB_CONFIG_STRING_UETIMERS_T300,                                NULL,   0,           iptr:&eMTCconfig->ccparams.ue_TimersAndConstants_t300,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T301,                                NULL,   0,           iptr:&eMTCconfig->ccparams.ue_TimersAndConstants_t301,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T310,                                NULL,   0,           iptr:&eMTCconfig->ccparams.ue_TimersAndConstants_t310,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T311,                                NULL,   0,           iptr:&eMTCconfig->ccparams.ue_TimersAndConstants_t311,               defintval:10000,           TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_N310,                                NULL,   0,           iptr:&eMTCconfig->ccparams.ue_TimersAndConstants_n310,               defintval:20,              TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_N311,                                NULL,   0,           iptr:&eMTCconfig->ccparams.ue_TimersAndConstants_n311,               defintval:1,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UE_TRANSMISSION_MODE,                         NULL,   0,           iptr:&eMTCconfig->ccparams.ue_TransmissionMode,                      defintval:1,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_SCHEDULING_INFO_SIB1_BR_R13,                  NULL,   0,           iptr:&eMTCconfig->schedulingInfoSIB1_BR_r13,                   defintval:4,                       TYPE_UINT,         0}, \
  {ENB_CONFIG_STRING_PRACH_CONFIG_COMMON_V1310,                    NULL,   0,           strptr:&eMTCconfig->prach_ConfigCommon_v1310,               defstrval:"ENABLE",        TYPE_STRING,     0}, \
  {ENB_CONFIG_STRING_MPDCCH_START_SF_CSS_RA_R13,                   NULL,   0,           strptr:&eMTCconfig->mpdcch_startSF_CSS_RA_r13,              defstrval:"fdd-r13",       TYPE_STRING,     0}, \
  {ENB_CONFIG_STRING_MPDCCH_START_SF_CSS_RA_R13_VAL,               NULL,   0,           strptr:&eMTCconfig->mpdcch_startSF_CSS_RA_r13_val,          defstrval:"v1",            TYPE_STRING,     0}, \
  {ENB_CONFIG_STRING_PRACH_HOPPING_OFFSET_R13,                     NULL,   0,           iptr:&eMTCconfig->prach_HoppingOffset_r13,                  defintval:0,               TYPE_INT,        0}, \
  {ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_A_R13,       NULL,   0,           strptr:&eMTCconfig->pdsch_maxNumRepetitionCEmodeA_r13,      defstrval:"r16",           TYPE_STRING,     0}, \
  {ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_A_R13,       NULL,   0,           strptr:&eMTCconfig->pusch_maxNumRepetitionCEmodeA_r13,      defstrval:"r8",            TYPE_STRING,     0},	\
  {ENB_CONFIG_STRING_PUSCH_REPETITION_LEVEL_CE_MODE_A_R13,		   NULL,   0,           strptr:&eMTCconfig->pusch_repetitionLevelCEmodeA_r13,       defstrval:"l1",      	   TYPE_STRING,     0}, \
{ENB_CONFIG_STRING_CELL_SELECTION_INFO_CE_R13,                           NULL,   0,           strptr:&eMTCconfig->cellSelectionInfoCE_r13,                   defstrval:"ENABLE",                TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_Q_RX_LEV_MIN_CE_R13,                                  NULL,   0,           iptr:&eMTCconfig->q_RxLevMinCE_r13,                            defintval:-70,                     TYPE_INT,          0}, \
{ENB_CONFIG_STRING_BANDWIDTH_REDUCED_ACCESS_RELATED_INFO_R13,            NULL,   0,           strptr:&eMTCconfig->bandwidthReducedAccessRelatedInfo_r13,     defstrval:"ENABLE",                TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_SI_WINDOW_LENGTH_BR_R13,                              NULL,   0,           strptr:&eMTCconfig->si_WindowLength_BR_r13,                    defstrval:"ms20",                  TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_SI_REPETITION_PATTERN_R13,                            NULL,   0,           strptr:&eMTCconfig->si_RepetitionPattern_r13,                  defstrval:"everyRF",               TYPE_STRING,       0},			\
{ENB_CONFIG_STRING_FDD_DOWNLINK_OR_TDD_SUBFRAME_BITMAP_BR_R13,           NULL,   0,           strptr:&eMTCconfig->fdd_DownlinkOrTddSubframeBitmapBR_r13,     defstrval:"subframePattern40-r13", TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_FDD_DOWNLINK_OR_TDD_SUBFRAME_BITMAP_BR_VAL_R13,       NULL,   0,           i64ptr:&eMTCconfig->fdd_DownlinkOrTddSubframeBitmapBR_val_r13, defint64val:0xFFFFFFFFFF,          TYPE_UINT64,       0}, \
{ENB_CONFIG_STRING_START_SYMBOL_BR_R13,                                  NULL,   0,           iptr:&eMTCconfig->startSymbolBR_r13,                           defintval:3,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_SI_HOPPING_CONFIG_COMMON_R13,                         NULL,   0,           strptr:&eMTCconfig->si_HoppingConfigCommon_r13 ,               defstrval:"off",                   TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_SI_VALIDITY_TIME_R13,                                 NULL,   0,           strptr:&eMTCconfig->si_ValidityTime_r13,                       defstrval:"true",                  TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_FREQ_HOPPING_PARAMETERS_DL_R13,                       NULL,   0,           strptr:&eMTCconfig->freqHoppingParametersDL_r13,               defstrval:"DISABLE",               TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_NB_R13,                          NULL,   0,           strptr:&eMTCconfig->mpdcch_pdsch_HoppingNB_r13,                defstrval:"nb2",                   TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13,         NULL,   0,           strptr:&eMTCconfig->interval_DLHoppingConfigCommonModeA_r13,   defstrval:"interval-FDD-r13",      TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13_VAL,     NULL,   0,           iptr:&eMTCconfig->interval_DLHoppingConfigCommonModeA_r13_val, defintval:0,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13,         NULL,   0,           strptr:&eMTCconfig->interval_DLHoppingConfigCommonModeB_r13,   defstrval:"interval-FDD-r13",      TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13_VAL,     NULL,   0,           iptr:&eMTCconfig->interval_DLHoppingConfigCommonModeB_r13_val, defintval:0,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_OFFSET_R13,                      NULL,   0,           iptr:&eMTCconfig->mpdcch_pdsch_HoppingOffset_r13,              defintval:1,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_PREAMBLE_TRANSMAX_CE_R13,                             NULL,   0,           iptr:&eMTCconfig->preambleTransMax_CE_r13,                   defintval:10,                   TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL0,                  NULL,   0,           strptr:&eMTCconfig->pucch_NumRepetitionCE_Msg4_Level0_r13,     defstrval:"n1",                    TYPE_STRING,     0}, \
{ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL1,                  NULL,   0,           strptr:&eMTCconfig->pucch_NumRepetitionCE_Msg4_Level1_r13,     defstrval:"",                      TYPE_STRING,     0}, \
{ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL2,                  NULL,   0,           strptr:&eMTCconfig->pucch_NumRepetitionCE_Msg4_Level2_r13,     defstrval:"",                      TYPE_STRING,     0}, \
  {ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL3,                  NULL,   0,           strptr:&eMTCconfig->pucch_NumRepetitionCE_Msg4_Level3_r13,     defstrval:"",                      TYPE_STRING,     0} \
 }

#define EMTCPARAMS_CHECK                 {                                     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_T300_OKVALUES, UETIMER_T300_MODVALUES,8}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_T301_OKVALUES, UETIMER_T301_MODVALUES,8}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_T310_OKVALUES, UETIMER_T310_MODVALUES,7}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_T311_OKVALUES, UETIMER_T311_MODVALUES,7}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_N310_OKVALUES, UETIMER_N310_MODVALUES,8}} , 					      \
             { .s1a= { config_check_modify_integer, UETIMER_N311_OKVALUES, UETIMER_N311_MODVALUES,8}} , 					      \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
             { {NULL}} ,						     \
}

#define SYSTEM_INFO_VALUE_TAG_SI_DESC(eMTCconfig) {			\
{"systemInfoValueTagSi_r13", NULL,   0,           iptr:&eMTCconfig->systemInfoValueTagSi_r13,                              defintval:0,              TYPE_UINT,       0} \
  }


#define SI_INFO_BR_DESC(eMTCconfig) {					\
{"si_Narrowband_r13", NULL,   0,           iptr:&eMTCconfig->si_Narrowband_r13,             defintval:5,             TYPE_UINT,       0}, \
{"si_TBS_r13",        NULL,   0,           iptr:&eMTCconfig->si_TBS_r13,                    defintval:5,             TYPE_UINT,       0} \
}

#define RSRP_RANGE_LIST_DESC(eMTCconfig) {				\
{"rsrp_range_br", NULL,   0,           iptr:&eMTCconfig->rsrp_range_br,                     defintval:0,     TYPE_UINT,       0} \
  }


#define RACH_CE_LEVELINFOLIST_R13_DESC(eMTCconfig) {			\
{ENB_CONFIG_STRING_FIRST_PREAMBLE_R13,                     NULL,   0,           iptr:&eMTCconfig->firstPreamble_r13,                 defintval:60,            TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_LAST_PREAMBLE_R13,                      NULL,   0,           iptr:&eMTCconfig->lastPreamble_r13,                  defintval:63,            TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_RA_RESPONSE_WINDOW_SIZE_R13,            NULL,   0,           iptr:&eMTCconfig->ra_ResponseWindowSize_r13,         defintval:20,        TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_MAC_CONTENTION_RESOLUTION_TIMER_R13,    NULL,   0,           iptr:&eMTCconfig->mac_ContentionResolutionTimer_r13, defintval:80,        TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_RAR_HOPPING_CONFIG_R13,                 NULL,   0,           iptr:&eMTCconfig->rar_HoppingConfig_r13,             defintval:1,         TYPE_UINT,     0}\
}

#define PRACH_PARAMS_CE_R13_DESC(eMTCconfig) {				\
{ENB_CONFIG_STRING_PRACH_CONFIG_INDEX_BR,                   NULL,   0,           iptr:&eMTCconfig->prach_config_index_br,                 defintval:3,             TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_PRACH_FREQ_OFFSET_BR,                    NULL,   0,           iptr:&eMTCconfig->prach_freq_offset_br,                  defintval:1,             TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_PRACH_STARTING_SUBFRAME_R13,             NULL,   0,           iptr:&eMTCconfig->prach_StartingSubframe_r13,            defintval:0,             TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_MAX_NUM_PER_PREAMBLE_ATTEMPT_CE_R13,     NULL,   0,           iptr:&eMTCconfig->maxNumPreambleAttemptCE_r13,         defintval:10,         TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_NUM_REPETITION_PER_PREAMBLE_ATTEMPT_R13, NULL,   0,           iptr:&eMTCconfig->numRepetitionPerPreambleAttempt_r13, defintval:1,          TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_MPDCCH_NUM_REPETITION_RA_R13,            NULL,   0,           iptr:&eMTCconfig->mpdcch_NumRepetition_RA_r13,         defintval:1,          TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_PRACH_HOPPING_CONFIG_R13,                NULL,   0,           iptr:&eMTCconfig->prach_HoppingConfig_r13,             defintval:0,         TYPE_UINT,     0}, \
{ENB_CONFIG_SRING_MAX_AVAILABLE_NARROW_BAND,                NULL,   0,           uptr:NULL,                                  defintarrayval:NULL,     TYPE_INTARRAY,   0} \
}

#define N1PUCCH_AN_INFOLIST_R13_DESC(eMTCconfig) {			\
{ENB_CONFIG_STRING_PUCCH_INFO_VALUE,                     NULL,   0,           iptr:&eMTCconfig->pucch_info_value,                    defintval:0,             TYPE_UINT,       0} \
}


#define PCCH_CONFIG_V1310_DESC(eMTCconfig) {				\
{ENB_CONFIG_STRING_PAGING_NARROWBANDS_R13,          NULL,   0,           iptr:&eMTCconfig->paging_narrowbands_r13,                   defintval:1,             TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_MPDCCH_NUMREPETITION_PAGING_R13, NULL,   0,           iptr:&eMTCconfig->mpdcch_numrepetition_paging_r13,          defintval:1,          TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_NB_V1310,                        NULL,   0,           iptr:&eMTCconfig->nb_v1310,                                 defintval:256,   TYPE_UINT,     0} \
}


#define SIB2_FREQ_HOPPING_R13_DESC(eMTCconfig) {			\
{ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_NB_R13,                      NULL,   0,           iptr:&eMTCconfig->sib2_mpdcch_pdsch_hoppingNB_r13,                  defintval:0,            TYPE_UINT,       0}, \
{"sib2_interval_DLHoppingConfigCommonModeA_r13",                     NULL,   0,           strptr:&eMTCconfig->sib2_interval_DLHoppingConfigCommonModeA_r13,       defstrval:"FDD",            TYPE_STRING,       0}, \
{"sib2_interval_DLHoppingConfigCommonModeA_r13_val",                 NULL,   0,           iptr:&eMTCconfig->sib2_interval_DLHoppingConfigCommonModeA_r13_val,   defintval:0,            TYPE_UINT,       0}, \
{"sib2_interval_DLHoppingConfigCommonModeB_r13",                     NULL,   0,           strptr:&eMTCconfig->sib2_interval_DLHoppingConfigCommonModeB_r13,       defstrval:"FDD",            TYPE_STRING,       0}, \
{"sib2_interval_DLHoppingConfigCommonModeB_r13_val",                 NULL,   0,           iptr:&eMTCconfig->sib2_interval_DLHoppingConfigCommonModeB_r13_val,   defintval:0,            TYPE_UINT,       0}, \
{"sib2_interval_ULHoppingConfigCommonModeA_r13",     NULL,   0,           strptr:&eMTCconfig->sib2_interval_ULHoppingConfigCommonModeA_r13,     defstrval:"FDD",         TYPE_STRING,       0}, \
{"sib2_interval_ULHoppingConfigCommonModeA_r13_val", NULL,   0,           iptr:&eMTCconfig->sib2_interval_ULHoppingConfigCommonModeA_r13_val, defintval:4,        TYPE_UINT,       0}, \
{"sib2_interval_ULHoppingConfigCommonModeB_r13",                     NULL,   0,           strptr:&eMTCconfig->sib2_interval_ULHoppingConfigCommonModeB_r13,       defstrval:"FDD",            TYPE_STRING,       0}, \
{"sib2_interval_ULHoppingConfigCommonModeB_r13_val",                 NULL,   0,           iptr:&eMTCconfig->sib2_interval_ULHoppingConfigCommonModeB_r13_val,   defintval:0,            TYPE_UINT,       0}, \
{"sib2_mpdcch_pdsch_hoppingOffset_r13",                              NULL,   0,           iptr:&eMTCconfig->sib2_mpdcch_pdsch_hoppingOffset_r13,                defintval:1,             TYPE_UINT,         0} \
}

#endif
