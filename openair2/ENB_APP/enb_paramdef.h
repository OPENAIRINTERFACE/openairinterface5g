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

/*! \file openair2/ENB_APP/enb_paramdef.h
 * \brief definition of configuration parameters for all eNodeB modules
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#ifndef ENB_PARAMDEF_H_
#define ENB_PARAMDEF_H_
#include "common/config/config_paramdesc.h"
#include "RRC_paramsvalues.h"
#include "s1ap_eNB_default_values.h"

#define ENB_CONFIG_STRING_CC_NODE_FUNCTION  "node_function"
#define ENB_CONFIG_STRING_CC_NODE_TIMING    "node_timing"
#define ENB_CONFIG_STRING_CC_NODE_SYNCH_REF "node_synch_ref"

// OTG config per ENB-UE DL
#define ENB_CONF_STRING_OTG_CONFIG          "otg_config"
#define ENB_CONF_STRING_OTG_UE_ID           "ue_id"
#define ENB_CONF_STRING_OTG_APP_TYPE        "app_type"
#define ENB_CONF_STRING_OTG_BG_TRAFFIC      "bg_traffic"

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


#define CONFIG_STRING_ACTIVE_RUS                   "Active_RUs"
/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*    RUs  configuration section name */
#define CONFIG_STRING_RU_LIST                      "RUs"
#define CONFIG_STRING_RU_CONFIG                   "ru_config"

/*    RUs configuration parameters name   */
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
#define CONFIG_STRING_RU_IS_SLAVE                 "is_slave"
#define CONFIG_STRING_RU_NBIOTRRC_LIST            "NbIoT_RRC_instances"
#define CONFIG_STRING_RU_SDR_ADDRS                "sdr_addrs"
#define CONFIG_STRING_RU_SDR_CLK_SRC              "clock_src"
#define CONFIG_STRING_RU_SDR_TME_SRC              "time_src"
#define CONFIG_STRING_RU_SF_EXTENSION             "sf_extension"
#define CONFIG_STRING_RU_END_OF_BURST_DELAY       "end_of_burst_delay"
#define CONFIG_STRING_RU_OTA_SYNC_ENABLE          "ota_sync_enabled"
#define CONFIG_STRING_RU_BF_WEIGHTS_LIST          "bf_weights"
#define CONFIG_STRING_RU_IF_FREQUENCY             "if_freq"
#define CONFIG_STRING_RU_IF_FREQ_OFFSET           "if_offset"
#define CONFIG_STRING_RU_DO_PRECODING             "do_precoding"

#define RU_LOCAL_IF_NAME_IDX          0
#define RU_LOCAL_ADDRESS_IDX          1
#define RU_REMOTE_ADDRESS_IDX         2
#define RU_LOCAL_PORTC_IDX            3
#define RU_REMOTE_PORTC_IDX           4
#define RU_LOCAL_PORTD_IDX            5
#define RU_REMOTE_PORTD_IDX           6
#define RU_TRANSPORT_PREFERENCE_IDX   7
#define RU_LOCAL_RF_IDX               8
#define RU_NB_TX_IDX                  9
#define RU_NB_RX_IDX                  10
#define RU_MAX_RS_EPRE_IDX            11
#define RU_MAX_RXGAIN_IDX             12
#define RU_BAND_LIST_IDX              13
#define RU_ENB_LIST_IDX               14
#define RU_ATT_TX_IDX                 15
#define RU_ATT_RX_IDX                 16
#define RU_IS_SLAVE_IDX               17
#define RU_NBIOTRRC_LIST_IDX          18
#define RU_SDR_ADDRS                  19
#define RU_SDR_CLK_SRC                20
#define RU_SDR_TME_SRC                21
#define RU_SF_EXTENSION_IDX           22
#define RU_END_OF_BURST_DELAY_IDX     23
#define RU_OTA_SYNC_ENABLE_IDX        24
#define RU_BF_WEIGHTS_LIST_IDX        25
#define RU_IF_FREQUENCY               26
#define RU_IF_FREQ_OFFSET             27
#define RU_DO_PRECODING               28
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            RU configuration parameters                                                                  */
/*   optname                                   helpstr   paramflags    XXXptr          defXXXval                   type      numelt        */
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
#define RUPARAMS_DESC { \
    {CONFIG_STRING_RU_LOCAL_IF_NAME,               NULL,       0,       strptr:NULL,     defstrval:"lo",          TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_LOCAL_ADDRESS,               NULL,       0,       strptr:NULL,     defstrval:"127.0.0.2",   TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_REMOTE_ADDRESS,              NULL,       0,       strptr:NULL,     defstrval:"127.0.0.1",   TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_LOCAL_PORTC,                 NULL,       0,       uptr:NULL,       defuintval:50000,        TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_REMOTE_PORTC,                NULL,       0,       uptr:NULL,       defuintval:50000,        TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_LOCAL_PORTD,                 NULL,       0,       uptr:NULL,       defuintval:50001,        TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_REMOTE_PORTD,                NULL,       0,       uptr:NULL,       defuintval:50001,        TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_TRANSPORT_PREFERENCE,        NULL,       0,       strptr:NULL,     defstrval:"udp_if5",     TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_LOCAL_RF,                    NULL,       0,       strptr:NULL,     defstrval:"yes",         TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_NB_TX,                       NULL,       0,       uptr:NULL,       defuintval:1,            TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_NB_RX,                       NULL,       0,       uptr:NULL,       defuintval:1,            TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_MAX_RS_EPRE,                 NULL,       0,       iptr:NULL,       defintval:-29,           TYPE_INT,         0}, \
    {CONFIG_STRING_RU_MAX_RXGAIN,                  NULL,       0,       iptr:NULL,       defintval:120,           TYPE_INT,         0}, \
    {CONFIG_STRING_RU_BAND_LIST,                   NULL,       0,       uptr:NULL,       defintarrayval:DEFBANDS, TYPE_INTARRAY,    1}, \
    {CONFIG_STRING_RU_ENB_LIST,                    NULL,       0,       uptr:NULL,       defintarrayval:DEFENBS,  TYPE_INTARRAY,    1}, \
    {CONFIG_STRING_RU_ATT_TX,                      NULL,       0,       uptr:NULL,       defintval:0,             TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_ATT_RX,                      NULL,       0,       uptr:NULL,       defintval:0,             TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_IS_SLAVE,                    NULL,       0,       strptr:NULL,     defstrval:"no",          TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_NBIOTRRC_LIST,               NULL,       0,       uptr:NULL,       defintarrayval:DEFENBS,  TYPE_INTARRAY,    1}, \
    {CONFIG_STRING_RU_SDR_ADDRS,                   NULL,       0,       strptr:NULL,     defstrval:"type=b200",   TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_SDR_CLK_SRC,                 NULL,       0,       strptr:NULL,     defstrval:"internal",    TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_SDR_TME_SRC,                 NULL,       0,       strptr:NULL,     defstrval:"internal",    TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_SF_EXTENSION,                NULL,       0,       uptr:NULL,       defuintval:312,          TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_END_OF_BURST_DELAY,          NULL,       0,       uptr:NULL,       defuintval:400,          TYPE_UINT,        0}, \
    {CONFIG_STRING_RU_OTA_SYNC_ENABLE,             NULL,       0,       strptr:NULL,     defstrval:"no",          TYPE_STRING,      0}, \
    {CONFIG_STRING_RU_BF_WEIGHTS_LIST,             NULL,       0,       iptr:NULL,       defintarrayval:DEFBFW,   TYPE_INTARRAY,    0}, \
    {CONFIG_STRING_RU_IF_FREQUENCY,                NULL,       0,       u64ptr:NULL,     defuintval:0,            TYPE_UINT64,      0}, \
    {CONFIG_STRING_RU_IF_FREQ_OFFSET,              NULL,       0,       iptr:NULL,       defintval:0,             TYPE_INT,         0}, \
    {CONFIG_STRING_RU_DO_PRECODING,                NULL,       0,       iptr:NULL,       defintval:0,             TYPE_INT,         0}, \
  }

/*---------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------------------------------------*/
/* value definitions for ASN1 verbosity parameter */
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE              "none"
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING          "annoying"
#define ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO              "info"


/* global parameters, not under a specific section   */
#define ENB_CONFIG_STRING_ASN1_VERBOSITY           "Asn1_verbosity"
#define ENB_CONFIG_STRING_ACTIVE_ENBS              "Active_eNBs"
#define ENB_CONFIG_STRING_NOS1                     "noS1"
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            global configuration parameters                                                                                   */
/*   optname                          helpstr      paramflags          XXXptr        defXXXval                                        type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define ENBSPARAMS_DESC {                                                                                             \
    {ENB_CONFIG_STRING_ASN1_VERBOSITY,      NULL,     0,                   uptr:NULL,   defstrval:ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE,   TYPE_STRING,      0},   \
    {ENB_CONFIG_STRING_ACTIVE_ENBS,         NULL,     0,                   uptr:NULL,   defstrval:NULL,                TYPE_STRINGLIST,  0},   \
    {ENB_CONFIG_STRING_NOS1,                NULL,     PARAMFLAG_BOOL,      uptr:NULL,   defintval:0,               TYPE_UINT,        0},   \
  }
#define ENB_ASN1_VERBOSITY_IDX                     0
#define ENB_ACTIVE_ENBS_IDX                        1
#define ENB_NOS1_IDX                               2


/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------------------------*/


