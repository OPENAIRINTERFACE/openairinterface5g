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

/*! \file openair2/GNB_APP/gnb_paramdef.f
 * \brief definition of configuration parameters for all gNodeB modules 
 * \author Francois TABURET, WEI-TAI CHEN
 * \date 2018
 * \version 0.1
 * \company NOKIA BellLabs France, NTUST
 * \email: francois.taburet@nokia-bell-labs.com, kroempa@gmail.com
 * \note
 * \warning
 */

#ifndef __GNB_APP_GNB_PARAMDEF__H__
#define __GNB_APP_GNB_PARAMDEF__H__

#include "common/config/config_paramdesc.h"
#include "RRC_nr_paramsvalues.h"


#define GNB_CONFIG_STRING_CC_NODE_FUNCTION        "node_function"
#define GNB_CONFIG_STRING_CC_NODE_TIMING          "node_timing"   
#define GNB_CONFIG_STRING_CC_NODE_SYNCH_REF       "node_synch_ref"   


// OTG config per GNB-UE DL
#define GNB_CONF_STRING_OTG_CONFIG                "otg_config"
#define GNB_CONF_STRING_OTG_UE_ID                 "ue_id"
#define GNB_CONF_STRING_OTG_APP_TYPE              "app_type"
#define GNB_CONF_STRING_OTG_BG_TRAFFIC            "bg_traffic"

extern int asn_debug;
extern int asn1_xer_print;

#ifdef LIBCONFIG_LONG
#define libconfig_int long
#else
#define libconfig_int int
#endif

typedef enum {
	NRRU     = 0,
	NRL1     = 1,
	NRL2     = 2,
	NRL3     = 3,
	NRS1     = 4,
	NRlastel = 5
} NRRC_config_functions_t;

#define CONFIG_STRING_ACTIVE_RUS                  "Active_RUs"
/*------------------------------------------------------------------------------------------------------------------------------------------*/

/*    RUs  configuration for gNB is the same for eNB */
/*    Check file enb_paramdef.h */

/*---------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------------------------------------*/
/* value definitions for ASN1 verbosity parameter */
#define GNB_CONFIG_STRING_ASN1_VERBOSITY_NONE              "none"
#define GNB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING          "annoying"
#define GNB_CONFIG_STRING_ASN1_VERBOSITY_INFO              "info"

/* global parameters, not under a specific section   */
#define GNB_CONFIG_STRING_ASN1_VERBOSITY                   "Asn1_verbosity"
#define GNB_CONFIG_STRING_ACTIVE_GNBS                      "Active_gNBs"
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            global configuration parameters                                                                                   */
/*   optname                                   helpstr   paramflags    XXXptr        defXXXval                                        type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define GNBSPARAMS_DESC {                                                                                             \
{GNB_CONFIG_STRING_ASN1_VERBOSITY,             NULL,     0,        uptr:NULL,   defstrval:GNB_CONFIG_STRING_ASN1_VERBOSITY_NONE,   TYPE_STRING,      0},   \
{GNB_CONFIG_STRING_ACTIVE_GNBS,                NULL,     0,        uptr:NULL,   defstrval:NULL, 				   TYPE_STRINGLIST,  0}    \
}

#define GNB_ASN1_VERBOSITY_IDX                     0
#define GNB_ACTIVE_GNBS_IDX                        1

/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------------------------*/


/* cell configuration parameters names */
#define GNB_CONFIG_STRING_GNB_ID                        "gNB_ID"
#define GNB_CONFIG_STRING_CELL_TYPE                     "cell_type"
#define GNB_CONFIG_STRING_GNB_NAME                      "gNB_name"
#define GNB_CONFIG_STRING_TRACKING_AREA_CODE            "tracking_area_code"
#define GNB_CONFIG_STRING_MOBILE_COUNTRY_CODE_OLD       "mobile_country_code"
#define GNB_CONFIG_STRING_MOBILE_NETWORK_CODE_OLD       "mobile_network_code"
#define GNB_CONFIG_STRING_TRANSPORT_S_PREFERENCE        "tr_s_preference"
#define GNB_CONFIG_STRING_LOCAL_S_IF_NAME               "local_s_if_name"
#define GNB_CONFIG_STRING_LOCAL_S_ADDRESS               "local_s_address"
#define GNB_CONFIG_STRING_REMOTE_S_ADDRESS              "remote_s_address"
#define GNB_CONFIG_STRING_LOCAL_S_PORTC                 "local_s_portc"
#define GNB_CONFIG_STRING_REMOTE_S_PORTC                "remote_s_portc"
#define GNB_CONFIG_STRING_LOCAL_S_PORTD                 "local_s_portd"
#define GNB_CONFIG_STRING_REMOTE_S_PORTD                "remote_s_portd"
#define GNB_CONFIG_STRING_SSBSUBCARRIEROFFSET           "ssb_SubcarrierOffset"
#define GNB_CONFIG_STRING_PDSCHANTENNAPORTS             "pdsch_AntennaPorts"
#define GNB_CONFIG_STRING_PUSCHANTENNAPORTS             "pusch_AntennaPorts"
#define GNB_CONFIG_STRING_SIB1TDA                       "sib1_tda"
#define GNB_CONFIG_STRING_DOCSIRS                       "do_CSIRS"
#define GNB_CONFIG_STRING_NRCELLID                      "nr_cellid"
#define GNB_CONFIG_STRING_MINRXTXTIMEPDSCH              "min_rxtxtime_pdsch"

