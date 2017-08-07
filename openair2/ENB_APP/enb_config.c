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
  enb_config.c
  -------------------
  AUTHOR  : Lionel GAUTHIER, navid nikaein, Laurent Winckel
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr, navid.nikaein@eurecom.fr
*/

#include <string.h>
#include <libconfig.h>
#include <inttypes.h>

#include "log.h"
#include "log_extern.h"
#include "assertions.h"
#include "enb_config.h"
#include "UTIL/OTG/otg.h"
#include "UTIL/OTG/otg_externs.h"
#if defined(OAI_EMU)
# include "OCG.h"
# include "OCG_extern.h"
#endif
#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
# endif
#endif
#include "sctp_default_values.h"
#include "SystemInformationBlockType2.h"
#include "LAYER2/MAC/extern.h"
#include "PHY/extern.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"

/* those macros are here to help diagnose problems in configuration files
 * if the lookup fails, a warning is printed
 * (yes we can use the function name for the macro itself, the C preprocessor
 * won't die in an infinite loop)
 */
#define config_setting_lookup_int(setting, name, value)			\
  (config_setting_lookup_int(setting, name, value) ||			\
   (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))
#define config_setting_lookup_int64(setting, name, value)		\
  (config_setting_lookup_int64(setting, name, value) ||			\
   (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))
#define config_setting_lookup_float(setting, name, value)		\
  (config_setting_lookup_float(setting, name, value) ||			\
   (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))
#define config_setting_lookup_bool(setting, name, value)		\
  (config_setting_lookup_bool(setting, name, value) ||			\
   (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))
#define config_setting_lookup_string(setting, name, value)		\
  (config_setting_lookup_string(setting, name, value) ||		\
   (printf("WARNING: setting '%s' not found in configuration file\n", name), 0))

#define ENB_CONFIG_STRING_ACTIVE_ENBS                   "Active_eNBs"

#define ENB_CONFIG_STRING_ENB_LIST                      "eNBs"
#define ENB_CONFIG_STRING_ENB_ID                        "eNB_ID"
#define ENB_CONFIG_STRING_CELL_TYPE                     "cell_type"
#define ENB_CONFIG_STRING_ENB_NAME                      "eNB_name"

#define ENB_CONFIG_STRING_TRACKING_AREA_CODE            "tracking_area_code"
#define ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE           "mobile_country_code"
#define ENB_CONFIG_STRING_MOBILE_NETWORK_CODE           "mobile_network_code"

#define ENB_CONFIG_STRING_LOCAL_S_IF_NAME               "local_s_if_name"
#define ENB_CONFIG_STRING_LOCAL_S_ADDRESS               "local_s_address"
#define ENB_CONFIG_STRING_REMOTE_S_ADDRESS              "remote_s_address"
#define ENB_CONFIG_STRING_LOCAL_S_PORTC                 "local_s_portc"
#define ENB_CONFIG_STRING_REMOTE_S_PORTC                "remote_s_portc"
#define ENB_CONFIG_STRING_LOCAL_S_PORTD                 "local_s_portd"
#define ENB_CONFIG_STRING_REMOTE_S_PORTD                "remote_s_portd"
#define ENB_CONFIG_STRING_TRANSPORT_S_PREFERENCE        "tr_s_preference"


#define ENB_CONFIG_STRING_COMPONENT_CARRIERS                            "component_carriers"

#define ENB_CONFIG_STRING_SCHEDULING_INFO_LIST                          "scheduling_info_br"
#define ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_LIST                    "system_info_value_tag_SI"

#define ENB_CONFIG_STRING_SI_NARROWBAND_R13                             "si_Narrowband_r13"
#define ENB_CONFIG_STRING_SI_TBS_R13                                    "si_TBS_r13"
#define ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_SI_R13                  "systemInfoValueTagSi_r13"

#define ENB_CONFIG_STRING_CC_NODE_FUNCTION                              "node_function"
#define ENB_CONFIG_STRING_CC_NODE_TIMING                                "node_timing"
#define ENB_CONFIG_STRING_CC_NODE_SYNCH_REF                             "node_synch_ref"

#define ENB_CONFIG_STRING_FRAME_TYPE                                    "frame_type"
#define ENB_CONFIG_STRING_TDD_CONFIG                                    "tdd_config"
#define ENB_CONFIG_STRING_TDD_CONFIG_S                                  "tdd_config_s"
#define ENB_CONFIG_STRING_PREFIX_TYPE                                   "prefix_type"
#define ENB_CONFIG_STRING_PBCH_REPETITION                               "pbch_repetition"
#define ENB_CONFIG_STRING_EUTRA_BAND                                    "eutra_band"
#define ENB_CONFIG_STRING_DOWNLINK_FREQUENCY                            "downlink_frequency"
#define ENB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET                       "uplink_frequency_offset"

#define ENB_CONFIG_STRING_NID_CELL                                      "Nid_cell"
#define ENB_CONFIG_STRING_N_RB_DL                                       "N_RB_DL"
#define ENB_CONFIG_STRING_CELL_MBSFN                                  "Nid_cell_mbsfn"
#define ENB_CONFIG_STRING_NB_ANT_PORTS                              "nb_antenna_ports"
#define ENB_CONFIG_STRING_NB_ANT_TX                                 "nb_antennas_tx"
#define ENB_CONFIG_STRING_NB_ANT_RX                                 "nb_antennas_rx"
#define ENB_CONFIG_STRING_TX_GAIN                                       "tx_gain"
#define ENB_CONFIG_STRING_RX_GAIN                                       "rx_gain"
#define ENB_CONFIG_STRING_PRACH_ROOT                                  "prach_root"
#define ENB_CONFIG_STRING_PRACH_CONFIG_INDEX                          "prach_config_index"
#define ENB_CONFIG_STRING_PRACH_HIGH_SPEED                          "prach_high_speed"
#define ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION                  "prach_zero_correlation"
#define ENB_CONFIG_STRING_PRACH_FREQ_OFFSET                         "prach_freq_offset"
#define ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT                         "pucch_delta_shift"
#define ENB_CONFIG_STRING_PUCCH_NRB_CQI                                 "pucch_nRB_CQI"
#define ENB_CONFIG_STRING_PUCCH_NCS_AN                                  "pucch_nCS_AN"
#if !defined(Rel10) && !defined(Rel14)
#define ENB_CONFIG_STRING_PUCCH_N1_AN                                 "pucch_n1_AN"
#endif

#define ENB_CONFIG_STRING_RACH_CE_LEVEL_INFO_LIST    "rach_CE_LevelInfoList_r13"
#define ENB_CONFIG_STRING_RSRP_RANGE_LIST            "rsrp_range_list"
#define ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13    "prach_parameters_ce_r13"
#define ENB_CONFIG_STRING_MAX_AVAILABLE_NARROW_BAND  "max_available_narrow_band"
#define ENB_CONFIG_STRING_N1_PUCCH_AN_INFO_LIST      "n1PUCCH_AN_InfoList_r13"

#define ENB_CONFIG_STRING_FIRST_PREAMBLE                        "firstPreamble_r13"
#define ENB_CONFIG_STRING_LAST_PREAMBLE                         "lastPreamble_r13"
#define ENB_CONFIG_STRING_RA_RESPONSE_WINDOW_SIZE_R13           "ra_ResponseWindowSize_r13"
#define ENB_CONFIG_STRING_MAC_CONTENT_RESOLUTION_TIMER_R13      "mac_ContentionResolutionTimer_r13"
#define ENB_CONFIG_STRING_RAR_HOPPING_CONFIG_R13                "rar_HoppingConfig_r13"
#define ENB_CONFIG_STRING_RSRP_RANGE_BR                         "rsrp_range_br"
#define ENB_CONFIG_STRING_PRACH_CONFIG_INDEX_BR                 "prach_config_index_br"
#define ENB_CONFIG_STRING_PRACH_FREQ_OFFSET_BR                  "prach_freq_offset_br"
#define ENB_CONFIG_STRING_PRACH_STARTING_SUBFRAME_R13           "prach_StartingSubframe_r13"
#define ENB_CONFIG_STRING_MAX_NUM_PREAMBLE_ATTEMPT_CE_R13       "maxNumPreambleAttemptCE_r13"
#define ENB_CONFIG_STRING_NUM_REPETITION_PREAMBLE_ATTEMPT_R13   "numRepetitionPerPreambleAttempt_r13"
#define ENB_CONFIG_STRING_MPDCCH_NUM_REPETITION_RA_R13          "mpdcch_NumRepetition_RA_r13"
#define ENB_CONFIG_STRING_PRACH_HOPPING_CONFIG_R13              "prach_HoppingConfig_r13"
#define ENB_CONFIG_STRING_MAX_AVAILABLE_NARROWBAND              "maxavailablenarrowband"
#define ENB_CONFIG_STRING_PUCCH_INFO_VALUE                      "pucch_info_value"


#define ENB_CONFIG_STRING_PDSCH_RS_EPRE                                 "pdsch_referenceSignalPower"
#define ENB_CONFIG_STRING_PDSCH_PB                                      "pdsch_p_b"
#define ENB_CONFIG_STRING_PUSCH_N_SB                                    "pusch_n_SB"
#define ENB_CONFIG_STRING_PUSCH_HOPPINGMODE                             "pusch_hoppingMode"
#define ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET                           "pusch_hoppingOffset"
#define ENB_CONFIG_STRING_PUSCH_ENABLE64QAM                             "pusch_enable64QAM"
#define ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN                        "pusch_groupHoppingEnabled"
#define ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT                        "pusch_groupAssignment"
#define ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN                     "pusch_sequenceHoppingEnabled"
#define ENB_CONFIG_STRING_PUSCH_NDMRS1                                  "pusch_nDMRS1"
#define ENB_CONFIG_STRING_PHICH_DURATION                                "phich_duration"
#define ENB_CONFIG_STRING_PHICH_RESOURCE                                "phich_resource"
#define ENB_CONFIG_STRING_SRS_ENABLE                                    "srs_enable"
#define ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG                          "srs_BandwidthConfig"
#define ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG                           "srs_SubframeConfig"
#define ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG                          "srs_ackNackST"
#define ENB_CONFIG_STRING_SRS_MAXUPPTS                                  "srs_MaxUpPts"
#define ENB_CONFIG_STRING_PUSCH_PO_NOMINAL                              "pusch_p0_Nominal"
#define ENB_CONFIG_STRING_PUSCH_ALPHA                                   "pusch_alpha"
#define ENB_CONFIG_STRING_PUCCH_PO_NOMINAL                              "pucch_p0_Nominal"
#define ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE                           "msg3_delta_Preamble"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1                          "pucch_deltaF_Format1"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b                         "pucch_deltaF_Format1b"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2                          "pucch_deltaF_Format2"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A                         "pucch_deltaF_Format2a"
#define ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B                         "pucch_deltaF_Format2b"
#define ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES                         "rach_numberOfRA_Preambles"
#define ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG                    "rach_preamblesGroupAConfig"
#define ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA                 "rach_sizeOfRA_PreamblesGroupA"
#define ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA                        "rach_messageSizeGroupA"
#define ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB                 "rach_messagePowerOffsetGroupB"
#define ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP                         "rach_powerRampingStep"
#define ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER       "rach_preambleInitialReceivedTargetPower"
#define ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX                         "rach_preambleTransMax"
#define ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE                     "rach_raResponseWindowSize"
#define ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER             "rach_macContentionResolutionTimer"
#define ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX                            "rach_maxHARQ_Msg3Tx"
#define ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE                     "pcch_default_PagingCycle"
#define ENB_CONFIG_STRING_PCCH_NB                                       "pcch_nB"
#define ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF                  "bcch_modificationPeriodCoeff"
#define ENB_CONFIG_STRING_UETIMERS_T300                                 "ue_TimersAndConstants_t300"
#define ENB_CONFIG_STRING_UETIMERS_T301                                 "ue_TimersAndConstants_t301"
#define ENB_CONFIG_STRING_UETIMERS_T310                                 "ue_TimersAndConstants_t310"
#define ENB_CONFIG_STRING_UETIMERS_T311                                 "ue_TimersAndConstants_t311"
#define ENB_CONFIG_STRING_UETIMERS_N310                                 "ue_TimersAndConstants_n310"
#define ENB_CONFIG_STRING_UETIMERS_N311                                 "ue_TimersAndConstants_n311"
#define ENB_CONFIG_STRING_UE_TRANSMISSION_MODE                          "ue_TransmissionMode"

#define ENB_CONFIG_STRING_SRB1                                          "srb1_parameters"
#define ENB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT                    "timer_poll_retransmit"
#define ENB_CONFIG_STRING_SRB1_TIMER_REORDERING                         "timer_reordering"
#define ENB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT                    "timer_status_prohibit"
#define ENB_CONFIG_STRING_SRB1_POLL_PDU                                 "poll_pdu"
#define ENB_CONFIG_STRING_SRB1_POLL_BYTE                                "poll_byte"
#define ENB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD                       "max_retx_threshold"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS                "mme_ip_address"
#define ENB_CONFIG_STRING_MME_IPV4_ADDRESS              "ipv4"
#define ENB_CONFIG_STRING_MME_IPV6_ADDRESS              "ipv6"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE         "active"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE     "preference"

#define ENB_CONFIG_STRING_SCTP_CONFIG                    "SCTP"
#define ENB_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define ENB_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"

#define ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG     "NETWORK_INTERFACES"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME "ENB_INTERFACE_NAME_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME   "ENB_IPV4_ADDRESS_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U    "ENB_INTERFACE_NAME_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U         "ENB_IPV4_ADDRESS_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_PORT_FOR_S1U              "ENB_PORT_FOR_S1U"

#define ENB_CONFIG_STRING_NETWORK_CONTROLLER_CONFIG     "NETWORK_CONTROLLER"
#define ENB_CONFIG_STRING_FLEXRAN_AGENT_INTERFACE_NAME      "FLEXRAN_AGENT_INTERFACE_NAME"
#define ENB_CONFIG_STRING_FLEXRAN_AGENT_IPV4_ADDRESS        "FLEXRAN_AGENT_IPV4_ADDRESS"
#define ENB_CONFIG_STRING_FLEXRAN_AGENT_PORT                "FLEXRAN_AGENT_PORT"
#define ENB_CONFIG_STRING_FLEXRAN_AGENT_CACHE               "FLEXRAN_AGENT_CACHE"




#define ENB_CONFIG_STRING_ASN1_VERBOSITY                   "Asn1_verbosity"
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE              "none"
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING          "annoying"
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO              "info"

// OTG config per ENB-UE DL
#define ENB_CONF_STRING_OTG_CONFIG                         "otg_config"
#define ENB_CONF_STRING_OTG_UE_ID                          "ue_id"
#define ENB_CONF_STRING_OTG_APP_TYPE                       "app_type"
#define ENB_CONF_STRING_OTG_BG_TRAFFIC                     "bg_traffic"

// per eNB configuration
#define ENB_CONFIG_STRING_LOG_CONFIG                       "log_config"
#define ENB_CONFIG_STRING_GLOBAL_LOG_LEVEL                 "global_log_level"
#define ENB_CONFIG_STRING_GLOBAL_LOG_VERBOSITY             "global_log_verbosity"
#define ENB_CONFIG_STRING_HW_LOG_LEVEL                     "hw_log_level"
#define ENB_CONFIG_STRING_HW_LOG_VERBOSITY                 "hw_log_verbosity"
#define ENB_CONFIG_STRING_PHY_LOG_LEVEL                    "phy_log_level"
#define ENB_CONFIG_STRING_PHY_LOG_VERBOSITY                "phy_log_verbosity"
#define ENB_CONFIG_STRING_MAC_LOG_LEVEL                    "mac_log_level"
#define ENB_CONFIG_STRING_MAC_LOG_VERBOSITY                "mac_log_verbosity"
#define ENB_CONFIG_STRING_RLC_LOG_LEVEL                    "rlc_log_level"
#define ENB_CONFIG_STRING_RLC_LOG_VERBOSITY                "rlc_log_verbosity"
#define ENB_CONFIG_STRING_PDCP_LOG_LEVEL                   "pdcp_log_level"
#define ENB_CONFIG_STRING_PDCP_LOG_VERBOSITY               "pdcp_log_verbosity"
#define ENB_CONFIG_STRING_RRC_LOG_LEVEL                    "rrc_log_level"
#define ENB_CONFIG_STRING_RRC_LOG_VERBOSITY                "rrc_log_verbosity"
#define ENB_CONFIG_STRING_GTPU_LOG_LEVEL                   "gtpu_log_level"
#define ENB_CONFIG_STRING_GTPU_LOG_VERBOSITY               "gtpu_log_verbosity"
#define ENB_CONFIG_STRING_UDP_LOG_LEVEL                    "udp_log_level"
#define ENB_CONFIG_STRING_UDP_LOG_VERBOSITY                "udp_log_verbosity"
#define ENB_CONFIG_STRING_OSA_LOG_LEVEL                    "osa_log_level"
#define ENB_CONFIG_STRING_OSA_LOG_VERBOSITY                "osa_log_verbosity"

#define CONFIG_STRING_MACRLC_LIST                          "MACRLCs"
#define CONFIG_STRING_MACRLC_CONFIG                        "macrlc_config"
#define CONFIG_STRING_MACRLC_CC                            "num_cc"
#define CONFIG_STRING_MACRLC_LOCAL_N_IF_NAME               "local_n_if_name"
#define CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS               "local_n_address"
#define CONFIG_STRING_MACRLC_REMOTE_N_ADDRESS              "remote_n_address"
#define CONFIG_STRING_MACRLC_LOCAL_N_PORTC                 "local_n_portc"
#define CONFIG_STRING_MACRLC_REMOTE_N_PORTC                "remote_n_portc"
#define CONFIG_STRING_MACRLC_LOCAL_N_PORTD                 "local_n_portd"
#define CONFIG_STRING_MACRLC_REMOTE_N_PORTD                "remote_n_portd"
#define CONFIG_STRING_MACRLC_LOCAL_S_IF_NAME               "local_s_if_name"
#define CONFIG_STRING_MACRLC_LOCAL_S_ADDRESS               "local_s_address"
#define CONFIG_STRING_MACRLC_REMOTE_S_ADDRESS              "remote_s_address"
#define CONFIG_STRING_MACRLC_LOCAL_S_PORTC                 "local_s_portc"
#define CONFIG_STRING_MACRLC_REMOTE_S_PORTC                "remote_s_portc"
#define CONFIG_STRING_MACRLC_LOCAL_S_PORTD                 "local_s_portd"
#define CONFIG_STRING_MACRLC_REMOTE_S_PORTD                "remote_s_portd"
#define CONFIG_STRING_MACRLC_TRANSPORT_S_PREFERENCE        "tr_s_preference"
#define CONFIG_STRING_MACRLC_TRANSPORT_N_PREFERENCE        "tr_n_preference"

#define CONFIG_STRING_L1_LIST                              "L1s"
#define CONFIG_STRING_L1_CONFIG                            "l1_config"
#define CONFIG_STRING_L1_CC                                "num_cc"
#define CONFIG_STRING_L1_LOCAL_N_IF_NAME                   "local_n_if_name"
#define CONFIG_STRING_L1_LOCAL_N_ADDRESS                   "local_n_address"
#define CONFIG_STRING_L1_REMOTE_N_ADDRESS                  "remote_n_address"
#define CONFIG_STRING_L1_LOCAL_N_PORTC                     "local_n_portc"
#define CONFIG_STRING_L1_REMOTE_N_PORTC                    "remote_n_portc"
#define CONFIG_STRING_L1_LOCAL_N_PORTD                     "local_n_portd"
#define CONFIG_STRING_L1_REMOTE_N_PORTD                    "remote_n_portd"
#define CONFIG_STRING_L1_TRANSPORT_N_PREFERENCE            "tr_n_preference"

#define CONFIG_STRING_ACTIVE_RUS                  "Active_RUs"
#define CONFIG_STRING_RU_LIST                     "RUs"
#define CONFIG_STRING_RU_CONFIG                   "ru_config"
#define CONFIG_STRING_RU_LOCAL_IF_NAME            "local_if_name"
#define CONFIG_STRING_RU_LOCAL_ADDRESS            "local_address"
#define CONFIG_STRING_RU_REMOTE_ADDRESS           "remote_address"
#define CONFIG_STRING_RU_LOCAL_PORTC              "local_portc"
#define CONFIG_STRING_RU_REMOTE_PORTC             "remote_portc"
#define CONFIG_STRING_RU_LOCAL_PORTD              "local_portd"
#define CONFIG_STRING_RU_REMOTE_PORTD             "remote_portd"
#define CONFIG_STRING_RU_LOCAL_RF                 "local_rf"
#define CONFIG_STRING_RU_TRANSPORT_PREFERENCE     "tr_preference"
#define CONFIG_STRING_RU_BAND_LIST                "bands"
#define CONFIG_STRING_RU_ENB_LIST                 "eNB_instances"
#define CONFIG_STRING_RU_NB_TX                    "nb_tx"
#define CONFIG_STRING_RU_NB_RX                    "nb_rx"
#define CONFIG_STRING_RU_ATT_TX                   "att_tx"
#define CONFIG_STRING_RU_ATT_RX                   "att_rx"
#define CONFIG_STRING_RU_MAX_RS_EPRE              "max_pdschReferenceSignalPower"
#define CONFIG_STRING_RU_MAX_RXGAIN               "max_rxgain"
#define CONFIG_STRING_RU_IF_COMPRESSION           "if_compression"

#define ENB_CONFIG_STRING_BR			  "br_parameters"
#define ENB_CONFIG_STRING_schedulingInfoSIB1	  "schedulingInfoSIB1_BR_r13"

#define ENB_CONFIG_STRING_HYPERSFN		  "hyperSFN_r13"
#define ENB_CONFIG_STRING_EDRX_ALLOWED		  "eDRX_Allowed_r13"
#define ENB_CONFIG_STRING_CELLSELECTIONINFOCE	  "cellSelectionInfoCE_r13"
#define ENB_CONFIG_STRING_Q_RXLEVMINCE		  "q_RxLevMinCE_r13"
#define ENB_CONFIG_STRING_Q_QUALMINRSRQ_CE	  "q_QualMinRSRQ_CE_r13"
#define ENB_CONFIG_STRING_BRAccessRelatedInfo	  "bandwidthReducedAccessRelatedInfo_r13"