/* cell configuration parameters names */
#define ENB_CONFIG_STRING_ENB_ID                        "eNB_ID"
#define ENB_CONFIG_STRING_CELL_TYPE                     "cell_type"
#define ENB_CONFIG_STRING_ENB_NAME                      "eNB_name"
#define ENB_CONFIG_STRING_TRACKING_AREA_CODE            "tracking_area_code"
#define ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE_OLD       "mobile_country_code"
#define ENB_CONFIG_STRING_MOBILE_NETWORK_CODE_OLD       "mobile_network_code"
#define ENB_CONFIG_STRING_TRANSPORT_S_PREFERENCE        "tr_s_preference"
#define ENB_CONFIG_STRING_LOCAL_S_IF_NAME               "local_s_if_name"
#define ENB_CONFIG_STRING_LOCAL_S_ADDRESS               "local_s_address"
#define ENB_CONFIG_STRING_REMOTE_S_ADDRESS              "remote_s_address"
#define ENB_CONFIG_STRING_LOCAL_S_PORTC                 "local_s_portc"
#define ENB_CONFIG_STRING_REMOTE_S_PORTC                "remote_s_portc"
#define ENB_CONFIG_STRING_LOCAL_S_PORTD                 "local_s_portd"
#define ENB_CONFIG_STRING_REMOTE_S_PORTD                "remote_s_portd"
#define ENB_CONFIG_STRING_NR_CELLID                     "nr_cellid"
#define ENB_CONFIG_STRING_RRC_INACTIVITY_THRESHOLD      "rrc_inactivity_threshold"
#define ENB_CONFIG_STRING_MEASUREMENT_REPORTS           "enable_measurement_reports"
#define ENB_CONFIG_STRING_X2                            "enable_x2"
#define ENB_CONFIG_STRING_ENB_M2                        "enable_enb_m2"
#define ENB_CONFIG_STRING_MCE_M2                        "enable_mce_m2"
#define ENB_CONFIG_STRING_S1SETUP_RSP_TIMER             "s1setup_rsp_timer"
#define ENB_CONFIG_STRING_S1SETUP_REQ_TIMER             "s1setup_req_timer"
#define ENB_CONFIG_STRING_S1SETUP_REQ_COUNT             "s1setup_req_count"
#define ENB_CONFIG_STRING_SCTP_REQ_TIMER                "sctp_req_timer"
#define ENB_CONFIG_STRING_SCTP_REQ_COUNT                "sctp_req_count"
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            cell configuration parameters                                                                */
/*   optname                                   helpstr   paramflags    XXXptr        defXXXval                   type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
#define ENBPARAMS_DESC {\
    {ENB_CONFIG_STRING_ENB_ID,                       NULL,   0,            uptr:NULL,   defintval:0,                 TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_CELL_TYPE,                    NULL,   0,            strptr:NULL, defstrval:"CELL_MACRO_ENB",  TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_ENB_NAME,                     NULL,   0,            strptr:NULL, defstrval:"OAIeNodeB",       TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_TRACKING_AREA_CODE,           NULL,   0,            uptr:NULL,   defuintval:0,                TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE_OLD,      NULL,   0,            strptr:NULL, defstrval:NULL,              TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_MOBILE_NETWORK_CODE_OLD,      NULL,   0,            strptr:NULL, defstrval:NULL,              TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_TRANSPORT_S_PREFERENCE,       NULL,   0,            strptr:NULL, defstrval:"local_mac",       TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_LOCAL_S_IF_NAME,              NULL,   0,            strptr:NULL, defstrval:"lo",              TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_LOCAL_S_ADDRESS,              NULL,   0,            strptr:NULL, defstrval:"127.0.0.1",       TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_REMOTE_S_ADDRESS,             NULL,   0,            strptr:NULL, defstrval:"127.0.0.2",       TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_LOCAL_S_PORTC,                NULL,   0,            uptr:NULL,   defuintval:50000,            TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_REMOTE_S_PORTC,               NULL,   0,            uptr:NULL,   defuintval:50000,            TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_LOCAL_S_PORTD,                NULL,   0,            uptr:NULL,   defuintval:50001,            TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_REMOTE_S_PORTD,               NULL,   0,            uptr:NULL,   defuintval:50001,            TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_NR_CELLID,                    NULL,   0,            u64ptr:NULL, defint64val:0,               TYPE_UINT64,    0},  \
    {ENB_CONFIG_STRING_RRC_INACTIVITY_THRESHOLD,     NULL,   0,            uptr:NULL,   defintval:0,                 TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_MEASUREMENT_REPORTS,          NULL,   0,            strptr:NULL, defstrval:NULL,              TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_X2,                           NULL,   0,            strptr:NULL, defstrval:NULL,              TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_ENB_M2,                       NULL,   0,            strptr:NULL, defstrval:"no",              TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_MCE_M2,                       NULL,   0,            strptr:NULL, defstrval:"no",              TYPE_STRING,    0},  \
    {ENB_CONFIG_STRING_S1SETUP_RSP_TIMER,            NULL,   0,            uptr:NULL,   defuintval:5,                TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_S1SETUP_REQ_TIMER,            NULL,   0,            uptr:NULL,   defuintval:5,                TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_S1SETUP_REQ_COUNT,            NULL,   0,            uptr:NULL,   defuintval:65535,            TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_SCTP_REQ_TIMER,               NULL,   0,            uptr:NULL,   defuintval:180,              TYPE_UINT,      0},  \
    {ENB_CONFIG_STRING_SCTP_REQ_COUNT,               NULL,   0,            uptr:NULL,   defuintval:65535,            TYPE_UINT,      0},  \
  }

#define ENB_ENB_ID_IDX                  0
#define ENB_CELL_TYPE_IDX               1
#define ENB_ENB_NAME_IDX                2
#define ENB_TRACKING_AREA_CODE_IDX      3
#define ENB_MOBILE_COUNTRY_CODE_IDX_OLD 4
#define ENB_MOBILE_NETWORK_CODE_IDX_OLD 5
#define ENB_TRANSPORT_S_PREFERENCE_IDX  6
#define ENB_LOCAL_S_IF_NAME_IDX         7
#define ENB_LOCAL_S_ADDRESS_IDX         8
#define ENB_REMOTE_S_ADDRESS_IDX        9
#define ENB_LOCAL_S_PORTC_IDX           10
#define ENB_REMOTE_S_PORTC_IDX          11
#define ENB_LOCAL_S_PORTD_IDX           12
#define ENB_REMOTE_S_PORTD_IDX          13
#define ENB_NRCELLID_IDX                14
#define ENB_RRC_INACTIVITY_THRES_IDX    15
#define ENB_ENABLE_MEASUREMENT_REPORTS  16
#define ENB_ENABLE_X2                   17
#define ENB_ENABLE_ENB_M2               18
#define ENB_ENABLE_MCE_M2               19
#define ENB_S1SETUP_RSP_TIMER_IDX       20
#define ENB_S1SETUP_REQ_TIMER_IDX       21
#define ENB_S1SETUP_REQ_COUNT_IDX       22
#define ENB_SCTP_REQ_TIMER_IDX          23
#define ENB_SCTP_REQ_COUNT_IDX          24

#define TRACKING_AREA_CODE_OKRANGE {0x0001,0xFFFD}
#define ENBPARAMS_CHECK {                                           \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s2 = { config_check_intrange, TRACKING_AREA_CODE_OKRANGE } },\
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
    { .s5 = { NULL } },                                             \
  }

/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/

/* PLMN ID configuration */

#define ENB_CONFIG_STRING_PLMN_LIST                     "plmn_list"

#define ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE           "mcc"
#define ENB_CONFIG_STRING_MOBILE_NETWORK_CODE           "mnc"
#define ENB_CONFIG_STRING_MNC_DIGIT_LENGTH              "mnc_length"

#define ENB_MOBILE_COUNTRY_CODE_IDX     0
#define ENB_MOBILE_NETWORK_CODE_IDX     1
#define ENB_MNC_DIGIT_LENGTH            2

#define PLMNPARAMS_DESC {                                                                  \
    /*   optname                              helpstr               paramflags XXXptr     def val          type    numelt */ \
    {ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE, "mobile country code",        0, uptr:NULL, defuintval:1000, TYPE_UINT, 0},    \
    {ENB_CONFIG_STRING_MOBILE_NETWORK_CODE, "mobile network code",        0, uptr:NULL, defuintval:1000, TYPE_UINT, 0},    \
    {ENB_CONFIG_STRING_MNC_DIGIT_LENGTH,    "length of the MNC (2 or 3)", 0, uptr:NULL, defuintval:0,    TYPE_UINT, 0},    \
  }

#define MCC_MNC_OKRANGES           {0,999}
#define MNC_DIGIT_LENGTH_OKVALUES  {2,3}

#define PLMNPARAMS_CHECK {                                           \
    { .s2 = { config_check_intrange, MCC_MNC_OKRANGES } },             \
    { .s2 = { config_check_intrange, MCC_MNC_OKRANGES } },             \
    { .s1 = { config_check_intval,   MNC_DIGIT_LENGTH_OKVALUES, 2 } }, \
  }


/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
#define ENB_CONFIG_STRING_MBMS_CONFIGURATION_DATA_LIST                     "mbms_configuration_data_list"

