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

/*! \file openair2/ENB_APP/enb_paramdef.f
 * \brief definition of configuration parameters for all eNodeB modules 
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#include "common/config/config_paramdesc.h"
#include "RRC_paramsvalues.h"








#define ENB_CONFIG_STRING_CC_NODE_FUNCTION                              "node_function"
#define ENB_CONFIG_STRING_CC_NODE_TIMING                                "node_timing"   
#define ENB_CONFIG_STRING_CC_NODE_SYNCH_REF                             "node_synch_ref"   


// OTG config per ENB-UE DL
#define ENB_CONF_STRING_OTG_CONFIG                         "otg_config"
#define ENB_CONF_STRING_OTG_UE_ID                          "ue_id"
#define ENB_CONF_STRING_OTG_APP_TYPE                       "app_type"
#define ENB_CONF_STRING_OTG_BG_TRAFFIC                     "bg_traffic"










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
#define CONFIG_STRING_RU_NBIOTRRC_LIST            "NbIoT_RRC_instances"

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
#define RU_NBIOTRRC_LIST_IDX          17



/*-----------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            RU configuration parameters                                                                  */
/*   optname                                   helpstr   paramflags    XXXptr        defXXXval                   type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
#define RUPARAMS_DESC { \
{CONFIG_STRING_RU_LOCAL_IF_NAME,            	 NULL,       0,       strptr:NULL,     defstrval:"lo",  	TYPE_STRING,	  0}, \
{CONFIG_STRING_RU_LOCAL_ADDRESS,            	 NULL,       0,       strptr:NULL,     defstrval:"127.0.0.2",	TYPE_STRING,	  0}, \
{CONFIG_STRING_RU_REMOTE_ADDRESS,           	 NULL,       0,       strptr:NULL,     defstrval:"127.0.0.1",	TYPE_STRING,	  0}, \
{CONFIG_STRING_RU_LOCAL_PORTC,              	 NULL,       0,       uptr:NULL,       defuintval:50000,	TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_REMOTE_PORTC,             	 NULL,       0,       uptr:NULL,       defuintval:50000,	TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_LOCAL_PORTD,              	 NULL,       0,       uptr:NULL,       defuintval:50001,	TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_REMOTE_PORTD,             	 NULL,       0,       uptr:NULL,       defuintval:50001,	TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_TRANSPORT_PREFERENCE,     	 NULL,       0,       strptr:NULL,     defstrval:"udp_if5",	TYPE_STRING,	  0}, \
{CONFIG_STRING_RU_LOCAL_RF,                 	 NULL,       0,       strptr:NULL,     defstrval:"yes", 	TYPE_STRING,	  0}, \
{CONFIG_STRING_RU_NB_TX,                    	 NULL,       0,       uptr:NULL,       defuintval:1,		TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_NB_RX,                    	 NULL,       0,       uptr:NULL,       defuintval:1,		TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_MAX_RS_EPRE,              	 NULL,       0,       iptr:NULL,       defintval:-29,		TYPE_INT,	  0}, \
{CONFIG_STRING_RU_MAX_RXGAIN,               	 NULL,       0,       iptr:NULL,       defintval:120,		TYPE_INT,	  0}, \
{CONFIG_STRING_RU_BAND_LIST,                	 NULL,       0,       uptr:NULL,       defintarrayval:DEFBANDS, TYPE_INTARRAY,    1}, \
{CONFIG_STRING_RU_ENB_LIST,                 	 NULL,       0,       uptr:NULL,       defintarrayval:DEFENBS,  TYPE_INTARRAY,    1}, \
{CONFIG_STRING_RU_ATT_TX,                   	 NULL,       0,       uptr:NULL,       defintval:0,		TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_ATT_RX,                   	 NULL,       0,       uptr:NULL,       defintval:0,		TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_NBIOTRRC_LIST,                 NULL,       0,       uptr:NULL,       defintarrayval:DEFENBS,  TYPE_INTARRAY,    1}, \
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
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            global configuration parameters                                                                                   */
/*   optname                                   helpstr   paramflags    XXXptr        defXXXval                                        type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define ENBSPARAMS_DESC {                                                                                             \
{ENB_CONFIG_STRING_ASN1_VERBOSITY,             NULL,     0,        uptr:NULL,   defstrval:ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE,   TYPE_STRING,      0},   \
{ENB_CONFIG_STRING_ACTIVE_ENBS,                NULL,     0,        uptr:NULL,   defstrval:NULL, 				   TYPE_STRINGLIST,  0}    \
}
#define ENB_ASN1_VERBOSITY_IDX                     0
#define ENB_ACTIVE_ENBS_IDX                        1


/*
{ENB_CONFIG_STRING_COMPONENT_CARRIERS,"",   "",   0,   uptr:NULL,defstrval:ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE,TYPE_STRING,0},           \
{ENB_CONFIG_STRING_CC_NODE_FUNCTION,"",   "",   0,   uptr:NULL,defstrval:ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE,TYPE_STRING,0},           \
{ENB_CONFIG_STRING_CC_NODE_TIMING,"",   "",   0,   uptr:NULL,defstrval:ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE,TYPE_STRING,0},           \
{ENB_CONFIG_STRING_CC_NODE_SYNCH_REF,"",   "",   0,   uptr:NULL,defstrval:ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE,TYPE_STRING,0},           \
*/



