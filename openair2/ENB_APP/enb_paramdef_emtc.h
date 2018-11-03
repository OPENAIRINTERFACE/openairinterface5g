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

#include "common/config/config_paramdesc.h"
#include "RRC_paramsvalues.h"

#define ENB_CONFIG_STRING_SCHEDULING_INFO_LIST                   "scheduling_info_br"



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

#define ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_A_R13        "pdsch_maxNumRepetitionCEmodeA_r13"
#define ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_A_R13        "pusch_maxNumRepetitionCEmodeA_r13"



#define ENB_CONFIG_STRING_BR_PARAMETERS                          "br_parameters"
#define BRPARAMS_DESC { \
{"eMTC_configured",                                                      NULL,   0,           iptr:&eMTC_configured,                             defintval:0,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_SCHEDULING_INFO_SIB1_BR_R13,                          NULL,   0,           iptr:&schedulingInfoSIB1_BR_r13,                   defintval:4,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_CELL_SELECTION_INFO_CE_R13,                           NULL,   0,           strptr:&cellSelectionInfoCE_r13,                   defstrval:"ENABLE",                TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_Q_RX_LEV_MIN_CE_R13,                                  NULL,   0,           iptr:&q_RxLevMinCE_r13,                            defintval:-70,                     TYPE_INT,          0}, \
{ENB_CONFIG_STRING_BANDWIDTH_REDUCED_ACCESS_RELATED_INFO_R13,            NULL,   0,           strptr:&bandwidthReducedAccessRelatedInfo_r13,     defstrval:"ENABLE",                TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_SI_WINDOW_LENGTH_BR_R13,                              NULL,   0,           strptr:&si_WindowLength_BR_r13,                    defstrval:"ms20",                  TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_SI_REPETITION_PATTERN_R13,                            NULL,   0,           strptr:&si_RepetitionPattern_r13,                  defstrval:"everyRF",               TYPE_STRING,       0},			\
{ENB_CONFIG_STRING_FDD_DOWNLINK_OR_TDD_SUBFRAME_BITMAP_BR_R13,           NULL,   0,           strptr:&fdd_DownlinkOrTddSubframeBitmapBR_r13,     defstrval:"subframePattern40-r13", TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_FDD_DOWNLINK_OR_TDD_SUBFRAME_BITMAP_BR_VAL_R13,       NULL,   0,           i64ptr:&fdd_DownlinkOrTddSubframeBitmapBR_val_r13, defint64val:0xFFFFFFFFFF,          TYPE_UINT64,       0}, \
{ENB_CONFIG_STRING_START_SYMBOL_BR_R13,                                  NULL,   0,           iptr:&startSymbolBR_r13,                           defintval:3,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_SI_HOPPING_CONFIG_COMMON_R13,                         NULL,   0,           strptr:&si_HoppingConfigCommon_r13 ,               defstrval:"off",                   TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_SI_VALIDITY_TIME_R13,                                 NULL,   0,           strptr:&si_ValidityTime_r13,                       defstrval:"true",                  TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_FREQ_HOPPING_PARAMETERS_DL_R13,                       NULL,   0,           strptr:&freqHoppingParametersDL_r13,               defstrval:"DISABLE",               TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_NB_R13,                          NULL,   0,           strptr:&mpdcch_pdsch_HoppingNB_r13,                defstrval:"nb2",                   TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13,         NULL,   0,           strptr:&interval_DLHoppingConfigCommonModeA_r13,   defstrval:"interval-FDD-r13",      TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13_VAL,     NULL,   0,           iptr:&interval_DLHoppingConfigCommonModeA_r13_val, defintval:0,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13,         NULL,   0,           strptr:&interval_DLHoppingConfigCommonModeB_r13,   defstrval:"interval-FDD-r13",      TYPE_STRING,       0}, \
{ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13_VAL,     NULL,   0,           iptr:&interval_DLHoppingConfigCommonModeB_r13_val, defintval:0,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_OFFSET_R13,                      NULL,   0,           iptr:&mpdcch_pdsch_HoppingOffset_r13,              defintval:1,                       TYPE_UINT,         0}, \
{ENB_CONFIG_STRING_PREAMBLE_TRANSMAX_CE_R13,                             NULL,   0,           strptr:&preambleTransMax_CE_r13,                   defstrval:"n10",                   TYPE_STRING,       0},  \
{ENB_CONFIG_STRING_PRACH_ROOT,                                           NULL,   0,           iptr:&prach_root_emtc,                                  defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PRACH_CONFIG_INDEX,                                   NULL,   0,           iptr:&prach_config_index_emtc,                          defintval:0,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PRACH_HIGH_SPEED,                                     NULL,   0,           strptr:&prach_high_speed_emtc,                          defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION,                               NULL,   0,           iptr:&prach_zero_correlation_emtc,                      defintval:1,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PRACH_FREQ_OFFSET,                                    NULL,   0,           iptr:&prach_freq_offset_emtc,                           defintval:2,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT,                                    NULL,   0,           iptr:&pucch_delta_shift_emtc,                           defintval:1,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_NRB_CQI,                                        NULL,   0,           iptr:&pucch_nRB_CQI_emtc,                               defintval:1,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_NCS_AN,                                         NULL,   0,           iptr:&pucch_nCS_AN_emtc,                                defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_N1_AN,                                          NULL,   0,           iptr:&pucch_n1_AN_emtc,                                 defintval:32,                      TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PDSCH_RS_EPRE,                                        NULL,   0,           iptr:&pdsch_referenceSignalPower_emtc,                  defintval:-29,                     TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PDSCH_PB,                                             NULL,   0,           iptr:&pdsch_p_b_emtc,                                   defintval:0,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_N_SB,                                           NULL,   0,           iptr:&pusch_n_SB_emtc,                                  defintval:1,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_HOPPINGMODE,                                    NULL,   0,           strptr:&pusch_hoppingMode_emtc,                         defstrval:"interSubFrame",         TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET,                                  NULL,   0,           iptr:&pusch_hoppingOffset_emtc,                         defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUSCH_ENABLE64QAM,                                    NULL,   0,           strptr:&pusch_enable64QAM_emtc,                         defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN,                               NULL,   0,           strptr:&pusch_groupHoppingEnabled_emtc,                 defstrval:"ENABLE",                TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT,                               NULL,   0,           iptr:&pusch_groupAssignment_emtc,                       defintval:0,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN,                            NULL,   0,           strptr:&pusch_sequenceHoppingEnabled_emtc,              defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_NDMRS1,                                         NULL,   0,           iptr:&pusch_nDMRS1_emtc,                                defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PHICH_DURATION,                                       NULL,   0,           strptr:&phich_duration_emtc,                            defstrval:"NORMAL",                TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PHICH_RESOURCE,                                       NULL,   0,           strptr:&phich_resource_emtc,                            defstrval:"ONESIXTH",              TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_ENABLE,                                           NULL,   0,           strptr:&srs_enable_emtc,                                defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG,                                 NULL,   0,           iptr:&srs_BandwidthConfig_emtc,                         defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG,                                  NULL,   0,           iptr:&srs_SubframeConfig_emtc,                          defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG,                                 NULL,   0,           strptr:&srs_ackNackST_emtc,                             defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_MAXUPPTS,                                         NULL,   0,           strptr:&srs_MaxUpPts_emtc,                              defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_PO_NOMINAL,                                     NULL,   0,           iptr:&pusch_p0_Nominal_emtc,                            defintval:-90,                     TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_ALPHA,                                          NULL,   0,           strptr:&pusch_alpha_emtc,                               defstrval:"AL1",                   TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_PO_NOMINAL,                                     NULL,   0,           iptr:&pucch_p0_Nominal_emtc,                            defintval:-96,                     TYPE_INT,        0},  \
{ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE,                                  NULL,   0,           iptr:&msg3_delta_Preamble_emtc,                         defintval:6,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1,                                 NULL,   0,           strptr:&pucch_deltaF_Format1_emtc,                      defstrval:"DELTAF2",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b,                                NULL,   0,           strptr:&pucch_deltaF_Format1b_emtc,                     defstrval:"deltaF3",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2,                                 NULL,   0,           strptr:&pucch_deltaF_Format2_emtc,                      defstrval:"deltaF0",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A,                                NULL,   0,           strptr:&pucch_deltaF_Format2a_emtc,                     defstrval:"deltaF0",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B,                                NULL,   0,           strptr:&pucch_deltaF_Format2b_emtc,                     defstrval:"deltaF0",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES,                                NULL,   0,           iptr:&rach_numberOfRA_Preambles_emtc,                   defstrval:"n4",                    TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG,                           NULL,   0,           strptr:&rach_preamblesGroupAConfig_emtc,                defstrval:"DISABLE",               TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA,                        NULL,   0,           iptr:&rach_sizeOfRA_PreamblesGroupA_emtc,               defintval:0,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA,                               NULL,   0,           iptr:&rach_messageSizeGroupA_emtc,                      defintval:56,                      TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB,                        NULL,   0,           strptr:&rach_messagePowerOffsetGroupB_emtc,             defstrval:"minusinfinity",         TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP,                                NULL,   0,           iptr:&rach_powerRampingStep_emtc,                       defintval:4,                       TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER,              NULL,   0,           iptr:&rach_preambleInitialReceivedTargetPower_emtc,     defintval:-100,                    TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX,                                NULL,   0,           iptr:&rach_preambleTransMax_emtc,                       defintval:10,                      TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE,                            NULL,   0,           iptr:&rach_raResponseWindowSize_emtc,                   defintval:10,                      TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER,                    NULL,   0,           iptr:&rach_macContentionResolutionTimer_emtc,           defintval:48,                      TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX,                                   NULL,   0,           iptr:&rach_maxHARQ_Msg3Tx_emtc,                         defintval:4,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE,                            NULL,   0,           iptr:&pcch_defaultPagingCycle_emtc,                     defintval:128,                 TYPE_INT,     0},  \
{ENB_CONFIG_STRING_PCCH_NB,                                              NULL,   0,           strptr:&pcch_nB_emtc,                                   defstrval:"oneT",                  TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF,                         NULL,   0,           iptr:&bcch_modificationPeriodCoeff_emtc,                defintval:2,                       TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T300,                                NULL,   0,           iptr:&ue_TimersAndConstants_t300_emtc,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T301,                                NULL,   0,           iptr:&ue_TimersAndConstants_t301_emtc,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T310,                                NULL,   0,           iptr:&ue_TimersAndConstants_t310_emtc,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T311,                                NULL,   0,           iptr:&ue_TimersAndConstants_t311_emtc,               defintval:10000,           TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_N310,                                NULL,   0,           iptr:&ue_TimersAndConstants_n310_emtc,               defintval:20,              TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_N311,                                NULL,   0,           iptr:&ue_TimersAndConstants_n311_emtc,               defintval:1,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UE_TRANSMISSION_MODE,                         NULL,   0,           iptr:&ue_TransmissionMode_emtc,                      defintval:1,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL0,                  NULL,   0,           strptr:&pucch_NumRepetitionCE_Msg4_Level0_r13,     defstrval:"n1",                    TYPE_STRING,     0}, \
{ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL1,                  NULL,   0,           strptr:&pucch_NumRepetitionCE_Msg4_Level1_r13,     defstrval:"",                      TYPE_STRING,     0}, \
{ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL2,                  NULL,   0,           strptr:&pucch_NumRepetitionCE_Msg4_Level2_r13,     defstrval:"",                      TYPE_STRING,     0}, \
{ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL3,                  NULL,   0,           strptr:&pucch_NumRepetitionCE_Msg4_Level3_r13,     defstrval:"",                      TYPE_STRING,     0}, \
{ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG,                           NULL,   0,           strptr:&rach_preamblesGroupAConfig_emtc,                defstrval:"",                      TYPE_STRING,     0}, \
}