#define ENB_CONFIG_STRING_MBMS_SYNC_AREA           "mbms_sync_area"

#define ENB_MBMS_SYNC_AREA_IDX     0

#define MBMS_CONFIG_PARAMS_DESC {                                                                  \
/*   optname                              helpstr               paramflags XXXptr     def val          type    numelt */ \
  {ENB_CONFIG_STRING_MBMS_SYNC_AREA           , NULL,        0, uptr:NULL, defuintval:0, TYPE_UINT, 0},    \
}


/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
#define ENB_CONFIG_STRING_MBMS_SERVICE_AREA_LIST                     "mbms_service_area_list"

#define ENB_CONFIG_STRING_MBMS_SERVICE_AREA           "mbms_service_area"

#define ENB_MBMS_SERVICE_AREA_IDX     0

#define MBMSPARAMS_DESC {                                                                  \
/*   optname                              helpstr               paramflags XXXptr     def val          type    numelt */ \
  {ENB_CONFIG_STRING_MBMS_SERVICE_AREA, NULL,        0, uptr:NULL, defuintval:0, TYPE_UINT, 0},    \
}


/* component carries configuration parameters name */

#define ENB_CONFIG_STRING_NB_ANT_PORTS                                  "nb_antenna_ports"
#define ENB_CONFIG_STRING_NB_ANT_TX                                     "nb_antennas_tx"
#define ENB_CONFIG_STRING_NB_ANT_RX                                     "nb_antennas_rx"
#define ENB_CONFIG_STRING_TX_GAIN                                       "tx_gain"
#define ENB_CONFIG_STRING_RX_GAIN                                       "rx_gain"
#define ENB_CONFIG_STRING_PRACH_ROOT                                    "prach_root"
#define ENB_CONFIG_STRING_PRACH_CONFIG_INDEX                            "prach_config_index"
#define ENB_CONFIG_STRING_PRACH_HIGH_SPEED                              "prach_high_speed"
#define ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION                        "prach_zero_correlation"
#define ENB_CONFIG_STRING_PRACH_FREQ_OFFSET                             "prach_freq_offset"
#define ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT                             "pucch_delta_shift"
#define ENB_CONFIG_STRING_PUCCH_NRB_CQI                                 "pucch_nRB_CQI"
#define ENB_CONFIG_STRING_PUCCH_NCS_AN                                  "pucch_nCS_AN"
#define ENB_CONFIG_STRING_PUCCH_N1_AN                                   "pucch_n1_AN"


#define ENB_CONFIG_STRING_PCCH_CONFIG_V1310                      "pcch_config_v1310"
#define ENB_CONFIG_STRING_PAGING_NARROWBANDS_R13                 "paging_narrowbands_r13"
#define ENB_CONFIG_STRING_MPDCCH_NUMREPETITION_PAGING_R13        "mpdcch_numrepetition_paging_r13"
#define ENB_CONFIG_STRING_NB_V1310                               "nb_v1310"


#define ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL0    "pucch_NumRepetitionCE_Msg4_Level0_r13"
#define ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL1    "pucch_NumRepetitionCE_Msg4_Level1_r13"
#define ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL2    "pucch_NumRepetitionCE_Msg4_Level2_r13"
#define ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL3    "pucch_NumRepetitionCE_Msg4_Level3_r13"

#define ENB_CONFIG_STRING_FREQ_HOPPING_PARAMETERS_R13                   "sib2_freq_hoppingParameters_r13"

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
#define ENB_CONFIG_STRING_DRX_CONFIG_PRESENT                            "drx_Config_present"
#define ENB_CONFIG_STRING_DRX_ONDURATIONTIMER                           "drx_onDurationTimer"
#define ENB_CONFIG_STRING_DRX_INACTIVITYTIMER                           "drx_InactivityTimer"
#define ENB_CONFIG_STRING_DRX_RETRANSMISSIONTIMER                       "drx_RetransmissionTimer"
#define ENB_CONFIG_STRING_DRX_LONGDRX_CYCLESTARTOFFSET_PRESENT          "drx_longDrx_CycleStartOffset_present"
#define ENB_CONFIG_STRING_DRX_LONGDRX_CYCLESTARTOFFSET                  "drx_longDrx_CycleStartOffset"
#define ENB_CONFIG_STRING_DRX_SHORTDRX_CYCLE                            "drx_shortDrx_Cycle"
#define ENB_CONFIG_STRING_DRX_SHORTDRX_SHORTCYCLETIMER                  "drx_shortDrx_ShortCycleTimer"
#define ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF                  "bcch_modificationPeriodCoeff"
#define ENB_CONFIG_STRING_UETIMERS_T300                                 "ue_TimersAndConstants_t300"
#define ENB_CONFIG_STRING_UETIMERS_T301                                 "ue_TimersAndConstants_t301"
#define ENB_CONFIG_STRING_UETIMERS_T310                                 "ue_TimersAndConstants_t310"
#define ENB_CONFIG_STRING_UETIMERS_T311                                 "ue_TimersAndConstants_t311"
#define ENB_CONFIG_STRING_UETIMERS_N310                                 "ue_TimersAndConstants_n310"
#define ENB_CONFIG_STRING_UETIMERS_N311                                 "ue_TimersAndConstants_n311"
#define ENB_CONFIG_STRING_UE_TRANSMISSION_MODE                          "ue_TransmissionMode"
#define ENB_CONFIG_STRING_UE_MULTIPLE_MAX                               "ue_multiple_max"
//SIB1-MBMS
#define ENB_CONFIG_STRING_MBMS_DEDICATED_SERVING_CELL       "mbms_dedicated_serving_cell"

//NSA NR Cell SSB Absolute Frequency
#define ENB_CONFIG_STRING_NR_SCG_SSB_FREQ                         "nr_scg_ssb_freq"



#define ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_A_R13        "pdsch_maxNumRepetitionCEmodeA_r13"
#define ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_B_R13        "pdsch_maxNumRepetitionCEmodeB_r13"

#define ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_A_R13        "pusch_maxNumRepetitionCEmodeA_r13"
#define ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_B_R13        "pusch_maxNumRepetitionCEmodeB_r13"
#define ENB_CONFIG_STRING_PUSCH_REPETITION_LEVEL_CE_MODE_A_R13			"pusch_repetitionLevelCEmodeA_r13"
#define ENB_CONFIG_STRING_PUSCH_HOPPING_OFFSET_V1310                    "pusch_HoppingOffset_v1310"


//TTN - for D2D
//SIB18
#define ENB_CONFIG_STRING_RXPOOL_SC_CP_LEN                              "rxPool_sc_CP_Len"
#define ENB_CONFIG_STRING_RXPOOL_SC_PRIOD                               "rxPool_sc_Period"
#define ENB_CONFIG_STRING_RXPOOL_DATA_CP_LEN                            "rxPool_data_CP_Len"
#define ENB_CONFIG_STRING_RXPOOL_RC_PRB_NUM                             "rxPool_ResourceConfig_prb_Num"
#define ENB_CONFIG_STRING_RXPOOL_RC_PRB_START                           "rxPool_ResourceConfig_prb_Start"
#define ENB_CONFIG_STRING_RXPOOL_RC_PRB_END                             "rxPool_ResourceConfig_prb_End"
#define ENB_CONFIG_STRING_RXPOOL_RC_OFFSETIND_PRESENT                   "rxPool_ResourceConfig_offsetIndicator_present"
#define ENB_CONFIG_STRING_RXPOOL_RC_OFFSETIND_CHOICE                    "rxPool_ResourceConfig_offsetIndicator_choice"
#define ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_PRESENT                    "rxPool_ResourceConfig_subframeBitmap_present"
#define ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_BUF              "rxPool_ResourceConfig_subframeBitmap_choice_bs_buf"
#define ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_SIZE             "rxPool_ResourceConfig_subframeBitmap_choice_bs_size"
#define ENB_CONFIG_STRING_RXPOOL_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED  "rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused"
//SIB19 for DiscRxPool
#define ENB_CONFIG_STRING_DISCRXPOOL_CP_LEN                                 "discRxPool_cp_Len"
#define ENB_CONFIG_STRING_DISCRXPOOL_DISCPERIOD                             "discRxPool_discPeriod"
#define ENB_CONFIG_STRING_DISCRXPOOL_NUMRETX                                "discRxPool_numRetx"
#define ENB_CONFIG_STRING_DISCRXPOOL_NUMREPETITION                          "discRxPool_numRepetition"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_NUM                             "discRxPool_ResourceConfig_prb_Num"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_START                           "discRxPool_ResourceConfig_prb_Start"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_PRB_END                             "discRxPool_ResourceConfig_prb_End"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_OFFSETIND_PRESENT                   "discRxPool_ResourceConfig_offsetIndicator_present"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_OFFSETIND_CHOICE                    "discRxPool_ResourceConfig_offsetIndicator_choice"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_PRESENT                    "discRxPool_ResourceConfig_subframeBitmap_present"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_BUF              "discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_SIZE             "discRxPool_ResourceConfig_subframeBitmap_choice_bs_size"
#define ENB_CONFIG_STRING_DISCRXPOOL_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED  "discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused"