/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------------------------*/


/* cell configuration parameters names */
#define ENB_CONFIG_STRING_ENB_ID                        "eNB_ID"
#define ENB_CONFIG_STRING_CELL_TYPE                     "cell_type"
#define ENB_CONFIG_STRING_ENB_NAME                      "eNB_name"
#define ENB_CONFIG_STRING_TRACKING_AREA_CODE            "tracking_area_code"
#define ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE           "mobile_country_code"
#define ENB_CONFIG_STRING_MOBILE_NETWORK_CODE           "mobile_network_code"
#define ENB_CONFIG_STRING_TRANSPORT_S_PREFERENCE        "tr_s_preference"
#define ENB_CONFIG_STRING_LOCAL_S_IF_NAME               "local_s_if_name"
#define ENB_CONFIG_STRING_LOCAL_S_ADDRESS               "local_s_address"
#define ENB_CONFIG_STRING_REMOTE_S_ADDRESS              "remote_s_address"
#define ENB_CONFIG_STRING_LOCAL_S_PORTC                 "local_s_portc"
#define ENB_CONFIG_STRING_REMOTE_S_PORTC                "remote_s_portc"
#define ENB_CONFIG_STRING_LOCAL_S_PORTD                 "local_s_portd"
#define ENB_CONFIG_STRING_REMOTE_S_PORTD                "remote_s_portd"