#define ENB_CONFIG_STRING_SI_WINDOWLENGTH_BR	  "si_WindowLength_BR_r13"
#define ENB_CONFIG_STRING_SI_REPETITIONPATTERN	  "si_RepetitionPattern_r13"
#define ENB_CONFIG_STRING_FDD_DLORTDD_SFB_BR	  "fdd_DownlinkOrTddSubframeBitmapBR_r13"
#define ENB_CONFIG_STRING_FDD_DLORTDDSFB_BR_VAL	  "fdd_DownlinkOrTddSubframeBitmapBR_val_r13"
#define ENB_CONFIG_STRING_FDD_ULSUBFRAMEBITMAPBR  "fdd_UplinkSubframeBitmapBR_r13"
#define ENB_CONFIG_STRING_STARTSYMBOLBR		  "startSymbolBR_r13"
#define ENB_CONFIG_STRING_SI_HOPPINGCONFIGCOMMON  "si_HoppingConfigCommon_r13"
#define ENB_CONFIG_STRING_SI_VALIDITYTIME	  "si_ValidityTime_r13"
#define ENB_CONFIG_STRING_FREQHOPPINGPARAMETERSDL "freqHoppingParametersDL_r13"
#define ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPINGNB  "mpdcch_pdsch_HoppingNB_r13"
#define ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEA "interval_DLHoppingConfigCommonModeA_r13"
#define ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEA_VAL "interval_DLHoppingConfigCommonModeA_r13_val"
#define ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB "interval_DLHoppingConfigCommonModeB_r13"
#define ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB_VAL "interval_DLHoppingConfigCommonModeB_r13_val"
#define ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPINGOFFSET			  "mpdcch_pdsch_HoppingOffset_r13"


#define ENB_CONFIG_STRING_PREAMBLETRANSMAX_CE_R13				"preambleTransMax_CE_r13"
#define ENB_CONFIG_STRING_PRACH_CONFIGCOMMON_V1310				"prach_ConfigCommon_v1310"
#define ENB_CONFIG_STRING_MPDCCH_STARTSF_CSS_RA_R13				"mpdcch_startSF_CSS_RA_r13"
#define ENB_CONFIG_STRING_MPDCCH_STARTSF_CSS_RA_R13_VAL			"mpdcch_startSF_CSS_RA_r13_val"
#define ENB_CONFIG_STRING_PRACH_HOPPINGOFFSET_R13			    "prach_HoppingOffset_r13"
#define KHz (1000UL)
#define MHz (1000 * KHz)

typedef struct eutra_band_s {
  int16_t             band;
  uint32_t            ul_min;
  uint32_t            ul_max;
  uint32_t            dl_min;
  uint32_t            dl_max;
  lte_frame_type_t    frame_type;
} eutra_band_t;

static const eutra_band_t eutra_bands[] = {
  { 1, 1920    * MHz, 1980    * MHz, 2110    * MHz, 2170    * MHz, FDD},
  { 2, 1850    * MHz, 1910    * MHz, 1930    * MHz, 1990    * MHz, FDD},
  { 3, 1710    * MHz, 1785    * MHz, 1805    * MHz, 1880    * MHz, FDD},
  { 4, 1710    * MHz, 1755    * MHz, 2110    * MHz, 2155    * MHz, FDD},
  { 5,  824    * MHz,  849    * MHz,  869    * MHz,  894    * MHz, FDD},
  { 6,  830    * MHz,  840    * MHz,  875    * MHz,  885    * MHz, FDD},
  { 7, 2500    * MHz, 2570    * MHz, 2620    * MHz, 2690    * MHz, FDD},
  { 8,  880    * MHz,  915    * MHz,  925    * MHz,  960    * MHz, FDD},
  { 9, 1749900 * KHz, 1784900 * KHz, 1844900 * KHz, 1879900 * KHz, FDD},
  {10, 1710    * MHz, 1770    * MHz, 2110    * MHz, 2170    * MHz, FDD},
  {11, 1427900 * KHz, 1452900 * KHz, 1475900 * KHz, 1500900 * KHz, FDD},
  {12,  698    * MHz,  716    * MHz,  728    * MHz,  746    * MHz, FDD},
  {13,  777    * MHz,  787    * MHz,  746    * MHz,  756    * MHz, FDD},
  {14,  788    * MHz,  798    * MHz,  758    * MHz,  768    * MHz, FDD},

  {17,  704    * MHz,  716    * MHz,  734    * MHz,  746    * MHz, FDD},
  {20,  832    * MHz,  862    * MHz,  791    * MHz,  821    * MHz, FDD},
  {33, 1900    * MHz, 1920    * MHz, 1900    * MHz, 1920    * MHz, TDD},
  {33, 1900    * MHz, 1920    * MHz, 1900    * MHz, 1920    * MHz, TDD},
  {34, 2010    * MHz, 2025    * MHz, 2010    * MHz, 2025    * MHz, TDD},
  {35, 1850    * MHz, 1910    * MHz, 1850    * MHz, 1910    * MHz, TDD},
  {36, 1930    * MHz, 1990    * MHz, 1930    * MHz, 1990    * MHz, TDD},
  {37, 1910    * MHz, 1930    * MHz, 1910    * MHz, 1930    * MHz, TDD},
  {38, 2570    * MHz, 2620    * MHz, 2570    * MHz, 2630    * MHz, TDD},
  {39, 1880    * MHz, 1920    * MHz, 1880    * MHz, 1920    * MHz, TDD},
  {40, 2300    * MHz, 2400    * MHz, 2300    * MHz, 2400    * MHz, TDD},
  {41, 2496    * MHz, 2690    * MHz, 2496    * MHz, 2690    * MHz, TDD},
  {42, 3400    * MHz, 3600    * MHz, 3400    * MHz, 3600    * MHz, TDD},
  {43, 3600    * MHz, 3800    * MHz, 3600    * MHz, 3800    * MHz, TDD},
  {44, 703    * MHz, 803    * MHz, 703    * MHz, 803    * MHz, TDD},
};




static int enb_check_band_frequencies(char* lib_config_file_name_pP,
                                      int ind,
                                      int16_t band,
                                      uint32_t downlink_frequency,
                                      int32_t uplink_frequency_offset,
                                      lte_frame_type_t frame_type)
{
  int errors = 0;

  if (band > 0) {
    int band_index;

    for (band_index = 0; band_index < sizeof (eutra_bands) / sizeof (eutra_bands[0]); band_index++) {
      if (band == eutra_bands[band_index].band) {
        uint32_t uplink_frequency = downlink_frequency + uplink_frequency_offset;

        AssertError (eutra_bands[band_index].dl_min < downlink_frequency, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d downlink frequency %u too low (%u) for band %d!",
                     lib_config_file_name_pP, ind, downlink_frequency, eutra_bands[band_index].dl_min, band);
        AssertError (downlink_frequency < eutra_bands[band_index].dl_max, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d downlink frequency %u too high (%u) for band %d!",
                     lib_config_file_name_pP, ind, downlink_frequency, eutra_bands[band_index].dl_max, band);

        AssertError (eutra_bands[band_index].ul_min < uplink_frequency, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d uplink frequency %u too low (%u) for band %d!",
                     lib_config_file_name_pP, ind, uplink_frequency, eutra_bands[band_index].ul_min, band);
        AssertError (uplink_frequency < eutra_bands[band_index].ul_max, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d uplink frequency %u too high (%u) for band %d!",
                     lib_config_file_name_pP, ind, uplink_frequency, eutra_bands[band_index].ul_max, band);

        AssertError (eutra_bands[band_index].frame_type == frame_type, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d invalid frame type (%d/%d) for band %d!",
                     lib_config_file_name_pP, ind, eutra_bands[band_index].frame_type, frame_type, band);
      }
    }
  }

  return errors;
}

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
extern int asn_debug;
extern int asn1_xer_print;
#endif

#ifdef LIBCONFIG_LONG
#define libconfig_int long
#else
#define libconfig_int int
#endif

typedef enum {
  RU     = 0,
  L1     = 1,
  L2     = 2,
  L3     = 3,
  S1     = 4,
  lastel = 5
} RC_config_functions_t;

void RCconfig_RU(void);
void RCconfig_L1(void);
void RCconfig_macrlc(void);
int  RCconfig_RRC(MessageDef *msg_p, uint32_t i, eNB_RRC_INST *rrc);
int  RCconfig_S1(MessageDef *msg_p, uint32_t i);


int load_config_file(config_t *cfg) {

  config_init(cfg);

  if (RC.config_file_name != NULL) {
    // Read the file. If there is an error, report it and exit.
    if (! config_read_file(cfg, RC.config_file_name)) {
      config_destroy(cfg);
      AssertFatal (0, "Failed to parse eNB configuration file %s!\n", RC.config_file_name);
    }
  } else {
    config_destroy(cfg);
    AssertFatal (0, "No eNB configuration file provided!\n");
  }

}
void RCconfig_RU() {

  config_t          cfg;
  config_setting_t *setting                       = NULL;
  config_setting_t *setting_ru                    = NULL;
  config_setting_t *setting_band                  = NULL;
  config_setting_t *setting_band_elem             = NULL;
  config_setting_t *setting_eNB_list              = NULL;
  config_setting_t *setting_eNB_list_elem         = NULL;
  char*             if_name                       = NULL;
  char*             ipv4                          = NULL;
  char*             ipv4_remote                   = NULL;
  char              *local_rf                     = NULL;

  char*             tr_preference                 = NULL;
  libconfig_int     local_portc                   = 0;
  libconfig_int     remote_portc                  = 0;
  libconfig_int     local_portd                   = 0;
  libconfig_int     remote_portd                  = 0;

  libconfig_int     nb_tx                         = 0;
  libconfig_int     nb_rx                         = 0;
  libconfig_int     att_tx                        = 0;
  libconfig_int     att_rx                        = 0;
  libconfig_int     max_pdschReferenceSignalPower = 0;
  libconfig_int     max_rxgain                    = 0;
  int               j                             = 0;
  int               i                             = 0;
  int               num_bands                     = 0;
  libconfig_int     band[256];
  int               num_eNB4RU                    = 0;
  libconfig_int     eNB_list[256];
  int               fronthaul_flag                = CONFIG_TRUE;

  load_config_file(&cfg);

  // Output a list of all RUs.
  setting = config_lookup(&cfg, CONFIG_STRING_RU_LIST);

  if (setting != NULL) {


    RC.ru = (RU_t**)malloc(RC.nb_RU*sizeof(RU_t*));

    RC.ru_mask=(1<<NB_RU) - 1;


    for (j = 0; j < RC.nb_RU; j++) {

      setting_ru = config_setting_get_elem(setting, j);
      printf("rru %d/%d\n",j,RC.nb_RU);


      if (  !(
          config_setting_lookup_string(setting_ru, CONFIG_STRING_RU_LOCAL_IF_NAME,(const char **)&if_name)
          )
        ) {
    fronthaul_flag = CONFIG_FALSE;
      }

      if (fronthaul_flag != CONFIG_TRUE) { // no fronthaul

    AssertFatal((setting_band = config_setting_get_member(setting_ru, CONFIG_STRING_RU_BAND_LIST))!=NULL,"No allowable LTE bands\n");

    if (setting_band != NULL) num_bands    = config_setting_length(setting_band);
    else num_bands=0;

    for (i=0;i<num_bands;i++) {
      setting_band_elem = config_setting_get_elem(setting_band,i);
      band[i] = config_setting_get_int(setting_band_elem);
      printf("RU %d: band %d\n",j,band[i]);
    }
      } // fronthaul_flag == CONFIG_FALSE
      else { // fronthaul_flag == CONFIG_TRUE
    if (  !(
        config_setting_lookup_string(setting_ru, CONFIG_STRING_RU_LOCAL_ADDRESS,        (const char **)&ipv4)
        && config_setting_lookup_string(setting_ru, CONFIG_STRING_RU_REMOTE_ADDRESS,       (const char **)&ipv4_remote)
        && config_setting_lookup_int   (setting_ru, CONFIG_STRING_RU_LOCAL_PORTC,          &local_portc)
        && config_setting_lookup_int   (setting_ru, CONFIG_STRING_RU_REMOTE_PORTC,         &remote_portc)
        && config_setting_lookup_int   (setting_ru, CONFIG_STRING_RU_LOCAL_PORTD,          &local_portd)
        && config_setting_lookup_int   (setting_ru, CONFIG_STRING_RU_REMOTE_PORTD,         &remote_portd)
        && config_setting_lookup_string(setting_ru, CONFIG_STRING_RU_TRANSPORT_PREFERENCE, (const char **)&tr_preference)
        )
          ) {
      AssertFatal (0,
               "Failed to parse configuration file %s, RU %d config !\n",
               RC.config_file_name, j);
      continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
    }
    AssertFatal((setting_eNB_list = config_setting_get_member(setting_ru, CONFIG_STRING_RU_ENB_LIST))!=NULL,"No RU<->eNB mappings\n");

    if (setting_eNB_list != NULL) num_eNB4RU    = config_setting_length(setting_eNB_list);
    else num_eNB4RU=0;
    AssertFatal(num_eNB4RU>0,"Number of eNBs is zero\n");

    for (i=0;i<num_eNB4RU;i++) {
      setting_eNB_list_elem = config_setting_get_elem(setting_eNB_list,i);
      eNB_list[i] = config_setting_get_int(setting_eNB_list_elem);
      printf("RU %d: eNB %d\n",j,eNB_list[i]);
    }
      }

      if ( !(
         config_setting_lookup_int(setting_ru, CONFIG_STRING_RU_NB_TX,  &nb_tx)
         && config_setting_lookup_int(setting_ru, CONFIG_STRING_RU_NB_RX,  &nb_rx)
         && config_setting_lookup_int(setting_ru, CONFIG_STRING_RU_ATT_TX, &att_tx)
         && config_setting_lookup_int(setting_ru, CONFIG_STRING_RU_ATT_RX, &att_rx)
         && config_setting_lookup_string(setting_ru, CONFIG_STRING_RU_LOCAL_RF,(const char **)&local_rf)
         )) {
    AssertFatal (0,
             "Failed to parse configuration file %s, RU %d config !\n",
             RC.config_file_name, j);
    continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
      }

      RC.ru[j]                                    = (RU_t*)malloc(sizeof(RU_t));
      memset((void*)RC.ru[j],0,sizeof(RU_t));

      RC.ru[j]->idx                                 = j;

      RC.ru[j]->if_timing                           = synch_to_ext_device;
      RC.ru[j]->num_eNB                             = num_eNB4RU;

      if (strcmp(local_rf, "yes") == 0) {
    if (fronthaul_flag == CONFIG_FALSE) {
      RC.ru[j]->if_south                        = LOCAL_RF;
      RC.ru[j]->function                        = eNodeB_3GPP;
        }
        else {
      if (strcmp(tr_preference, "udp") == 0) {
        RC.ru[j]->if_south                        = LOCAL_RF;
        RC.ru[j]->function                        = NGFI_RRU_IF5;
        RC.ru[j]->eth_params.transp_preference    = ETH_UDP_MODE;
      } else if (strcmp(tr_preference, "raw") == 0) {
        RC.ru[j]->if_south                        = LOCAL_RF;
        RC.ru[j]->function                        = NGFI_RRU_IF5;
        RC.ru[j]->eth_params.transp_preference    = ETH_RAW_MODE;
      } else if (strcmp(tr_preference, "udp_if4p5") == 0) {
        RC.ru[j]->if_south                        = LOCAL_RF;
        RC.ru[j]->function                        = NGFI_RRU_IF4p5;
        RC.ru[j]->eth_params.transp_preference    = ETH_UDP_IF4p5_MODE;
      } else if (strcmp(tr_preference, "raw_if4p5") == 0) {
        RC.ru[j]->if_south                        = LOCAL_RF;
        RC.ru[j]->function                        = NGFI_RRU_IF4p5;
        RC.ru[j]->eth_params.transp_preference    = ETH_RAW_IF4p5_MODE;
      }
    }

    RC.ru[j]->max_pdschReferenceSignalPower     = max_pdschReferenceSignalPower;
    RC.ru[j]->max_rxgain                        = max_rxgain;
    RC.ru[j]->num_bands                         = num_bands;
    for (i=0;i<num_bands;i++) RC.ru[j]->band[i] = band[i];
      }
      else {
    printf("RU %d: Transport %s\n",j,tr_preference);

    RC.ru[j]->eth_params.local_if_name            = strdup(if_name);
    RC.ru[j]->eth_params.my_addr                  = strdup(ipv4);
    RC.ru[j]->eth_params.remote_addr              = strdup(ipv4_remote);
    RC.ru[j]->eth_params.my_portc                 = local_portc;
    RC.ru[j]->eth_params.remote_portc             = remote_portc;
    RC.ru[j]->eth_params.my_portd                 = local_portd;
    RC.ru[j]->eth_params.remote_portd             = remote_portd;
    for (i=0;i<num_eNB4RU;i++) RC.ru[j]->eNB_list[i] = RC.eNB[eNB_list[i]][0];

    if (strcmp(tr_preference, "udp") == 0) {
      RC.ru[j]->if_south                     = REMOTE_IF5;
      RC.ru[j]->function                     = NGFI_RAU_IF5;
      RC.ru[j]->eth_params.transp_preference = ETH_UDP_MODE;
    } else if (strcmp(tr_preference, "raw") == 0) {
      RC.ru[j]->if_south                     = REMOTE_IF5;
      RC.ru[j]->function                     = NGFI_RAU_IF5;
      RC.ru[j]->eth_params.transp_preference = ETH_RAW_MODE;
    } else if (strcmp(tr_preference, "udp_if4p5") == 0) {
      RC.ru[j]->if_south                     = REMOTE_IF4p5;
      RC.ru[j]->function                     = NGFI_RAU_IF4p5;
      RC.ru[j]->eth_params.transp_preference = ETH_UDP_IF4p5_MODE;
    } else if (strcmp(tr_preference, "raw_if4p5") == 0) {
      RC.ru[j]->if_south                     = REMOTE_IF4p5;
      RC.ru[j]->function                     = NGFI_RAU_IF4p5;
      RC.ru[j]->eth_params.transp_preference = ETH_RAW_IF4p5_MODE;
    } else if (strcmp(tr_preference, "raw_if5_mobipass") == 0) {
      RC.ru[j]->if_south                     = REMOTE_IF5;
      RC.ru[j]->function                     = NGFI_RAU_IF5;
      RC.ru[j]->if_timing                    = synch_to_other;
      RC.ru[j]->eth_params.transp_preference = ETH_RAW_IF5_MOBIPASS;
    }
    RC.ru[j]->att_tx                         = att_tx;
    RC.ru[j]->att_rx                         = att_rx;
      }

      RC.ru[j]->nb_tx                             = nb_tx;
      RC.ru[j]->nb_rx                             = nb_rx;

    }// j=0..num_rus
  } else {
    RC.nb_RU = 0;
  } // setting != NULL

  return;

}

void RCconfig_L1() {

  int               i,j;

  config_t          cfg;
  config_setting_t *setting                         = NULL;
  config_setting_t *setting_l1                      = NULL;

  char*             if_name_n                       = NULL;
  char*             ipv4_n                          = NULL;
  char*             ipv4_n_remote                   = NULL;

  char*             tr_n_preference                 = NULL;
  libconfig_int     local_n_portc                   = 0;
  libconfig_int     remote_n_portc                  = 0;
  libconfig_int     local_n_portd                   = 0;
  libconfig_int     remote_n_portd                  = 0;

  load_config_file(&cfg);

  setting = config_lookup(&cfg, CONFIG_STRING_L1_LIST);

  if (setting != NULL) {

    if (RC.eNB == NULL) {
      RC.eNB                               = (PHY_VARS_eNB ***)malloc((1+NUMBER_OF_eNB_MAX)*sizeof(PHY_VARS_eNB***));
      LOG_I(PHY,"RC.eNB = %p\n",RC.eNB);
      memset(RC.eNB,0,(1+NUMBER_OF_eNB_MAX)*sizeof(PHY_VARS_eNB***));
      RC.nb_L1_CC = malloc((1+RC.nb_L1_inst)*sizeof(int));
    }

    for (j = 0; j < RC.nb_L1_inst; j++) {

      setting_l1 = config_setting_get_elem(setting, j);
      if (!config_setting_lookup_int   (setting_l1,CONFIG_STRING_L1_CC,&RC.nb_L1_CC[j]))
    AssertFatal (0,
             "Failed to parse configuration file %s, L1 %d config !\n",
             RC.config_file_name, j);

      if (RC.eNB[j] == NULL) {
    RC.eNB[j]                       = (PHY_VARS_eNB **)malloc((1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB**));
    LOG_I(PHY,"RC.eNB[%d] = %p\n",j,RC.eNB[j]);
    memset(RC.eNB[j],0,(1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB***));
      }


      for (i=0;i<RC.nb_L1_CC[j];i++) {
    if (RC.eNB[j][i] == NULL) {
      RC.eNB[j][i] = (PHY_VARS_eNB *)malloc(sizeof(PHY_VARS_eNB));
      memset((void*)RC.eNB[j][i],0,sizeof(PHY_VARS_eNB));
      LOG_I(PHY,"RC.eNB[%d][%d] = %p\n",j,i,RC.eNB[j][i]);
      RC.eNB[j][i]->Mod_id  = j;
      RC.eNB[j][i]->CC_id   = i;
    }
      }



      printf("l1 %d/%d (nb CC %d)\n",j,RC.nb_inst,RC.nb_CC[j]);


      printf("RU %d: Transport %s\n",j,tr_n_preference);
      if (!(config_setting_lookup_string(setting_l1, CONFIG_STRING_L1_TRANSPORT_N_PREFERENCE, (const char **)&tr_n_preference))) {
    AssertFatal (0,
             "Failed to parse configuration file %s, L1 %d config !\n",
             RC.config_file_name, j);
      }

      if (strcmp(tr_n_preference, "local_mac") == 0) {

      }
      else if (strcmp(tr_n_preference, "nfapi") == 0) {
    if (  !(
        config_setting_lookup_string(setting_l1, CONFIG_STRING_L1_LOCAL_N_IF_NAME,        (const char **)&if_name_n)
        && config_setting_lookup_string(setting_l1, CONFIG_STRING_L1_LOCAL_N_ADDRESS,        (const char **)&ipv4_n)
        && config_setting_lookup_string(setting_l1, CONFIG_STRING_L1_REMOTE_N_ADDRESS,       (const char **)&ipv4_n_remote)
        && config_setting_lookup_int   (setting_l1, CONFIG_STRING_L1_LOCAL_N_PORTC,          &local_n_portc)
        && config_setting_lookup_int   (setting_l1, CONFIG_STRING_L1_REMOTE_N_PORTC,         &remote_n_portc)
        && config_setting_lookup_int   (setting_l1, CONFIG_STRING_L1_LOCAL_N_PORTD,          &local_n_portd)
        && config_setting_lookup_int   (setting_l1, CONFIG_STRING_L1_REMOTE_N_PORTD,         &remote_n_portd)
        )
          ) {
      AssertFatal (0,
               "Failed to parse configuration file %s, L1 %d config !\n",
               RC.config_file_name, j);
      continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
    }
    RC.eNB[j][0]->eth_params_n.local_if_name            = strdup(if_name_n);
    RC.eNB[j][0]->eth_params_n.my_addr                  = strdup(ipv4_n);
    RC.eNB[j][0]->eth_params_n.remote_addr              = strdup(ipv4_n_remote);
    RC.eNB[j][0]->eth_params_n.my_portc                 = local_n_portc;
    RC.eNB[j][0]->eth_params_n.remote_portc             = remote_n_portc;
    RC.eNB[j][0]->eth_params_n.my_portd                 = local_n_portd;
    RC.eNB[j][0]->eth_params_n.remote_portd             = remote_n_portd;
    RC.eNB[j][0]->eth_params_n.transp_preference          = ETH_UDP_MODE;
      }

      else { // other midhaul
      }
    }// j=0..num_inst
    printf("Initializing northbound interface for L1\n");
    l1_north_init_eNB();
  }
}