//SIB19 for DiscRxPoolPS
#define ENB_CONFIG_STRING_DISCRXPOOLPS_CP_LEN                                 "DISCRXPOOLPS_cp_Len"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_DISCPERIOD                             "DISCRXPOOLPS_discPeriod"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_NUMRETX                                "DISCRXPOOLPS_numRetx"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_NUMREPETITION                          "DISCRXPOOLPS_numRepetition"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_NUM                             "DISCRXPOOLPS_ResourceConfig_prb_Num"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_START                           "DISCRXPOOLPS_ResourceConfig_prb_Start"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_PRB_END                             "DISCRXPOOLPS_ResourceConfig_prb_End"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_OFFSETIND_PRESENT                   "DISCRXPOOLPS_ResourceConfig_offsetIndicator_present"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_OFFSETIND_CHOICE                    "DISCRXPOOLPS_ResourceConfig_offsetIndicator_choice"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_PRESENT                    "DISCRXPOOLPS_ResourceConfig_subframeBitmap_present"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_BUF              "DISCRXPOOLPS_ResourceConfig_subframeBitmap_choice_bs_buf"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_SIZE             "DISCRXPOOLPS_ResourceConfig_subframeBitmap_choice_bs_size"
#define ENB_CONFIG_STRING_DISCRXPOOLPS_RC_SFBITMAP_CHOICE_BS_ASN_BITS_UNUSED  "DISCRXPOOLPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused"

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     component carriers configuration parameters                                                                                                     */
/*   optname                                                   helpstr   paramflags    XXXptr                                        defXXXval                    type         numelt  */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* init for checkedparam_t structure */

typedef struct ccparams_lte_s {
  char             *frame_type;
  int32_t           tdd_config;
  int32_t           tdd_config_s;
  char             *prefix_type;
  char             *pbch_repetition;
  int32_t           eutra_band;
  long long int     downlink_frequency;
  int32_t           uplink_frequency_offset;
  int32_t           Nid_cell;
  int32_t           Nid_cell_mbsfn;
  int32_t           N_RB_DL;
  int32_t           nb_antenna_ports;
  int32_t           prach_root;
  int32_t           prach_config_index;
  char             *prach_high_speed;
  int32_t           prach_zero_correlation;
  int32_t           prach_freq_offset;
  int32_t           pucch_delta_shift;
  int32_t           pucch_nRB_CQI;
  int32_t           pucch_nCS_AN;
  int32_t           pucch_n1_AN;
  int32_t           pdsch_referenceSignalPower;
  int32_t           pdsch_p_b;
  int32_t           pusch_n_SB;
  char             *pusch_hoppingMode;
  int32_t           pusch_hoppingOffset;
  char             *pusch_enable64QAM;
  char             *pusch_groupHoppingEnabled;
  int32_t           pusch_groupAssignment;
  char             *pusch_sequenceHoppingEnabled;
  int32_t           pusch_nDMRS1;
  char             *phich_duration;
  char             *phich_resource;
  char             *srs_enable;
  int32_t           srs_BandwidthConfig;
  int32_t           srs_SubframeConfig;
  char             *srs_ackNackST;
  char             *srs_MaxUpPts;
  int32_t           pusch_p0_Nominal;
  char             *pusch_alpha;
  int32_t           pucch_p0_Nominal;
  int32_t           msg3_delta_Preamble;
  int32_t           ul_CyclicPrefixLength;
  char             *pucch_deltaF_Format1;
  char             *pucch_deltaF_Format1a;
  char             *pucch_deltaF_Format1b;
  char             *pucch_deltaF_Format2;
  char             *pucch_deltaF_Format2a;
  char             *pucch_deltaF_Format2b;
  int32_t           rach_numberOfRA_Preambles;
  char             *rach_preamblesGroupAConfig;
  int32_t           rach_sizeOfRA_PreamblesGroupA;
  int32_t           rach_messageSizeGroupA;
  char             *rach_messagePowerOffsetGroupB;
  int32_t           rach_powerRampingStep;
  int32_t           rach_preambleInitialReceivedTargetPower;
  int32_t           rach_preambleTransMax;
  int32_t           rach_raResponseWindowSize;
  int32_t           rach_macContentionResolutionTimer;
  int32_t           rach_maxHARQ_Msg3Tx;
  int32_t           pcch_defaultPagingCycle;
  char             *pcch_nB;
  char             *drx_Config_present;
  char             *drx_onDurationTimer;
  char             *drx_InactivityTimer;
  char             *drx_RetransmissionTimer;
  char             *drx_longDrx_CycleStartOffset_present;
  int32_t           drx_longDrx_CycleStartOffset;
  char             *drx_shortDrx_Cycle;
  int32_t           drx_shortDrx_ShortCycleTimer;
  int32_t           bcch_modificationPeriodCoeff;
  int32_t           ue_TimersAndConstants_t300;
  int32_t           ue_TimersAndConstants_t301;
  int32_t           ue_TimersAndConstants_t310;
  int32_t           ue_TimersAndConstants_t311;
  int32_t           ue_TimersAndConstants_n310;
  int32_t           ue_TimersAndConstants_n311;
  int32_t           ue_TransmissionMode;
  int32_t           ue_multiple_max;
  char             *mbms_dedicated_serving_cell;
  int32_t           srb1_timer_poll_retransmit;
  int32_t           srb1_timer_reordering;
  int32_t           srb1_timer_status_prohibit;
  int32_t           srb1_poll_pdu;
  int32_t           srb1_poll_byte;
  int32_t           srb1_max_retx_threshold;
  int32_t           nr_scg_ssb_freq;
} ccparams_lte_t;