/*-----------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            cell configuration parameters                                                                */
/*   optname                                   helpstr   paramflags    XXXptr        defXXXval                   type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
#define ENBPARAMS_DESC {\
{ENB_CONFIG_STRING_ENB_ID,                       NULL,   0,            uptr:NULL,   defintval:0,                 TYPE_UINT,      0},  \
{ENB_CONFIG_STRING_CELL_TYPE,                    NULL,   0,            strptr:NULL, defstrval:"CELL_MACRO_ENB",  TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_ENB_NAME,                     NULL,   0,            strptr:NULL, defstrval:"OAIeNodeB",       TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_TRACKING_AREA_CODE,           NULL,   0,            strptr:NULL, defstrval:"0",               TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_MOBILE_COUNTRY_CODE,          NULL,   0,            strptr:NULL, defstrval:"0",               TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_MOBILE_NETWORK_CODE,          NULL,   0,            strptr:NULL, defstrval:"0",               TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_TRANSPORT_S_PREFERENCE,       NULL,   0,            strptr:NULL, defstrval:"local_mac",       TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_LOCAL_S_IF_NAME,              NULL,   0,            strptr:NULL, defstrval:"lo",              TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_LOCAL_S_ADDRESS,              NULL,   0,            strptr:NULL, defstrval:"127.0.0.1",       TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_REMOTE_S_ADDRESS,             NULL,   0,            strptr:NULL, defstrval:"127.0.0.2",       TYPE_STRING,    0},  \
{ENB_CONFIG_STRING_LOCAL_S_PORTC,                NULL,   0,            uptr:NULL,   defuintval:50000,            TYPE_UINT,      0},  \
{ENB_CONFIG_STRING_REMOTE_S_PORTC,               NULL,   0,            uptr:NULL,   defuintval:50000,            TYPE_UINT,      0},  \
{ENB_CONFIG_STRING_LOCAL_S_PORTD,                NULL,   0,            uptr:NULL,   defuintval:50001,            TYPE_UINT,      0},  \
{ENB_CONFIG_STRING_REMOTE_S_PORTD,               NULL,   0,            uptr:NULL,   defuintval:50001,            TYPE_UINT,      0},  \
}															     	
#define ENB_ENB_ID_IDX                  0
#define ENB_CELL_TYPE_IDX               1
#define ENB_ENB_NAME_IDX                2
#define ENB_TRACKING_AREA_CODE_IDX      3
#define ENB_MOBILE_COUNTRY_CODE_IDX     4
#define ENB_MOBILE_NETWORK_CODE_IDX     5
#define ENB_TRANSPORT_S_PREFERENCE_IDX  6
#define ENB_LOCAL_S_IF_NAME_IDX         7
#define ENB_LOCAL_S_ADDRESS_IDX         8
#define ENB_REMOTE_S_ADDRESS_IDX        9
#define ENB_LOCAL_S_PORTC_IDX           10
#define ENB_REMOTE_S_PORTC_IDX          11
#define ENB_LOCAL_S_PORTD_IDX           12
#define ENB_REMOTE_S_PORTD_IDX          13
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/		  


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
//#if !defined(Rel10) && !defined(Rel14)
#define ENB_CONFIG_STRING_PUCCH_N1_AN                                   "pucch_n1_AN"
//#endif
#define ENB_CONFIG_STRING_PDSCH_RS_EPRE                                 "pdsch_referenceSignalPower"
#define ENB_CONFIG_STRING_PDSCH_PB                                      "pdsch_p_b"
#define ENB_CONFIG_STRING_PUSCH_N_SB                                     "pusch_n_SB"
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

/* init for checkedparam_t structure */