#define ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_LIST                    "system_info_value_tag_SI"
#define SYSTEM_INFO_VALUE_TAG_SI_DESC {							\
{"systemInfoValueTagSi_r13", NULL,   0,           iptr:&systemInfoValueTagSi_r13,                              defintval:0,              TYPE_UINT,       0} \
  }


#define ENB_CONFIG_STRING_SCHEDULING_INFO_BR                           "scheduling_info_br"
#define SI_INFO_BR_DESC { \
{"si_Narrowband_r13", NULL,   0,           iptr:&si_Narrowband_r13,             defintval:5,             TYPE_UINT,       0}, \
{"si_TBS_r13",        NULL,   0,           iptr:&si_TBS_r13,                    defintval:5,             TYPE_UINT,       0} \
}

#define ENB_CONFIG_STRING_RSRP_RANGE_LIST "rsrp_range_list"
#define RSRP_RANGE_LIST_DESC { \
{"rsrp_range_br", NULL,   0,           iptr:&rsrp_range_br,                     defintval:0,     TYPE_UINT,       0} \
  }


#define ENB_CONFIG_STRING_FIRST_PREAMBLE_R13                      "firstPreamble_r13"
#define ENB_CONFIG_STRING_LAST_PREAMBLE_R13                       "lastPreamble_r13"
#define ENB_CONFIG_STRING_RA_RESPONSE_WINDOW_SIZE_R13             "ra_ResponseWindowSize_r13"
#define ENB_CONFIG_STRING_MAC_CONTENTION_RESOLUTION_TIMER_R13     "mac_ContentionResolutionTimer_r13"
#define ENB_CONFIG_STRING_RAR_HOPPING_CONFIG_R13                  "rar_HoppingConfig_r13  "