#define CCPARAMS_CHECK {                             \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,                             \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { { NULL } } ,						     \
    { .s1a= { config_check_modify_integer, UETIMER_T300_OKVALUES, UETIMER_T300_MODVALUES,8}} ,\
    { .s1a= { config_check_modify_integer, UETIMER_T301_OKVALUES, UETIMER_T301_MODVALUES,8}} ,\
    { .s1a= { config_check_modify_integer, UETIMER_T310_OKVALUES, UETIMER_T310_MODVALUES,7}} ,\
    { .s1a= { config_check_modify_integer, UETIMER_T311_OKVALUES, UETIMER_T311_MODVALUES,7}} ,\
    { .s1a= { config_check_modify_integer, UETIMER_N310_OKVALUES, UETIMER_N310_MODVALUES,8}} ,\
    { .s1a= { config_check_modify_integer, UETIMER_N311_OKVALUES, UETIMER_N311_MODVALUES,8}} ,\
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } } ,						     \
    {  { NULL } }                               \
}

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     component carriers configuration parameters                                                                                                               */
/*   optname                                                    helpstr    paramflags   XXXptr                                                   defXXXval                  type        numelt   */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CCPARAMS_DESC(ccparams) {					\
  {ENB_CONFIG_STRING_FRAME_TYPE,                                   NULL,   0,           strptr:&ccparams.frame_type,                             defstrval:"FDD",           TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_TDD_CONFIG,                                   NULL,   0,           iptr:&ccparams.tdd_config,                               defintval:3,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_TDD_CONFIG_S,                                 NULL,   0,           iptr:&ccparams.tdd_config_s,                             defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PREFIX_TYPE,                                  NULL,   0,           strptr:&ccparams.prefix_type,                            defstrval:"NORMAL",        TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PBCH_REPETITION,                              NULL,   0,           strptr:&ccparams.pbch_repetition,                        defstrval:"FALSE",         TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_EUTRA_BAND,                                   NULL,   0,           iptr:&ccparams.eutra_band,                               defintval:7,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_DOWNLINK_FREQUENCY,                           NULL,   0,           i64ptr:(int64_t *)&ccparams.downlink_frequency,          defint64val:2680000000,    TYPE_UINT64,     0},  \
  {ENB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET,                      NULL,   0,           iptr:&ccparams.uplink_frequency_offset,                  defintval:-120000000,      TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_NID_CELL,                                     NULL,   0,           iptr:&ccparams.Nid_cell,                                 defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_N_RB_DL,                                      NULL,   0,           iptr:&ccparams.N_RB_DL,                                  defintval:25,              TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_CELL_MBSFN,                                   NULL,   0,           iptr:&ccparams.Nid_cell_mbsfn,                           defintval:0,               TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_NB_ANT_PORTS,                                 NULL,   0,           iptr:&ccparams.nb_antenna_ports,                         defintval:1,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PRACH_ROOT,                                   NULL,   0,           iptr:&ccparams.prach_root,                               defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PRACH_CONFIG_INDEX,                           NULL,   0,           iptr:&ccparams.prach_config_index,                       defintval:0,               TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_PRACH_HIGH_SPEED,                             NULL,   0,           strptr:&ccparams.prach_high_speed,                       defstrval:"DISABLE",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION,                       NULL,   0,           iptr:&ccparams.prach_zero_correlation,                   defintval:1,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PRACH_FREQ_OFFSET,                            NULL,   0,           iptr:&ccparams.prach_freq_offset,                        defintval:2,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT,                            NULL,   0,           iptr:&ccparams.pucch_delta_shift,                        defintval:1,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PUCCH_NRB_CQI,                                NULL,   0,           iptr:&ccparams.pucch_nRB_CQI,                            defintval:1,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PUCCH_NCS_AN,                                 NULL,   0,           iptr:&ccparams.pucch_nCS_AN,                             defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PUCCH_N1_AN,                                  NULL,   0,           iptr:&ccparams.pucch_n1_AN,                              defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PDSCH_RS_EPRE,                                NULL,   0,           iptr:&ccparams.pdsch_referenceSignalPower,               defintval:-29,             TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_PDSCH_PB,                                     NULL,   0,           iptr:&ccparams.pdsch_p_b,                                defintval:0,               TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_PUSCH_N_SB,                                   NULL,   0,           iptr:&ccparams.pusch_n_SB,                               defintval:1,               TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_PUSCH_HOPPINGMODE,                            NULL,   0,           strptr:&ccparams.pusch_hoppingMode,                      defstrval:"interSubFrame", TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET,                          NULL,   0,           iptr:&ccparams.pusch_hoppingOffset,                      defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PUSCH_ENABLE64QAM,                            NULL,   0,           strptr:&ccparams.pusch_enable64QAM,                      defstrval:"DISABLE",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN,                       NULL,   0,           strptr:&ccparams.pusch_groupHoppingEnabled,              defstrval:"ENABLE",        TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT,                       NULL,   0,           iptr:&ccparams.pusch_groupAssignment,                    defintval:0,               TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN,                    NULL,   0,           strptr:&ccparams.pusch_sequenceHoppingEnabled,           defstrval:"DISABLE",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUSCH_NDMRS1,                                 NULL,   0,           iptr:&ccparams.pusch_nDMRS1,                             defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PHICH_DURATION,                               NULL,   0,           strptr:&ccparams.phich_duration,                         defstrval:"NORMAL",        TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PHICH_RESOURCE,                               NULL,   0,           strptr:&ccparams.phich_resource,                         defstrval:"ONESIXTH",      TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_SRS_ENABLE,                                   NULL,   0,           strptr:&ccparams.srs_enable,                             defstrval:"DISABLE",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG,                         NULL,   0,           iptr:&ccparams.srs_BandwidthConfig,                      defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG,                          NULL,   0,           iptr:&ccparams.srs_SubframeConfig,                       defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG,                         NULL,   0,           strptr:&ccparams.srs_ackNackST,                          defstrval:"DISABLE",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_SRS_MAXUPPTS,                                 NULL,   0,           strptr:&ccparams.srs_MaxUpPts,                           defstrval:"DISABLE",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUSCH_PO_NOMINAL,                             NULL,   0,           iptr:&ccparams.pusch_p0_Nominal,                         defintval:-90,             TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_PUSCH_ALPHA,                                  NULL,   0,           strptr:&ccparams.pusch_alpha,                            defstrval:"AL1",           TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUCCH_PO_NOMINAL,                             NULL,   0,           iptr:&ccparams.pucch_p0_Nominal,                         defintval:-96,             TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE,                          NULL,   0,           iptr:&ccparams.msg3_delta_Preamble,                      defintval:6,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1,                         NULL,   0,           strptr:&ccparams.pucch_deltaF_Format1,                   defstrval:"DELTAF2",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b,                        NULL,   0,           strptr:&ccparams.pucch_deltaF_Format1b,                  defstrval:"deltaF3",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2,                         NULL,   0,           strptr:&ccparams.pucch_deltaF_Format2,                   defstrval:"deltaF0",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A,                        NULL,   0,           strptr:&ccparams.pucch_deltaF_Format2a,                  defstrval:"deltaF0",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B,                        NULL,   0,           strptr:&ccparams.pucch_deltaF_Format2b,                  defstrval:"deltaF0",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES,                        NULL,   0,           iptr:&ccparams.rach_numberOfRA_Preambles,                defintval:4,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG,                   NULL,   0,           strptr:&ccparams.rach_preamblesGroupAConfig,             defstrval:"DISABLE",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA,                NULL,   0,           iptr:&ccparams.rach_sizeOfRA_PreamblesGroupA,            defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA,                       NULL,   0,           iptr:&ccparams.rach_messageSizeGroupA,                   defintval:56,              TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB,                NULL,   0,           strptr:&ccparams.rach_messagePowerOffsetGroupB,          defstrval:"minusinfinity", TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP,                        NULL,   0,           iptr:&ccparams.rach_powerRampingStep,                    defintval:4,               TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER,      NULL,   0,           iptr:&ccparams.rach_preambleInitialReceivedTargetPower,  defintval:-100,            TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX,                        NULL,   0,           iptr:&ccparams.rach_preambleTransMax,                    defintval:10,              TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE,                    NULL,   0,           iptr:&ccparams.rach_raResponseWindowSize,                defintval:10,              TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER,            NULL,   0,           iptr:&ccparams.rach_macContentionResolutionTimer,        defintval:48,              TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX,                           NULL,   0,           iptr:&ccparams.rach_maxHARQ_Msg3Tx,                      defintval:4,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE,                    NULL,   0,           iptr:&ccparams.pcch_defaultPagingCycle,                  defintval:128,             TYPE_INT,        0},  \
  {ENB_CONFIG_STRING_PCCH_NB,                                      NULL,   0,           strptr:&ccparams.pcch_nB,                                defstrval:"oneT",          TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_DRX_CONFIG_PRESENT,                           NULL,   0,           strptr:&ccparams.drx_Config_present,                     defstrval:"prNothing",     TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_DRX_ONDURATIONTIMER,                          NULL,   0,           strptr:&ccparams.drx_onDurationTimer,                    defstrval:"psf10",         TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_DRX_INACTIVITYTIMER,                          NULL,   0,           strptr:&ccparams.drx_InactivityTimer,                    defstrval:"psf10",         TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_DRX_RETRANSMISSIONTIMER,                      NULL,   0,           strptr:&ccparams.drx_RetransmissionTimer,                defstrval:"psf8",          TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_DRX_LONGDRX_CYCLESTARTOFFSET_PRESENT,         NULL,   0,           strptr:&ccparams.drx_longDrx_CycleStartOffset_present,   defstrval:"prSf128",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_DRX_LONGDRX_CYCLESTARTOFFSET,                 NULL,   0,           iptr:&ccparams.drx_longDrx_CycleStartOffset,             defintval:0,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_DRX_SHORTDRX_CYCLE,                           NULL,   0,           strptr:&ccparams.drx_shortDrx_Cycle,                     defstrval:"sf32",          TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_DRX_SHORTDRX_SHORTCYCLETIMER,                 NULL,   0,           iptr:&ccparams.drx_shortDrx_ShortCycleTimer,             defintval:3,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF,                 NULL,   0,           iptr:&ccparams.bcch_modificationPeriodCoeff,             defintval:2,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_UETIMERS_T300,                                NULL,   0,           iptr:&ccparams.ue_TimersAndConstants_t300,               defintval:1000,            TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_UETIMERS_T301,                                NULL,   0,           iptr:&ccparams.ue_TimersAndConstants_t301,               defintval:1000,            TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_UETIMERS_T310,                                NULL,   0,           iptr:&ccparams.ue_TimersAndConstants_t310,               defintval:1000,            TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_UETIMERS_T311,                                NULL,   0,           iptr:&ccparams.ue_TimersAndConstants_t311,               defintval:10000,           TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_UETIMERS_N310,                                NULL,   0,           iptr:&ccparams.ue_TimersAndConstants_n310,               defintval:20,              TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_UETIMERS_N311,                                NULL,   0,           iptr:&ccparams.ue_TimersAndConstants_n311,               defintval:1,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_UE_TRANSMISSION_MODE,                         NULL,   0,           iptr:&ccparams.ue_TransmissionMode,                      defintval:1,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_UE_MULTIPLE_MAX,                              NULL,   0,           iptr:&ccparams.ue_multiple_max,                          defintval:4,               TYPE_UINT,       0},  \
  {ENB_CONFIG_STRING_MBMS_DEDICATED_SERVING_CELL,                  NULL,   0,           strptr:&ccparams.mbms_dedicated_serving_cell,            defstrval:"DISABLE",       TYPE_STRING,     0},  \
  {ENB_CONFIG_STRING_NR_SCG_SSB_FREQ,                              NULL,   0,           iptr:&ccparams.nr_scg_ssb_freq,                        defintval:641272,          TYPE_INT,        0}   \
}