#define CCPARAMS_CHECK                 {                                     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s5= {NULL }} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_T300_OKVALUES, UETIMER_T300_MODVALUES,8}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_T301_OKVALUES, UETIMER_T301_MODVALUES,8}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_T310_OKVALUES, UETIMER_T310_MODVALUES,7}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_T311_OKVALUES, UETIMER_T311_MODVALUES,7}} ,						     \
             { .s1a= { config_check_modify_integer, UETIMER_N310_OKVALUES, UETIMER_N310_MODVALUES,8}} , 					      \
             { .s1a= { config_check_modify_integer, UETIMER_N311_OKVALUES, UETIMER_N311_MODVALUES,8}} , 					      \
             { .s5= {NULL }} ,						     \
}
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     component carriers configuration parameters                                                                                                                   */
/*   optname                                                   helpstr   paramflags    XXXptr                                        defXXXval                    type         numelt  checked_param */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CCPARAMS_DESC { \
{ENB_CONFIG_STRING_FRAME_TYPE,                                   NULL,   0,           strptr:&frame_type,                             defstrval:"FDD",           TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_TDD_CONFIG,                                   NULL,   0,           iptr:&tdd_config,                               defintval:3,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_TDD_CONFIG_S,                                 NULL,   0,           iptr:&tdd_config_s,                             defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PREFIX_TYPE,                                  NULL,   0,           strptr:&prefix_type,                            defstrval:"NORMAL",        TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PBCH_REPETITION,                              NULL,   0,           strptr:&pbch_repetition,                        defstrval:"FALSE",         TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_EUTRA_BAND,                                   NULL,   0,           iptr:&eutra_band,                               defintval:7,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_DOWNLINK_FREQUENCY,                           NULL,   0,           i64ptr:(int64_t *)&downlink_frequency,          defint64val:2680000000,    TYPE_UINT64,     0},  \
{ENB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET,                      NULL,   0,           iptr:&uplink_frequency_offset,                  defintval:-120000000,      TYPE_INT,        0},  \
{ENB_CONFIG_STRING_NID_CELL,                                     NULL,   0,           iptr:&Nid_cell,                                 defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_N_RB_DL,                                      NULL,   0,           iptr:&N_RB_DL,                                  defintval:25,              TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_CELL_MBSFN,                                   NULL,   0,           iptr:&Nid_cell_mbsfn,                           defintval:0,               TYPE_INT,        0},  \
{ENB_CONFIG_STRING_NB_ANT_PORTS,                                 NULL,   0,           iptr:&nb_antenna_ports,                         defintval:1,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PRACH_ROOT,                                   NULL,   0,           iptr:&prach_root,                               defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PRACH_CONFIG_INDEX,                           NULL,   0,           iptr:&prach_config_index,                       defintval:0,               TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PRACH_HIGH_SPEED,                             NULL,   0,           strptr:&prach_high_speed,                       defstrval:"DISABLE",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION,                       NULL,   0,           iptr:&prach_zero_correlation,                   defintval:1,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PRACH_FREQ_OFFSET,                            NULL,   0,           iptr:&prach_freq_offset,                        defintval:2,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT,                            NULL,   0,           iptr:&pucch_delta_shift,                        defintval:1,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_NRB_CQI,                                NULL,   0,           iptr:&pucch_nRB_CQI,                            defintval:1,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_NCS_AN,                                 NULL,   0,           iptr:&pucch_nCS_AN,                             defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_N1_AN,                                  NULL,   0,           iptr:&pucch_n1_AN,                              defintval:32,              TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PDSCH_RS_EPRE,                                NULL,   0,           iptr:&pdsch_referenceSignalPower,               defintval:-29,             TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PDSCH_PB,                                     NULL,   0,           iptr:&pdsch_p_b,                                defintval:0,               TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_N_SB,                                   NULL,   0,           iptr:&pusch_n_SB,                               defintval:1,               TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_HOPPINGMODE,                            NULL,   0,           strptr:&pusch_hoppingMode,                      defstrval:"interSubFrame", TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET,                          NULL,   0,           iptr:&pusch_hoppingOffset,                      defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUSCH_ENABLE64QAM,                            NULL,   0,           strptr:&pusch_enable64QAM,                      defstrval:"DISABLE",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN,                       NULL,   0,           strptr:&pusch_groupHoppingEnabled,              defstrval:"ENABLE",        TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT,                       NULL,   0,           iptr:&pusch_groupAssignment,                    defintval:0,               TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN,                    NULL,   0,           strptr:&pusch_sequenceHoppingEnabled,           defstrval:"DISABLE",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_NDMRS1,                                 NULL,   0,           iptr:&pusch_nDMRS1,                             defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PHICH_DURATION,                               NULL,   0,           strptr:&phich_duration,                         defstrval:"NORMAL",        TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PHICH_RESOURCE,                               NULL,   0,           strptr:&phich_resource,                         defstrval:"ONESIXTH",      TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_ENABLE,                                   NULL,   0,           strptr:&srs_enable,                             defstrval:"DISABLE",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG,                         NULL,   0,           iptr:&srs_BandwidthConfig,                      defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG,                          NULL,   0,           iptr:&srs_SubframeConfig,                       defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG,                         NULL,   0,           strptr:&srs_ackNackST,                          defstrval:"DISABLE",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_SRS_MAXUPPTS,                                 NULL,   0,           strptr:&srs_MaxUpPts,                           defstrval:"DISABLE",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUSCH_PO_NOMINAL,                             NULL,   0,           iptr:&pusch_p0_Nominal,                         defintval:-90,             TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PUSCH_ALPHA,                                  NULL,   0,           strptr:&pusch_alpha,                            defstrval:"AL1",           TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_PO_NOMINAL,                             NULL,   0,           iptr:&pucch_p0_Nominal,                         defintval:-96,             TYPE_INT,        0},  \
{ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE,                          NULL,   0,           iptr:&msg3_delta_Preamble,                      defintval:6,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1,                         NULL,   0,           strptr:&pucch_deltaF_Format1,                   defstrval:"DELTAF2",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b,                        NULL,   0,           strptr:&pucch_deltaF_Format1b,                  defstrval:"deltaF3",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2,                         NULL,   0,           strptr:&pucch_deltaF_Format2,                   defstrval:"deltaF0",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A,                        NULL,   0,           strptr:&pucch_deltaF_Format2a,                  defstrval:"deltaF0",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B,                        NULL,   0,           strptr:&pucch_deltaF_Format2b,                  defstrval:"deltaF0",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES,                        NULL,   0,           iptr:&rach_numberOfRA_Preambles,                defintval:4,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG,                   NULL,   0,           strptr:&rach_preamblesGroupAConfig,             defstrval:"DISABLE",       TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA,                NULL,   0,           iptr:&rach_sizeOfRA_PreamblesGroupA,            defintval:0,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA,                       NULL,   0,           iptr:&rach_messageSizeGroupA,                   defintval:56,              TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB,                NULL,   0,           strptr:&rach_messagePowerOffsetGroupB,          defstrval:"minusinfinity", TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP,                        NULL,   0,           iptr:&rach_powerRampingStep,                    defintval:4,               TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER,      NULL,   0,           iptr:&rach_preambleInitialReceivedTargetPower,  defintval:-100,            TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX,                        NULL,   0,           iptr:&rach_preambleTransMax,                    defintval:10,              TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE,                    NULL,   0,           iptr:&rach_raResponseWindowSize,                defintval:10,              TYPE_INT,        0},  \
{ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER,            NULL,   0,           iptr:&rach_macContentionResolutionTimer,        defintval:48,              TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX,                           NULL,   0,           iptr:&rach_maxHARQ_Msg3Tx,                      defintval:4,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE,                    NULL,   0,           iptr:&pcch_defaultPagingCycle,                  defintval:128,             TYPE_INT,        0},  \
{ENB_CONFIG_STRING_PCCH_NB,                                      NULL,   0,           strptr:&pcch_nB,                                defstrval:"oneT",          TYPE_STRING,     0},  \
{ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF,                 NULL,   0,           iptr:&bcch_modificationPeriodCoeff,             defintval:2,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T300,                                NULL,   0,           iptr:&ue_TimersAndConstants_t300,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T301,                                NULL,   0,           iptr:&ue_TimersAndConstants_t301,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T310,                                NULL,   0,           iptr:&ue_TimersAndConstants_t310,               defintval:1000,            TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_T311,                                NULL,   0,           iptr:&ue_TimersAndConstants_t311,               defintval:10000,           TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_N310,                                NULL,   0,           iptr:&ue_TimersAndConstants_n310,               defintval:20,              TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UETIMERS_N311,                                NULL,   0,           iptr:&ue_TimersAndConstants_n311,               defintval:1,               TYPE_UINT,       0},  \
{ENB_CONFIG_STRING_UE_TRANSMISSION_MODE,                         NULL,   0,           iptr:&ue_TransmissionMode,                      defintval:1,               TYPE_UINT,       0}   \
}