#define ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13 "rach_CE_LevelInfoList_r13"
#define RACH_CE_LEVELINFOLIST_R13_DESC { \
{ENB_CONFIG_STRING_FIRST_PREAMBLE_R13,                     NULL,   0,           iptr:&firstPreamble_r13,                 defintval:60,            TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_LAST_PREAMBLE_R13,                      NULL,   0,           iptr:&lastPreamble_r13,                  defintval:63,            TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_RA_RESPONSE_WINDOW_SIZE_R13,            NULL,   0,           iptr:&ra_ResponseWindowSize_r13,         defintval:20,        TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_MAC_CONTENTION_RESOLUTION_TIMER_R13,    NULL,   0,           iptr:&mac_ContentionResolutionTimer_r13, defintval:80,        TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_RAR_HOPPING_CONFIG_R13,                 NULL,   0,           iptr:&rar_HoppingConfig_r13,             defintval:0,         TYPE_UINT,     0}\
}

#define ENB_CONFIG_STRING_PRACH_CONFIG_INDEX_BR                     "prach_config_index_br"
#define ENB_CONFIG_STRING_PRACH_FREQ_OFFSET_BR                      "prach_freq_offset_br"
#define ENB_CONFIG_STRING_PRACH_STARTING_SUBFRAME_R13               "prach_StartingSubframe_r13"
#define ENB_CONFIG_STRING_MAX_NUM_PER_PREAMBLE_ATTEMPT_CE_R13       "maxNumPreambleAttemptCE_r13"
#define ENB_CONFIG_STRING_NUM_REPETITION_PER_PREAMBLE_ATTEMPT_R13   "numRepetitionPerPreambleAttempt_r13"
#define ENB_CONFIG_STRING_MPDCCH_NUM_REPETITION_RA_R13              "mpdcch_NumRepetition_RA_r13"
#define ENB_CONFIG_STRING_PRACH_HOPPING_CONFIG_R13                  "prach_HoppingConfig_r13"
#define ENB_CONFIG_SRING_MAX_AVAILABLE_NARROW_BAND                  "max_available_narrow_band"