#define ENB_CONFIG_FRAME_TYPE_IDX                                  0
#define ENB_CONFIG_TDD_CONFIG_IDX                                  1
#define ENB_CONFIG_TDD_CONFIG_S_IDX                                2
#define ENB_CONFIG_PREFIX_TYPE_IDX                                 3
#define ENB_CONFIG_PBCH_REPETITION_IDX                             4
#define ENB_CONFIG_EUTRA_BAND_IDX                                  5
#define ENB_CONFIG_DOWNLINK_FREQUENCY_IDX                          6
#define ENB_CONFIG_UPLINK_FREQUENCY_OFFSET_IDX                     7
#define ENB_CONFIG_NID_CELL_IDX                                    8
#define ENB_CONFIG_N_RB_DL_IDX                                     9
#define ENB_CONFIG_CELL_MBSFN_IDX                                  10
#define ENB_CONFIG_NB_ANT_PORTS_IDX                                11
#define ENB_CONFIG_PRACH_ROOT_IDX                                  12
#define ENB_CONFIG_PRACH_CONFIG_INDEX_IDX                          13
#define ENB_CONFIG_PRACH_HIGH_SPEED_IDX                            14
#define ENB_CONFIG_PRACH_ZERO_CORRELATION_IDX                      15
#define ENB_CONFIG_PRACH_FREQ_OFFSET_IDX                           16
#define ENB_CONFIG_PUCCH_DELTA_SHIFT_IDX                           17
#define ENB_CONFIG_PUCCH_NRB_CQI_IDX                               18
#define ENB_CONFIG_PUCCH_NCS_AN_IDX                                19
#define ENB_CONFIG_PUCCH_N1_AN_IDX                                 20
#define ENB_CONFIG_PDSCH_RS_EPRE_IDX                               21
#define ENB_CONFIG_PDSCH_PB_IDX                                    22
#define ENB_CONFIG_PUSCH_N_SB_IDX                                  23
#define ENB_CONFIG_PUSCH_HOPPINGMODE_IDX                           24
#define ENB_CONFIG_PUSCH_HOPPINGOFFSET_IDX                         25
#define ENB_CONFIG_PUSCH_ENABLE64QAM_IDX                           26
#define ENB_CONFIG_PUSCH_GROUP_HOPPING_EN_IDX                      27
#define ENB_CONFIG_PUSCH_GROUP_ASSIGNMENT_IDX                      28
#define ENB_CONFIG_PUSCH_SEQUENCE_HOPPING_EN_IDX                   29
#define ENB_CONFIG_PUSCH_NDMRS1_IDX                                30
#define ENB_CONFIG_PHICH_DURATION_IDX                              31
#define ENB_CONFIG_PHICH_RESOURCE_IDX                              32
#define ENB_CONFIG_SRS_ENABLE_IDX                                  33
#define ENB_CONFIG_SRS_BANDWIDTH_CONFIG_IDX                        34
#define ENB_CONFIG_SRS_SUBFRAME_CONFIG_IDX                         35
#define ENB_CONFIG_SRS_ACKNACKST_CONFIG_IDX                        36
#define ENB_CONFIG_SRS_MAXUPPTS_IDX                                37
#define ENB_CONFIG_PUSCH_PO_NOMINAL_IDX                            38
#define ENB_CONFIG_PUSCH_ALPHA_IDX                                 39
#define ENB_CONFIG_PUCCH_PO_NOMINAL_IDX                            40
#define ENB_CONFIG_MSG3_DELTA_PREAMBLE_IDX                         41
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT1_IDX                        42
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT1b_IDX                       43
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT2_IDX                        44
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT2A_IDX                       45
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT2B_IDX                       46
#define ENB_CONFIG_RACH_NUM_RA_PREAMBLES_IDX                       47
#define ENB_CONFIG_RACH_PREAMBLESGROUPACONFIG_IDX                  48
#define ENB_CONFIG_RACH_SIZEOFRA_PREAMBLESGROUPA_IDX               49
#define ENB_CONFIG_RACH_MESSAGESIZEGROUPA_IDX                      50
#define ENB_CONFIG_RACH_MESSAGEPOWEROFFSETGROUPB_IDX               51
#define ENB_CONFIG_RACH_POWERRAMPINGSTEP_IDX                       52
#define ENB_CONFIG_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER_IDX     53
#define ENB_CONFIG_RACH_PREAMBLETRANSMAX_IDX                       54
#define ENB_CONFIG_RACH_RARESPONSEWINDOWSIZE_IDX                   55
#define ENB_CONFIG_RACH_MACCONTENTIONRESOLUTIONTIMER_IDX           56
#define ENB_CONFIG_RACH_MAXHARQMSG3TX_IDX                          57
#define ENB_CONFIG_PCCH_DEFAULT_PAGING_CYCLE_IDX                   58
#define ENB_CONFIG_PCCH_NB_IDX                                     59
#define ENB_CONFIG_STRING_DRX_CONFIG_PRESENT_IDX                   60
#define ENB_CONFIG_STRING_DRX_ONDURATIONTIMER_IDX                  61
#define ENB_CONFIG_STRING_DRX_INACTIVITYTIMER_IDX                  62
#define ENB_CONFIG_STRING_DRX_RETRANSMISSIONTIMER_IDX              63
#define ENB_CONFIG_STRING_DRX_LONGDRX_CYCLESTARTOFFSET_PRESENT_IDX 64
#define ENB_CONFIG_STRING_DRX_LONGDRX_CYCLESTARTOFFSET_IDX         65
#define ENB_CONFIG_STRING_DRX_SHORTDRX_CYCLE_IDX                   66
#define ENB_CONFIG_STRING_DRX_SHORTDRX_SHORTCYCLETIMER_IDX         67
#define ENB_CONFIG_BCCH_MODIFICATIONPERIODCOEFF_IDX                68
#define ENB_CONFIG_UETIMERS_T300_IDX                               69
#define ENB_CONFIG_UETIMERS_T301_IDX                               70
#define ENB_CONFIG_UETIMERS_T310_IDX                               71
#define ENB_CONFIG_UETIMERS_T311_IDX                               72
#define ENB_CONFIG_UETIMERS_N310_IDX                               73
#define ENB_CONFIG_UETIMERS_N311_IDX                               74
#define ENB_CONFIG_UE_TRANSMISSION_MODE_IDX                        75
#define ENB_CONFIG_MBMS_DEDICATED_SERVING_CELL_IDX                 76

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* SRB1 configuration parameters section name */
#define ENB_CONFIG_STRING_SRB1                                          "srb1_parameters"

/* SRB1 configuration parameters names   */
#define ENB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT                    "timer_poll_retransmit"
#define ENB_CONFIG_STRING_SRB1_TIMER_REORDERING                         "timer_reordering"
#define ENB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT                    "timer_status_prohibit"
#define ENB_CONFIG_STRING_SRB1_POLL_PDU                                 "poll_pdu"
#define ENB_CONFIG_STRING_SRB1_POLL_BYTE                                "poll_byte"
#define ENB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD                       "max_retx_threshold"

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            SRB1 configuration parameters                                                                                  */
/*   optname                                          helpstr   paramflags    XXXptr                             defXXXval         type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
typedef struct srb1_params_s {
  int32_t     srb1_timer_poll_retransmit;
  int32_t     srb1_timer_reordering;
  int32_t     srb1_timer_status_prohibit;
  int32_t     srb1_poll_pdu;
  int32_t     srb1_poll_byte;
  int32_t     srb1_max_retx_threshold;
} srb1_params_t;

#define SRB1PARAMS_DESC(srb1_params) {          \
    {ENB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT,         NULL,   0,            iptr:&srb1_params.srb1_timer_poll_retransmit,   defintval:80,     TYPE_UINT,      0},       \
    {ENB_CONFIG_STRING_SRB1_TIMER_REORDERING,              NULL,   0,            iptr:&srb1_params.srb1_timer_reordering,        defintval:35,     TYPE_UINT,      0},       \
    {ENB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT,         NULL,   0,            iptr:&srb1_params.srb1_timer_status_prohibit,   defintval:0,      TYPE_UINT,      0},       \
    {ENB_CONFIG_STRING_SRB1_POLL_PDU,                      NULL,   0,            iptr:&srb1_params.srb1_poll_pdu,                defintval:4,      TYPE_UINT,      0},       \
    {ENB_CONFIG_STRING_SRB1_POLL_BYTE,                     NULL,   0,            iptr:&srb1_params.srb1_poll_byte,               defintval:99999,  TYPE_UINT,      0},       \
    {ENB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD,            NULL,   0,            iptr:&srb1_params.srb1_max_retx_threshold,      defintval:4,      TYPE_UINT,      0}        \
  }

/* MME configuration parameters section name */
#define ENB_CONFIG_STRING_MME_IP_ADDRESS                "mme_ip_address"


#define ENB_CONFIG_STRING_MME_IPV4_ADDRESS              "ipv4"
#define ENB_CONFIG_STRING_MME_IPV6_ADDRESS              "ipv6"
#define ENB_CONFIG_STRING_MME_PORT                      "port"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE         "active"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE     "preference"
#define ENB_CONFIG_STRING_MME_BROADCAST_PLMN_INDEX      "broadcast_plmn_index"


/*-------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            MME configuration parameters                                                             */
/*   optname                                           helpstr    paramflags XXXptr       defXXXval            type            numelt  */
/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define S1PARAMS_DESC {  \
  {ENB_CONFIG_STRING_MME_IPV4_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,      TYPE_STRING,    0},    \
  {ENB_CONFIG_STRING_MME_IPV6_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,      TYPE_STRING,    0},    \
  {ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE,              NULL,      0,         uptr:NULL,   defstrval:NULL,      TYPE_STRING,    0},    \
  {ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE,          NULL,      0,         uptr:NULL,   defstrval:NULL,      TYPE_STRING,    0},    \
  {ENB_CONFIG_STRING_MME_BROADCAST_PLMN_INDEX,           NULL,      0,         uptr:NULL,   defintarrayval:NULL, TYPE_UINTARRAY, 6},    \
  {ENB_CONFIG_STRING_MME_PORT,                           NULL,      0,         u16ptr:NULL, defuintval:S1AP_PORT_NUMBER, TYPE_UINT16, 0},    \
}





#define ENB_MME_IPV4_ADDRESS_IDX          0
#define ENB_MME_IPV6_ADDRESS_IDX          1
#define ENB_MME_IP_ADDRESS_ACTIVE_IDX     2
#define ENB_MME_IP_ADDRESS_PREFERENCE_IDX 3
#define ENB_MME_BROADCAST_PLMN_INDEX      4
#define ENB_MME_PORT_IDX                  5
/*---------------------------------------------------------------------------------------------------------------------------------------*/