void RCconfig_macrlc() {

  int               i,j;

  config_t          cfg;
  config_setting_t *setting                         = NULL;
  config_setting_t *setting_macrlc                  = NULL;
  char*             if_name_s                       = NULL;
  char*             ipv4_s                          = NULL;
  char*             ipv4_s_remote                   = NULL;

  char*             tr_s_preference                 = NULL;
  libconfig_int     local_s_portc                   = 0;
  libconfig_int     remote_s_portc                  = 0;
  libconfig_int     local_s_portd                   = 0;
  libconfig_int     remote_s_portd                  = 0;
  char*             if_name_n                       = NULL;
  char*             ipv4_n                          = NULL;
  char*             ipv4_n_remote                   = NULL;

  char*             tr_n_preference                 = NULL;
  libconfig_int     local_n_portc                   = 0;
  libconfig_int     remote_n_portc                  = 0;
  libconfig_int     local_n_portd                   = 0;
  libconfig_int     remote_n_portd                  = 0;

  load_config_file(&cfg);

  setting = config_lookup(&cfg, CONFIG_STRING_MACRLC_LIST);

  if (setting != NULL) {



    if ((RC.nb_macrlc_inst=config_setting_length(setting))>0) mac_top_init_eNB();
    else AssertFatal(1==0,"improper macrlc setting\n");

    for (j=0;j<RC.nb_macrlc_inst;j++) {
      setting_macrlc = config_setting_get_elem(setting, j);

      printf("macrlc %d/%d \n",j,RC.nb_macrlc_inst);

      if (!(config_setting_lookup_string(setting_macrlc, CONFIG_STRING_MACRLC_TRANSPORT_N_PREFERENCE, (const char **)&tr_n_preference))) {
    AssertFatal (0,
             "Failed to parse configuration file %s, L1 %d config !\n",
             RC.config_file_name, j);
      }

      printf("MACRLC %d: Northbound Transport %s\n",j,tr_n_preference);

      if (strcmp(tr_n_preference, "local_RRC") == 0) {
    // check number of instances is same as RRC/PDCP

      }
      else if (strcmp(tr_n_preference, "cudu") == 0) {
    if (  !(
        config_setting_lookup_string(setting_macrlc, CONFIG_STRING_MACRLC_LOCAL_N_IF_NAME,        (const char **)&if_name_n)
        && config_setting_lookup_string(setting_macrlc, CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS,        (const char **)&ipv4_n)
        && config_setting_lookup_string(setting_macrlc, CONFIG_STRING_MACRLC_REMOTE_N_ADDRESS,       (const char **)&ipv4_n_remote)
        && config_setting_lookup_int   (setting_macrlc, CONFIG_STRING_MACRLC_LOCAL_N_PORTC,          &local_n_portc)
        && config_setting_lookup_int   (setting_macrlc, CONFIG_STRING_MACRLC_REMOTE_N_PORTC,         &remote_n_portc)
        && config_setting_lookup_int   (setting_macrlc, CONFIG_STRING_MACRLC_LOCAL_N_PORTD,          &local_n_portd)
        && config_setting_lookup_int   (setting_macrlc, CONFIG_STRING_MACRLC_REMOTE_N_PORTD,         &remote_n_portd)
        )
          ) {
      AssertFatal (0,
               "Failed to parse configuration file %s, L1 %d config !\n",
               RC.config_file_name, j);
      continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
    }
    RC.mac[j]->eth_params_n.local_if_name            = strdup(if_name_n);
    RC.mac[j]->eth_params_n.my_addr                  = strdup(ipv4_n);
    RC.mac[j]->eth_params_n.remote_addr              = strdup(ipv4_n_remote);
    RC.mac[j]->eth_params_n.my_portc                 = local_n_portc;
    RC.mac[j]->eth_params_n.remote_portc             = remote_n_portc;
    RC.mac[j]->eth_params_n.my_portd                 = local_n_portd;
    RC.mac[j]->eth_params_n.remote_portd             = remote_n_portd;
    RC.mac[j]->eth_params_n.transp_preference        = ETH_UDP_MODE;
      }

      else { // other midhaul
    AssertFatal(1==0,"MACRLC %d: unknown northbound midhaul\n",j);
      }

      if (!(config_setting_lookup_string(setting_macrlc, CONFIG_STRING_MACRLC_TRANSPORT_S_PREFERENCE, (const char **)&tr_s_preference))) {
    AssertFatal (0,
             "Failed to parse configuration file %s, L1 %d config !\n",
             RC.config_file_name, j);
    continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
      }

      printf("MACRLC %d: Southbound Transport %s\n",j,tr_s_preference);

      if (strcmp(tr_s_preference, "local_L1") == 0) {


      }
      else if (strcmp(tr_s_preference, "nfapi") == 0) {
    if (  !(
        config_setting_lookup_string(setting_macrlc, CONFIG_STRING_MACRLC_LOCAL_S_IF_NAME,        (const char **)&if_name_s)
        && config_setting_lookup_string(setting_macrlc, CONFIG_STRING_MACRLC_LOCAL_S_ADDRESS,        (const char **)&ipv4_s)
        && config_setting_lookup_string(setting_macrlc, CONFIG_STRING_MACRLC_REMOTE_S_ADDRESS,       (const char **)&ipv4_s_remote)
        && config_setting_lookup_int   (setting_macrlc, CONFIG_STRING_MACRLC_LOCAL_S_PORTC,          &local_s_portc)
        && config_setting_lookup_int   (setting_macrlc, CONFIG_STRING_MACRLC_REMOTE_S_PORTC,         &remote_s_portc)
        && config_setting_lookup_int   (setting_macrlc, CONFIG_STRING_MACRLC_LOCAL_S_PORTD,          &local_s_portd)
        && config_setting_lookup_int   (setting_macrlc, CONFIG_STRING_MACRLC_REMOTE_S_PORTD,         &remote_s_portd)
         )){
      AssertFatal (0,
               "Failed to parse configuration file %s, L1 %d config !\n",
               RC.config_file_name, j);
      continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
    }

    RC.mac[j]->eth_params_s.local_if_name            = strdup(if_name_s);
    RC.mac[j]->eth_params_s.my_addr                  = strdup(ipv4_s);
    RC.mac[j]->eth_params_s.remote_addr              = strdup(ipv4_s_remote);
    RC.mac[j]->eth_params_s.my_portc                 = local_s_portc;
    RC.mac[j]->eth_params_s.remote_portc             = remote_s_portc;
    RC.mac[j]->eth_params_s.my_portd                 = local_s_portd;
    RC.mac[j]->eth_params_s.remote_portd             = remote_s_portd;
    RC.mac[j]->eth_params_s.transp_preference        = ETH_UDP_MODE;
      }

      else { // other midhaul
    AssertFatal(1==0,"MACRLC %d: unknown southbound midhaul\n",j);
      }
    }// j=0..num_inst
  }
}

int RCconfig_RRC(MessageDef *msg_p, uint32_t i, eNB_RRC_INST *rrc) {
  config_t          cfg;
  config_setting_t *setting                       = NULL;
  config_setting_t *subsetting                    = NULL;
  config_setting_t *setting_component_carriers     = NULL;
  config_setting_t *component_carrier              = NULL;
  config_setting_t *setting_srb1                   = NULL;
  config_setting_t *setting_mme_addresses          = NULL;
  config_setting_t *setting_mme_address            = NULL;
  config_setting_t *setting_ru                     = NULL;
  config_setting_t *setting_enb                    = NULL;
  config_setting_t *setting_otg                    = NULL;
  config_setting_t *subsetting_otg                 = NULL;
  config_setting_t *scheduling_info_br_list        = NULL;
  config_setting_t *scheduling_info_br             = NULL;
  config_setting_t *system_info_value_tag_SI_list  = NULL;
  config_setting_t *system_info_value_tag_SI       = NULL;

  config_setting_t *rach_ce_level_info_r13_list    = NULL;
  config_setting_t *rach_ce_level_info_r13         = NULL;

  config_setting_t *rsrp_range_list                = NULL;
  config_setting_t *rsrp_range_member              = NULL;

  config_setting_t *prach_parameters_ce_r13_list   = NULL;
  config_setting_t *prach_parameters_ce_r13        = NULL;

  config_setting_t *max_available_narrow_band_list = NULL;
  config_setting_t *max_available_narrow_band      = NULL;

  config_setting_t *n1_pucch_AN_info_r13_list      = NULL;
  config_setting_t *n1_pucch_AN_info_r13           = NULL;

#if	defined(Rel14)
  config_setting_t *setting_br13 = NULL;
#endif // REL14
  int               parse_errors                  = 0;
  int               num_enbs                      = 0;
  int               num_mme_address               = 0;
  int               num_otg_elements              = 0;
  int               num_component_carriers        = 0;
  int               j                             = 0;
  int               sched_info_idx                = 0;
  int               sys_info_idx                  = 0;
  libconfig_int     enb_id                        = 0;
  int               nb_cc                         = 0;

  char*             if_name_s                       = NULL;
  char*             ipv4_s                          = NULL;
  char*             ipv4_s_remote                   = NULL;

  char*             tr_s_preference                 = NULL;
  libconfig_int     local_s_portc                   = 0;
  libconfig_int     remote_s_portc                  = 0;
  libconfig_int     local_s_portd                   = 0;
  libconfig_int     remote_s_portd                  = 0;

  const char*       cell_type                     = NULL;
  const char*       tac                           = 0;
  const char*       enb_name                      = NULL;
  const char*       mcc                           = 0;
  const char*       mnc                           = 0;
  const char*       frame_type                    = NULL;
  libconfig_int     tdd_config                    = 0;
  libconfig_int     tdd_config_s                  = 0;
  const char*       prefix_type                   = NULL;
  const char*       pbch_repetition               = NULL;
  libconfig_int     eutra_band                    = 0;
  long long int     downlink_frequency            = 0;
  libconfig_int     uplink_frequency_offset       = 0;
  libconfig_int     Nid_cell                      = 0;
  libconfig_int     Nid_cell_mbsfn                = 0;
  libconfig_int     N_RB_DL                       = 0;
  libconfig_int     nb_antenna_ports              = 0;
  libconfig_int     nb_antennas_tx                = 0;
  libconfig_int     nb_antennas_rx                = 0;
  libconfig_int     tx_gain                       = 0;
  libconfig_int     rx_gain                       = 0;
  libconfig_int     prach_root                    = 0;
  libconfig_int     prach_config_index            = 0;
  const char*       prach_high_speed              = NULL;
  libconfig_int     prach_zero_correlation        = 0;
  libconfig_int     prach_freq_offset             = 0;
  libconfig_int     pucch_delta_shift             = 0;
  libconfig_int     pucch_nRB_CQI                 = 0;
  libconfig_int     pucch_nCS_AN                  = 0;

  libconfig_int     pucch_n1_AN                   = 0;

  libconfig_int     pdsch_referenceSignalPower    = 0;
  libconfig_int     pdsch_p_b                     = 0;
  libconfig_int     pusch_n_SB                    = 0;
  const char *      pusch_hoppingMode             = NULL;
  libconfig_int     pusch_hoppingOffset           = 0;
  const char*       pusch_enable64QAM             = NULL;
  const char*       pusch_groupHoppingEnabled     = NULL;
  libconfig_int     pusch_groupAssignment         = 0;
  const char*       pusch_sequenceHoppingEnabled  = NULL;
  libconfig_int     pusch_nDMRS1                  = 0;
  const char*       phich_duration                = NULL;
  const char*       phich_resource                = NULL;
  const char*       srs_enable                    = NULL;
  libconfig_int     srs_BandwidthConfig           = 0;
  libconfig_int     srs_SubframeConfig            = 0;
  const char*       srs_ackNackST                 = NULL;
  const char*       srs_MaxUpPts                  = NULL;
  libconfig_int     pusch_p0_Nominal              = 0;
  const char*       pusch_alpha                   = NULL;
  libconfig_int     pucch_p0_Nominal              = 0;
  libconfig_int     msg3_delta_Preamble           = 0;
  //libconfig_int     ul_CyclicPrefixLength         = 0;
  const char*       pucch_deltaF_Format1          = NULL;
  //const char*       pucch_deltaF_Format1a         = NULL;
  const char*       pucch_deltaF_Format1b         = NULL;
  const char*       pucch_deltaF_Format2          = NULL;
  const char*       pucch_deltaF_Format2a         = NULL;
  const char*       pucch_deltaF_Format2b         = NULL;
  libconfig_int     rach_numberOfRA_Preambles     = 0;
  const char*       rach_preamblesGroupAConfig    = NULL;
  libconfig_int     rach_sizeOfRA_PreamblesGroupA = 0;
  libconfig_int     rach_messageSizeGroupA        = 0;
  const char*       rach_messagePowerOffsetGroupB = NULL;
  libconfig_int     rach_powerRampingStep         = 0;
  libconfig_int     rach_preambleInitialReceivedTargetPower    = 0;
  libconfig_int     rach_preambleTransMax         = 0;
  libconfig_int     rach_raResponseWindowSize     = 0;
  libconfig_int     rach_macContentionResolutionTimer = 0;
  libconfig_int     rach_maxHARQ_Msg3Tx           = 0;
  libconfig_int     pcch_defaultPagingCycle       = 0;
  const char*       pcch_nB                       = NULL;
  libconfig_int     bcch_modificationPeriodCoeff  = 0;
  libconfig_int     ue_TimersAndConstants_t300    = 0;
  libconfig_int     ue_TimersAndConstants_t301    = 0;
  libconfig_int     ue_TimersAndConstants_t310    = 0;
  libconfig_int     ue_TimersAndConstants_t311    = 0;
  libconfig_int     ue_TimersAndConstants_n310    = 0;
  libconfig_int     ue_TimersAndConstants_n311    = 0;
  libconfig_int     ue_TransmissionMode           = 0;

  libconfig_int     si_Narrowband_r13             = 0;
  libconfig_int     si_TBS_r13                    = 0;

  libconfig_int     systemInfoValueTagSi_r13      = 0;

  libconfig_int     firstPreamble_r13                     = 0;
  libconfig_int     lastPreamble_r13                      = 0;
  libconfig_int     ra_ResponseWindowSize_r13             = 0;
  libconfig_int     mac_ContentionResolutionTimer_r13     = 0;
  libconfig_int     rar_HoppingConfig_r13                 = 0;
  libconfig_int     rsrp_range_br                         = 0;
  libconfig_int     prach_config_index_br                 = 0;
  libconfig_int     prach_freq_offset_br                  = 0;
  libconfig_int     prach_StartingSubframe_r13            = 0;
  libconfig_int     maxNumPreambleAttemptCE_r13           = 0;
  libconfig_int     numRepetitionPerPreambleAttempt_r13   = 0;
  libconfig_int     mpdcch_NumRepetition_RA_r13           = 0;
  libconfig_int     prach_HoppingConfig_r13               = 0;
  libconfig_int     maxavailablenarrowband                = 0;
  libconfig_int     pucch_info_value                      = 0;

  libconfig_int     srb1_timer_poll_retransmit    = 0;
  libconfig_int     srb1_timer_reordering         = 0;
  libconfig_int     srb1_timer_status_prohibit    = 0;
  libconfig_int     srb1_poll_pdu                 = 0;
  libconfig_int     srb1_poll_byte                = 0;
  libconfig_int     srb1_max_retx_threshold       = 0;

  libconfig_int     my_int;


  const char*       active_enb[MAX_ENB];
  char*             enb_interface_name_for_S1U    = NULL;
  char*             enb_ipv4_address_for_S1U      = NULL;
  libconfig_int     enb_port_for_S1U              = 0;
  char*             enb_interface_name_for_S1_MME = NULL;
  char*             enb_ipv4_address_for_S1_MME   = NULL;
  char             *address                       = NULL;
  char             *cidr                          = NULL;
  char             *astring                       = NULL;
  char*             flexran_agent_interface_name      = NULL;
  char*             flexran_agent_ipv4_address        = NULL;
  libconfig_int     flexran_agent_port                = 0;
  char*             flexran_agent_cache               = NULL;
  libconfig_int     otg_ue_id                     = 0;
  char*             otg_app_type                  = NULL;
  char*             otg_bg_traffic                = NULL;
  char*             glog_level                    = NULL;
  char*             glog_verbosity                = NULL;
  char*             hw_log_level                  = NULL;
  char*             hw_log_verbosity              = NULL;
  char*             phy_log_level                 = NULL;
  char*             phy_log_verbosity             = NULL;
  char*             mac_log_level                 = NULL;
  char*             mac_log_verbosity             = NULL;
  char*             rlc_log_level                 = NULL;
  char*             rlc_log_verbosity             = NULL;
  char*             pdcp_log_level                = NULL;
  char*             pdcp_log_verbosity            = NULL;
  char*             rrc_log_level                 = NULL;
  char*             rrc_log_verbosity             = NULL;
  char*             udp_log_verbosity             = NULL;
  char*             osa_log_level                 = NULL;
  char*             osa_log_verbosity             = NULL;

  // for no gcc warnings
  (void)astring;
  (void)my_int;

  memset((char*)active_enb,     0 , MAX_ENB * sizeof(char*));

  config_init(&cfg);

  if (RC.config_file_name != NULL) {
    // Read the file. If there is an error, report it and exit.
    if (! config_read_file(&cfg, RC.config_file_name)) {
      config_destroy(&cfg);
      AssertFatal (0, "Failed to parse eNB configuration file %s!\n", RC.config_file_name);
    }
  }
  else {
    config_destroy(&cfg);
    AssertFatal(0, "No eNB configuration file provided!\n");
  }

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)

  if (  (config_lookup_string( &cfg, ENB_CONFIG_STRING_ASN1_VERBOSITY, (const char **)&astring) )) {
    if (strcasecmp(astring , ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0) {
      asn_debug = 0;
      asn1_xer_print = 0;
    }
    else if (strcasecmp(astring, ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0) {
      asn_debug = 1;
      asn1_xer_print = 1;
    }
    else if (strcasecmp(astring, ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0) {
      asn_debug = 1;
      asn1_xer_print = 2;
    }
    else {
      asn_debug = 0;
      asn1_xer_print = 0;
    }
  }

#endif

  // Get list of active eNBs, (only these will be configured)
  setting = config_lookup(&cfg, ENB_CONFIG_STRING_ACTIVE_ENBS);

  if (setting != NULL) {
    num_enbs = config_setting_length(setting);
    setting_enb   = config_setting_get_elem(setting, i);
    active_enb[i] = config_setting_get_string (setting_enb);
    AssertFatal (active_enb[i] != NULL,
         "Failed to parse config file %s, %uth attribute %s \n",
         RC.config_file_name, i, ENB_CONFIG_STRING_ACTIVE_ENBS);
    active_enb[i] = strdup(active_enb[i]);
  }


  if (num_enbs>0) {
    // Output a list of all eNBs.
    setting = config_lookup(&cfg, ENB_CONFIG_STRING_ENB_LIST);

    if (setting != NULL) {
      num_enbs = config_setting_length(setting);


      setting_enb = config_setting_get_elem(setting, i);

      if (! config_setting_lookup_int(setting_enb, ENB_CONFIG_STRING_ENB_ID, &enb_id)) {
    // Calculate a default eNB ID
# if defined(ENABLE_USE_MME)
    uint32_t hash;

    hash = s1ap_generate_eNB_id ();
    enb_id = i + (hash & 0xFFFF8);
# else
    enb_id = i;
# endif
      }

      if (  !(       config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_CELL_TYPE,           &cell_type)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_ENB_NAME,            &enb_name)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_TRACKING_AREA_CODE,  &tac)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE, &mcc)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_MOBILE_NETWORK_CODE, &mnc)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_TRANSPORT_S_PREFERENCE, (const char **)&tr_s_preference)
             )
        ) {
    AssertFatal (0,
             "Failed to parse eNB configuration file %s, %u th enb\n",
             RC.config_file_name, i);
      }

      printf("RRC %d: Southbound Transport %s\n",j,tr_s_preference);

      if (strcmp(tr_s_preference, "local_mac") == 0) {


      }
      else if (strcmp(tr_s_preference, "cudu") == 0) {
    if (  !(config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_LOCAL_S_IF_NAME,        (const char **)&if_name_s)
        && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_LOCAL_S_ADDRESS,        (const char **)&ipv4_s)
        && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_REMOTE_S_ADDRESS,       (const char **)&ipv4_s_remote)
        && config_setting_lookup_int   (setting_enb, ENB_CONFIG_STRING_LOCAL_S_PORTC,          &local_s_portc)
        && config_setting_lookup_int   (setting_enb, ENB_CONFIG_STRING_REMOTE_S_PORTC,         &remote_s_portc)
        && config_setting_lookup_int   (setting_enb, ENB_CONFIG_STRING_LOCAL_S_PORTD,          &local_s_portd)
        && config_setting_lookup_int   (setting_enb, ENB_CONFIG_STRING_REMOTE_S_PORTD,         &remote_s_portd)
        )
          ) {
      AssertFatal (0,
               "Failed to parse eNB configuration file %s, %u th enb\n",
               RC.config_file_name, i);
    }
    rrc->eth_params_s.local_if_name            = strdup(if_name_s);
    rrc->eth_params_s.my_addr                  = strdup(ipv4_s);
    rrc->eth_params_s.remote_addr              = strdup(ipv4_s_remote);
    rrc->eth_params_s.my_portc                 = local_s_portc;
    rrc->eth_params_s.remote_portc             = remote_s_portc;
    rrc->eth_params_s.my_portd                 = local_s_portd;
    rrc->eth_params_s.remote_portd             = remote_s_portd;
    rrc->eth_params_s.transp_preference        = ETH_UDP_MODE;
      }

      else { // other midhaul
      }

      // search if in active list

      for (j=0; j < num_enbs; j++) {
    if (strcmp(active_enb[j], enb_name) == 0) {

      RRC_CONFIGURATION_REQ (msg_p).cell_identity = enb_id;

      /*
        if (strcmp(cell_type, "CELL_MACRO_ENB") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cell_type = CELL_MACRO_ENB;
        } else  if (strcmp(cell_type, "CELL_HOME_ENB") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cell_type = CELL_HOME_ENB;
        } else {
        AssertFatal (0,
        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
        lib_config_file_name_pP, i, cell_type);
        }

        enb_properties_loc.properties[enb_properties_loc_index]->eNB_name         = strdup(enb_name);
      */
      RRC_CONFIGURATION_REQ (msg_p).tac              = (uint16_t)atoi(tac);
      RRC_CONFIGURATION_REQ (msg_p).mcc              = (uint16_t)atoi(mcc);
      RRC_CONFIGURATION_REQ (msg_p).mnc              = (uint16_t)atoi(mnc);
      RRC_CONFIGURATION_REQ (msg_p).mnc_digit_length = strlen(mnc);
      AssertFatal((RRC_CONFIGURATION_REQ (msg_p).mnc_digit_length == 2) ||
              (RRC_CONFIGURATION_REQ (msg_p).mnc_digit_length == 3),
              "BAD MNC DIGIT LENGTH %d",
              RRC_CONFIGURATION_REQ (msg_p).mnc_digit_length);

      LOG_I(RRC,"RRC config (%s,%s,%s)\n",mcc,mnc,tac);
      // Parse optional physical parameters


      setting_component_carriers = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_COMPONENT_CARRIERS);
      nb_cc = 0;

      if (setting_component_carriers != NULL) {

        num_component_carriers     = config_setting_length(setting_component_carriers);
        LOG_I(RRC,"num component carriers %d \n", num_component_carriers);

        //enb_properties_loc.properties[enb_properties_loc_index]->nb_cc = num_component_carriers;
        for (j = 0; j < num_component_carriers ;j++) { // && j < MAX_NUM_CCs; j++) {
          component_carrier = config_setting_get_elem(setting_component_carriers, j);

          //printf("Component carrier %d\n",component_carrier);
          if (!(config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_FRAME_TYPE, &frame_type)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_TDD_CONFIG, &tdd_config)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_TDD_CONFIG_S, &tdd_config_s)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PREFIX_TYPE, &prefix_type)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PBCH_REPETITION, &pbch_repetition)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_EUTRA_BAND, &eutra_band)
            && config_setting_lookup_int64(component_carrier, ENB_CONFIG_STRING_DOWNLINK_FREQUENCY, &downlink_frequency)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET, &uplink_frequency_offset)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_NID_CELL, &Nid_cell)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_N_RB_DL, &N_RB_DL)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_CELL_MBSFN, &Nid_cell_mbsfn)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_NB_ANT_PORTS, &nb_antenna_ports)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PRACH_ROOT, &prach_root)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PRACH_CONFIG_INDEX, &prach_config_index)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PRACH_HIGH_SPEED, &prach_high_speed)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION, &prach_zero_correlation)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PRACH_FREQ_OFFSET, &prach_freq_offset)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT, &pucch_delta_shift)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_NRB_CQI, &pucch_nRB_CQI)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_NCS_AN, &pucch_nCS_AN)