/*-----------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            cell configuration parameters                                                                */
/*   optname                                   helpstr   paramflags    XXXptr        defXXXval                   type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
#define GNBPARAMS_DESC {\
{GNB_CONFIG_STRING_GNB_ID,                       NULL,   0,            uptr:NULL,   defintval:0,                 TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_CELL_TYPE,                    NULL,   0,            strptr:NULL, defstrval:"CELL_MACRO_GNB",  TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_GNB_NAME,                     NULL,   0,            strptr:NULL, defstrval:"OAIgNodeB",       TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_TRACKING_AREA_CODE,           NULL,   0,            uptr:NULL,   defuintval:0,                TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_MOBILE_COUNTRY_CODE_OLD,      NULL,   0,            strptr:NULL, defstrval:NULL,              TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_MOBILE_NETWORK_CODE_OLD,      NULL,   0,            strptr:NULL, defstrval:NULL,              TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_TRANSPORT_S_PREFERENCE,       NULL,   0,            strptr:NULL, defstrval:"local_mac",       TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_LOCAL_S_IF_NAME,              NULL,   0,            strptr:NULL, defstrval:"lo",              TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_LOCAL_S_ADDRESS,              NULL,   0,            strptr:NULL, defstrval:"127.0.0.1",       TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_REMOTE_S_ADDRESS,             NULL,   0,            strptr:NULL, defstrval:"127.0.0.2",       TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_LOCAL_S_PORTC,                NULL,   0,            uptr:NULL,   defuintval:50000,            TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_REMOTE_S_PORTC,               NULL,   0,            uptr:NULL,   defuintval:50000,            TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_LOCAL_S_PORTD,                NULL,   0,            uptr:NULL,   defuintval:50001,            TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_REMOTE_S_PORTD,               NULL,   0,            uptr:NULL,   defuintval:50001,            TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_SSBSUBCARRIEROFFSET,          NULL,   0,            iptr:NULL,   defintval:31,                TYPE_INT,       0},  \
{GNB_CONFIG_STRING_PDSCHANTENNAPORTS,            NULL,   0,            iptr:NULL,   defintval:1,                 TYPE_INT,       0},  \
{GNB_CONFIG_STRING_PUSCHANTENNAPORTS,            NULL,   0,            iptr:NULL,   defintval:1,                 TYPE_INT,       0},  \
{GNB_CONFIG_STRING_SIB1TDA,                      NULL,   0,            iptr:NULL,   defintval:0,                 TYPE_INT,       0},  \
{GNB_CONFIG_STRING_DOCSIRS,                      NULL,   0,            iptr:NULL,   defintval:0,                 TYPE_INT,       0},  \
{GNB_CONFIG_STRING_NRCELLID,                     NULL,   0,            u64ptr:NULL, defint64val:1,               TYPE_UINT64,    0},  \
{GNB_CONFIG_STRING_MINRXTXTIMEPDSCH,             NULL,   0,            iptr:NULL,   defintval:2,                 TYPE_INT,       0}   \
}															     	

#define GNB_GNB_ID_IDX                  0
#define GNB_CELL_TYPE_IDX               1
#define GNB_GNB_NAME_IDX                2
#define GNB_TRACKING_AREA_CODE_IDX      3
#define GNB_MOBILE_COUNTRY_CODE_IDX_OLD 4
#define GNB_MOBILE_NETWORK_CODE_IDX_OLD 5
#define GNB_TRANSPORT_S_PREFERENCE_IDX  6
#define GNB_LOCAL_S_IF_NAME_IDX         7
#define GNB_LOCAL_S_ADDRESS_IDX         8
#define GNB_REMOTE_S_ADDRESS_IDX        9
#define GNB_LOCAL_S_PORTC_IDX           10
#define GNB_REMOTE_S_PORTC_IDX          11
#define GNB_LOCAL_S_PORTD_IDX           12
#define GNB_REMOTE_S_PORTD_IDX          13
#define GNB_SSB_SUBCARRIEROFFSET_IDX    14
#define GNB_PDSCH_ANTENNAPORTS_IDX      15
#define GNB_PUSCH_ANTENNAPORTS_IDX      16
#define GNB_SIB1_TDA_IDX                17
#define GNB_DO_CSIRS_IDX                18
#define GNB_NRCELLID_IDX                19
#define GNB_MINRXTXTIMEPDSCH_IDX        20

#define TRACKING_AREA_CODE_OKRANGE {0x0001,0xFFFD}
#define GNBPARAMS_CHECK {                                         \
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
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/		  

/* PLMN ID configuration */