#define ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13 "prach_parameters_ce_r13"							
#define PRACH_PARAMS_CE_R13_DESC { \
{ENB_CONFIG_STRING_PRACH_CONFIG_INDEX_BR,                   NULL,   0,           iptr:&prach_config_index_br,                 defintval:3,             TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_PRACH_FREQ_OFFSET_BR,                    NULL,   0,           iptr:&prach_freq_offset_br,                  defintval:1,             TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_PRACH_STARTING_SUBFRAME_R13,             NULL,   0,           iptr:&prach_StartingSubframe_r13,            defintval:0,             TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_MAX_NUM_PER_PREAMBLE_ATTEMPT_CE_R13,     NULL,   0,           iptr:&maxNumPreambleAttemptCE_r13,         defintval:10,         TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_NUM_REPETITION_PER_PREAMBLE_ATTEMPT_R13, NULL,   0,           iptr:&numRepetitionPerPreambleAttempt_r13, defintval:1,          TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_MPDCCH_NUM_REPETITION_RA_R13,            NULL,   0,           iptr:&mpdcch_NumRepetition_RA_r13,         defintval:1,          TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_PRACH_HOPPING_CONFIG_R13,                NULL,   0,           iptr:&prach_HoppingConfig_r13,             defintval:0,         TYPE_UINT,     0}, \
{ENB_CONFIG_SRING_MAX_AVAILABLE_NARROW_BAND,                NULL,   0,           uptr:NULL,                                  defintarrayval:NULL,     TYPE_INTARRAY,   0} \
}