#if !defined(Rel10) && !defined(Rel14)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_N1_AN, &pucch_n1_AN)
#endif
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PDSCH_RS_EPRE, &pdsch_referenceSignalPower)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PDSCH_PB, &pdsch_p_b)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_N_SB, &pusch_n_SB)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE, &pusch_hoppingMode)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET, &pusch_hoppingOffset)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM, &pusch_enable64QAM)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN, &pusch_groupHoppingEnabled)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT, &pusch_groupAssignment)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN, &pusch_sequenceHoppingEnabled)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_NDMRS1, &pusch_nDMRS1)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PHICH_DURATION, &phich_duration)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PHICH_RESOURCE, &phich_resource)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_SRS_ENABLE, &srs_enable)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUSCH_PO_NOMINAL, &pusch_p0_Nominal)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUSCH_ALPHA, &pusch_alpha)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PUCCH_PO_NOMINAL, &pucch_p0_Nominal)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE, &msg3_delta_Preamble)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1, &pucch_deltaF_Format1)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b, &pucch_deltaF_Format1b)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2, &pucch_deltaF_Format2)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A, &pucch_deltaF_Format2a)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B, &pucch_deltaF_Format2b)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES, &rach_numberOfRA_Preambles)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG, &rach_preamblesGroupAConfig)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP, &rach_powerRampingStep)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER, &rach_preambleInitialReceivedTargetPower)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX, &rach_preambleTransMax)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE, &rach_raResponseWindowSize)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER, &rach_macContentionResolutionTimer)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX, &rach_maxHARQ_Msg3Tx)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX, &bcch_modificationPeriodCoeff)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE,  &pcch_defaultPagingCycle)
            && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_PCCH_NB,  &pcch_nB)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF,  &bcch_modificationPeriodCoeff)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_T300,  &ue_TimersAndConstants_t300)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_T301,  &ue_TimersAndConstants_t301)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_T310,  &ue_TimersAndConstants_t310)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_T311,  &ue_TimersAndConstants_t311)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_N310,  &ue_TimersAndConstants_n310)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UETIMERS_N311,  &ue_TimersAndConstants_n311)
            && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_UE_TRANSMISSION_MODE,  &ue_TransmissionMode)

#if defined(Rel10) || defined(Rel14)


#endif
            )) {
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, Component Carrier %d!\n",
                 RC.config_file_name, nb_cc++);
        continue; // FIXME this prevents segfaults below, not sure what happens after function exit
          }

          nb_cc++;
          /*
        if (strcmp(cc_node_function, "eNodeB_3GPP") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = eNodeB_3GPP;
        } else if (strcmp(cc_node_function, "eNodeB_3GPP_BBU") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = eNodeB_3GPP_BBU;
        } else if (strcmp(cc_node_function, "NGFI_RCC_IF4p5") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = NGFI_RCC_IF4p5;
        } else if (strcmp(cc_node_function, "NGFI_RAU_IF4p5") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = NGFI_RAU_IF4p5;
        } else if (strcmp(cc_node_function, "NGFI_RRU_IF4p5") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = NGFI_RRU_IF4p5;
        } else if (strcmp(cc_node_function, "NGFI_RRU_IF5") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = NGFI_RRU_IF5;
        } else {
        AssertError (0, parse_errors ++,
        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for node_function choice: eNodeB_3GPP or eNodeB_3GPP_BBU or NGFI_IF4_RCC or NGFI_IF4_RRU or NGFI_IF5_RRU !\n",
        lib_config_file_name_pP, i, cc_node_function);
        }

        if (strcmp(cc_node_timing, "synch_to_ext_device") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_timing[j] = synch_to_ext_device;
        } else if (strcmp(cc_node_timing, "synch_to_other") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_timing[j] = synch_to_other;
        } else {
        AssertError (0, parse_errors ++,
        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for node_function choice: SYNCH_TO_DEVICE or SYNCH_TO_OTHER !\n",
        lib_config_file_name_pP, i, cc_node_timing);
        }

        if ((cc_node_synch_ref >= -1) && (cc_node_synch_ref < num_component_carriers)) {
        enb_properties_loc.properties[enb_properties_loc_index]->cc_node_synch_ref[j] = (int16_t) cc_node_synch_ref;
        } else {
        AssertError (0, parse_errors ++,
        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for node_synch_ref choice: valid CC_id or -1 !\n",
        lib_config_file_name_pP, i, cc_node_synch_ref);
        }
          */

          RRC_CONFIGURATION_REQ (msg_p).tdd_config[j] = tdd_config;

          AssertFatal (tdd_config <= TDD_Config__subframeAssignment_sa6,
               "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
               RC.config_file_name, i, tdd_config, TDD_Config__subframeAssignment_sa6);

          RRC_CONFIGURATION_REQ (msg_p).tdd_config_s[j] = tdd_config_s;
          AssertFatal (tdd_config_s <= TDD_Config__specialSubframePatterns_ssp8,
               "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
               RC.config_file_name, i, tdd_config_s, TDD_Config__specialSubframePatterns_ssp8);

          if (!prefix_type)
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                 RC.config_file_name, i, ENB_CONFIG_STRING_PREFIX_TYPE);
          else if (strcmp(prefix_type, "NORMAL") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).prefix_type[j] = NORMAL;
          }
          else  if (strcmp(prefix_type, "EXTENDED") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).prefix_type[j] = EXTENDED;
          }
          else {
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                RC.config_file_name, i, prefix_type);
          }
#ifdef Rel14
          if (!pbch_repetition)
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d define %s: TRUE,FALSE!\n",
                RC.config_file_name, i, ENB_CONFIG_STRING_PBCH_REPETITION);
          else if (strcmp(pbch_repetition, "TRUE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).pbch_repetition[j] = 1;
          }
          else  if (strcmp(pbch_repetition, "FALSE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).pbch_repetition[j] = 0;
          }
          else {
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pbch_repetition choice: TRUE or FALSE !\n",
                RC.config_file_name, i, pbch_repetition);
          }
#endif

          RRC_CONFIGURATION_REQ (msg_p).eutra_band[j] = eutra_band;
          RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j] = (uint32_t) downlink_frequency;
          RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j] = (unsigned int) uplink_frequency_offset;
          RRC_CONFIGURATION_REQ (msg_p).Nid_cell[j]= Nid_cell;

          if (Nid_cell>503) {
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
                 RC.config_file_name, i, Nid_cell);
          }

          RRC_CONFIGURATION_REQ (msg_p).N_RB_DL[j]= N_RB_DL;

          if ((N_RB_DL!=6) && (N_RB_DL!=15) && (N_RB_DL!=25) && (N_RB_DL!=50) && (N_RB_DL!=75) && (N_RB_DL!=100)) {
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
                 RC.config_file_name, i, N_RB_DL);
          }

          if (strcmp(frame_type, "FDD") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).frame_type[j] = FDD;
          }
          else  if (strcmp(frame_type, "TDD") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).frame_type[j] = TDD;
          }
          else {
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
                RC.config_file_name, i, frame_type);
          }


          RRC_CONFIGURATION_REQ (msg_p).tdd_config[j] = tdd_config;
          AssertFatal (tdd_config <= TDD_Config__subframeAssignment_sa6,
               "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
               RC.config_file_name, i, tdd_config, TDD_Config__subframeAssignment_sa6);


          RRC_CONFIGURATION_REQ (msg_p).tdd_config_s[j] = tdd_config_s;
          AssertFatal (tdd_config_s <= TDD_Config__specialSubframePatterns_ssp8,
               "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
               RC.config_file_name, i, tdd_config_s, TDD_Config__specialSubframePatterns_ssp8);



          if (!prefix_type)
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                 RC.config_file_name, i, ENB_CONFIG_STRING_PREFIX_TYPE);
          else if (strcmp(prefix_type, "NORMAL") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).prefix_type[j] = NORMAL;
          }
          else  if (strcmp(prefix_type, "EXTENDED") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).prefix_type[j] = EXTENDED;
          }
          else {
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                RC.config_file_name, i, prefix_type);
          }



          RRC_CONFIGURATION_REQ (msg_p).eutra_band[j] = eutra_band;
          // printf( "\teutra band:\t%d\n",RRC_CONFIGURATION_REQ (msg_p).eutra_band);



          RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j] = (uint32_t) downlink_frequency;
          //printf( "\tdownlink freq:\t%u\n",RRC_CONFIGURATION_REQ (msg_p).downlink_frequency);


          RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j] = (unsigned int) uplink_frequency_offset;

          if (enb_check_band_frequencies(RC.config_file_name,
                         j,
                         RRC_CONFIGURATION_REQ (msg_p).eutra_band[j],
                         RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j],
                         RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j],
                         RRC_CONFIGURATION_REQ (msg_p).frame_type[j])) {
        AssertFatal(0, "error calling enb_check_band_frequencies\n");
          }

          if ((nb_antenna_ports <1) || (nb_antenna_ports > 2))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for nb_antenna_ports choice: 1..2 !\n",
                 RC.config_file_name, i, nb_antenna_ports);

          RRC_CONFIGURATION_REQ (msg_p).nb_antenna_ports[j] = nb_antenna_ports;


          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_root =  prach_root;

          if ((prach_root <0) || (prach_root > 1023))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
                 RC.config_file_name, i, prach_root);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_config_index = prach_config_index;

          if ((prach_config_index <0) || (prach_config_index > 63))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
                 RC.config_file_name, i, prach_config_index);

          if (!prach_high_speed)
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                 RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
          else if (strcmp(prach_high_speed, "ENABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].prach_high_speed = TRUE;
          }
          else if (strcmp(prach_high_speed, "DISABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].prach_high_speed = FALSE;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
                RC.config_file_name, i, prach_high_speed);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_zero_correlation =prach_zero_correlation;

          if ((prach_zero_correlation <0) || (prach_zero_correlation > 15))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
                 RC.config_file_name, i, prach_zero_correlation);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_freq_offset = prach_freq_offset;

          if ((prach_freq_offset <0) || (prach_freq_offset > 94))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
                 RC.config_file_name, i, prach_freq_offset);


          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_delta_shift = pucch_delta_shift-1;

          if ((pucch_delta_shift <1) || (pucch_delta_shift > 3))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
                 RC.config_file_name, i, pucch_delta_shift);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nRB_CQI = pucch_nRB_CQI;

          if ((pucch_nRB_CQI <0) || (pucch_nRB_CQI > 98))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
                 RC.config_file_name, i, pucch_nRB_CQI);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nCS_AN = pucch_nCS_AN;

          if ((pucch_nCS_AN <0) || (pucch_nCS_AN > 7))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
                 RC.config_file_name, i, pucch_nCS_AN);

#if !defined(Rel10) && !defined(Rel14)
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_n1_AN = pucch_n1_AN;

          if ((pucch_n1_AN <0) || (pucch_n1_AN > 2047))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
                 RC.config_file_name, i, pucch_n1_AN);

#endif
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_referenceSignalPower = pdsch_referenceSignalPower;

          if ((pdsch_referenceSignalPower <-60) || (pdsch_referenceSignalPower > 50))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
                 RC.config_file_name, i, pdsch_referenceSignalPower);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_p_b = pdsch_p_b;

          if ((pdsch_p_b <0) || (pdsch_p_b > 3))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
                 RC.config_file_name, i, pdsch_p_b);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_n_SB = pusch_n_SB;

          if ((pusch_n_SB <1) || (pusch_n_SB > 4))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
                 RC.config_file_name, i, pusch_n_SB);

          if (!pusch_hoppingMode)
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
                 RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
          else if (strcmp(pusch_hoppingMode, "interSubFrame") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
          }
          else if (strcmp(pusch_hoppingMode, "intraAndInterSubFrame") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
                RC.config_file_name, i, pusch_hoppingMode);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingOffset = pusch_hoppingOffset;

          if ((pusch_hoppingOffset<0) || (pusch_hoppingOffset>98))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
                 RC.config_file_name, i, pusch_hoppingMode);

          if (!pusch_enable64QAM)
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                 RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
          else if (strcmp(pusch_enable64QAM, "ENABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_enable64QAM = TRUE;
          }
          else if (strcmp(pusch_enable64QAM, "DISABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_enable64QAM = FALSE;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
                RC.config_file_name, i, pusch_enable64QAM);

          if (!pusch_groupHoppingEnabled)
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
          else if (strcmp(pusch_groupHoppingEnabled, "ENABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled = TRUE;
          }
          else if (strcmp(pusch_groupHoppingEnabled, "DISABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled = FALSE;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
                RC.config_file_name, i, pusch_groupHoppingEnabled);


          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupAssignment = pusch_groupAssignment;

          if ((pusch_groupAssignment<0)||(pusch_groupAssignment>29))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
                 RC.config_file_name, i, pusch_groupAssignment);

          if (!pusch_sequenceHoppingEnabled)
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                 RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
          else if (strcmp(pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = TRUE;
          }
          else if (strcmp(pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = FALSE;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
                RC.config_file_name, i, pusch_sequenceHoppingEnabled);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_nDMRS1 = pusch_nDMRS1;  //cyclic_shift in RRC!

          if ((pusch_nDMRS1 <0) || (pusch_nDMRS1>7))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
                 RC.config_file_name, i, pusch_nDMRS1);

          if (strcmp(phich_duration, "NORMAL") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_duration = PHICH_Config__phich_Duration_normal;
          }
          else if (strcmp(phich_duration, "EXTENDED") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_duration = PHICH_Config__phich_Duration_extended;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
                RC.config_file_name, i, phich_duration);

          if (strcmp(phich_resource, "ONESIXTH") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource = PHICH_Config__phich_Resource_oneSixth;
          }
          else if (strcmp(phich_resource, "HALF") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource = PHICH_Config__phich_Resource_half;
          }
          else if (strcmp(phich_resource, "ONE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource = PHICH_Config__phich_Resource_one;
          }
          else if (strcmp(phich_resource, "TWO") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource = PHICH_Config__phich_Resource_two;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
                RC.config_file_name, i, phich_resource);

          printf("phich.resource %d (%s), phich.duration %d (%s)\n",
             (int)RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource, phich_resource,
             (int)RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_duration, phich_duration);

          if (strcmp(srs_enable, "ENABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_enable = TRUE;
          }
          else if (strcmp(srs_enable, "DISABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_enable = FALSE;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                RC.config_file_name, i, srs_enable);

          if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable == TRUE) {
        if (!(config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG, &srs_BandwidthConfig)
              && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG, &srs_SubframeConfig)
              && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG, &srs_ackNackST)
              && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_SRS_MAXUPPTS, &srs_MaxUpPts)
              ))
          AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown values for srs_BandwidthConfig, srs_SubframeConfig, srs_ackNackST, srs_MaxUpPts\n",
                  RC.config_file_name, i);

        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_BandwidthConfig = srs_BandwidthConfig;

        if ((srs_BandwidthConfig < 0) || (srs_BandwidthConfig >7))
          AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
                   RC.config_file_name, i, srs_BandwidthConfig);

        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_SubframeConfig = srs_SubframeConfig;

        if ((srs_SubframeConfig<0) || (srs_SubframeConfig>15))
          AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
                   RC.config_file_name, i, srs_SubframeConfig);

        if (strcmp(srs_ackNackST, "ENABLE") == 0) {
          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_ackNackST = TRUE;
        }
        else if (strcmp(srs_ackNackST, "DISABLE") == 0) {
          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_ackNackST = FALSE;
        }
        else
          AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                  RC.config_file_name, i, srs_ackNackST);

        if (strcmp(srs_MaxUpPts, "ENABLE") == 0) {
          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_MaxUpPts = TRUE;
        }
        else if (strcmp(srs_MaxUpPts, "DISABLE") == 0) {
          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_MaxUpPts = FALSE;
        }
        else
          AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
                  RC.config_file_name, i, srs_MaxUpPts);
          }

          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_p0_Nominal = pusch_p0_Nominal;

          if ((pusch_p0_Nominal < -126) || (pusch_p0_Nominal > 24))
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
                RC.config_file_name, i, pusch_p0_Nominal);

#ifndef Rel14
          if (strcmp(pusch_alpha, "AL0") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al0;
          }
          else if (strcmp(pusch_alpha, "AL04") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al04;
          }
          else if (strcmp(pusch_alpha, "AL05") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al05;
          }
          else if (strcmp(pusch_alpha, "AL06") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al06;
          }
          else if (strcmp(pusch_alpha, "AL07") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al07;
          }
          else if (strcmp(pusch_alpha, "AL08") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al08;
          }
          else if (strcmp(pusch_alpha, "AL09") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al09;
          }
          else if (strcmp(pusch_alpha, "AL1") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al1;
          }
#else
          if (strcmp(pusch_alpha, "AL0") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al0;
          }
          else if (strcmp(pusch_alpha, "AL04") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al04;
          }
          else if (strcmp(pusch_alpha, "AL05") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al05;
          }
          else if (strcmp(pusch_alpha, "AL06") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al06;
          }
          else if (strcmp(pusch_alpha, "AL07") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al07;
          }
          else if (strcmp(pusch_alpha, "AL08") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al08;
          }
          else if (strcmp(pusch_alpha, "AL09") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al09;
          }
          else if (strcmp(pusch_alpha, "AL1") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al1;
          }