#define GNB_CONFIG_STRING_PLMN_LIST                     "plmn_list"

#define GNB_CONFIG_STRING_MOBILE_COUNTRY_CODE           "mcc"
#define GNB_CONFIG_STRING_MOBILE_NETWORK_CODE           "mnc"
#define GNB_CONFIG_STRING_MNC_DIGIT_LENGTH              "mnc_length"

#define GNB_MOBILE_COUNTRY_CODE_IDX     0
#define GNB_MOBILE_NETWORK_CODE_IDX     1
#define GNB_MNC_DIGIT_LENGTH            2

#define GNBPLMNPARAMS_DESC {                                                                  \
/*   optname                              helpstr               paramflags XXXptr     def val          type    numelt */ \
  {GNB_CONFIG_STRING_MOBILE_COUNTRY_CODE, "mobile country code",        0, uptr:NULL, defuintval:1000, TYPE_UINT, 0},    \
  {GNB_CONFIG_STRING_MOBILE_NETWORK_CODE, "mobile network code",        0, uptr:NULL, defuintval:1000, TYPE_UINT, 0},    \
  {GNB_CONFIG_STRING_MNC_DIGIT_LENGTH,    "length of the MNC (2 or 3)", 0, uptr:NULL, defuintval:0,    TYPE_UINT, 0},    \
}

#define MCC_MNC_OKRANGES           {0,999}
#define MNC_DIGIT_LENGTH_OKVALUES  {2,3}

#define PLMNPARAMS_CHECK {                                           \
  { .s2 = { config_check_intrange, MCC_MNC_OKRANGES } },             \
  { .s2 = { config_check_intrange, MCC_MNC_OKRANGES } },             \
  { .s1 = { config_check_intval,   MNC_DIGIT_LENGTH_OKVALUES, 2 } }, \
}


/* SNSSAI ID configuration */

#define GNB_CONFIG_STRING_SNSSAI_LIST                   "snssaiList"

#define GNB_CONFIG_STRING_SLICE_SERIVE_TYPE             "sst"
#define GNB_CONFIG_STRING_SLICE_DIFFERENTIATOR          "sd"

#define GNB_SLICE_SERIVE_TYPE_IDX        0
#define GNB_SLICE_DIFFERENTIATOR_IDX     1

#define GNBSNSSAIPARAMS_DESC {                                                                  \
/*   optname                               helpstr                 paramflags XXXptr     def val          type    numelt */ \
  {GNB_CONFIG_STRING_SLICE_SERIVE_TYPE,    "slice serive type",            0, uptr:NULL, defuintval:1,    TYPE_UINT, 0},    \
  {GNB_CONFIG_STRING_SLICE_DIFFERENTIATOR, "slice differentiator",         0, uptr:NULL, defuintval:0,    TYPE_UINT, 0},    \
}

#define SLICE_SERIVE_TYPE_OKRANGES           {1,2,3,4}

#define SNSSAIPARAMS_CHECK {                                           \
  { .s1 = { config_check_intval, SLICE_SERIVE_TYPE_OKRANGES, 4 } },             \
  { .s5 = { NULL } },             \
}

/* AMF configuration parameters section name */
#define GNB_CONFIG_STRING_AMF_IP_ADDRESS                "amf_ip_address"

/* SRB1 configuration parameters names   */


#define GNB_CONFIG_STRING_AMF_IPV4_ADDRESS              "ipv4"
#define GNB_CONFIG_STRING_AMF_IPV6_ADDRESS              "ipv6"
#define GNB_CONFIG_STRING_AMF_IP_ADDRESS_ACTIVE         "active"
#define GNB_CONFIG_STRING_AMF_IP_ADDRESS_PREFERENCE     "preference"
#define GNB_CONFIG_STRING_AMF_BROADCAST_PLMN_INDEX      "broadcast_plmn_index"