#define ENB_CONFIG_FRAME_TYPE_IDX                            0  			     
#define ENB_CONFIG_TDD_CONFIG_IDX                            1  			     
#define ENB_CONFIG_TDD_CONFIG_S_IDX			     2
#define ENB_CONFIG_PREFIX_TYPE_IDX 			     3
#define ENB_CONFIG_PBCH_REPETITION_IDX			     4
#define ENB_CONFIG_EUTRA_BAND_IDX  			     5
#define ENB_CONFIG_DOWNLINK_FREQUENCY_IDX  		     6
#define ENB_CONFIG_UPLINK_FREQUENCY_OFFSET_IDX		     7
#define ENB_CONFIG_NID_CELL_IDX				     8
#define ENB_CONFIG_N_RB_DL_IDX				     9
#define ENB_CONFIG_CELL_MBSFN_IDX  			     10
#define ENB_CONFIG_NB_ANT_PORTS_IDX			     11
#define ENB_CONFIG_PRACH_ROOT_IDX  			     12
#define ENB_CONFIG_PRACH_CONFIG_INDEX_IDX  		     13
#define ENB_CONFIG_PRACH_HIGH_SPEED_IDX			     14
#define ENB_CONFIG_PRACH_ZERO_CORRELATION_IDX		     15
#define ENB_CONFIG_PRACH_FREQ_OFFSET_IDX	             16     
#define ENB_CONFIG_PUCCH_DELTA_SHIFT_IDX		     17     
#define ENB_CONFIG_PUCCH_NRB_CQI_IDX			     18
#define ENB_CONFIG_PUCCH_NCS_AN_IDX			     19
#define ENB_CONFIG_PUCCH_N1_AN_IDX 			     20
#define ENB_CONFIG_PDSCH_RS_EPRE_IDX			     21
#define ENB_CONFIG_PDSCH_PB_IDX				     22
#define ENB_CONFIG_PUSCH_N_SB_IDX  			     23
#define ENB_CONFIG_PUSCH_HOPPINGMODE_IDX		     24     
#define ENB_CONFIG_PUSCH_HOPPINGOFFSET_IDX 		     25
#define ENB_CONFIG_PUSCH_ENABLE64QAM_IDX		     26     
#define ENB_CONFIG_PUSCH_GROUP_HOPPING_EN_IDX		     27
#define ENB_CONFIG_PUSCH_GROUP_ASSIGNMENT_IDX		     28
#define ENB_CONFIG_PUSCH_SEQUENCE_HOPPING_EN_IDX	     29     
#define ENB_CONFIG_PUSCH_NDMRS1_IDX			     30
#define ENB_CONFIG_PHICH_DURATION_IDX			     31
#define ENB_CONFIG_PHICH_RESOURCE_IDX			     32
#define ENB_CONFIG_SRS_ENABLE_IDX  			     33
#define ENB_CONFIG_SRS_BANDWIDTH_CONFIG_IDX		     34
#define ENB_CONFIG_SRS_SUBFRAME_CONFIG_IDX 		     35
#define ENB_CONFIG_SRS_ACKNACKST_CONFIG_IDX		     36
#define ENB_CONFIG_SRS_MAXUPPTS_IDX			     37
#define ENB_CONFIG_PUSCH_PO_NOMINAL_IDX			     38
#define ENB_CONFIG_PUSCH_ALPHA_IDX 			     39
#define ENB_CONFIG_PUCCH_PO_NOMINAL_IDX			     40
#define ENB_CONFIG_MSG3_DELTA_PREAMBLE_IDX 		     41
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT1_IDX		     42
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT1b_IDX		     43
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT2_IDX		     44
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT2A_IDX		     45
#define ENB_CONFIG_PUCCH_DELTAF_FORMAT2B_IDX		     46
#define ENB_CONFIG_RACH_NUM_RA_PREAMBLES_IDX		     47
#define ENB_CONFIG_RACH_PREAMBLESGROUPACONFIG_IDX  	     48
#define ENB_CONFIG_RACH_SIZEOFRA_PREAMBLESGROUPA_IDX	     49
#define ENB_CONFIG_RACH_MESSAGESIZEGROUPA_IDX		     50
#define ENB_CONFIG_RACH_MESSAGEPOWEROFFSETGROUPB_IDX	     51
#define ENB_CONFIG_RACH_POWERRAMPINGSTEP_IDX		     52
#define ENB_CONFIG_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER_IDX 53 
#define ENB_CONFIG_RACH_PREAMBLETRANSMAX_IDX		     54
#define ENB_CONFIG_RACH_RARESPONSEWINDOWSIZE_IDX	     55     
#define ENB_CONFIG_RACH_MACCONTENTIONRESOLUTIONTIMER_IDX     56	     
#define ENB_CONFIG_RACH_MAXHARQMSG3TX_IDX  		     57
#define ENB_CONFIG_PCCH_DEFAULT_PAGING_CYCLE_IDX	     58     
#define ENB_CONFIG_PCCH_NB_IDX				     59
#define ENB_CONFIG_BCCH_MODIFICATIONPERIODCOEFF_IDX	     60
#define ENB_CONFIG_UETIMERS_T300_IDX			     61
#define ENB_CONFIG_UETIMERS_T301_IDX			     62
#define ENB_CONFIG_UETIMERS_T310_IDX			     63
#define ENB_CONFIG_UETIMERS_T311_IDX			     64
#define ENB_CONFIG_UETIMERS_N310_IDX			     65
#define ENB_CONFIG_UETIMERS_N311_IDX			     66
#define ENB_CONFIG_UE_TRANSMISSION_MODE_IDX		     67