/* X2 configuration parameters section name */
#define ENB_CONFIG_STRING_TARGET_ENB_X2_IP_ADDRESS                "target_enb_x2_ip_address"

/* X2 configuration parameters names   */


#define ENB_CONFIG_STRING_TARGET_ENB_X2_IPV4_ADDRESS              "ipv4"
#define ENB_CONFIG_STRING_TARGET_ENB_X2_IPV6_ADDRESS              "ipv6"
#define ENB_CONFIG_STRING_TARGET_ENB_X2_IP_ADDRESS_PREFERENCE     "preference"


/*-------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            X2 configuration parameters                                                              */
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define X2PARAMS_DESC {  \
  {ENB_CONFIG_STRING_TARGET_ENB_X2_IPV4_ADDRESS,             NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},   \
  {ENB_CONFIG_STRING_TARGET_ENB_X2_IPV6_ADDRESS,             NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},   \
  {ENB_CONFIG_STRING_TARGET_ENB_X2_IP_ADDRESS_PREFERENCE,    NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},   \
}

#define ENB_X2_IPV4_ADDRESS_IDX          0
#define ENB_X2_IPV6_ADDRESS_IDX          1
#define ENB_X2_IP_ADDRESS_PREFERENCE_IDX 2


/*---------------------------------------------------------------------------------------------------------------------------------------*/

/* M2 configuration parameters section name */
#define ENB_CONFIG_STRING_TARGET_MCE_M2_IP_ADDRESS                "target_mce_m2_ip_address"

/* M2 configuration parameters names   */


#define ENB_CONFIG_STRING_TARGET_MCE_M2_IPV4_ADDRESS              "ipv4"
#define ENB_CONFIG_STRING_TARGET_MCE_M2_IPV6_ADDRESS              "ipv6"
#define ENB_CONFIG_STRING_TARGET_MCE_M2_IP_ADDRESS_PREFERENCE     "preference"