/*-------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            MME configuration parameters                                                             */
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define GNBNGPARAMS_DESC {  \
{GNB_CONFIG_STRING_AMF_IPV4_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{GNB_CONFIG_STRING_AMF_IPV6_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{GNB_CONFIG_STRING_AMF_IP_ADDRESS_ACTIVE,              NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{GNB_CONFIG_STRING_AMF_IP_ADDRESS_PREFERENCE,          NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
}

#define GNB_AMF_IPV4_ADDRESS_IDX          0
#define GNB_AMF_IPV6_ADDRESS_IDX          1
#define GNB_AMF_IP_ADDRESS_ACTIVE_IDX     2
#define GNB_AMF_IP_ADDRESS_PREFERENCE_IDX 3
#define GNB_AMF_BROADCAST_PLMN_INDEX      4
/*---------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------------------------------------*/
/* SCTP configuration parameters section name */
#define GNB_CONFIG_STRING_SCTP_CONFIG                    "SCTP"

/* SCTP configuration parameters names   */
#define GNB_CONFIG_STRING_SCTP_INSTREAMS                 "SCTP_INSTREAMS"
#define GNB_CONFIG_STRING_SCTP_OUTSTREAMS                "SCTP_OUTSTREAMS"



/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            SRB1 configuration parameters                                                                                  */
/*   optname                                          helpstr   paramflags    XXXptr                             defXXXval         type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define GNBSCTPPARAMS_DESC {  \
{GNB_CONFIG_STRING_SCTP_INSTREAMS,                       NULL,   0,   uptr:NULL,   defintval:-1,    TYPE_UINT,   0},       \
{GNB_CONFIG_STRING_SCTP_OUTSTREAMS,                      NULL,   0,   uptr:NULL,   defintval:-1,    TYPE_UINT,   0}        \
}

#define GNB_SCTP_INSTREAMS_IDX          0
#define GNB_SCTP_OUTSTREAMS_IDX         1
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* S1 interface configuration parameters section name */
#define GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG     "NETWORK_INTERFACES"

#define GNB_INTERFACE_NAME_FOR_NG_AMF_IDX          0
#define GNB_IPV4_ADDRESS_FOR_NG_AMF_IDX            1
#define GNB_INTERFACE_NAME_FOR_NGU_IDX             2
#define GNB_IPV4_ADDR_FOR_NGU_IDX                  3
#define GNB_PORT_FOR_NGU_IDX                       4
#define GNB_IPV4_ADDR_FOR_X2C_IDX      			   5
#define GNB_PORT_FOR_X2C_IDX         			   6

/* S1 interface configuration parameters names   */
#define GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_S1_MME "GNB_INTERFACE_NAME_FOR_S1_MME"
#define GNB_CONFIG_STRING_GNB_IPV4_ADDRESS_FOR_S1_MME   "GNB_IPV4_ADDRESS_FOR_S1_MME"
#define GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_S1U    "GNB_INTERFACE_NAME_FOR_S1U"
#define GNB_CONFIG_STRING_GNB_IPV4_ADDRESS_FOR_S1U      "GNB_IPV4_ADDRESS_FOR_S1U"
#define GNB_CONFIG_STRING_GNB_PORT_FOR_S1U              "GNB_PORT_FOR_S1U"

#define GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_NG_AMF "GNB_INTERFACE_NAME_FOR_NG_AMF"
#define GNB_CONFIG_STRING_GNB_IPV4_ADDRESS_FOR_NG_AMF   "GNB_IPV4_ADDRESS_FOR_NG_AMF"
#define GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_NGU    "GNB_INTERFACE_NAME_FOR_NGU"
#define GNB_CONFIG_STRING_GNB_IPV4_ADDR_FOR_NGU         "GNB_IPV4_ADDRESS_FOR_NGU"
#define GNB_CONFIG_STRING_GNB_PORT_FOR_NGU              "GNB_PORT_FOR_NGU"

/* X2 interface configuration parameters names */
#define GNB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_X2C	        "GNB_IPV4_ADDRESS_FOR_X2C"
#define GNB_CONFIG_STRING_ENB_PORT_FOR_X2C				"GNB_PORT_FOR_X2C"

/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            S1 interface configuration parameters                                                                 */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval             type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
#define GNBNETPARAMS_DESC {  \
{GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_NG_AMF,        NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_GNB_IPV4_ADDRESS_FOR_NG_AMF,          NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_NGU,           NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_GNB_IPV4_ADDR_FOR_NGU,                NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_GNB_PORT_FOR_NGU,                     NULL,      0,         uptr:NULL,           defintval:2152L,     TYPE_UINT,        0},      \
{GNB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_X2C,                NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_ENB_PORT_FOR_X2C,                     NULL,      0,         uptr:NULL,           defintval:0L,        TYPE_UINT,        0}      \
}   



/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            GTPU  configuration parameters                                                                                                      */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval                                           type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define GNBGTPUPARAMS_DESC { \
{GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_NGU,           NULL,    0,            strptr:&gnb_interface_name_for_NGU,      defstrval:"lo",                TYPE_STRING,   0},        \
{GNB_CONFIG_STRING_GNB_IPV4_ADDR_FOR_NGU,                NULL,    0,            strptr:&gnb_ipv4_address_for_NGU,        defstrval:"127.0.0.1",         TYPE_STRING,   0},        \
{GNB_CONFIG_STRING_GNB_PORT_FOR_NGU,                     NULL,    0,            uptr:&gnb_port_for_NGU,                  defintval:2152,                TYPE_UINT,     0},        \
{GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_S1U,           NULL,    0,            strptr:&gnb_interface_name_for_S1U,      defstrval:"lo",                TYPE_STRING,   0},        \
{GNB_CONFIG_STRING_GNB_IPV4_ADDRESS_FOR_S1U,             NULL,    0,            strptr:&gnb_ipv4_address_for_S1U,        defstrval:"127.0.0.1",         TYPE_STRING,   0},        \
{GNB_CONFIG_STRING_GNB_PORT_FOR_S1U,                     NULL,    0,            uptr:&gnb_port_for_S1U,                  defintval:2152,                TYPE_UINT,     0}         \
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* L1 configuration section names   */
#define CONFIG_STRING_L1_LIST                              "L1s"
#define CONFIG_STRING_L1_CONFIG                            "l1_config"



/*----------------------------------------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CONFIG_STRING_NETWORK_CONTROLLER_CONFIG         "NETWORK_CONTROLLER"

#define CONFIG_STRING_FLEXRAN_ENABLED                   "FLEXRAN_ENABLED"
#define CONFIG_STRING_FLEXRAN_INTERFACE_NAME            "FLEXRAN_INTERFACE_NAME"
#define CONFIG_STRING_FLEXRAN_IPV4_ADDRESS              "FLEXRAN_IPV4_ADDRESS"
#define CONFIG_STRING_FLEXRAN_PORT                      "FLEXRAN_PORT"
#define CONFIG_STRING_FLEXRAN_CACHE                     "FLEXRAN_CACHE"
#define CONFIG_STRING_FLEXRAN_AWAIT_RECONF              "FLEXRAN_AWAIT_RECONF"

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


/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* security configuration                                                                                                                                                           */
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CONFIG_STRING_SECURITY             "security"

#define SECURITY_CONFIG_CIPHERING          "ciphering_algorithms"
#define SECURITY_CONFIG_INTEGRITY          "integrity_algorithms"
#define SECURITY_CONFIG_DO_DRB_CIPHERING   "drb_ciphering"
#define SECURITY_CONFIG_DO_DRB_INTEGRITY   "drb_integrity"

/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*   security configuration                                                                                                                                                         */
/*   optname                               help                                          paramflags         XXXptr               defXXXval                 type              numelt */
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define SECURITY_GLOBALPARAMS_DESC { \
    {SECURITY_CONFIG_CIPHERING,            "preferred ciphering algorithms\n",            0,                strlistptr:NULL,      defstrlistval:NULL,       TYPE_STRINGLIST,  0}, \
    {SECURITY_CONFIG_INTEGRITY,            "preferred integrity algorithms\n",            0,                strlistptr:NULL,      defstrlistval:NULL,       TYPE_STRINGLIST,  0}, \
    {SECURITY_CONFIG_DO_DRB_CIPHERING,     "use ciphering for DRBs",                      0,                strptr:NULL,          defstrval:"yes",          TYPE_STRING,      0}, \
    {SECURITY_CONFIG_DO_DRB_INTEGRITY,     "use integrity for DRBs",                      0,                strptr:NULL,          defstrval:"no",           TYPE_STRING,      0}, \
}

#define SECURITY_CONFIG_CIPHERING_IDX          0
#define SECURITY_CONFIG_INTEGRITY_IDX          1
#define SECURITY_CONFIG_DO_DRB_CIPHERING_IDX   2
#define SECURITY_CONFIG_DO_DRB_INTEGRITY_IDX   3

/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#endif