/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
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
#define SRB1PARAMS_DESC {                                                                                                \
{ENB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT,         NULL,   0,            iptr:&srb1_timer_poll_retransmit,   defintval:80,     TYPE_UINT,      0},       \
{ENB_CONFIG_STRING_SRB1_TIMER_REORDERING,              NULL,   0,            iptr:&srb1_timer_reordering,        defintval:35,     TYPE_UINT,      0},       \
{ENB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT,         NULL,   0,            iptr:&srb1_timer_status_prohibit,   defintval:0,      TYPE_UINT,      0},       \
{ENB_CONFIG_STRING_SRB1_POLL_PDU,                      NULL,   0,            iptr:&srb1_poll_pdu,                defintval:4,      TYPE_UINT,      0},       \
{ENB_CONFIG_STRING_SRB1_POLL_BYTE,                     NULL,   0,            iptr:&srb1_poll_byte,               defintval:99999,  TYPE_UINT,      0},       \
{ENB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD,            NULL,   0,            iptr:&srb1_max_retx_threshold,      defintval:4,      TYPE_UINT,      0}        \
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* MME configuration parameters section name */
#define ENB_CONFIG_STRING_MME_IP_ADDRESS                "mme_ip_address"

/* SRB1 configuration parameters names   */


#define ENB_CONFIG_STRING_MME_IPV4_ADDRESS              "ipv4"
#define ENB_CONFIG_STRING_MME_IPV6_ADDRESS              "ipv6"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE         "active"
#define ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE     "preference"