/*---------------------------------------------------------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            M2 configuration parameters                                                             */
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define M2PARAMS_DESC {  \
{ENB_CONFIG_STRING_TARGET_MCE_M2_IPV4_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{ENB_CONFIG_STRING_TARGET_MCE_M2_IPV6_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{ENB_CONFIG_STRING_TARGET_MCE_M2_IP_ADDRESS_PREFERENCE,          NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
}

#define ENB_M2_IPV4_ADDRESS_IDX          0
#define ENB_M2_IPV6_ADDRESS_IDX          1
#define ENB_M2_IP_ADDRESS_PREFERENCE_IDX 2
/*---------------------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------------------*/
/* SCTP configuration parameters section name */
#define ENB_CONFIG_STRING_SCTP_CONFIG                    "SCTP"

/* SCTP configuration parameters names   */
#define ENB_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define ENB_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"



/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            SRB1 configuration parameters                                                                                  */
/*   optname                                          helpstr   paramflags    XXXptr                             defXXXval         type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define SCTPPARAMS_DESC {  \
    {ENB_CONFIG_STRING_SCTP_INSTREAMS,                       NULL,   0,   uptr:NULL,   defintval:-1,    TYPE_UINT,   0},       \
    {ENB_CONFIG_STRING_SCTP_OUTSTREAMS,                      NULL,   0,   uptr:NULL,   defintval:-1,    TYPE_UINT,   0}        \
  }

#define ENB_SCTP_INSTREAMS_IDX          0
#define ENB_SCTP_OUTSTREAMS_IDX         1
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* S1 interface configuration parameters section name */
#define ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG     "NETWORK_INTERFACES"

#define ENB_INTERFACE_NAME_FOR_S1_MME_IDX          0
#define ENB_IPV4_ADDRESS_FOR_S1_MME_IDX            1
#define ENB_INTERFACE_NAME_FOR_S1U_IDX             2
#define ENB_IPV4_ADDR_FOR_S1U_IDX                  3
#define ENB_PORT_FOR_S1U_IDX                       4
#define ENB_IPV4_ADDR_FOR_X2C_IDX                  5
#define ENB_PORT_FOR_X2C_IDX                       6
#define ENB_IPV4_ADDR_FOR_M2C_IDX                  7
#define ENB_PORT_FOR_M2C_IDX                       8
#define MCE_IPV4_ADDR_FOR_M2C_IDX                  9
#define MCE_PORT_FOR_M2C_IDX                       10

/* S1 interface configuration parameters names   */
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME "ENB_INTERFACE_NAME_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME   "ENB_IPV4_ADDRESS_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U    "ENB_INTERFACE_NAME_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U         "ENB_IPV4_ADDRESS_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_PORT_FOR_S1U              "ENB_PORT_FOR_S1U"

/* X2 interface configuration parameters names */
#define ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_X2C         "ENB_IPV4_ADDRESS_FOR_X2C"
#define ENB_CONFIG_STRING_ENB_PORT_FOR_X2C              "ENB_PORT_FOR_X2C"

/* M2 interface configuration parameters names */
#define ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_M2C         "ENB_IPV4_ADDRESS_FOR_M2C"
#define ENB_CONFIG_STRING_ENB_PORT_FOR_M2C              "ENB_PORT_FOR_M2C"
#define ENB_CONFIG_STRING_MCE_IPV4_ADDR_FOR_M2C         "MCE_IPV4_ADDRESS_FOR_M2C"
#define ENB_CONFIG_STRING_MCE_PORT_FOR_M2C              "MCE_PORT_FOR_M2C"


/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            S1/X2 interface configuration parameters                                                                 */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval             type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
#define NETPARAMS_DESC {  \
    {ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME,        NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
    {ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME,          NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
    {ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U,           NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
    {ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U,                NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
    {ENB_CONFIG_STRING_ENB_PORT_FOR_S1U,                     NULL,      0,         uptr:NULL,           defintval:2152L,     TYPE_UINT,        0},      \
    {ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_X2C,                NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
    {ENB_CONFIG_STRING_ENB_PORT_FOR_X2C,                     NULL,      0,         uptr:NULL,           defintval:0L,        TYPE_UINT,        0},      \
    {ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_M2C,                NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
    {ENB_CONFIG_STRING_ENB_PORT_FOR_M2C,                     NULL,      0,         uptr:NULL,           defintval:0L,        TYPE_UINT,        0},      \
    {ENB_CONFIG_STRING_MCE_IPV4_ADDR_FOR_M2C,                NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
    {ENB_CONFIG_STRING_MCE_PORT_FOR_M2C,                     NULL,      0,         uptr:NULL,           defintval:0L,        TYPE_UINT,        0},      \
  }

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            GTPU  configuration parameters                                                                                                      */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval                                           type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define GTPUPARAMS_DESC { \
    {ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U,           NULL,    0,            strptr:&enb_interface_name_for_S1U,      defstrval:"lo",                TYPE_STRING,   0},        \
    {ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U,                NULL,    0,            strptr:&enb_ipv4_address_for_S1U,        defstrval:"127.0.0.1",         TYPE_STRING,   0},        \
    {ENB_CONFIG_STRING_ENB_PORT_FOR_S1U,                     NULL,    0,            uptr:&enb_port_for_S1U,                  defintval:2152,                TYPE_UINT,     0}         \
  }
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* L1 configuration section names   */
#define CONFIG_STRING_L1_LIST                              "L1s"
#define CONFIG_STRING_L1_CONFIG                            "l1_config"

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CONFIG_STRING_NETWORK_CONTROLLER_CONFIG         "NETWORK_CONTROLLER"

#define CONFIG_STRING_FLEXRAN_ENABLED             "FLEXRAN_ENABLED"
#define CONFIG_STRING_FLEXRAN_INTERFACE_NAME      "FLEXRAN_INTERFACE_NAME"
#define CONFIG_STRING_FLEXRAN_IPV4_ADDRESS        "FLEXRAN_IPV4_ADDRESS"
#define CONFIG_STRING_FLEXRAN_PORT                "FLEXRAN_PORT"
#define CONFIG_STRING_FLEXRAN_CACHE               "FLEXRAN_CACHE"
#define CONFIG_STRING_FLEXRAN_AWAIT_RECONF        "FLEXRAN_AWAIT_RECONF"

#define FLEXRAN_ENABLED                               0
#define FLEXRAN_INTERFACE_NAME_IDX                    1
#define FLEXRAN_IPV4_ADDRESS_IDX                      2
#define FLEXRAN_PORT_IDX                              3
#define FLEXRAN_CACHE_IDX                             4
#define FLEXRAN_AWAIT_RECONF_IDX                      5

#define FLEXRANPARAMS_DESC { \
    {CONFIG_STRING_FLEXRAN_ENABLED,                NULL,   0,   strptr:NULL,   defstrval:"no",                    TYPE_STRING,   0},           \
    {CONFIG_STRING_FLEXRAN_INTERFACE_NAME,         NULL,   0,   strptr:NULL,   defstrval:"lo",                    TYPE_STRING,   0},           \
    {CONFIG_STRING_FLEXRAN_IPV4_ADDRESS,           NULL,   0,   strptr:NULL,   defstrval:"127.0.0.1",             TYPE_STRING,   0},           \
    {CONFIG_STRING_FLEXRAN_PORT,                   NULL,   0,   uptr:NULL,     defintval:2210,                    TYPE_UINT,     0},           \
    {CONFIG_STRING_FLEXRAN_CACHE,                  NULL,   0,   strptr:NULL,   defstrval:"/mnt/oai_agent_cache",  TYPE_STRING,   0},           \
    {CONFIG_STRING_FLEXRAN_AWAIT_RECONF,           NULL,   0,   strptr:NULL,   defstrval:"no",                    TYPE_STRING,   0}            \
  }

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/* CU/DU configuration section names*/
#define CONFIG_STRING_DU_LIST			            "DU"
#define CONFIG_STRING_CU_LIST			            "CU"
#define DU_TYPE_LTE                                 0
#define DU_TYPE_WIFI                                1
#define ENB_CONFIG_STRING_CU_INTERFACES_CONFIG		"CU_INTERFACES"
#define ENB_CONFIG_STRING_CU_INTERFACE_NAME_FOR_F1U "CU_INTERFACE_NAME_FOR_F1U"
#define ENB_CONFIG_STRING_CU_IPV4_ADDRESS_FOR_F1U   "CU_IPV4_ADDRESS_FOR_F1U"
#define ENB_CONFIG_STRING_CU_PORT_FOR_F1U           "CU_PORT_FOR_F1U"
#define ENB_CONFIG_STRING_DU_TYPE	                "DU_TYPE"
#define ENB_CONFIG_STRING_F1_U_CU_TRANSPORT_TYPE    "F1_U_CU_TRANSPORT_TYPE"
#define ENB_CONFIG_STRING_DU_INTERFACES_CONFIG		"DU_INTERFACES"
#define ENB_CONFIG_STRING_DU_INTERFACE_NAME_FOR_F1U "DU_INTERFACE_NAME_FOR_F1U"
#define ENB_CONFIG_STRING_DU_IPV4_ADDRESS_FOR_F1U   "DU_IPV4_ADDRESS_FOR_F1U"
#define ENB_CONFIG_STRING_DU_PORT_FOR_F1U           "DU_PORT_FOR_F1U"
#define ENB_CONFIG_STRING_F1_U_DU_TRANSPORT_TYPE    "F1_U_DU_TRANSPORT_TYPE"

#define CONFIG_STRING_CU_BALANCING      "CU_BALANCING"

#define CUPARAMS_DESC { \
  {ENB_CONFIG_STRING_CU_INTERFACE_NAME_FOR_F1U, NULL,   0,   strptr:NULL,   defstrval:"eth0",         TYPE_STRING,   0}, \
  {ENB_CONFIG_STRING_CU_IPV4_ADDRESS_FOR_F1U,   NULL,   0,   strptr:NULL,   defstrval:"127.0.0.1",    TYPE_STRING,   0}, \
  {ENB_CONFIG_STRING_CU_PORT_FOR_F1U,           NULL,   0,   uptr:NULL,     defintval:2210,           TYPE_UINT,     0}, \
  {ENB_CONFIG_STRING_F1_U_CU_TRANSPORT_TYPE,    NULL,   0,   strptr:NULL,   defstrval:"TCP",          TYPE_STRING,   0}, \
  {ENB_CONFIG_STRING_DU_TYPE,                   NULL,   0,   strptr:NULL,   defstrval:"LTE",          TYPE_STRING,   0}, \
}

#define DUPARAMS_DESC { \
  {ENB_CONFIG_STRING_DU_INTERFACE_NAME_FOR_F1U, NULL,   0,   strptr:NULL,   defstrval:"eth0",       TYPE_STRING,   0}, \
  {ENB_CONFIG_STRING_DU_IPV4_ADDRESS_FOR_F1U,   NULL,   0,   strptr:NULL,   defstrval:"127.0.0.1",  TYPE_STRING,   0}, \
  {ENB_CONFIG_STRING_DU_PORT_FOR_F1U,           NULL,   0,   uptr:NULL,     defintval:2210,   		TYPE_UINT,     0}, \
  {ENB_CONFIG_STRING_F1_U_DU_TRANSPORT_TYPE,    NULL,   0,   strptr:NULL,   defstrval:"TCP",   		TYPE_STRING,   0}, \
}

#define CU_BAL_DESC { \
  {CONFIG_STRING_CU_BALANCING,                  NULL,   0,   strptr:NULL,   defstrval:"ALL",        TYPE_STRING,   0}, \
}

#define CU_INTERFACE_F1U 	                     0
#define CU_ADDRESS_F1U                           1
#define CU_PORT_F1U 	              		     2
#define CU_TYPE_F1U 	              		     3

#define DU_INTERFACE_F1U                         0
#define DU_ADDRESS_F1U                           1
#define DU_PORT_F1U                              2
#define DU_TYPE_F1U                              3
#define DU_TECH                                  4

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/* MACRLC configuration section names   */
#define CONFIG_STRING_MACRLC_LIST                          "MACRLCs"
#define CONFIG_STRING_MACRLC_CONFIG                        "macrlc_config"


/* MACRLC configuration parameters names   */
#define CONFIG_STRING_MACRLC_CC                            "num_cc"
#define CONFIG_STRING_MACRLC_TRANSPORT_N_PREFERENCE        "tr_n_preference"
#define CONFIG_STRING_MACRLC_LOCAL_N_IF_NAME               "local_n_if_name"
#define CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS               "local_n_address"
#define CONFIG_STRING_MACRLC_REMOTE_N_ADDRESS              "remote_n_address"
#define CONFIG_STRING_MACRLC_LOCAL_N_PORTC                 "local_n_portc"
#define CONFIG_STRING_MACRLC_REMOTE_N_PORTC                "remote_n_portc"
#define CONFIG_STRING_MACRLC_LOCAL_N_PORTD                 "local_n_portd"
#define CONFIG_STRING_MACRLC_REMOTE_N_PORTD                "remote_n_portd"
#define CONFIG_STRING_MACRLC_TRANSPORT_S_PREFERENCE        "tr_s_preference"
#define CONFIG_STRING_MACRLC_LOCAL_S_IF_NAME               "local_s_if_name"
#define CONFIG_STRING_MACRLC_LOCAL_S_ADDRESS               "local_s_address"
#define CONFIG_STRING_MACRLC_REMOTE_S_ADDRESS              "remote_s_address"
#define CONFIG_STRING_MACRLC_LOCAL_S_PORTC                 "local_s_portc"
#define CONFIG_STRING_MACRLC_REMOTE_S_PORTC                "remote_s_portc"
#define CONFIG_STRING_MACRLC_LOCAL_S_PORTD                 "local_s_portd"
#define CONFIG_STRING_MACRLC_REMOTE_S_PORTD                "remote_s_portd"
#define CONFIG_STRING_MACRLC_SCHED_MODE                    "scheduler_mode"

#define MACRLC_CC_IDX                                          0
#define MACRLC_TRANSPORT_N_PREFERENCE_IDX                      1
#define MACRLC_LOCAL_N_IF_NAME_IDX                             2
#define MACRLC_LOCAL_N_ADDRESS_IDX                             3
#define MACRLC_REMOTE_N_ADDRESS_IDX                            4
#define MACRLC_LOCAL_N_PORTC_IDX                               5
#define MACRLC_REMOTE_N_PORTC_IDX                              6
#define MACRLC_LOCAL_N_PORTD_IDX                               7
#define MACRLC_REMOTE_N_PORTD_IDX                              8
#define MACRLC_TRANSPORT_S_PREFERENCE_IDX                      9
#define MACRLC_LOCAL_S_IF_NAME_IDX                             10
#define MACRLC_LOCAL_S_ADDRESS_IDX                             11
#define MACRLC_REMOTE_S_ADDRESS_IDX                            12
#define MACRLC_LOCAL_S_PORTC_IDX                               13
#define MACRLC_REMOTE_S_PORTC_IDX                              14
#define MACRLC_LOCAL_S_PORTD_IDX                               15
#define MACRLC_REMOTE_S_PORTD_IDX                              16
#define MACRLC_SCHED_MODE_IDX                                  17
/*---------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* thread configuration parameters section name */
#define THREAD_CONFIG_STRING_THREAD_STRUCT                "THREAD_STRUCT"

/* thread configuration parameters names   */
#define THREAD_CONFIG_STRING_PARALLEL              "parallel_config"
#define THREAD_CONFIG_STRING_WORKER                "worker_config"


#define THREAD_PARALLEL_IDX          0
#define THREAD_WORKER_IDX            1

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                                             thread configuration parameters                                                                 */
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval                                 type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define THREAD_CONF_DESC {  \
    {THREAD_CONFIG_STRING_PARALLEL,          CONFIG_HLP_PARALLEL,      0,       strptr:NULL,   defstrval:"PARALLEL_RU_L1_TRX_SPLIT",   TYPE_STRING,   0},          \
    {THREAD_CONFIG_STRING_WORKER,            CONFIG_HLP_WORKER,        0,       strptr:NULL,   defstrval:"WORKER_ENABLE",              TYPE_STRING,   0}           \
  }


#define CONFIG_HLP_WORKER                          "coding and FEP worker thread WORKER_DISABLE or WORKER_ENABLE\n"
#define CONFIG_HLP_PARALLEL                        "PARALLEL_SINGLE_THREAD, PARALLEL_RU_L1_SPLIT, or PARALLEL_RU_L1_TRX_SPLIT(RU_L1_TRX_SPLIT by defult)\n"
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include "enb_paramdef_emtc.h"
#include "enb_paramdef_sidelink.h"
#include "enb_paramdef_mce.h"
#include "enb_paramdef_mme.h"
#endif