#define ENB_CONFIG_STRING_PUCCH_INFO_VALUE                      "pucch_info_value"

#define ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13 "n1PUCCH_AN_InfoList_r13"
#define N1PUCCH_AN_INFOLIST_R13_DESC { \
{ENB_CONFIG_STRING_PUCCH_INFO_VALUE,                     NULL,   0,           iptr:&pucch_info_value,                    defintval:0,             TYPE_UINT,       0} \
}

#define ENB_CONFIG_STRING_PCCH_CONFIG_V1310 "pcch_config_v1310"
#define PCCH_CONFIG_V1310_DESC { \
{ENB_CONFIG_STRING_PAGING_NARROWBANDS_R13,          NULL,   0,           iptr:&paging_narrowbands_r13,                   defintval:1,             TYPE_UINT,       0}, \
{ENB_CONFIG_STRING_MPDCCH_NUMREPETITION_PAGING_R13, NULL,   0,           iptr:&mpdcch_numrepetition_paging_r13,          defintval:1,          TYPE_UINT,     0}, \
{ENB_CONFIG_STRING_NB_V1310,                        NULL,   0,           iptr:&nb_v1310,                                 defintval:256,   TYPE_UINT,     0} \
}

#define ENB_CONFIG_STRING_SIB2_FREQ_HOPPINGPARAMETERS_R13          "sib2_freq_hoppingParameters_r13" 
#define SIB2_FREQ_HOPPING_R13_DESC { \
{ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_NB_R13,                      NULL,   0,           iptr:&sib2_mpdcch_pdsch_hoppingNB_r13,                  defintval:0,            TYPE_UINT,       0}, \
{"sib2_interval_DLHoppingConfigCommonModeA_r13",                     NULL,   0,           strptr:&sib2_interval_DLHoppingConfigCommonModeA_r13,       defstrval:"FDD",            TYPE_STRING,       0}, \
{"sib2_interval_DLHoppingConfigCommonModeA_r13_val",                 NULL,   0,           iptr:&sib2_interval_DLHoppingConfigCommonModeA_r13_val,   defintval:0,            TYPE_UINT,       0}, \
{"sib2_interval_DLHoppingConfigCommonModeB_r13",                     NULL,   0,           strptr:&sib2_interval_DLHoppingConfigCommonModeB_r13,       defstrval:"FDD",            TYPE_STRING,       0}, \
{"sib2_interval_DLHoppingConfigCommonModeB_r13_val",                 NULL,   0,           iptr:&sib2_interval_DLHoppingConfigCommonModeB_r13_val,   defintval:0,            TYPE_UINT,       0}, \
{"sib2_interval_ULHoppingConfigCommonModeA_r13",     NULL,   0,           strptr:&sib2_interval_ULHoppingConfigCommonModeA_r13,     defstrval:"FDD",         TYPE_STRING,       0}, \
{"sib2_interval_ULHoppingConfigCommonModeA_r13_val", NULL,   0,           iptr:&sib2_interval_ULHoppingConfigCommonModeA_r13_val, defintval:4,        TYPE_UINT,       0}, \
{"sib2_interval_ULHoppingConfigCommonModeB_r13",                     NULL,   0,           strptr:&sib2_interval_ULHoppingConfigCommonModeB_r13,       defstrval:"FDD",            TYPE_STRING,       0}, \
{"sib2_interval_ULHoppingConfigCommonModeB_r13_val",                 NULL,   0,           iptr:&sib2_interval_ULHoppingConfigCommonModeB_r13_val,   defintval:0,            TYPE_UINT,       0}, \
{"sib2_mpdcch_pdsch_hoppingOffset_r13",                              NULL,   0,           iptr:&sib2_mpdcch_pdsch_hoppingOffset_r13,                defintval:1,             TYPE_UINT,         0} \
}