#endif
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
                RC.config_file_name, i, pusch_alpha);

          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_p0_Nominal = pucch_p0_Nominal;

          if ((pucch_p0_Nominal < -127) || (pucch_p0_Nominal > -96))
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
                RC.config_file_name, i, pucch_p0_Nominal);

          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].msg3_delta_Preamble = msg3_delta_Preamble;

          if ((msg3_delta_Preamble < -1) || (msg3_delta_Preamble > 6))
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
                RC.config_file_name, i, msg3_delta_Preamble);


          if (strcmp(pucch_deltaF_Format1, "deltaF_2") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1 = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
          }
          else if (strcmp(pucch_deltaF_Format1, "deltaF0") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
          }
          else if (strcmp(pucch_deltaF_Format1, "deltaF2") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
                RC.config_file_name, i, pucch_deltaF_Format1);

          if (strcmp(pucch_deltaF_Format1b, "deltaF1") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
          }
          else if (strcmp(pucch_deltaF_Format1b, "deltaF3") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
          }
          else if (strcmp(pucch_deltaF_Format1b, "deltaF5") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
                RC.config_file_name, i, pucch_deltaF_Format1b);


          if (strcmp(pucch_deltaF_Format2, "deltaF_2") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
          }
          else if (strcmp(pucch_deltaF_Format2, "deltaF0") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
          }
          else if (strcmp(pucch_deltaF_Format2, "deltaF1") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
          }
          else if (strcmp(pucch_deltaF_Format2, "deltaF2") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
                RC.config_file_name, i, pucch_deltaF_Format2);

          if (strcmp(pucch_deltaF_Format2a, "deltaF_2") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
          }
          else if (strcmp(pucch_deltaF_Format2a, "deltaF0") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
          }
          else if (strcmp(pucch_deltaF_Format2a, "deltaF2") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
                RC.config_file_name, i, pucch_deltaF_Format2a);

          if (strcmp(pucch_deltaF_Format2b, "deltaF_2") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
          }
          else if (strcmp(pucch_deltaF_Format2b, "deltaF0") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
          }
          else if (strcmp(pucch_deltaF_Format2b, "deltaF2") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
                RC.config_file_name, i, pucch_deltaF_Format2b);




          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles= (rach_numberOfRA_Preambles / 4) - 1;

          if ((rach_numberOfRA_Preambles < 4) || (rach_numberOfRA_Preambles > 64) || ((rach_numberOfRA_Preambles & 3) != 0))
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                RC.config_file_name, i, rach_numberOfRA_Preambles);

          if (strcmp(rach_preamblesGroupAConfig, "ENABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= TRUE;

        if (!(config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA, &rach_sizeOfRA_PreamblesGroupA)
              && config_setting_lookup_int(component_carrier, ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA, &rach_messageSizeGroupA)
              && config_setting_lookup_string(component_carrier, ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB, &rach_messagePowerOffsetGroupB)))
          AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d  rach_sizeOfRA_PreamblesGroupA, messageSizeGroupA,messagePowerOffsetGroupB!\n",
                   RC.config_file_name, i);

        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_sizeOfRA_PreamblesGroupA= (rach_sizeOfRA_PreamblesGroupA/4)-1;

        if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>60) || ((rach_numberOfRA_Preambles&3)!=0))
          AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
                   RC.config_file_name, i, rach_sizeOfRA_PreamblesGroupA);


        switch (rach_messageSizeGroupA) {
        case 56:
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
          break;

        case 144:
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
          break;

        case 208:
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
          break;

        case 256:
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
          break;

        default:
          AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
                   RC.config_file_name, i, rach_messageSizeGroupA);
          break;
        }

        if (strcmp(rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
        }

        else if (strcmp(rach_messagePowerOffsetGroupB,"dB0")==0) {
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
        }

        else if (strcmp(rach_messagePowerOffsetGroupB,"dB5")==0) {
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
        }

        else if (strcmp(rach_messagePowerOffsetGroupB,"dB8")==0) {
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
        }

        else if (strcmp(rach_messagePowerOffsetGroupB,"dB10")==0) {
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
        }

        else if (strcmp(rach_messagePowerOffsetGroupB,"dB12")==0) {
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
        }

        else if (strcmp(rach_messagePowerOffsetGroupB,"dB15")==0) {
          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
        }

        else if (strcmp(rach_messagePowerOffsetGroupB,"dB18")==0) {
          RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
        }
        else
          AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
                  RC.config_file_name, i, rach_messagePowerOffsetGroupB);

          }
          else if (strcmp(rach_preamblesGroupAConfig, "DISABLE") == 0) {
        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= FALSE;
          }
          else
        AssertFatal(0,
                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
                RC.config_file_name, i, rach_preamblesGroupAConfig);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleInitialReceivedTargetPower= (rach_preambleInitialReceivedTargetPower+120)/2;

          if ((rach_preambleInitialReceivedTargetPower<-120) || (rach_preambleInitialReceivedTargetPower>-90) || ((rach_preambleInitialReceivedTargetPower&1)!=0))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                 RC.config_file_name, i, rach_preambleInitialReceivedTargetPower);


          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_powerRampingStep= rach_powerRampingStep/2;

          if ((rach_powerRampingStep<0) || (rach_powerRampingStep>6) || ((rach_powerRampingStep&1)!=0))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
                 RC.config_file_name, i, rach_powerRampingStep);



          switch (rach_preambleTransMax) {
#ifndef Rel14
        _	      case 3:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n3;
        break;

          case 4:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n4;
        break;

          case 5:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n5;
        break;

          case 6:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n6;
        break;

          case 7:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n7;
        break;

          case 8:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n8;
        break;

          case 10:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
        break;

          case 20:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n20;
        break;

          case 50:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n50;
        break;

          case 100:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n100;
        break;

          case 200:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n200;
        break;

#else

          case 3:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n3;
        break;

          case 4:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n4;
        break;

          case 5:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n5;
        break;

          case 6:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n6;
        break;

          case 7:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n7;
        break;

          case 8:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n8;
        break;

          case 10:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n10;
        break;

          case 20:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n20;
        break;

          case 50:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n50;
        break;

          case 100:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n100;
        break;

          case 200:
        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n200;
        break;
#endif

          default:
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
                 RC.config_file_name, i, rach_preambleTransMax);
        break;
          }

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_raResponseWindowSize=  (rach_raResponseWindowSize==10)?7:rach_raResponseWindowSize-2;

          if ((rach_raResponseWindowSize<0)||(rach_raResponseWindowSize==9)||(rach_raResponseWindowSize>10))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                 RC.config_file_name, i, rach_preambleTransMax);


          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_macContentionResolutionTimer= (rach_macContentionResolutionTimer/8)-1;

          if ((rach_macContentionResolutionTimer<8) || (rach_macContentionResolutionTimer>64) || ((rach_macContentionResolutionTimer&7)!=0))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                 RC.config_file_name, i, rach_preambleTransMax);

          RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_maxHARQ_Msg3Tx= rach_maxHARQ_Msg3Tx;

          if ((rach_maxHARQ_Msg3Tx<0) || (rach_maxHARQ_Msg3Tx>8))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
                 RC.config_file_name, i, rach_preambleTransMax);


          switch (pcch_defaultPagingCycle) {
          case 32:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf32;
              break;

          case 64:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf64;
              break;

          case 128:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf128;
              break;

          case 256:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf256;
              break;

          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
                           RC.config_file_name, i, pcch_defaultPagingCycle);
              break;
          }

          if (strcmp(pcch_nB, "fourT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_fourT;
          }
          else if (strcmp(pcch_nB, "twoT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_twoT;
          }
          else if (strcmp(pcch_nB, "oneT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_oneT;
          }
          else if (strcmp(pcch_nB, "halfT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_halfT;
          }
          else if (strcmp(pcch_nB, "quarterT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_quarterT;
          }
          else if (strcmp(pcch_nB, "oneEighthT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_oneEighthT;
          }
          else if (strcmp(pcch_nB, "oneSixteenthT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_oneSixteenthT;
          }
          else if (strcmp(pcch_nB, "oneThirtySecondT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_oneThirtySecondT;
          }
          else
              AssertFatal(0,
                          "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                          RC.config_file_name, i, pcch_defaultPagingCycle);



          switch (bcch_modificationPeriodCoeff) {
          case 2:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n2;
              break;

          case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n4;
              break;

          case 8:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n8;
              break;

          case 16:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n16;
              break;

          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
                           RC.config_file_name, i, bcch_modificationPeriodCoeff);

              break;
          }


          switch (ue_TimersAndConstants_t300) {
          case 100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= UE_TimersAndConstants__t300_ms100;
              break;

          case 200:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= UE_TimersAndConstants__t300_ms200;
              break;

          case 300:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= UE_TimersAndConstants__t300_ms300;
              break;

          case 400:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= UE_TimersAndConstants__t300_ms400;
              break;

          case 600:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= UE_TimersAndConstants__t300_ms600;
              break;

          case 1000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= UE_TimersAndConstants__t300_ms1000;
              break;

          case 1500:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= UE_TimersAndConstants__t300_ms1500;
              break;

          case 2000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= UE_TimersAndConstants__t300_ms2000;
              break;

          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t300 choice: 100,200,300,400,600,1000,1500,2000 ",
                           RC.config_file_name, i, ue_TimersAndConstants_t300);
              break;

          }

          switch (ue_TimersAndConstants_t301) {
          case 100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms100;
              break;

          case 200:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms200;
              break;

          case 300:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms300;
              break;

          case 400:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms400;
              break;

          case 600:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms600;
              break;

          case 1000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1000;
              break;

          case 1500:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1500;
              break;

          case 2000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms2000;
              break;

          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t301 choice: 100,200,300,400,600,1000,1500,2000 ",
                           RC.config_file_name, i, ue_TimersAndConstants_t301);
              break;

          }

          switch (ue_TimersAndConstants_t310) {
          case 0:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms0;
              break;

          case 50:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms50;
              break;

          case 100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms100;
              break;

          case 200:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms200;
              break;

          case 500:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms500;
              break;

          case 1000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms1000;
              break;

          case 2000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms2000;
              break;

          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t310 choice: 0,50,100,200,500,1000,1500,2000 ",
                           RC.config_file_name, i, ue_TimersAndConstants_t310);
              break;

          }

          switch (ue_TimersAndConstants_t311) {
          case 1000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms1000;
              break;

          case 3110:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms3000;
              break;

          case 5000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms5000;
              break;

          case 10000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms10000;
              break;

          case 15000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms15000;
              break;

          case 20000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms20000;
              break;

          case 31100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms30000;
              break;

          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 choice: 1000,3000,5000,10000,150000,20000,30000",
                           RC.config_file_name, i, ue_TimersAndConstants_t311);
              break;

          }

          switch (ue_TimersAndConstants_n310) {
          case 1:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n1;
              break;

          case 2:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n2;
              break;

          case 3:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n3;
              break;

          case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n4;
              break;

          case 6:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n6;
              break;

          case 8:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n8;
              break;

          case 10:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n10;
              break;

          case 20:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n20;
              break;

          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n310 choice: 1,2,3,4,6,6,8,10,20",
                           RC.config_file_name, i, ue_TimersAndConstants_n311);
              break;

          }

          switch (ue_TimersAndConstants_n311) {
          case 1:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n1;
              break;

          case 2:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n2;
              break;

          case 3:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n3;
              break;

          case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n4;
              break;

          case 5:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n5;
              break;

          case 6:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n6;
              break;

          case 8:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n8;
              break;

          case 10:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n10;
              break;

          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 choice: 1,2,3,4,5,6,8,10",
                           RC.config_file_name, i, ue_TimersAndConstants_t311);
              break;

          }

          switch (ue_TransmissionMode) {
          case 1:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm1;
              break;
          case 2:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm2;
              break;
          case 3:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm3;
              break;
          case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm4;
              break;
          case 5:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm5;
              break;
          case 6:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm6;
              break;
          case 7:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm7;
              break;
          default:
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
                           RC.config_file_name, i, ue_TransmissionMode);
              break;
          }
#ifdef Rel14
          setting_br13 = config_setting_get_member(component_carrier, ENB_CONFIG_STRING_BR);
          if (setting_br13 == NULL) {
              puts("setting BR is NULL");
          } else {
              puts("setting BR is NOT NULL");
          }

          if (setting_br13 != NULL) {

              scheduling_info_br_list       = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_SCHEDULING_INFO_LIST);
              int num_scheduling_info       = config_setting_length(scheduling_info_br_list);
              RRC_CONFIGURATION_REQ (msg_p).scheduling_info_br_size[j] = num_scheduling_info;

              for (sched_info_idx = 0; sched_info_idx < num_scheduling_info; ++sched_info_idx)
              {
                  scheduling_info_br = config_setting_get_elem(scheduling_info_br_list, sched_info_idx);
                  if (! config_setting_lookup_int(scheduling_info_br, ENB_CONFIG_STRING_SI_NARROWBAND_R13, &si_Narrowband_r13)
                     && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_SI_TBS_R13, &si_TBS_r13)
                  )
                  {
                      AssertFatal (0, "Failed to parse eNB configuration file %s, scheduking info br %d!\n", RC.config_file_name, nb_cc++);
                  }
                  RRC_CONFIGURATION_REQ (msg_p).si_Narrowband_r13[j][sched_info_idx] = si_Narrowband_r13;
                  RRC_CONFIGURATION_REQ (msg_p).si_TBS_r13[j][sched_info_idx] = si_TBS_r13;
              }

              system_info_value_tag_SI_list = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_LIST);
              int num_system_info = config_setting_length(system_info_value_tag_SI_list);
              RRC_CONFIGURATION_REQ (msg_p).system_info_value_tag_SI_size[j] = num_system_info;

              for (sys_info_idx = 0; sys_info_idx < num_system_info; ++sys_info_idx)
              {
                  system_info_value_tag_SI = config_setting_get_elem(system_info_value_tag_SI_list, sys_info_idx);
                  if ( !(config_setting_lookup_int(system_info_value_tag_SI, ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_SI_R13, &systemInfoValueTagSi_r13)) )
                  {
                      AssertFatal (0, "Failed to parse eNB configuration file %s, system info value tag %d!\n", RC.config_file_name, nb_cc++);
                  }
                  RRC_CONFIGURATION_REQ (msg_p).systemInfoValueTagSi_r13[j][sys_info_idx] = systemInfoValueTagSi_r13;
              }


              rach_ce_level_info_r13_list = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_RACH_CE_LEVEL_INFO_LIST);
              int num_rach_ce_level_info_list = config_setting_length(rach_ce_level_info_r13_list);
              RRC_CONFIGURATION_REQ (msg_p).rach_CE_LevelInfoList_r13_size[j] = num_rach_ce_level_info_list;
              int rach_info_level_idx;
              for (rach_info_level_idx = 0; rach_info_level_idx < num_rach_ce_level_info_list; ++rach_info_level_idx)
              {
                  rach_ce_level_info_r13 = config_setting_get_elem(rach_ce_level_info_r13_list, rach_info_level_idx);
                  if (!     (config_setting_lookup_int(rach_ce_level_info_r13, ENB_CONFIG_STRING_FIRST_PREAMBLE,                   &firstPreamble_r13)
                          && config_setting_lookup_int(rach_ce_level_info_r13, ENB_CONFIG_STRING_LAST_PREAMBLE,                    &lastPreamble_r13)
                          && config_setting_lookup_int(rach_ce_level_info_r13, ENB_CONFIG_STRING_RA_RESPONSE_WINDOW_SIZE_R13,      &ra_ResponseWindowSize_r13)
                          && config_setting_lookup_int(rach_ce_level_info_r13, ENB_CONFIG_STRING_MAC_CONTENT_RESOLUTION_TIMER_R13, &mac_ContentionResolutionTimer_r13)
                          && config_setting_lookup_int(rach_ce_level_info_r13, ENB_CONFIG_STRING_RAR_HOPPING_CONFIG_R13,           &rar_HoppingConfig_r13) )
                          )
                  {
                      AssertFatal (0,
                           "Failed to parse eNB configuration file %s, rach_ce_level_info_r13 %d!\n",
                           RC.config_file_name, nb_cc++);
                  }

                  RRC_CONFIGURATION_REQ (msg_p).firstPreamble_r13[j][rach_info_level_idx]                 = firstPreamble_r13;
                  RRC_CONFIGURATION_REQ (msg_p).lastPreamble_r13[j][rach_info_level_idx]                  = lastPreamble_r13;
                  RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rach_info_level_idx]         = ra_ResponseWindowSize_r13;
                  RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rach_info_level_idx] = mac_ContentionResolutionTimer_r13;
                  RRC_CONFIGURATION_REQ (msg_p).rar_HoppingConfig_r13[j][rach_info_level_idx]             = rar_HoppingConfig_r13;

              }

              rsrp_range_list = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_RSRP_RANGE_LIST);
              int num_rsrp_list = config_setting_length(rsrp_range_list);
              RRC_CONFIGURATION_REQ (msg_p).rsrp_range_list_size[j] = num_rsrp_list;
              int rsrp_range_idx;
              for (rsrp_range_idx = 0; rsrp_range_idx < num_rsrp_list; ++rsrp_range_idx)
              {
                  rsrp_range_member = config_setting_get_elem(rsrp_range_list, rsrp_range_idx);
                  if (!(config_setting_lookup_int(rsrp_range_member, ENB_CONFIG_STRING_RSRP_RANGE_BR, &rsrp_range_br)))
                  {
                      AssertFatal (0,
                                   "Failed to parse eNB configuration file %s, rsrp_range %d!\n",
                                   RC.config_file_name, nb_cc++);
                  }
                  RRC_CONFIGURATION_REQ (msg_p).rsrp_range[j][rsrp_range_idx] = rsrp_range_br;
              }

              prach_parameters_ce_r13_list = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13);
              int num_prach_parameters_ce_r13 = config_setting_length(prach_parameters_ce_r13_list);
              RRC_CONFIGURATION_REQ (msg_p).prach_parameters_list_size[j] = num_prach_parameters_ce_r13;
              int prach_parameters_index;
              for (prach_parameters_index = 0; prach_parameters_index < num_prach_parameters_ce_r13; ++prach_parameters_index)
              {
                  prach_parameters_ce_r13 = config_setting_get_elem(prach_parameters_ce_r13_list, prach_parameters_index);
                  if (!     (config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_PRACH_CONFIG_INDEX_BR, &prach_config_index_br)
                          && config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_PRACH_FREQ_OFFSET_BR, &prach_freq_offset_br)
                          && config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_NUM_REPETITION_PREAMBLE_ATTEMPT_R13, &numRepetitionPerPreambleAttempt_r13)
                          && config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_MPDCCH_NUM_REPETITION_RA_R13, &mpdcch_NumRepetition_RA_r13)
                          && config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_PRACH_HOPPING_CONFIG_R13, &prach_HoppingConfig_r13) )
                          )
                  {
                      AssertFatal (0,
                           "Failed to parse eNB configuration file %s, prach_parameters_ce_r13_list %d!\n",
                           RC.config_file_name, nb_cc++);
                  }

                  RRC_CONFIGURATION_REQ (msg_p).prach_config_index[j][prach_parameters_index]                  = prach_config_index_br;
                  RRC_CONFIGURATION_REQ (msg_p).prach_freq_offset[j][prach_parameters_index]                   = prach_freq_offset_br;
                  RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[j][prach_parameters_index]          = prach_StartingSubframe_r13;
                  RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prach_parameters_index]         = maxNumPreambleAttemptCE_r13;
                  RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prach_parameters_index] = numRepetitionPerPreambleAttempt_r13;
                  RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prach_parameters_index]         = mpdcch_NumRepetition_RA_r13;
                  RRC_CONFIGURATION_REQ (msg_p).prach_HoppingConfig_r13[j][prach_parameters_index]             = prach_HoppingConfig_r13;

                  if (config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_PRACH_STARTING_SUBFRAME_R13, &prach_StartingSubframe_r13))
                  {
                      RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[j][prach_parameters_index] = calloc(1, sizeof(long));
                      RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[j][prach_parameters_index] = prach_StartingSubframe_r13;
                  }
                  else
                  {
                      RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[j][prach_parameters_index] = NULL;
                  }

                  if (config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_MAX_NUM_PREAMBLE_ATTEMPT_CE_R13, &maxNumPreambleAttemptCE_r13))
                  {
                      RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prach_parameters_index] = calloc(1, sizeof(long));
                      RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prach_parameters_index] = maxNumPreambleAttemptCE_r13;
                  }
                  else
                  {
                      RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prach_parameters_index] = NULL;
                  }

              }


              // TODO
//              max_available_narrow_band = config_setting_get_member(prach_parameters_ce_r13, ENB_CONFIG_STRING_MAX_AVAILABLE_NARROW_BAND);


              n1_pucch_AN_info_r13_list = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_N1_PUCCH_AN_INFO_LIST);

              int num_pucch_an_info = config_setting_length(n1_pucch_AN_info_r13_list);
              RRC_CONFIGURATION_REQ (msg_p).pucch_info_value_size[j] = num_pucch_an_info;
              int pucch_info_idx;
              for (pucch_info_idx = 0; pucch_info_idx < num_pucch_an_info; ++pucch_info_idx)
              {
                  n1_pucch_AN_info_r13 = config_setting_get_elem(n1_pucch_AN_info_r13_list, pucch_info_idx);
                  if (! (config_setting_lookup_int(n1_pucch_AN_info_r13, ENB_CONFIG_STRING_PUCCH_INFO_VALUE, &pucch_info_value)) )
                  {
                      AssertFatal (0,
                           "Failed to parse eNB configuration file %s, n1_pucch_AN_info_list_r13_list %d!\n",
                           RC.config_file_name, nb_cc++);
                  }

                  RRC_CONFIGURATION_REQ (msg_p).pucch_info_value[j][pucch_info_idx] = pucch_info_value;
              }

        // Rel8 RadioResourceConfigCommon Parameters
        if (!(config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_FRAME_TYPE, &frame_type)

              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_ROOT, &prach_root)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_CONFIG_INDEX, &prach_config_index)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PRACH_HIGH_SPEED, &prach_high_speed)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION, &prach_zero_correlation)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_FREQ_OFFSET, &prach_freq_offset)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT, &pucch_delta_shift)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_NRB_CQI, &pucch_nRB_CQI)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_NCS_AN, &pucch_nCS_AN)