/*-------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            MME configuration parameters                                                             */
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define S1PARAMS_DESC {  \
{ENB_CONFIG_STRING_MME_IPV4_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{ENB_CONFIG_STRING_MME_IPV6_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{ENB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE,              NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{ENB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE,          NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
}

#define ENB_MME_IPV4_ADDRESS_IDX          0
#define ENB_MME_IPV6_ADDRESS_IDX          1
#define ENB_MME_IP_ADDRESS_ACTIVE_IDX     2
#define ENB_MME_IP_ADDRESS_PREFERENCE_IDX 3
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

/* S1 interface configuration parameters names   */
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME "ENB_INTERFACE_NAME_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME   "ENB_IPV4_ADDRESS_FOR_S1_MME"
#define ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U    "ENB_INTERFACE_NAME_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U         "ENB_IPV4_ADDRESS_FOR_S1U"
#define ENB_CONFIG_STRING_ENB_PORT_FOR_S1U              "ENB_PORT_FOR_S1U"

/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            S1 interface configuration parameters                                                                 */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval             type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
#define NETPARAMS_DESC {  \
{ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1_MME,        NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{ENB_CONFIG_STRING_ENB_IPV4_ADDRESS_FOR_S1_MME,          NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{ENB_CONFIG_STRING_ENB_INTERFACE_NAME_FOR_S1U,           NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{ENB_CONFIG_STRING_ENB_IPV4_ADDR_FOR_S1U,                NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{ENB_CONFIG_STRING_ENB_PORT_FOR_S1U,                     NULL,      0,         uptr:NULL,           defintval:2152L,     TYPE_UINT,        0}       \
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
/* MACRLC configuration section names   */
#define CONFIG_STRING_MACRLC_LIST                          "MACRLCs"
#define CONFIG_STRING_MACRLC_CONFIG                        "macrlc_config"