#if !defined(Rel10) && !defined(Rel14)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_N1_AN, &pucch_n1_AN)
#endif
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PDSCH_RS_EPRE, &pdsch_referenceSignalPower)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PDSCH_PB, &pdsch_p_b)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_N_SB, &pusch_n_SB)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE, &pusch_hoppingMode)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET, &pusch_hoppingOffset)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM, &pusch_enable64QAM)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN, &pusch_groupHoppingEnabled)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT, &pusch_groupAssignment)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN, &pusch_sequenceHoppingEnabled)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_NDMRS1, &pusch_nDMRS1)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PHICH_DURATION, &phich_duration)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PHICH_RESOURCE, &phich_resource)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_SRS_ENABLE, &srs_enable)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_PO_NOMINAL, &pusch_p0_Nominal)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_ALPHA, &pusch_alpha)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_PO_NOMINAL, &pucch_p0_Nominal)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE, &msg3_delta_Preamble)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1, &pucch_deltaF_Format1)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b, &pucch_deltaF_Format1b)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2, &pucch_deltaF_Format2)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A, &pucch_deltaF_Format2a)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B, &pucch_deltaF_Format2b)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES, &rach_numberOfRA_Preambles)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG, &rach_preamblesGroupAConfig)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP, &rach_powerRampingStep)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER, &rach_preambleInitialReceivedTargetPower)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX, &rach_preambleTransMax)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE, &rach_raResponseWindowSize)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER, &rach_macContentionResolutionTimer)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX, &rach_maxHARQ_Msg3Tx)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE,  &pcch_defaultPagingCycle)
              && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PCCH_NB,  &pcch_nB)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF,  &bcch_modificationPeriodCoeff)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_UETIMERS_T300,  &ue_TimersAndConstants_t300)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_UETIMERS_T301,  &ue_TimersAndConstants_t301)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_UETIMERS_T310,  &ue_TimersAndConstants_t310)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_UETIMERS_T311,  &ue_TimersAndConstants_t311)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_UETIMERS_N310,  &ue_TimersAndConstants_n310)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_UETIMERS_N311,  &ue_TimersAndConstants_n311)
              && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_UE_TRANSMISSION_MODE,  &ue_TransmissionMode)
              )) {
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, Component Carrier %d!\n",
                   RC.config_file_name, nb_cc++);
              continue; // FIXME this prevents segfaults below, not sure what happens after function exit
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_root =  prach_root;

            if ((prach_root <0) || (prach_root > 1023))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
                   RC.config_file_name, i, prach_root);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_config_index = prach_config_index;

            if ((prach_config_index <0) || (prach_config_index > 63))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
                   RC.config_file_name, i, prach_config_index);

            if (!prach_high_speed)
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                   RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
            else if (strcmp(prach_high_speed, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_high_speed = TRUE;
            }
            else if (strcmp(prach_high_speed, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_high_speed = FALSE;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
                  RC.config_file_name, i, prach_high_speed);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_zero_correlation =prach_zero_correlation;

            if ((prach_zero_correlation <0) || (prach_zero_correlation > 15))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
                   RC.config_file_name, i, prach_zero_correlation);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_freq_offset = prach_freq_offset;

            if ((prach_freq_offset <0) || (prach_freq_offset > 94))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
                   RC.config_file_name, i, prach_freq_offset);


            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_delta_shift = pucch_delta_shift-1;

            if ((pucch_delta_shift <1) || (pucch_delta_shift > 3))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
                   RC.config_file_name, i, pucch_delta_shift);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_nRB_CQI = pucch_nRB_CQI;

            if ((pucch_nRB_CQI <0) || (pucch_nRB_CQI > 98))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
                   RC.config_file_name, i, pucch_nRB_CQI);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_nCS_AN = pucch_nCS_AN;

            if ((pucch_nCS_AN <0) || (pucch_nCS_AN > 7))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
                   RC.config_file_name, i, pucch_nCS_AN);

#if !defined(Rel10) && !defined(Rel14)
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_n1_AN = pucch_n1_AN;

            if ((pucch_n1_AN <0) || (pucch_n1_AN > 2047))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
                   RC.config_file_name, i, pucch_n1_AN);

#endif
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pdsch_referenceSignalPower = pdsch_referenceSignalPower;

            if ((pdsch_referenceSignalPower <-60) || (pdsch_referenceSignalPower > 50))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
                   RC.config_file_name, i, pdsch_referenceSignalPower);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pdsch_p_b = pdsch_p_b;

            if ((pdsch_p_b <0) || (pdsch_p_b > 3))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
                   RC.config_file_name, i, pdsch_p_b);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_n_SB = pusch_n_SB;

            if ((pusch_n_SB <1) || (pusch_n_SB > 4))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
                   RC.config_file_name, i, pusch_n_SB);

            if (!pusch_hoppingMode)
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
                   RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
            else if (strcmp(pusch_hoppingMode, "interSubFrame") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
            }
            else if (strcmp(pusch_hoppingMode, "intraAndInterSubFrame") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
                  RC.config_file_name, i, pusch_hoppingMode);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_hoppingOffset = pusch_hoppingOffset;

            if ((pusch_hoppingOffset<0) || (pusch_hoppingOffset>98))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
                   RC.config_file_name, i, pusch_hoppingMode);

            if (!pusch_enable64QAM)
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                   RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
            else if (strcmp(pusch_enable64QAM, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_enable64QAM = TRUE;
            }
            else if (strcmp(pusch_enable64QAM, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_enable64QAM = FALSE;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
                  RC.config_file_name, i, pusch_enable64QAM);

            if (!pusch_groupHoppingEnabled)
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                  RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
            else if (strcmp(pusch_groupHoppingEnabled, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_groupHoppingEnabled = TRUE;
            }
            else if (strcmp(pusch_groupHoppingEnabled, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_groupHoppingEnabled = FALSE;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
                  RC.config_file_name, i, pusch_groupHoppingEnabled);


            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_groupAssignment = pusch_groupAssignment;

            if ((pusch_groupAssignment<0)||(pusch_groupAssignment>29))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
                   RC.config_file_name, i, pusch_groupAssignment);

            if (!pusch_sequenceHoppingEnabled)
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                   RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
            else if (strcmp(pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_sequenceHoppingEnabled = TRUE;
            }
            else if (strcmp(pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_sequenceHoppingEnabled = FALSE;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
                  RC.config_file_name, i, pusch_sequenceHoppingEnabled);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_nDMRS1 = pusch_nDMRS1;  //cyclic_shift in RRC!

            if ((pusch_nDMRS1 <0) || (pusch_nDMRS1>7))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
                   RC.config_file_name, i, pusch_nDMRS1);

            if (strcmp(phich_duration, "NORMAL") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_duration = PHICH_Config__phich_Duration_normal;
            }
            else if (strcmp(phich_duration, "EXTENDED") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_duration = PHICH_Config__phich_Duration_extended;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
                  RC.config_file_name, i, phich_duration);

            if (strcmp(phich_resource, "ONESIXTH") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource = PHICH_Config__phich_Resource_oneSixth;
            }
            else if (strcmp(phich_resource, "HALF") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource = PHICH_Config__phich_Resource_half;
            }
            else if (strcmp(phich_resource, "ONE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource = PHICH_Config__phich_Resource_one;
            }
            else if (strcmp(phich_resource, "TWO") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource = PHICH_Config__phich_Resource_two;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
                  RC.config_file_name, i, phich_resource);

            printf("phich.resource %d (%s), phich.duration %d (%s)\n",
               RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource, phich_resource,
               RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_duration, phich_duration);

            if (strcmp(srs_enable, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_enable = TRUE;
            }
            else if (strcmp(srs_enable, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_enable = FALSE;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                  RC.config_file_name, i, srs_enable);

            if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_enable == TRUE) {
              if (!(config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG, &srs_BandwidthConfig)
                && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG, &srs_SubframeConfig)
                && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG, &srs_ackNackST)
                && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_SRS_MAXUPPTS, &srs_MaxUpPts)
                ))
            AssertFatal(0,
                    "Failed to parse eNB configuration file %s, enb %d unknown values for srs_BandwidthConfig, srs_SubframeConfig, srs_ackNackST, srs_MaxUpPts\n",
                    RC.config_file_name, i);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_BandwidthConfig = srs_BandwidthConfig;

              if ((srs_BandwidthConfig < 0) || (srs_BandwidthConfig >7))
            AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
                     RC.config_file_name, i, srs_BandwidthConfig);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_SubframeConfig = srs_SubframeConfig;

              if ((srs_SubframeConfig<0) || (srs_SubframeConfig>15))
            AssertFatal (0,
                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
                     RC.config_file_name, i, srs_SubframeConfig);

              if (strcmp(srs_ackNackST, "ENABLE") == 0) {
            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_ackNackST = TRUE;
              }
              else if (strcmp(srs_ackNackST, "DISABLE") == 0) {
            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_ackNackST = FALSE;
              }
              else
            AssertFatal(0,
                    "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                    RC.config_file_name, i, srs_ackNackST);

              if (strcmp(srs_MaxUpPts, "ENABLE") == 0) {
            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_MaxUpPts = TRUE;
              }
              else if (strcmp(srs_MaxUpPts, "DISABLE") == 0) {
            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_MaxUpPts = FALSE;
              }
              else
            AssertFatal(0,
                    "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
                    RC.config_file_name, i, srs_MaxUpPts);
            }

            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_p0_Nominal = pusch_p0_Nominal;

            if ((pusch_p0_Nominal < -126) || (pusch_p0_Nominal > 24))
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
                  RC.config_file_name, i, pusch_p0_Nominal);

#ifndef Rel14
            if (strcmp(pusch_alpha, "AL0") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = UplinkPowerControlCommon__alpha_al0;
            }
            else if (strcmp(pusch_alpha, "AL04") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = UplinkPowerControlCommon__alpha_al04;
            }
            else if (strcmp(pusch_alpha, "AL05") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = UplinkPowerControlCommon__alpha_al05;
            }
            else if (strcmp(pusch_alpha, "AL06") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = UplinkPowerControlCommon__alpha_al06;
            }
            else if (strcmp(pusch_alpha, "AL07") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = UplinkPowerControlCommon__alpha_al07;
            }
            else if (strcmp(pusch_alpha, "AL08") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = UplinkPowerControlCommon__alpha_al08;
            }
            else if (strcmp(pusch_alpha, "AL09") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = UplinkPowerControlCommon__alpha_al09;
            }
            else if (strcmp(pusch_alpha, "AL1") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = UplinkPowerControlCommon__alpha_al1;
            }
#else
            if (strcmp(pusch_alpha, "AL0") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al0;
            }
            else if (strcmp(pusch_alpha, "AL04") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al04;
            }
            else if (strcmp(pusch_alpha, "AL05") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al05;
            }
            else if (strcmp(pusch_alpha, "AL06") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al06;
            }
            else if (strcmp(pusch_alpha, "AL07") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al07;
            }
            else if (strcmp(pusch_alpha, "AL08") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al08;
            }
            else if (strcmp(pusch_alpha, "AL09") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al09;
            }
            else if (strcmp(pusch_alpha, "AL1") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al1;
            }
#endif
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
                  RC.config_file_name, i, pusch_alpha);

            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_p0_Nominal = pucch_p0_Nominal;

            if ((pucch_p0_Nominal < -127) || (pucch_p0_Nominal > -96))
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
                  RC.config_file_name, i, pucch_p0_Nominal);

            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].msg3_delta_Preamble = msg3_delta_Preamble;

            if ((msg3_delta_Preamble < -1) || (msg3_delta_Preamble > 6))
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
                  RC.config_file_name, i, msg3_delta_Preamble);


            if (strcmp(pucch_deltaF_Format1, "deltaF_2") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1 = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
            }
            else if (strcmp(pucch_deltaF_Format1, "deltaF0") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
            }
            else if (strcmp(pucch_deltaF_Format1, "deltaF2") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
                  RC.config_file_name, i, pucch_deltaF_Format1);

            if (strcmp(pucch_deltaF_Format1b, "deltaF1") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
            }
            else if (strcmp(pucch_deltaF_Format1b, "deltaF3") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
            }
            else if (strcmp(pucch_deltaF_Format1b, "deltaF5") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
                  RC.config_file_name, i, pucch_deltaF_Format1b);


            if (strcmp(pucch_deltaF_Format2, "deltaF_2") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
            }
            else if (strcmp(pucch_deltaF_Format2, "deltaF0") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
            }
            else if (strcmp(pucch_deltaF_Format2, "deltaF1") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
            }
            else if (strcmp(pucch_deltaF_Format2, "deltaF2") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
                  RC.config_file_name, i, pucch_deltaF_Format2);

            if (strcmp(pucch_deltaF_Format2a, "deltaF_2") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
            }
            else if (strcmp(pucch_deltaF_Format2a, "deltaF0") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
            }
            else if (strcmp(pucch_deltaF_Format2a, "deltaF2") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
                  RC.config_file_name, i, pucch_deltaF_Format2a);

            if (strcmp(pucch_deltaF_Format2b, "deltaF_2") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
            }
            else if (strcmp(pucch_deltaF_Format2b, "deltaF0") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
            }
            else if (strcmp(pucch_deltaF_Format2b, "deltaF2") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
                  RC.config_file_name, i, pucch_deltaF_Format2b);




            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles= (rach_numberOfRA_Preambles / 4) - 1;


            if ((rach_numberOfRA_Preambles < 4) || (rach_numberOfRA_Preambles > 64) || ((rach_numberOfRA_Preambles & 3) != 0))
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                  RC.config_file_name, i, rach_numberOfRA_Preambles);

            if (strcmp(rach_preamblesGroupAConfig, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_preamblesGroupAConfig= TRUE;

              if (!(config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA, &rach_sizeOfRA_PreamblesGroupA)
                && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA, &rach_messageSizeGroupA)
                && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB, &rach_messagePowerOffsetGroupB)))
            AssertFatal (0,
                     "Failed to parse eNB configuration file %s, enb %d  rach_sizeOfRA_PreamblesGroupA, messageSizeGroupA,messagePowerOffsetGroupB!\n",
                     RC.config_file_name, i);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_sizeOfRA_PreamblesGroupA= (rach_sizeOfRA_PreamblesGroupA/4)-1;

              if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>60) || ((rach_numberOfRA_Preambles&3)!=0))
            AssertFatal (0,
                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
                     RC.config_file_name, i, rach_sizeOfRA_PreamblesGroupA);


              switch (rach_messageSizeGroupA) {
              case 56:
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
            break;

              case 144:
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
            break;

              case 208:
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
            break;

              case 256:
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
            break;

              default:
            AssertFatal (0,
                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
                     RC.config_file_name, i, rach_messageSizeGroupA);
            break;
              }

              if (strcmp(rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
              }

              else if (strcmp(rach_messagePowerOffsetGroupB,"dB0")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
              }

              else if (strcmp(rach_messagePowerOffsetGroupB,"dB5")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
              }

              else if (strcmp(rach_messagePowerOffsetGroupB,"dB8")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
              }

              else if (strcmp(rach_messagePowerOffsetGroupB,"dB10")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
              }

              else if (strcmp(rach_messagePowerOffsetGroupB,"dB12")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
              }

              else if (strcmp(rach_messagePowerOffsetGroupB,"dB15")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
              }

              else if (strcmp(rach_messagePowerOffsetGroupB,"dB18")==0) {
                  RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
              }
              else
                  AssertFatal(0,
                              "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
                              RC.config_file_name, i, rach_messagePowerOffsetGroupB);

            }
            else if (strcmp(rach_preamblesGroupAConfig, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_preamblesGroupAConfig= FALSE;
            }
            else
                AssertFatal(0,
                            "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
                            RC.config_file_name, i, rach_preamblesGroupAConfig);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleInitialReceivedTargetPower= (rach_preambleInitialReceivedTargetPower+120)/2;

            if ((rach_preambleInitialReceivedTargetPower<-120) || (rach_preambleInitialReceivedTargetPower>-90) || ((rach_preambleInitialReceivedTargetPower&1)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                             RC.config_file_name, i, rach_preambleInitialReceivedTargetPower);


            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_powerRampingStep= rach_powerRampingStep/2;

            if ((rach_powerRampingStep<0) || (rach_powerRampingStep>6) || ((rach_powerRampingStep&1)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
                             RC.config_file_name, i, rach_powerRampingStep);



            switch (rach_preambleTransMax) {
#ifndef Rel14
            case 3:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n3;
              break;

            case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n4;
              break;

            case 5:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n5;
              break;

            case 6:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n6;
              break;

            case 7:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n7;
              break;

            case 8:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n8;
              break;

            case 10:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
              break;

            case 20:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n20;
              break;

            case 50:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n50;
              break;

            case 100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n100;
              break;

            case 200:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n200;
              break;

#else

            case 3:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n3;
              break;

            case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n4;
              break;

            case 5:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n5;
              break;

            case 6:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n6;
              break;

            case 7:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n7;
              break;

            case 8:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n8;
              break;

            case 10:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n10;
              break;

            case 20:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n20;
              break;

            case 50:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n50;
              break;

            case 100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n100;
              break;

            case 200:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n200;
              break;
#endif

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
                   RC.config_file_name, i, rach_preambleTransMax);
              break;
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_raResponseWindowSize=  (rach_raResponseWindowSize==10)?7:rach_raResponseWindowSize-2;

            if ((rach_raResponseWindowSize<0)||(rach_raResponseWindowSize==9)||(rach_raResponseWindowSize>10))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                   RC.config_file_name, i, rach_preambleTransMax);


            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_macContentionResolutionTimer= (rach_macContentionResolutionTimer/8)-1;

            if ((rach_macContentionResolutionTimer<8) || (rach_macContentionResolutionTimer>64) || ((rach_macContentionResolutionTimer&7)!=0))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                   RC.config_file_name, i, rach_preambleTransMax);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_maxHARQ_Msg3Tx= rach_maxHARQ_Msg3Tx;

            if ((rach_maxHARQ_Msg3Tx<0) || (rach_maxHARQ_Msg3Tx>8))
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
                   RC.config_file_name, i, rach_preambleTransMax);


            switch (pcch_defaultPagingCycle) {
            case 32:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf32;
              break;

            case 64:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf64;
              break;

            case 128:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf128;
              break;

            case 256:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf256;
              break;

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
                   RC.config_file_name, i, pcch_defaultPagingCycle);
              break;
            }

            if (strcmp(pcch_nB, "fourT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_fourT;
            }
            else if (strcmp(pcch_nB, "twoT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_twoT;
            }
            else if (strcmp(pcch_nB, "oneT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneT;
            }
            else if (strcmp(pcch_nB, "halfT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_halfT;
            }
            else if (strcmp(pcch_nB, "quarterT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_quarterT;
            }
            else if (strcmp(pcch_nB, "oneEighthT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneEighthT;
            }
            else if (strcmp(pcch_nB, "oneSixteenthT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneSixteenthT;
            }
            else if (strcmp(pcch_nB, "oneThirtySecondT") == 0) {
              RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneThirtySecondT;
            }
            else
              AssertFatal(0,
                  "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                  RC.config_file_name, i, pcch_defaultPagingCycle);



            switch (bcch_modificationPeriodCoeff) {
            case 2:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n2;
              break;

            case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n4;
              break;

            case 8:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n8;
              break;

            case 16:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n16;
              break;

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
                   RC.config_file_name, i, bcch_modificationPeriodCoeff);

              break;
            }


            switch (ue_TimersAndConstants_t300) {
            case 100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms100;
              break;

            case 200:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms200;
              break;

            case 300:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms300;
              break;

            case 400:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms400;
              break;

            case 600:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms600;
              break;

            case 1000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms1000;
              break;

            case 1500:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms1500;
              break;

            case 2000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms2000;
              break;

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t300 choice: 100,200,300,400,600,1000,1500,2000 ",
                   RC.config_file_name, i, ue_TimersAndConstants_t300);
              break;

            }

            switch (ue_TimersAndConstants_t301) {
            case 100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301 = UE_TimersAndConstants__t301_ms100;
              break;

            case 200:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301 = UE_TimersAndConstants__t301_ms200;
              break;

            case 300:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms300;
              break;

            case 400:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms400;
              break;

            case 600:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms600;
              break;

            case 1000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1000;
              break;

            case 1500:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1500;
              break;

            case 2000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms2000;
              break;

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t301 choice: 100,200,300,400,600,1000,1500,2000 ",
                   RC.config_file_name, i, ue_TimersAndConstants_t301);
              break;

            }

            switch (ue_TimersAndConstants_t310) {
            case 0:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms0;
              break;

            case 50:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms50;
              break;

            case 100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms100;
              break;

            case 200:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms200;
              break;

            case 500:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms500;
              break;

            case 1000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms1000;
              break;

            case 2000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310= UE_TimersAndConstants__t310_ms2000;
              break;

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t310 choice: 0,50,100,200,500,1000,1500,2000 ",
                   RC.config_file_name, i, ue_TimersAndConstants_t310);
              break;

            }

            switch (ue_TimersAndConstants_t311) {
            case 1000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms1000;
              break;

            case 3110:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms3000;
              break;

            case 5000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms5000;
              break;

            case 10000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms10000;
              break;

            case 15000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms15000;
              break;

            case 20000:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms20000;
              break;

            case 31100:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311= UE_TimersAndConstants__t311_ms30000;
              break;

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 choice: 1000,3000,5000,10000,150000,20000,30000",
                   RC.config_file_name, i, ue_TimersAndConstants_t311);
              break;

            }

            switch (ue_TimersAndConstants_n310) {
            case 1:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n1;
              break;

            case 2:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n2;
              break;

            case 3:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n3;
              break;

            case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n4;
              break;

            case 6:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n6;
              break;

            case 8:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n8;
              break;

            case 10:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n10;
              break;

            case 20:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= UE_TimersAndConstants__n310_n20;
              break;

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n310 choice: 1,2,3,4,6,6,8,10,20",
                   RC.config_file_name, i, ue_TimersAndConstants_n311);
              break;

            }

            switch (ue_TimersAndConstants_n311) {
            case 1:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n1;
              break;

            case 2:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n2;
              break;

            case 3:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n3;
              break;

            case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n4;
              break;

            case 5:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n5;
              break;

            case 6:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n6;
              break;

            case 8:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n8;
              break;

            case 10:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= UE_TimersAndConstants__n311_n10;
              break;

            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 choice: 1,2,3,4,5,6,8,10",
                   RC.config_file_name, i, ue_TimersAndConstants_t311);
              break;

            }

            switch (ue_TransmissionMode) {
            case 1:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm1;
              break;
            case 2:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm2;
              break;
            case 3:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm3;
              break;
            case 4:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm4;
              break;
            case 5:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm5;
              break;
            case 6:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm6;
              break;
            case 7:
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm7;
              break;
            default:
              AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
                   RC.config_file_name, i, ue_TransmissionMode);
              break;
            }

            int hyperSFN_r13;
            int eDRX_Allowed_r13;
            int q_RxLevMinCE_r13;
            int q_QualMinRSRQ_CE_r13;
            int si_WindowLength_BR_r13;
            int si_RepetitionPattern_r13;
            uint64_t fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
            int startSymbolBR_r13;
            int si_HoppingConfigCommon_r13;
            int si_ValidityTime_r13;
            int mpdcch_pdsch_HoppingNB_r13;
            int interval_DLHoppingConfigCommonModeA_r13_val;
            int interval_DLHoppingConfigCommonModeB_r13_val;
            int mpdcch_pdsch_HoppingOffset_r13;
            int preambleTransMax_CE_r13;
            int mpdcch_startSF_CSS_RA_r13_val;
            int prach_HoppingOffset_r13;
            int schedulingInfoSIB1_BR_r13;
            char* cellSelectionInfoCE_r13 = NULL;
            char* bandwidthReducedAccessRelatedInfo_r13 = NULL;
            char* fdd_DownlinkOrTddSubframeBitmapBR_r13 = NULL;
            char* fdd_UplinkSubframeBitmapBR_r13 = NULL;
            char* freqHoppingParametersDL_r13 = NULL;
            char* interval_DLHoppingConfigCommonModeA_r13 = NULL;
            char* interval_DLHoppingConfigCommonModeB_r13 = NULL;
            char* prach_ConfigCommon_v1310 = NULL;
            char* mpdcch_startSF_CSS_RA_r13;


            if (!config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_schedulingInfoSIB1, &schedulingInfoSIB1_BR_r13))
                AssertFatal(0,
                            "Failed to parse eNB configuration file %s, enb %d  schedulingInfoSIB1_BR_r13!\n",
                            RC.config_file_name, i);
            RRC_CONFIGURATION_REQ(msg_p).schedulingInfoSIB1_BR_r13[j]= schedulingInfoSIB1_BR_r13;


            if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_HYPERSFN, &hyperSFN_r13))
            {
                RRC_CONFIGURATION_REQ(msg_p).hyperSFN_r13[j]= calloc(1, sizeof(uint16_t));
                *RRC_CONFIGURATION_REQ(msg_p).hyperSFN_r13[j]= (uint16_t)hyperSFN_r13;
            }
            else
                RRC_CONFIGURATION_REQ(msg_p).hyperSFN_r13[j]= NULL;

            if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_EDRX_ALLOWED, &eDRX_Allowed_r13))
            {
                RRC_CONFIGURATION_REQ(msg_p).eDRX_Allowed_r13[j]= calloc(1, sizeof(long));
                *RRC_CONFIGURATION_REQ(msg_p).eDRX_Allowed_r13[j]= eDRX_Allowed_r13;
            }
            else
                RRC_CONFIGURATION_REQ(msg_p).eDRX_Allowed_r13[j]= NULL;

            if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_CELLSELECTIONINFOCE, &cellSelectionInfoCE_r13) && !strcmp(cellSelectionInfoCE_r13, "ENABLE"))
            {
                RRC_CONFIGURATION_REQ(msg_p).cellSelectionInfoCE_r13[j]= TRUE;
                if (!config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_Q_RXLEVMINCE, &q_RxLevMinCE_r13))
                {
                    AssertFatal(0,
                                "Failed to parse eNB configuration file %s, enb %d  q_RxLevMinCE_r13!\n",
                                RC.config_file_name, i);

                }
                RRC_CONFIGURATION_REQ(msg_p).q_RxLevMinCE_r13[j]= q_RxLevMinCE_r13;

                if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_Q_QUALMINRSRQ_CE, &q_QualMinRSRQ_CE_r13))
                {
                    RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[j]= calloc(1, sizeof(long));
                    *RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[j]= q_QualMinRSRQ_CE_r13;

                }
                else
                    RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[j]= NULL;

            }
            else
                RRC_CONFIGURATION_REQ(msg_p).cellSelectionInfoCE_r13[j]= FALSE;

            if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_BRAccessRelatedInfo, &bandwidthReducedAccessRelatedInfo_r13) && !strcmp(bandwidthReducedAccessRelatedInfo_r13, "ENABLE"))
            {
                RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[j] = TRUE;
                if (!config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_SI_WINDOWLENGTH_BR, &si_WindowLength_BR_r13) ||
                        !config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_SI_REPETITIONPATTERN, &si_RepetitionPattern_r13) ||
                        !config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_FDD_DLORTDD_SFB_BR, &fdd_DownlinkOrTddSubframeBitmapBR_r13) ||
                        !config_setting_lookup_int64(setting_br13, ENB_CONFIG_STRING_FDD_DLORTDDSFB_BR_VAL, &fdd_DownlinkOrTddSubframeBitmapBR_val_r13) ||
                        !config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_FDD_ULSUBFRAMEBITMAPBR, &fdd_UplinkSubframeBitmapBR_r13) ||
                        !config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_STARTSYMBOLBR, &startSymbolBR_r13) ||
                        !config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_SI_HOPPINGCONFIGCOMMON, &si_HoppingConfigCommon_r13))
                {
                    AssertFatal(0,
                                "Failed to parse eNB configuration file %s, enb %d  si_WindowLength_BR_r13, si_RepetitionPattern_r13, fdd_DownlinkOrTddSubframeBitmapBR_r13, fdd_UplinkSubframeBitmapBR_r13!\n",
                                RC.config_file_name, i);

                }
                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = si_WindowLength_BR_r13;
                RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = si_RepetitionPattern_r13;
                if (!strcmp(fdd_DownlinkOrTddSubframeBitmapBR_r13, "subframePattern40-r13"))
                    RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[j] = FALSE;
                else
                    RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[j] = TRUE;
                RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_val_r13[j] = fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
                RRC_CONFIGURATION_REQ(msg_p).fdd_UplinkSubframeBitmapBR_r13[j] = fdd_UplinkSubframeBitmapBR_r13;
                RRC_CONFIGURATION_REQ(msg_p).si_HoppingConfigCommon_r13[j] = si_HoppingConfigCommon_r13;
                RRC_CONFIGURATION_REQ(msg_p).startSymbolBR_r13[j] = startSymbolBR_r13;

                if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_Q_QUALMINRSRQ_CE, &si_ValidityTime_r13))
                {
                    RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[j] = calloc(1, sizeof(long));
                    *RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[j] = si_ValidityTime_r13;

                }
                else
                    RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[j] = NULL;

            }
            else
                RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[j] = FALSE;

            if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_FREQHOPPINGPARAMETERSDL, &freqHoppingParametersDL_r13) && !strcmp(freqHoppingParametersDL_r13, "ENABLE"))
            {
                RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[j] = TRUE;
                if (!config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB, &interval_DLHoppingConfigCommonModeA_r13) ||
                        !config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB_VAL, &interval_DLHoppingConfigCommonModeA_r13_val) ||
                        !config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB, &interval_DLHoppingConfigCommonModeB_r13) ||
                        !config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB_VAL, &interval_DLHoppingConfigCommonModeB_r13_val))
                {
                    AssertFatal(0,
                                "Failed to parse eNB configuration file %s, enb %d  si_WindowLength_BR_r13, si_RepetitionPattern_r13, fdd_DownlinkOrTddSubframeBitmapBR_r13, fdd_UplinkSubframeBitmapBR_r13!\n",
                                RC.config_file_name, i);

                }
                if (!strcmp(interval_DLHoppingConfigCommonModeA_r13, "interval-TDD-r13"))
                    RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13[j] = FALSE;
                else
                    RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13[j] = TRUE;
                RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13_val[j] = interval_DLHoppingConfigCommonModeA_r13_val;

                if (!strcmp(interval_DLHoppingConfigCommonModeB_r13, "interval-TDD-r13"))
                    RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13[j] = FALSE;
                else
                    RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13[j] = TRUE;
                RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13_val[j] = interval_DLHoppingConfigCommonModeB_r13_val;




            if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPINGNB, &mpdcch_pdsch_HoppingNB_r13))
            {
                RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = calloc(1, sizeof(long));
                *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = mpdcch_pdsch_HoppingNB_r13;

            }
            else
                RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = NULL;

            if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPINGOFFSET, &mpdcch_pdsch_HoppingOffset_r13))
            {
                RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[j] = calloc(1, sizeof(long));
                *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[j] = mpdcch_pdsch_HoppingOffset_r13;
            }
            else
                RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[j] = NULL;

            }
            else
                RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[j] = FALSE;

            /////SIB2 Parameters

            if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PREAMBLETRANSMAX_CE_R13, &preambleTransMax_CE_r13))
            {
                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = calloc(1, sizeof(long));
                *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = preambleTransMax_CE_r13;
            }
            else
                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = NULL;

            if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PRACH_CONFIGCOMMON_V1310, &prach_ConfigCommon_v1310) && !strcmp(prach_ConfigCommon_v1310, "ENABLE"))
            {
                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_ConfigCommon_v1310 = TRUE;

                if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_MPDCCH_STARTSF_CSS_RA_R13, &mpdcch_startSF_CSS_RA_r13))
                {
                    if (!config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_MPDCCH_STARTSF_CSS_RA_R13_VAL, &mpdcch_startSF_CSS_RA_r13_val))
                    {
                        AssertFatal(0,
                                    "Failed to parse eNB configuration file %s, enb %d mpdcch_startSF_CSS_RA_r13_val!\n",
                                    RC.config_file_name, i);

                    }
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13 = calloc(1, sizeof(BOOLEAN_t));
                    if (!strcmp(mpdcch_startSF_CSS_RA_r13, "tdd-r13"))
                        *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13 = FALSE;
                    else
                        *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13 = TRUE;

                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = mpdcch_startSF_CSS_RA_r13_val;

                }
                else
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13 = NULL;

                if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_HOPPINGOFFSET_R13, &prach_HoppingOffset_r13))
                {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_HoppingOffset_r13 = calloc(1, sizeof(long));
                    *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_HoppingOffset_r13 = prach_HoppingOffset_r13;

                }
                else
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_HoppingOffset_r13 = NULL;

            }
            else
                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_ConfigCommon_v1310 = TRUE;
        }
        else
        RRC_CONFIGURATION_REQ(msg_p).schedulingInfoSIB1_BR_r13[j] = 0;
      }
#endif
        }


        setting_srb1 = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_SRB1);

        if (setting_srb1 != NULL) {
          if (!(config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT, &srb1_timer_poll_retransmit)
            && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_TIMER_REORDERING,      &srb1_timer_reordering)
            && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT, &srb1_timer_status_prohibit)
            && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD,    &srb1_max_retx_threshold)
            && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_POLL_PDU,              &srb1_poll_pdu)
            && config_setting_lookup_int(setting_srb1, ENB_CONFIG_STRING_SRB1_POLL_BYTE,             &srb1_poll_byte)))
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d  timer_poll_retransmit, timer_reordering, "
                 "timer_status_prohibit, poll_pdu, poll_byte, max_retx_threshold !\n",
                 RC.config_file_name, i);

          switch (srb1_max_retx_threshold) {
          case 1:
        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t1;
        break;

          case 2:
        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t2;
        break;

          case 3:
        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t3;
        break;

          case 4:
        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t4;
        break;

          case 6:
        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t6;
        break;

          case 8:
        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t8;
        break;

          case 16:
        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t16;
        break;

          case 32:
        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t32;
        break;

          default:
        AssertFatal (0,
                 "Bad config value when parsing eNB configuration file %s, enb %d  srb1_max_retx_threshold %u!\n",
                 RC.config_file_name, i, srb1_max_retx_threshold);
          }


          switch (srb1_poll_pdu) {
          case 4:
        rrc->srb1_poll_pdu = PollPDU_p4;
        break;

          case 8:
        rrc->srb1_poll_pdu = PollPDU_p8;
        break;

          case 16:
        rrc->srb1_poll_pdu = PollPDU_p16;
        break;

          case 32:
        rrc->srb1_poll_pdu = PollPDU_p32;
        break;

          case 64:
        rrc->srb1_poll_pdu = PollPDU_p64;
        break;

          case 128:
        rrc->srb1_poll_pdu = PollPDU_p128;
        break;

          case 256:
        rrc->srb1_poll_pdu = PollPDU_p256;
        break;

          default:
        if (srb1_poll_pdu >= 10000)
          rrc->srb1_poll_pdu = PollPDU_pInfinity;
        else
          AssertFatal (0,
                   "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_pdu %u!\n",
                   RC.config_file_name, i, srb1_poll_pdu);
          }

          rrc->srb1_poll_byte             = srb1_poll_byte;

          switch (srb1_poll_byte) {
          case 25:
        rrc->srb1_poll_byte = PollByte_kB25;
        break;

          case 50:
        rrc->srb1_poll_byte = PollByte_kB50;
        break;

          case 75:
        rrc->srb1_poll_byte = PollByte_kB75;
        break;

          case 100:
        rrc->srb1_poll_byte = PollByte_kB100;
        break;

          case 125:
        rrc->srb1_poll_byte = PollByte_kB125;
        break;

          case 250:
        rrc->srb1_poll_byte = PollByte_kB250;
        break;

          case 375:
        rrc->srb1_poll_byte = PollByte_kB375;
        break;

          case 500:
        rrc->srb1_poll_byte = PollByte_kB500;
        break;

          case 750:
        rrc->srb1_poll_byte = PollByte_kB750;
        break;

          case 1000:
        rrc->srb1_poll_byte = PollByte_kB1000;
        break;

          case 1250:
        rrc->srb1_poll_byte = PollByte_kB1250;
        break;

          case 1500:
        rrc->srb1_poll_byte = PollByte_kB1500;
        break;

          case 2000:
        rrc->srb1_poll_byte = PollByte_kB2000;
        break;

          case 3000:
        rrc->srb1_poll_byte = PollByte_kB3000;
        break;

          default:
        if (srb1_poll_byte >= 10000)
          rrc->srb1_poll_byte = PollByte_kBinfinity;
        else
          AssertFatal (0,
                   "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_byte %u!\n",
                   RC.config_file_name, i, srb1_poll_byte);
          }

          if (srb1_timer_poll_retransmit <= 250) {
        rrc->srb1_timer_poll_retransmit = (srb1_timer_poll_retransmit - 5) / 5;
          }
          else if (srb1_timer_poll_retransmit <= 500) {
        rrc->srb1_timer_poll_retransmit = (srb1_timer_poll_retransmit - 300) / 50 + 50;
          }
          else {
        AssertFatal(0,
                "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_poll_retransmit %u!\n",
                RC.config_file_name, i, srb1_timer_poll_retransmit);
          }

          if (srb1_timer_status_prohibit <= 250) {
        rrc->srb1_timer_status_prohibit = srb1_timer_status_prohibit / 5;
          }
          else if ((srb1_timer_poll_retransmit >= 300) && (srb1_timer_poll_retransmit <= 500)) {
        rrc->srb1_timer_status_prohibit = (srb1_timer_status_prohibit - 300) / 50 + 51;
          }
          else {
        AssertFatal(0,
                "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_status_prohibit %u!\n",
                RC.config_file_name, i, srb1_timer_status_prohibit);
          }

          switch (srb1_timer_reordering) {
          case 0:
        rrc->srb1_timer_reordering = T_Reordering_ms0;
        break;

          case 5:
        rrc->srb1_timer_reordering = T_Reordering_ms5;
        break;

          case 10:
        rrc->srb1_timer_reordering = T_Reordering_ms10;
        break;

          case 15:
        rrc->srb1_timer_reordering = T_Reordering_ms15;
        break;

          case 20:
        rrc->srb1_timer_reordering = T_Reordering_ms20;
        break;

          case 25:
        rrc->srb1_timer_reordering = T_Reordering_ms25;
        break;

          case 30:
        rrc->srb1_timer_reordering = T_Reordering_ms30;
        break;

          case 35:
        rrc->srb1_timer_reordering = T_Reordering_ms35;
        break;

          case 40:
        rrc->srb1_timer_reordering = T_Reordering_ms40;
        break;

          case 45:
        rrc->srb1_timer_reordering = T_Reordering_ms45;
        break;

          case 50:
        rrc->srb1_timer_reordering = T_Reordering_ms50;
        break;

          case 55:
        rrc->srb1_timer_reordering = T_Reordering_ms55;
        break;

          case 60:
        rrc->srb1_timer_reordering = T_Reordering_ms60;
        break;

          case 65:
        rrc->srb1_timer_reordering = T_Reordering_ms65;
        break;

          case 70:
        rrc->srb1_timer_reordering = T_Reordering_ms70;
        break;

          case 75:
        rrc->srb1_timer_reordering = T_Reordering_ms75;
        break;

          case 80:
        rrc->srb1_timer_reordering = T_Reordering_ms80;
        break;

          case 85:
        rrc->srb1_timer_reordering = T_Reordering_ms85;
        break;

          case 90:
        rrc->srb1_timer_reordering = T_Reordering_ms90;
        break;

          case 95:
        rrc->srb1_timer_reordering = T_Reordering_ms95;
        break;

          case 100:
        rrc->srb1_timer_reordering = T_Reordering_ms100;
        break;

          case 110:
        rrc->srb1_timer_reordering = T_Reordering_ms110;
        break;

          case 120:
        rrc->srb1_timer_reordering = T_Reordering_ms120;
        break;

          case 130:
        rrc->srb1_timer_reordering = T_Reordering_ms130;
        break;

          case 140:
        rrc->srb1_timer_reordering = T_Reordering_ms140;
        break;

          case 150:
        rrc->srb1_timer_reordering = T_Reordering_ms150;
        break;

          case 160:
        rrc->srb1_timer_reordering = T_Reordering_ms160;
        break;

          case 170:
        rrc->srb1_timer_reordering = T_Reordering_ms170;
        break;

          case 180:
        rrc->srb1_timer_reordering = T_Reordering_ms180;
        break;

          case 190:
        rrc->srb1_timer_reordering = T_Reordering_ms190;
        break;

          case 200:
        rrc->srb1_timer_reordering = T_Reordering_ms200;
        break;

          default:
        AssertFatal (0,
                 "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_reordering %u!\n",
                 RC.config_file_name, i, srb1_timer_reordering);
          }

        }
        else {
          rrc->srb1_timer_poll_retransmit = T_PollRetransmit_ms80;
          rrc->srb1_timer_reordering = T_Reordering_ms35;
          rrc->srb1_timer_status_prohibit = T_StatusProhibit_ms0;
          rrc->srb1_poll_pdu = PollPDU_p4;
          rrc->srb1_poll_byte             = PollByte_kBinfinity;
          rrc->srb1_max_retx_threshold    = UL_AM_RLC__maxRetxThreshold_t8;
        }
        /*
        // Network Controller
        subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_NETWORK_CONTROLLER_CONFIG);

        if (subsetting != NULL) {
        if (  (
        config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_FLEXRAN_AGENT_INTERFACE_NAME,
        (const char **)&flexran_agent_interface_name)
        && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_FLEXRAN_AGENT_IPV4_ADDRESS,
        (const char **)&flexran_agent_ipv4_address)
        && config_setting_lookup_int(subsetting, ENB_CONFIG_STRING_FLEXRAN_AGENT_PORT,
        &flexran_agent_port)
        && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_FLEXRAN_AGENT_CACHE,
        (const char **)&flexran_agent_cache)
        )
        ) {
        enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_interface_name = strdup(flexran_agent_interface_name);
        cidr = flexran_agent_ipv4_address;
        address = strtok(cidr, "/");
        //enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_ipv4_address = strdup(address);
        if (address) {
        IPV4_STR_ADDR_TO_INT_NWBO (address, enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_ipv4_address, "BAD IP ADDRESS FORMAT FOR eNB Agent !\n" );
        }

        enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_port = flexran_agent_port;
        enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_cache = strdup(flexran_agent_cache);
        }
        }
        */

        /*
        // OTG _CONFIG
        setting_otg = config_setting_get_member (setting_enb, ENB_CONF_STRING_OTG_CONFIG);

        if (setting_otg != NULL) {
        num_otg_elements  = config_setting_length(setting_otg);
        printf("num otg elements %d \n", num_otg_elements);
        enb_properties_loc.properties[enb_properties_loc_index]->num_otg_elements = 0;

        for (j = 0; j < num_otg_elements; j++) {
        subsetting_otg=config_setting_get_elem(setting_otg, j);

        if (config_setting_lookup_int(subsetting_otg, ENB_CONF_STRING_OTG_UE_ID, &otg_ue_id)) {
        enb_properties_loc.properties[enb_properties_loc_index]->otg_ue_id[j] = otg_ue_id;
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->otg_ue_id[j] = 1;
        }

        if (config_setting_lookup_string(subsetting_otg, ENB_CONF_STRING_OTG_APP_TYPE, (const char **)&otg_app_type)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->otg_app_type[j] = map_str_to_int(otg_app_type_names,otg_app_type))== -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->otg_app_type[j] = BCBR;
        }
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->otg_app_type[j] = NO_PREDEFINED_TRAFFIC; // 0
        }

        if (config_setting_lookup_string(subsetting_otg, ENB_CONF_STRING_OTG_BG_TRAFFIC, (const char **)&otg_bg_traffic)) {

        if ((enb_properties_loc.properties[enb_properties_loc_index]->otg_bg_traffic[j] = map_str_to_int(switch_names,otg_bg_traffic)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->otg_bg_traffic[j]=0;
        }
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->otg_bg_traffic[j] = 0;
        printf("otg bg %s\n", otg_bg_traffic);
        }

        enb_properties_loc.properties[enb_properties_loc_index]->num_otg_elements+=1;

        }
        }

        // log_config
        subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_LOG_CONFIG);

        if (subsetting != NULL) {
        // global
        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GLOBAL_LOG_LEVEL, (const char **)  &glog_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->glog_level = map_str_to_int(log_level_names, glog_level)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->glog_level = LOG_INFO;
        }

        //printf( "\tGlobal log level :\t%s->%d\n",glog_level, enb_properties_loc.properties[enb_properties_loc_index]->glog_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->glog_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GLOBAL_LOG_VERBOSITY,(const char **)  &glog_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity = map_str_to_int(log_verbosity_names, glog_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity = LOG_MED;
        }

        //printf( "\tGlobal log verbosity:\t%s->%d\n",glog_verbosity, enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity = LOG_MED;
        }

        // HW
        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_HW_LOG_LEVEL, (const char **) &hw_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level = map_str_to_int(log_level_names,hw_log_level)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level = LOG_INFO;
        }

        //printf( "\tHW log level :\t%s->%d\n",hw_log_level,enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_HW_LOG_VERBOSITY, (const char **) &hw_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity = map_str_to_int(log_verbosity_names,hw_log_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity = LOG_MED;
        }

        //printf( "\tHW log verbosity:\t%s->%d\n",hw_log_verbosity, enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity = LOG_MED;
        }

        // phy
        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PHY_LOG_LEVEL,(const char **) &phy_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level = map_str_to_int(log_level_names,phy_log_level)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level = LOG_INFO;
        }

        //printf( "\tPHY log level :\t%s->%d\n",phy_log_level,enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PHY_LOG_VERBOSITY, (const char **)&phy_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity = map_str_to_int(log_verbosity_names,phy_log_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity = LOG_MED;
        }

        //printf( "\tPHY log verbosity:\t%s->%d\n",phy_log_level,enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity = LOG_MED;
        }

        //mac
        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_MAC_LOG_LEVEL, (const char **)&mac_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level = map_str_to_int(log_level_names,mac_log_level)) == -1 ) {
        enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level = LOG_INFO;
        }

        //printf( "\tMAC log level :\t%s->%d\n",mac_log_level,enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_MAC_LOG_VERBOSITY, (const char **)&mac_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity = map_str_to_int(log_verbosity_names,mac_log_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity = LOG_MED;
        }

        //printf( "\tMAC log verbosity:\t%s->%d\n",mac_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity = LOG_MED;
        }

        //rlc
        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RLC_LOG_LEVEL, (const char **)&rlc_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level = map_str_to_int(log_level_names,rlc_log_level)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level = LOG_INFO;
        }

        //printf( "\tRLC log level :\t%s->%d\n",rlc_log_level, enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RLC_LOG_VERBOSITY, (const char **)&rlc_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity = map_str_to_int(log_verbosity_names,rlc_log_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity = LOG_MED;
        }

        //printf( "\tRLC log verbosity:\t%s->%d\n",rlc_log_verbosity, enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity = LOG_MED;
        }

        //pdcp
        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PDCP_LOG_LEVEL, (const char **)&pdcp_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level = map_str_to_int(log_level_names,pdcp_log_level)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level = LOG_INFO;
        }

        //printf( "\tPDCP log level :\t%s->%d\n",pdcp_log_level, enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PDCP_LOG_VERBOSITY, (const char **)&pdcp_log_verbosity)) {
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_verbosity = map_str_to_int(log_verbosity_names,pdcp_log_verbosity);
        //printf( "\tPDCP log verbosity:\t%s->%d\n",pdcp_log_verbosity, enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_verbosity = LOG_MED;
        }

        //rrc
        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RRC_LOG_LEVEL, (const char **)&rrc_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level = map_str_to_int(log_level_names,rrc_log_level)) == -1 ) {
        enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level = LOG_INFO;
        }

        //printf( "\tRRC log level :\t%s->%d\n",rrc_log_level,enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RRC_LOG_VERBOSITY, (const char **)&rrc_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity = map_str_to_int(log_verbosity_names,rrc_log_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity = LOG_MED;
        }

        //printf( "\tRRC log verbosity:\t%s->%d\n",rrc_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity = LOG_MED;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GTPU_LOG_LEVEL, (const char **)&gtpu_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level = map_str_to_int(log_level_names,gtpu_log_level)) == -1 ) {
        enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level = LOG_INFO;
        }

        //printf( "\tGTPU log level :\t%s->%d\n",gtpu_log_level,enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GTPU_LOG_VERBOSITY, (const char **)&gtpu_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity = map_str_to_int(log_verbosity_names,gtpu_log_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity = LOG_MED;
        }

        //printf( "\tGTPU log verbosity:\t%s->%d\n",gtpu_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity = LOG_MED;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_UDP_LOG_LEVEL, (const char **)&udp_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level = map_str_to_int(log_level_names,udp_log_level)) == -1 ) {
        enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level = LOG_INFO;
        }

        //printf( "\tUDP log level :\t%s->%d\n",udp_log_level,enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_UDP_LOG_VERBOSITY, (const char **)&udp_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->udp_log_verbosity = map_str_to_int(log_verbosity_names,udp_log_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->udp_log_verbosity = LOG_MED;
        }

        //printf( "\tUDP log verbosity:\t%s->%d\n",udp_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->udp_log_verbosity = LOG_MED;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_OSA_LOG_LEVEL, (const char **)&osa_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level = map_str_to_int(log_level_names,osa_log_level)) == -1 ) {
        enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level = LOG_INFO;
        }

        //printf( "\tOSA log level :\t%s->%d\n",osa_log_level,enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level = LOG_INFO;
        }

        if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_OSA_LOG_VERBOSITY, (const char **)&osa_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->osa_log_verbosity = map_str_to_int(log_verbosity_names,osa_log_verbosity)) == -1) {
        enb_properties_loc.properties[enb_properties_loc_index]->osa_log_verbosity = LOG_MED;
        }

        //printf( "\tOSA log verbosity:\t%s->%d\n",osa_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->gosa_log_verbosity);
        } else {
        enb_properties_loc.properties[enb_properties_loc_index]->osa_log_verbosity = LOG_MED;
        }

        } else { // not configuration is given
        enb_properties_loc.properties[enb_properties_loc_index]->glog_level         = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity     = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level       = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity   = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level      = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity  = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level      = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity  = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level      = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity  = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level     = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_verbosity = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level      = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity  = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level     = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level      = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->udp_log_verbosity  = LOG_MED;
        enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level      = LOG_INFO;
        enb_properties_loc.properties[enb_properties_loc_index]->osa_log_verbosity  = LOG_MED;
        }
        */
        break;
      }
    }
      }
    }
  }

  int RCconfig_gtpu() {
    config_t          cfg;
    config_setting_t *setting                       = NULL;
    config_setting_t *subsetting                    = NULL;
    config_setting_t *setting_enb                   = NULL;
    int               num_enbs                      = 0;
    libconfig_int     enb_id                        = 0;



    char*             enb_interface_name_for_S1U    = NULL;
    char*             enb_ipv4_address_for_S1U      = NULL;
    libconfig_int     enb_port_for_S1U              = 0;
    char             *address                       = NULL;
    char             *cidr                          = NULL;
    char             *astring                       = NULL;

    // for no gcc warnings
    (void)astring;


    LOG_I(GTPU,"Configuring GTPu\n");

    config_init(&cfg);

    if (RC.config_file_name != NULL) {
      // Read the file. If there is an error, report it and exit.
      if (! config_read_file(&cfg, RC.config_file_name)) {
    config_destroy(&cfg);
    AssertFatal (0, "Failed to parse eNB configuration file %s!\n", RC.config_file_name);
      }
    } else {
      config_destroy(&cfg);
      AssertFatal (0, "No eNB configuration file provided!\n");
    }

    // Get list of active eNBs, (only these will be configured)
    setting = config_lookup(&cfg, ENB_CONFIG_STRING_ACTIVE_ENBS);

    if (setting != NULL) {
      num_enbs = config_setting_length(setting);
      setting_enb   = config_setting_get_elem(setting, 0);
    }

    if (num_enbs>0) {
      // Output a list of all eNBs.
      setting = config_lookup(&cfg, ENB_CONFIG_STRING_ENB_LIST);

      if (setting != NULL) {


    setting_enb = config_setting_get_elem(setting, 0);
    subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

    if (subsetting != NULL) {
      if (  (config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U,
                           (const char **)&enb_interface_name_for_S1U)
         && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U,
                          (const char **)&enb_ipv4_address_for_S1U)
         && config_setting_lookup_int(subsetting, ENB_CONFIG_STRING_ENB_PORT_FOR_S1U,
                          &enb_port_for_S1U)
         )
        ) {

        cidr = enb_ipv4_address_for_S1U;
        address = strtok(cidr, "/");

        if (address) {
          IPV4_STR_ADDR_TO_INT_NWBO ( address, RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );

          LOG_I(GTPU,"Configuring GTPu address : %s -> %x\n",address,RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up);

        }

        RC.gtpv1u_data_g->enb_port_for_S1u_S12_S4_up = enb_port_for_S1U;

      }
    }
      }
    }
  }


  int RCconfig_S1(MessageDef *msg_p, uint32_t i) {
    config_t          cfg;
    config_setting_t *setting                       = NULL;
    config_setting_t *subsetting                    = NULL;
    config_setting_t *setting_mme_addresses         = NULL;
    config_setting_t *setting_mme_address           = NULL;
    config_setting_t *setting_enb                   = NULL;
    config_setting_t *setting_otg                   = NULL;
    config_setting_t *subsetting_otg                = NULL;
    int               parse_errors                  = 0;
    int               num_enbs                      = 0;
    int               num_mme_address               = 0;
    int               num_otg_elements              = 0;
    int               num_component_carriers        = 0;
    int               j                             = 0;
    libconfig_int     enb_id                        = 0;


    const char*       cell_type                     = NULL;
    const char*       tac                           = 0;
    const char*       enb_name                      = NULL;
    const char*       mcc                           = 0;
    const char*       mnc                           = 0;

    libconfig_int     my_int;


    char*             if_name                       = NULL;
    char*             ipv4                          = NULL;
    char*             ipv4_remote                   = NULL;
    char*             ipv6                          = NULL;
    char*             local_rf                      = NULL;
    char*             preference                    = NULL;
    char*             active                        = NULL;

    char*             tr_preference                 = NULL;
    libconfig_int     local_port                    = 0;
    libconfig_int     remote_port                   = 0;
    const char*       active_enb[MAX_ENB];
    char*             enb_interface_name_for_S1U    = NULL;
    char*             enb_ipv4_address_for_S1U      = NULL;
    libconfig_int     enb_port_for_S1U              = 0;
    char*             enb_interface_name_for_S1_MME = NULL;
    char*             enb_ipv4_address_for_S1_MME   = NULL;
    char             *address                       = NULL;
    char             *cidr                          = NULL;
    char             *astring                       = NULL;

    // for no gcc warnings
    (void)astring;
    (void)my_int;

    memset((char*)active_enb,     0 , MAX_ENB * sizeof(char*));

    config_init(&cfg);

    if (RC.config_file_name != NULL) {
      // Read the file. If there is an error, report it and exit.
      if (! config_read_file(&cfg, RC.config_file_name)) {
    config_destroy(&cfg);
    AssertFatal (0, "Failed to parse eNB configuration file %s!\n", RC.config_file_name);
      }
    } else {
      config_destroy(&cfg);
      AssertFatal (0, "No eNB configuration file provided!\n");
    }

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)

    if (  (config_lookup_string( &cfg, ENB_CONFIG_STRING_ASN1_VERBOSITY, (const char **)&astring) )) {
      if (strcasecmp(astring , ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0) {
    asn_debug      = 0;
    asn1_xer_print = 0;
      } else if (strcasecmp(astring , ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0) {
    asn_debug      = 1;
    asn1_xer_print = 1;
      } else if (strcasecmp(astring , ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0) {
    asn_debug      = 1;
    asn1_xer_print = 2;
      } else {
    asn_debug      = 0;
    asn1_xer_print = 0;
      }
    }

#endif

    // Get list of active eNBs, (only these will be configured)
    setting = config_lookup(&cfg, ENB_CONFIG_STRING_ACTIVE_ENBS);

    if (setting != NULL) {
      num_enbs = config_setting_length(setting);
      setting_enb   = config_setting_get_elem(setting, i);
      active_enb[i] = config_setting_get_string (setting_enb);
      AssertFatal (active_enb[i] != NULL,
           "Failed to parse config file %s, %uth attribute %s \n",
           RC.config_file_name, i, ENB_CONFIG_STRING_ACTIVE_ENBS);
      active_enb[i] = strdup(active_enb[i]);
    }


    if (num_enbs>0) {
      // Output a list of all eNBs.
      setting = config_lookup(&cfg, ENB_CONFIG_STRING_ENB_LIST);

      if (setting != NULL) {
    num_enbs = config_setting_length(setting);

    for (i = 0; i < num_enbs; i++) {
      setting_enb = config_setting_get_elem(setting, i);

      if (! config_setting_lookup_int(setting_enb, ENB_CONFIG_STRING_ENB_ID, &enb_id)) {
        // Calculate a default eNB ID

# if defined(ENABLE_USE_MME)
        uint32_t hash;

        hash = s1ap_generate_eNB_id ();
        enb_id = i + (hash & 0xFFFF8);
# else
        enb_id = i;
# endif
      }

      if (  !(       config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_CELL_TYPE,           &cell_type)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_ENB_NAME,            &enb_name)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_TRACKING_AREA_CODE,  &tac)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE, &mcc)
             && config_setting_lookup_string(setting_enb, ENB_CONFIG_STRING_MOBILE_NETWORK_CODE, &mnc)


             )
        ) {
        AssertFatal (0,
             "Failed to parse eNB configuration file %s, %u th enb\n",
             RC.config_file_name, i);
        continue; // FIXME this prevents segfaults below, not sure what happens after function exit
      }

      // search if in active list
      for (j=0; j < num_enbs; j++) {
        if (strcmp(active_enb[j], enb_name) == 0) {

          S1AP_REGISTER_ENB_REQ (msg_p).eNB_id = enb_id;

          if (strcmp(cell_type, "CELL_MACRO_ENB") == 0) {
        S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_MACRO_ENB;
          } else  if (strcmp(cell_type, "CELL_HOME_ENB") == 0) {
        S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_HOME_ENB;
          } else {
        AssertFatal (0,
                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                 RC.config_file_name, i, cell_type);
          }

          S1AP_REGISTER_ENB_REQ (msg_p).eNB_name         = strdup(enb_name);
          S1AP_REGISTER_ENB_REQ (msg_p).tac              = (uint16_t)atoi(tac);
          S1AP_REGISTER_ENB_REQ (msg_p).mcc              = (uint16_t)atoi(mcc);
          S1AP_REGISTER_ENB_REQ (msg_p).mnc              = (uint16_t)atoi(mnc);
          S1AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length = strlen(mnc);
          S1AP_REGISTER_ENB_REQ (msg_p).default_drx      = 0;
          AssertFatal((S1AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length == 2) ||
              (S1AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length == 3),
              "BAD MNC DIGIT LENGTH %d",
              S1AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length);



          setting_mme_addresses = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_MME_IP_ADDRESS);
          num_mme_address     = config_setting_length(setting_mme_addresses);
          S1AP_REGISTER_ENB_REQ (msg_p).nb_mme = 0;

          for (j = 0; j < num_mme_address; j++) {
        setting_mme_address = config_setting_get_elem(setting_mme_addresses, j);

        if (  !(
            config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IPV4_ADDRESS, (const char **)&ipv4)
            && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IPV6_ADDRESS, (const char **)&ipv6)
            && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE, (const char **)&active)
            && config_setting_lookup_string(setting_mme_address, ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE, (const char **)&preference)
            )
              ) {
          AssertFatal (0,
                   "Failed to parse eNB configuration file %s, %u th enb %u th mme address !\n",
                   RC.config_file_name, i, j);
          continue; // FIXME will prevent segfaults below, not sure what happens at function exit...
        }

        S1AP_REGISTER_ENB_REQ (msg_p).nb_mme += 1;

        strcpy(S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv4_address,ipv4);
        strcpy(S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv6_address,ipv6);

        if (strcmp(active, "yes") == 0) {
#if defined(ENABLE_USE_MME)
          EPC_MODE_ENABLED = 1;
#endif
        }
        if (strcmp(preference, "ipv4") == 0) {
          S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv4 = 1;
        } else if (strcmp(preference, "ipv6") == 0) {
          S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv6 = 1;
        } else if (strcmp(preference, "no") == 0) {
          S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv4 = 1;
          S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv6 = 1;
        }
          }


          // SCTP SETTING
          S1AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = SCTP_OUT_STREAMS;
          S1AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams  = SCTP_IN_STREAMS;
# if defined(ENABLE_USE_MME)
          subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_SCTP_CONFIG);

          if (subsetting != NULL) {
        if ( (config_setting_lookup_int( subsetting, ENB_CONFIG_STRING_SCTP_INSTREAMS, &my_int) )) {
          S1AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams = (uint16_t)my_int;
        }

        if ( (config_setting_lookup_int( subsetting, ENB_CONFIG_STRING_SCTP_OUTSTREAMS, &my_int) )) {
          S1AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = (uint16_t)my_int;
        }
          }
#endif

          // NETWORK_INTERFACES
          subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);

          if (subsetting != NULL) {
        if (  (
               config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME,
                             (const char **)&enb_interface_name_for_S1_MME)
               && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME,
                            (const char **)&enb_ipv4_address_for_S1_MME)
               && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U,
                            (const char **)&enb_interface_name_for_S1U)
               && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U,
                            (const char **)&enb_ipv4_address_for_S1U)
               && config_setting_lookup_int(subsetting, ENB_CONFIG_STRING_ENB_PORT_FOR_S1U,
                            &enb_port_for_S1U)
               )
              ) {
          //		S1AP_REGISTER_ENB_REQ (msg_p).enb_interface_name_for_S1U = strdup(enb_interface_name_for_S1U);
          cidr = enb_ipv4_address_for_S1U;
          address = strtok(cidr, "/");

          S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv6 = 0;
          S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv4 = 1;

          strcpy(S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv4_address, address);

          /*
            in_addr_t  ipv4_address;

            if (address) {
            IPV4_STR_ADDR_TO_INT_NWBO ( address, ipv4_address, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
            }
            strcpy(S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv4_address, inet_ntoa(ipv4_address));
            //		S1AP_REGISTER_ENB_REQ (msg_p).enb_port_for_S1U = enb_port_for_S1U;


            S1AP_REGISTER_ENB_REQ (msg_p).enb_interface_name_for_S1_MME = strdup(enb_interface_name_for_S1_MME);
            cidr = enb_ipv4_address_for_S1_MME;
            address = strtok(cidr, "/");

            if (address) {
            IPV4_STR_ADDR_TO_INT_NWBO ( address, S1AP_REGISTER_ENB_REQ(msg_p).enb_ipv4_address_for_S1_MME, "BAD IP ADDRESS FORMAT FOR eNB S1_MME !\n" );
            }
          */
        }
          }




          break;
        }
      }
    }
      }
    }
  }

  void RCConfig(const char *config_file_name) {

    config_t          cfg;
    config_setting_t *setting                       = NULL;
    config_setting_t *setting_enb                   = NULL;
    config_setting_t *setting_component_carriers    = NULL;
    config_init(&cfg);

    if (config_file_name != NULL) {

      RC.config_file_name = config_file_name;
      if (! config_read_file(&cfg, RC.config_file_name)) {
    config_destroy(&cfg);
    AssertFatal (0, "Failed to parse eNB configuration file %s!\n", RC.config_file_name);
      }
      // Get num eNB instances
      setting = config_lookup(&cfg, ENB_CONFIG_STRING_ACTIVE_ENBS);

      if (setting != NULL) RC.nb_inst = config_setting_length(setting);
      if (RC.nb_inst > 0) {
    printf("Number of eNB RRC instances %d\n",RC.nb_inst);
    setting = config_lookup(&cfg, ENB_CONFIG_STRING_ENB_LIST);
    RC.nb_CC = (int *)malloc((1+RC.nb_inst)*sizeof(int));
    for (int i=0;i<RC.nb_inst;i++) {
      setting_enb                  = config_setting_get_elem(setting, i);
      setting_component_carriers   = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_COMPONENT_CARRIERS);
      AssertFatal(setting_component_carriers != NULL, "No component carrier definitions in %s\n",RC.config_file_name);
      RC.nb_CC[i]                = config_setting_length(setting_component_carriers);
      printf("Setting nb_CC to %d for instance %d\n",RC.nb_CC[i],i);

    }
      }

      // Get num MACRLC instances
      setting = config_lookup(&cfg, CONFIG_STRING_MACRLC_LIST);
      if (setting != NULL) RC.nb_macrlc_inst = config_setting_length(setting);

      // Get num L1 instances
      setting = config_lookup(&cfg, CONFIG_STRING_L1_LIST);
      if (setting != NULL) RC.nb_L1_inst = config_setting_length(setting);

      // Get num RU instances
      setting = config_lookup(&cfg, CONFIG_STRING_RU_LIST);
      if (setting != NULL) RC.nb_RU     = config_setting_length(setting);
    }
    else {
      config_destroy(&cfg);
      AssertFatal(0,"Configuration file is null\n");
    }

  }
