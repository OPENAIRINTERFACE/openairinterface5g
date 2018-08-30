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
} NRRC_config_functions_t;

#define CONFIG_STRING_ACTIVE_RUS                  "Active_RUs"
/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*    RUs  configuration section name */
#define CONFIG_STRING_RU_LIST                     "RUs"
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
#define CONFIG_STRING_RU_GNB_LIST                 "gNB_instances"
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
#define RU_GNB_LIST_IDX               14
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
{CONFIG_STRING_RU_GNB_LIST,                 	 NULL,       0,       uptr:NULL,       defintarrayval:DEFGNBS,  TYPE_INTARRAY,    1}, \
{CONFIG_STRING_RU_ATT_TX,                   	 NULL,       0,       uptr:NULL,       defintval:0,		TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_ATT_RX,                   	 NULL,       0,       uptr:NULL,       defintval:0,		TYPE_UINT,	  0}, \
{CONFIG_STRING_RU_NBIOTRRC_LIST,               NULL,       0,       uptr:NULL,       defintarrayval:DEFGNBS,  TYPE_INTARRAY,    1}, \
}

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
#define GNB_CONFIG_STRING_MOBILE_COUNTRY_CODE           "mobile_country_code"
#define GNB_CONFIG_STRING_MOBILE_NETWORK_CODE           "mobile_network_code"
#define GNB_CONFIG_STRING_TRANSPORT_S_PREFERENCE        "tr_s_preference"
#define GNB_CONFIG_STRING_LOCAL_S_IF_NAME               "local_s_if_name"
#define GNB_CONFIG_STRING_LOCAL_S_ADDRESS               "local_s_address"
#define GNB_CONFIG_STRING_REMOTE_S_ADDRESS              "remote_s_address"
#define GNB_CONFIG_STRING_LOCAL_S_PORTC                 "local_s_portc"
#define GNB_CONFIG_STRING_REMOTE_S_PORTC                "remote_s_portc"
#define GNB_CONFIG_STRING_LOCAL_S_PORTD                 "local_s_portd"
#define GNB_CONFIG_STRING_REMOTE_S_PORTD                "remote_s_portd"

/*-----------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            cell configuration parameters                                                                */
/*   optname                                   helpstr   paramflags    XXXptr        defXXXval                   type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------*/
#define GNBPARAMS_DESC {\
{GNB_CONFIG_STRING_GNB_ID,                       NULL,   0,            uptr:NULL,   defintval:0,                 TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_CELL_TYPE,                    NULL,   0,            strptr:NULL, defstrval:"CELL_MACRO_GNB",  TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_GNB_NAME,                     NULL,   0,            strptr:NULL, defstrval:"OAIgNodeB",       TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_TRACKING_AREA_CODE,           NULL,   0,            strptr:NULL, defstrval:"0",               TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_MOBILE_COUNTRY_CODE,          NULL,   0,            strptr:NULL, defstrval:"0",               TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_MOBILE_NETWORK_CODE,          NULL,   0,            strptr:NULL, defstrval:"0",               TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_TRANSPORT_S_PREFERENCE,       NULL,   0,            strptr:NULL, defstrval:"local_mac",       TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_LOCAL_S_IF_NAME,              NULL,   0,            strptr:NULL, defstrval:"lo",              TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_LOCAL_S_ADDRESS,              NULL,   0,            strptr:NULL, defstrval:"127.0.0.1",       TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_REMOTE_S_ADDRESS,             NULL,   0,            strptr:NULL, defstrval:"127.0.0.2",       TYPE_STRING,    0},  \
{GNB_CONFIG_STRING_LOCAL_S_PORTC,                NULL,   0,            uptr:NULL,   defuintval:50000,            TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_REMOTE_S_PORTC,               NULL,   0,            uptr:NULL,   defuintval:50000,            TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_LOCAL_S_PORTD,                NULL,   0,            uptr:NULL,   defuintval:50001,            TYPE_UINT,      0},  \
{GNB_CONFIG_STRING_REMOTE_S_PORTD,               NULL,   0,            uptr:NULL,   defuintval:50001,            TYPE_UINT,      0},  \
}															     	
#define GNB_GNB_ID_IDX                  0
#define GNB_CELL_TYPE_IDX               1
#define GNB_GNB_NAME_IDX                2
#define GNB_TRACKING_AREA_CODE_IDX      3
#define GNB_MOBILE_COUNTRY_CODE_IDX     4
#define GNB_MOBILE_NETWORK_CODE_IDX     5
#define GNB_TRANSPORT_S_PREFERENCE_IDX  6
#define GNB_LOCAL_S_IF_NAME_IDX         7
#define GNB_LOCAL_S_ADDRESS_IDX         8
#define GNB_REMOTE_S_ADDRESS_IDX        9
#define GNB_LOCAL_S_PORTC_IDX           10
#define GNB_REMOTE_S_PORTC_IDX          11
#define GNB_LOCAL_S_PORTD_IDX           12
#define GNB_REMOTE_S_PORTD_IDX          13
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/		  


/* component carries configuration parameters name */

#define GNB_CONFIG_STRING_NB_ANT_PORTS                                                 "nb_antenna_ports"
#define GNB_CONFIG_STRING_NB_ANT_TX                                                    "nb_antennas_tx"
#define GNB_CONFIG_STRING_NB_ANT_RX                                                    "nb_antennas_rx"
#define GNB_CONFIG_STRING_TX_GAIN                                                      "tx_gain"
#define GNB_CONFIG_STRING_RX_GAIN                                                      "rx_gain"

  ///NR
  //MIB
#define GNB_CONFIG_STRING_MIB_SUBCARRIERSPACINGCOMMON                                  "MIB_subCarrierSpacingCommon"
#define GNB_CONFIG_STRING_MIB_SSB_SUBCARRIEROFFSET                                     "MIB_ssb_SubcarrierOffset"
#define GNB_CONFIG_STRING_MIB_DMRS_TYPEA_POSITION                                      "MIB_dmrs_TypeA_Position"
#define GNB_CONFIG_STRING_PDCCH_CONFIGSIB1                                             "pdcch_ConfigSIB1"

  //SIB1
#define GNB_CONFIG_STRING_SIB1_FREQUENCYOFFSETSSB                                      "SIB1_frequencyOffsetSSB"
#define GNB_CONFIG_STRING_SIB1_SSB_PERIODICITYSERVINGCELL                              "SIB1_ssb_PeriodicityServingCell"
#define GNB_CONFIG_STRING_SIB1_SS_PBCH_BLOCKPOWER                                      "SIB1_ss_PBCH_BlockPower"
  //NR FREQUENCYINFODL
#define GNB_CONFIG_STRING_ABSOLUTEFREQUENCYSSB                                         "absoluteFrequencySSB"
#define GNB_CONFIG_STRING_DL_FREQBANDINDICATORNR                                       "DL_FreqBandIndicatorNR"
#define GNB_CONFIG_STRING_DL_ABSOLUTEFREQUENCYPOINTA                                   "DL_absoluteFrequencyPointA"

  //NR DL SCS-SPECIFICCARRIER
#define GNB_CONFIG_STRING_DL_OFFSETTOCARRIER                                           "DL_offsetToCarrier"
#define GNB_CONFIG_STRING_DL_SCS_SUBCARRIERSPACING                                     "DL_SCS_SubcarrierSpacing"
#define GNB_CONFIG_STRING_DL_CARRIERBANDWIDTH                                          "DL_carrierBandwidth"

  // NR BWP-DOWNLINKCOMMON
#define GNB_CONFIG_STRING_DL_LOCATIONANDBANDWIDTH                                      "DL_locationAndBandwidth"
#define GNB_CONFIG_STRING_DL_BWP_SUBCARRIERSPACING                                     "DL_BWP_SubcarrierSpacing"
#define GNB_CONFIG_STRING_DL_BWP_PREFIX_TYPE                                           "DL_BWP_prefix_type"

  //NR FREQUENCYINFOUL
#define GNB_CONFIG_STRING_UL_FREQBANDINDICATORNR                                       "UL_FreqBandIndicatorNR"
#define GNB_CONFIG_STRING_UL_ABSOLUTEFREQUENCYPOINTA                                   "UL_absoluteFrequencyPointA"
#define GNB_CONFIG_STRING_UL_ADDITIONALSPECTRUMEMISSION                                "UL_additionalSpectrumEmission"
#define GNB_CONFIG_STRING_UL_P_MAX                                                     "UL_p_Max"
#define GNB_CONFIG_STRING_UL_FREQUENCYSHIFT7P5KHZ                                      "UL_frequencyShift7p5khz"

  //NR UL SCS-SPECIFICCARRIER
#define GNB_CONFIG_STRING_UL_OFFSETTOCARRIER                                           "UL_offsetToCarrier"
#define GNB_CONFIG_STRING_UL_SCS_SUBCARRIERSPACING                                     "UL_SCS_SubcarrierSpacing"
#define GNB_CONFIG_STRING_UL_CARRIERBANDWIDTH                                          "UL_carrierBandwidth"

  // NR BWP-UPLINKCOMMON
#define GNB_CONFIG_STRING_UL_LOCATIONANDBANDWIDTH                                      "UL_locationAndBandwidth"
#define GNB_CONFIG_STRING_UL_BWP_SUBCARRIERSPACING                                     "UL_BWP_SubcarrierSpacing"
#define GNB_CONFIG_STRING_UL_BWP_PREFIX_TYPE                                           "UL_BWP_prefix_type"
#define GNB_CONFIG_STRING_UL_TIMEALIGNMENTTIMERCOMMON                                  "UL_timeAlignmentTimerCommon"

#define GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_N_TIMINGADVANCEOFFSET                "ServingCellConfigCommon_n_TimingAdvanceOffset"
#define GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_SSB_POSITIONSINBURST_PR              "ServingCellConfigCommon_ssb_PositionsInBurst_PR"
#define GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_SSB_PERIODICITYSERVINGCELL           "ServingCellConfigCommon_ssb_periodicityServingCell"
#define GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_DMRS_TYPEA_POSITION                  "ServingCellConfigCommon_dmrs_TypeA_Position"
#define GNB_CONFIG_STRING_NIA_SUBCARRIERSPACING                                        "NIA_SubcarrierSpacing"
#define GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_SS_PBCH_BLOCKPOWER                   "ServingCellConfigCommon_ss_PBCH_BlockPower"


  //NR TDD-UL-DL-CONFIGCOMMON
#define GNB_CONFIG_STRING_REFERENCESUBCARRIERSPACING                                   "referenceSubcarrierSpacing"
#define GNB_CONFIG_STRING_DL_UL_TRANSMISSIONPERIODICITY                                "dl_UL_TransmissionPeriodicity"
#define GNB_CONFIG_STRING_NROFDOWNLINKSLOTS                                            "nrofDownlinkSlots"
#define GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS                                          "nrofDownlinkSymbols"
#define GNB_CONFIG_STRING_NROFUPLINKSLOTS                                              "nrofUplinkSlots"
#define GNB_CONFIG_STRING_NROFUPLINKSYMBOLS                                            "nrofUplinkSymbols"

  //NR RACH-CONFIGCOMMON
#define GNB_CONFIG_STRING_RACH_TOTALNUMBEROFRA_PREAMBLES                               "rach_totalNumberOfRA_Preambles"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_CHOICE        "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_choice"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEEIGHTH     "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneEighth"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEFOURTH     "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneFourth"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEHALF       "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneHalf"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONE           "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_one"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_TWO           "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_two"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_FOUR          "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_four"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_EIGHT         "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_eight"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_SIXTEEN       "rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_sixteen"
#define GNB_CONFIG_STRING_RACH_GROUPBCONFIGURED                                        "rach_groupBconfigured"
#define GNB_CONFIG_STRING_RACH_RA_MSG3SIZEGROUPA                                       "rach_ra_Msg3SizeGroupA"
#define GNB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB                                "rach_messagePowerOffsetGroupB"
#define GNB_CONFIG_STRING_RACH_NUMBEROFRA_PREAMBLESGROUPA                              "rach_numberOfRA_PreamblesGroupA"
#define GNB_CONFIG_STRING_RACH_RA_CONTENTIONRESOLUTIONTIMER                            "rach_ra_ContentionResolutionTimer"
#define GNB_CONFIG_STRING_RSRP_THRESHOLDSSB                                            "rsrp_ThresholdSSB"
#define GNB_CONFIG_STRING_RSRP_THRESHOLDSSB_SUL                                        "rsrp_ThresholdSSB_SUL"
#define GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_CHOICE                               "prach_RootSequenceIndex_choice"
#define GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_L839                                 "prach_RootSequenceIndex_l839"
#define GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_L139                                 "prach_RootSequenceIndex_l139"
#define GNB_CONFIG_STRING_PRACH_MSG1_SUBCARRIERSPACING                                 "prach_msg1_SubcarrierSpacing"
#define GNB_CONFIG_STRING_RESTRICTEDSETCONFIG                                          "restrictedSetConfig"
#define GNB_CONFIG_STRING_MSG3_TRANSFORMPRECODING                                      "msg3_transformPrecoding"
  //SSB-PERRACH-OCCASIONANDCB-PREAMBLESPERSSB NOT SURE

  //NR RACH-CONFIGGENERIC
#define GNB_CONFIG_STRING_PRACH_CONFIGURATIONINDEX                                     "prach_ConfigurationIndex"
#define GNB_CONFIG_STRING_PRACH_MSG1_FDM                                               "prach_msg1_FDM"
#define GNB_CONFIG_STRING_PRACH_MSG1_FREQUENCYSTART                                    "prach_msg1_FrequencyStart"
#define GNB_CONFIG_STRING_ZEROCORRELATIONZONECONFIG                                    "zeroCorrelationZoneConfig"
#define GNB_CONFIG_STRING_PREAMBLERECEIVEDTARGETPOWER                                  "preambleReceivedTargetPower"
#define GNB_CONFIG_STRING_PREAMBLETRANSMAX                                             "preambleTransMax"
#define GNB_CONFIG_STRING_POWERRAMPINGSTEP                                             "powerRampingStep"
#define GNB_CONFIG_STRING_RA_RESPONSEWINDOW                                            "ra_ResponseWindow"

  //PUSCH-CONFIGCOMMON
#define GNB_CONFIG_STRING_GROUPHOPPINGENABLEDTRANSFORMPRECODING                        "groupHoppingEnabledTransformPrecoding"
#define GNB_CONFIG_STRING_MSG3_DELTAPREAMBLE                                           "msg3_DeltaPreamble"
#define GNB_CONFIG_STRING_P0_NOMINALWITHGRANT                                          "p0_NominalWithGrant"

  ///PUSCH-TIMEDOMAINRESOURCEALLOCATION
#define GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_K2                        "PUSCH_TimeDomainResourceAllocation_k2"
#define GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE               "PUSCH_TimeDomainResourceAllocation_mappingType"
#define GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_STARTSYMBOLANDLENGTH      "PUSCH_TimeDomainResourceAllocation_startSymbolAndLength"

  //PUCCH-CONFIGCOMMON
#define GNB_CONFIG_STRING_PUCCH_RESOURCECOMMON                                         "pucch_ResourceCommon"
#define GNB_CONFIG_STRING_PUCCH_GROUPHOPPING                                           "pucch_GroupHopping"
#define GNB_CONFIG_STRING_HOPPINGID                                                    "hoppingId"
#define GNB_CONFIG_STRING_P0_NOMINAL                                                   "p0_nominal"

  //PDSCH-CONFIGCOMMON
  //PDSCH-TIMEDOMAINRESOURCEALLOCATION
#define GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_K0                        "PDSCH_TimeDomainResourceAllocation_k0"
#define GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE               "PDSCH_TimeDomainResourceAllocation_mappingType"
#define GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_STARTSYMBOLANDLENGTH      "PDSCH_TimeDomainResourceAllocation_startSymbolAndLength"
  //RATEMATCHPATTERN  IS USED TO CONFIGURE ONE RATE MATCHING PATTERN FOR PDSCH
#define GNB_CONFIG_STRING_RATEMATCHPATTERNID                                           "rateMatchPatternId"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_PATTERNTYPE                                 "RateMatchPattern_patternType"
#define GNB_CONFIG_STRING_SYMBOLSINRESOURCEBLOCK                                       "symbolsInResourceBlock"
#define GNB_CONFIG_STRING_PERIODICITYANDPATTERN                                        "periodicityAndPattern"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_CONTROLRESOURCESET                          "RateMatchPattern_controlResourceSet"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_SUBCARRIERSPACING                           "RateMatchPattern_subcarrierSpacing"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_MODE                                        "RateMatchPattern_mode"

  //PDCCH-CONFIGCOMMON
#define GNB_CONFIG_STRING_CONTROLRESOURCESETZERO                                       "controlResourceSetZero"
#define GNB_CONFIG_STRING_SEARCHSPACEZERO                                              "searchSpaceZero"
#define GNB_CONFIG_STRING_SEARCHSPACESIB1                                              "searchSpaceSIB1"
#define GNB_CONFIG_STRING_SEARCHSPACEOTHERSYSTEMINFORMATION                            "searchSpaceOtherSystemInformation"
#define GNB_CONFIG_STRING_PAGINGSEARCHSPACE                                            "pagingSearchSpace"
#define GNB_CONFIG_STRING_RA_SEARCHSPACE                                               "ra_SearchSpace"
  //NR PDCCH-CONFIGCOMMON COMMONCONTROLRESOURCESSETS
#define GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESETID                            "PDCCH_common_controlResourceSetId"
#define GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESET_DURATION                     "PDCCH_common_ControlResourceSet_duration"
#define GNB_CONFIG_STRING_PDCCH_CCE_REG_MAPPINGTYPE                                    "PDCCH_cce_REG_MappingType"
#define GNB_CONFIG_STRING_PDCCH_REG_BUNDLESIZE                                         "PDCCH_reg_BundleSize"
#define GNB_CONFIG_STRING_PDCCH_INTERLEAVERSIZE                                        "PDCCH_interleaverSize"
#define GNB_CONFIG_STRING_PDCCH_SHIFTINDEX                                             "PDCCH_shiftIndex"
#define GNB_CONFIG_STRING_PDCCH_PRECODERGRANULARITY                                    "PDCCH_precoderGranularity"
#define GNB_CONFIG_STRING_PDCCH_TCI_STATEID                                            "PDCCH_TCI_StateId"
#define GNB_CONFIG_STRING_TCI_PRESENTINDCI                                             "tci_PresentInDCI"
#define GNB_CONFIG_STRING_PDCCH_DMRS_SCRAMBLINGID                                      "pdcch_DMRS_ScramblingID"

  //NR PDCCH-ConfigCommon commonSearchSpaces
#define GNB_CONFIG_STRING_SEARCHSPACEID                                                "SearchSpaceId"
#define GNB_CONFIG_STRING_COMMONSEARCHSPACES_CONTROLRESOURCESETID                      "commonSearchSpaces_controlResourceSetId"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_CHOICE        "SearchSpace_monitoringSlotPeriodicityAndOffset_choice"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_VALUE         "SearchSpace_monitoringSlotPeriodicityAndOffset_value"
#define GNB_CONFIG_STRING_SEARCHSPACE_DURATION                                         "SearchSpace_duration"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL1                 "SearchSpace_nrofCandidates_aggregationLevel1"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL2                 "SearchSpace_nrofCandidates_aggregationLevel2"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL4                 "SearchSpace_nrofCandidates_aggregationLevel4"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL8                 "SearchSpace_nrofCandidates_aggregationLevel8"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL16                "SearchSpace_nrofCandidates_aggregationLevel16"
#define GNB_CONFIG_STRING_SEARCHSPACE_SEARCHSPACETYPE                                  "SearchSpace_searchSpaceType"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL1    "Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel1"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL2    "Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel2"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL4    "Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel4"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL8    "Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel8"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL16   "Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel16"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_3_MONITORINGPERIODICITY                   "Common_dci_Format2_3_monitoringPeriodicity"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_3_NROFPDCCH_CANDIDATES                    "Common_dci_Format2_3_nrofPDCCH_Candidates"
#define GNB_CONFIG_STRING_UE_SPECIFIC__DCI_FORMATS                                     "ue_Specific__dci_Formats"
#define GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_CARRIERFREQDL                        "RateMatchPatternLTE_CRS_carrierFreqDL"
#define GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_CARRIERBANDWIDTHDL                   "RateMatchPatternLTE_CRS_carrierBandwidthDL"
#define GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_NROFCRS_PORTS                        "RateMatchPatternLTE_CRS_nrofCRS_Ports"
#define GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_V_SHIFT                              "RateMatchPatternLTE_CRS_v_Shift"
#define GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_RADIOFRAMEALLOCATIONPERIOD           "RateMatchPatternLTE_CRS_radioframeAllocationPeriod"
#define GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_RADIOFRAMEALLOCATIONOFFSET           "RateMatchPatternLTE_CRS_radioframeAllocationOffset"
#define GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_SUBFRAMEALLOCATION_CHOICE            "RateMatchPatternLTE_CRS_subframeAllocation_choice"

/* init for checkedparam_t structure */

#define NRCCPARAMS_CHECK  {                                     \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
             { .s5= {NULL }} ,                 \
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                                                             component carriers configuration parameters                                                                                                                           */
/*   optname                                                                        helpstr   paramflags    XXXptr                                                                  defXXXval                       type         numelt  checked_param  */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define NRCCPARAMS_DESC { \
{GNB_CONFIG_STRING_FRAME_TYPE,                                                       NULL,        0,        strptr:&frame_type,                                                     defstrval:"FDD",                TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_DL_PREFIX_TYPE,                                                   NULL,        0,        strptr:&DL_prefix_type,                                                 defstrval:"NORMAL",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_UL_PREFIX_TYPE,                                                   NULL,        0,        strptr:&UL_prefix_type,                                                 defstrval:"NORMAL",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_EUTRA_BAND,                                                       NULL,        0,        iptr:&eutra_band,                                                       defintval:7,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DOWNLINK_FREQUENCY,                                               NULL,        0,        i64ptr:(int64_t *)&downlink_frequency,                                  defint64val:2680000000,         TYPE_UINT64,     0},  \
{GNB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET,                                          NULL,        0,        iptr:&uplink_frequency_offset,                                          defintval:-120000000,           TYPE_INT,        0},  \
{GNB_CONFIG_STRING_NID_CELL,                                                         NULL,        0,        iptr:&Nid_cell,                                                         defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_N_RB_DL,                                                          NULL,        0,        iptr:&N_RB_DL,                                                          defintval:25,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NB_ANT_PORTS,                                                     NULL,        0,        iptr:&nb_antenna_ports,                                                 defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_MIB_SUBCARRIERSPACINGCOMMON,                                      NULL,        0,        iptr:&MIB_subCarrierSpacingCommon,                                      defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_MIB_SSB_SUBCARRIEROFFSET,                                         NULL,        0,        iptr:&MIB_ssb_SubcarrierOffset,                                         defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_MIB_DMRS_TYPEA_POSITION,                                          NULL,        0,        iptr:&MIB_dmrs_TypeA_Position,                                          defintval:2,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_CONFIGSIB1,                                                 NULL,        0,        iptr:&pdcch_ConfigSIB1,                                                 defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SIB1_FREQUENCYOFFSETSSB,                                          NULL,        0,        strptr:&SIB1_frequencyOffsetSSB,                                        defstrval:"khz5",               TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_SIB1_SSB_PERIODICITYSERVINGCELL,                                  NULL,        0,        iptr:&SIB1_ssb_PeriodicityServingCell,                                  defintval:5,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SIB1_SS_PBCH_BLOCKPOWER,                                          NULL,        0,        iptr:&SIB1_ss_PBCH_BlockPower,                                          defintval:-60,                  TYPE_INT,        0},  \
{GNB_CONFIG_STRING_ABSOLUTEFREQUENCYSSB,                                             NULL,        0,        iptr:&absoluteFrequencySSB,                                             defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_FREQBANDINDICATORNR,                                           NULL,        0,        iptr:&DL_FreqBandIndicatorNR,                                           defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_ABSOLUTEFREQUENCYPOINTA,                                       NULL,        0,        iptr:&DL_absoluteFrequencyPointA,                                       defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_OFFSETTOCARRIER,                                               NULL,        0,        iptr:&DL_offsetToCarrier,                                               defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_SCS_SUBCARRIERSPACING,                                         NULL,        0,        strptr:&DL_SCS_SubcarrierSpacing,                                       defstrval:"kHz15",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_DL_CARRIERBANDWIDTH,                                              NULL,        0,        iptr:&DL_carrierBandwidth,                                              defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_LOCATIONANDBANDWIDTH,                                          NULL,        0,        iptr:&DL_locationAndBandwidth,                                          defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_BWP_SUBCARRIERSPACING,                                         NULL,        0,        strptr:&DL_BWP_SubcarrierSpacing,                                       defstrval:"kHz15",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_DL_BWP_PREFIX_TYPE,                                               NULL,        0,        strptr:&DL_BWP_prefix_type,                                             defstrval:"NORMAL",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_UL_FREQBANDINDICATORNR,                                           NULL,        0,        iptr:&UL_FreqBandIndicatorNR,                                           defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_ABSOLUTEFREQUENCYPOINTA,                                       NULL,        0,        iptr:&UL_absoluteFrequencyPointA,                                       defintval:13,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_ADDITIONALSPECTRUMEMISSION,                                    NULL,        0,        iptr:&UL_additionalSpectrumEmission,                                    defintval:3,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_P_MAX,                                                         NULL,        0,        iptr:&UL_p_Max,                                                         defintval:-1,                   TYPE_INT,        0},  \
{GNB_CONFIG_STRING_UL_FREQUENCYSHIFT7P5KHZ,                                          NULL,        0,        strptr:&UL_frequencyShift7p5khz,                                        defstrval:"TRUE",               TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_UL_OFFSETTOCARRIER,                                               NULL,        0,        iptr:&UL_offsetToCarrier,                                               defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_SCS_SUBCARRIERSPACING,                                         NULL,        0,        strptr:&UL_SCS_SubcarrierSpacing,                                       defstrval:"kHz15",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_UL_CARRIERBANDWIDTH,                                              NULL,        0,        iptr:&UL_carrierBandwidth,                                              defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_LOCATIONANDBANDWIDTH,                                          NULL,        0,        iptr:&UL_locationAndBandwidth,                                          defintval:15,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_BWP_SUBCARRIERSPACING,                                         NULL,        0,        strptr:&UL_BWP_SubcarrierSpacing,                                       defstrval:"kHz15",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_UL_BWP_PREFIX_TYPE,                                               NULL,        0,        strptr:&UL_BWP_prefix_type,                                             defstrval:"NORMAL",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_UL_TIMEALIGNMENTTIMERCOMMON,                                      NULL,        0,        strptr:&UL_timeAlignmentTimerCommon,                                    defstrval:"infinity",           TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_N_TIMINGADVANCEOFFSET,                    NULL,        0,        strptr:&ServingCellConfigCommon_n_TimingAdvanceOffset,                  defstrval:"n0",                 TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_SSB_POSITIONSINBURST_PR,                  NULL,        0,        strptr:&ServingCellConfigCommon_ssb_PositionsInBurst_PR,                defstrval:"shortBitmap",        TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_SSB_PERIODICITYSERVINGCELL,               NULL,        0,        iptr:&ServingCellConfigCommon_ssb_periodicityServingCell,               defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_DMRS_TYPEA_POSITION,                      NULL,        0,        iptr:&ServingCellConfigCommon_dmrs_TypeA_Position,                      defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NIA_SUBCARRIERSPACING,                                            NULL,        0,        strptr:&NIA_SubcarrierSpacing,                                          defstrval:"kHz15",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_SS_PBCH_BLOCKPOWER,                       NULL,        0,        iptr:&ServingCellConfigCommon_ss_PBCH_BlockPower,                       defintval:-60,                  TYPE_INT,        0},  \
{GNB_CONFIG_STRING_REFERENCESUBCARRIERSPACING,                                       NULL,        0,        strptr:&referenceSubcarrierSpacing,                                     defstrval:"kHz15",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_DL_UL_TRANSMISSIONPERIODICITY,                                    NULL,        0,        strptr:&dl_UL_TransmissionPeriodicity,                                  defstrval:"ms0p5",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_NROFDOWNLINKSLOTS,                                                NULL,        0,        iptr:&nrofDownlinkSlots,                                                defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS,                                              NULL,        0,        iptr:&nrofDownlinkSymbols,                                              defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NROFUPLINKSLOTS,                                                  NULL,        0,        iptr:&nrofUplinkSlots,                                                  defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NROFUPLINKSYMBOLS,                                                NULL,        0,        iptr:&nrofUplinkSymbols,                                                defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_TOTALNUMBEROFRA_PREAMBLES,                                   NULL,        0,        iptr:&rach_totalNumberOfRA_Preambles,                                   defintval:63,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_CHOICE,            NULL,        0,        strptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_choice,          defstrval:"oneEighth",          TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEEIGHTH,         NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneEighth,         defintval:4,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEFOURTH,         NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneFourth,         defintval:8,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEHALF,           NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneHalf,           defintval:16,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONE,               NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_one,               defintval:24,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_TWO,               NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_two,               defintval:32,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_FOUR,              NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_four,              defintval:8,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_EIGHT,             NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_eight,             defintval:4,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_SIXTEEN,           NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_sixteen,           defintval:2,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_GROUPBCONFIGURED,                                            NULL,        0,        strptr:&rach_groupBconfigured,                                          defstrval:"ENABLE",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_RACH_RA_MSG3SIZEGROUPA,                                           NULL,        0,        iptr:&rach_ra_Msg3SizeGroupA,                                           defintval:56,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB,                                    NULL,        0,        strptr:&rach_messagePowerOffsetGroupB,                                  defstrval:"dB0",                TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_RACH_NUMBEROFRA_PREAMBLESGROUPA,                                  NULL,        0,        iptr:&rach_numberOfRA_PreamblesGroupA,                                  defintval:32,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_RA_CONTENTIONRESOLUTIONTIMER,                                NULL,        0,        iptr:&rach_ra_ContentionResolutionTimer,                                defintval:8,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RSRP_THRESHOLDSSB,                                                NULL,        0,        iptr:&rsrp_ThresholdSSB,                                                defintval:64,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RSRP_THRESHOLDSSB_SUL,                                            NULL,        0,        iptr:&rsrp_ThresholdSSB_SUL,                                            defintval:64,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_CHOICE,                                   NULL,        0,        strptr:&prach_RootSequenceIndex_choice,                                 defstrval:"l839",               TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_L839,                                     NULL,        0,        iptr:&prach_RootSequenceIndex_l839,                                     defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_L139,                                     NULL,        0,        iptr:&prach_RootSequenceIndex_l139,                                     defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_MSG1_SUBCARRIERSPACING,                                     NULL,        0,        strptr:&prach_msg1_SubcarrierSpacing,                                   defstrval:"kHz15",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_RESTRICTEDSETCONFIG,                                              NULL,        0,        strptr:&restrictedSetConfig,                                            defstrval:"unrestrictedSet",    TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_MSG3_TRANSFORMPRECODING,                                          NULL,        0,        strptr:&msg3_transformPrecoding,                                        defstrval:"ENABLE",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PRACH_CONFIGURATIONINDEX,                                         NULL,        0,        iptr:&prach_ConfigurationIndex,                                         defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_MSG1_FDM,                                                   NULL,        0,        strptr:&prach_msg1_FDM,                                                 defstrval:"one",                TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PRACH_MSG1_FREQUENCYSTART,                                        NULL,        0,        iptr:&prach_msg1_FrequencyStart,                                        defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_ZEROCORRELATIONZONECONFIG,                                        NULL,        0,        iptr:&zeroCorrelationZoneConfig,                                        defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PREAMBLERECEIVEDTARGETPOWER,                                      NULL,        0,        iptr:&preambleReceivedTargetPower,                                      defintval:-150,                 TYPE_INT,        0},  \
{GNB_CONFIG_STRING_PREAMBLETRANSMAX,                                                 NULL,        0,        iptr:&preambleTransMax,                                                 defintval:6,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_POWERRAMPINGSTEP,                                                 NULL,        0,        strptr:&powerRampingStep,                                               defstrval:"dB0",                TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_RA_RESPONSEWINDOW,                                                NULL,        0,        iptr:&ra_ResponseWindow,                                                defintval:8,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_GROUPHOPPINGENABLEDTRANSFORMPRECODING,                            NULL,        0,        strptr:&groupHoppingEnabledTransformPrecoding,                          defstrval:"ENABLE",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_MSG3_DELTAPREAMBLE,                                               NULL,        0,        iptr:&msg3_DeltaPreamble,                                               defintval:0,                    TYPE_INT,        0},  \
{GNB_CONFIG_STRING_P0_NOMINALWITHGRANT,                                              NULL,        0,        iptr:&p0_NominalWithGrant,                                              defintval:0,                    TYPE_INT,        0},  \
{GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_K2,                            NULL,        0,        iptr:&PUSCH_TimeDomainResourceAllocation_k2,                            defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE,                   NULL,        0,        strptr:&PUSCH_TimeDomainResourceAllocation_mappingType,                 defstrval:"typeA",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_STARTSYMBOLANDLENGTH,          NULL,        0,        iptr:&PUSCH_TimeDomainResourceAllocation_startSymbolAndLength,          defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PUCCH_RESOURCECOMMON,                                             NULL,        0,        iptr:&pucch_ResourceCommon,                                             defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PUCCH_GROUPHOPPING,                                               NULL,        0,        strptr:&pucch_GroupHopping,                                             defstrval:"neither",            TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_HOPPINGID,                                                        NULL,        0,        iptr:&hoppingId,                                                        defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_P0_NOMINAL,                                                       NULL,        0,        iptr:&p0_nominal,                                                       defintval:-30,                  TYPE_INT,        0},  \
{GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_K0,                            NULL,        0,        iptr:&PDSCH_TimeDomainResourceAllocation_k0,                            defintval:2,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE,                   NULL,        0,        strptr:&PDSCH_TimeDomainResourceAllocation_mappingType,                 defstrval:"typeA",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_STARTSYMBOLANDLENGTH,          NULL,        0,        iptr:&PDSCH_TimeDomainResourceAllocation_startSymbolAndLength,          defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNID,                                               NULL,        0,        iptr:&rateMatchPatternId,                                               defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERN_PATTERNTYPE,                                     NULL,        0,        strptr:&RateMatchPattern_patternType,                                   defstrval:"bitmaps",            TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_SYMBOLSINRESOURCEBLOCK,                                           NULL,        0,        strptr:&symbolsInResourceBlock,                                         defstrval:"oneSlot",            TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PERIODICITYANDPATTERN,                                            NULL,        0,        iptr:&periodicityAndPattern,                                            defintval:2,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERN_CONTROLRESOURCESET,                              NULL,        0,        iptr:&RateMatchPattern_controlResourceSet,                              defintval:5,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERN_SUBCARRIERSPACING,                               NULL,        0,        strptr:&RateMatchPattern_subcarrierSpacing,                             defstrval:"kHz15",              TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERN_MODE,                                            NULL,        0,        strptr:&RateMatchPattern_mode,                                          defstrval:"dynamic",            TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_CONTROLRESOURCESETZERO,                                           NULL,        0,        iptr:&controlResourceSetZero,                                           defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACEZERO,                                                  NULL,        0,        iptr:&searchSpaceZero,                                                  defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACESIB1,                                                  NULL,        0,        iptr:&searchSpaceSIB1,                                                  defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACEOTHERSYSTEMINFORMATION,                                NULL,        0,        iptr:&searchSpaceOtherSystemInformation,                                defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PAGINGSEARCHSPACE,                                                NULL,        0,        iptr:&pagingSearchSpace,                                                defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RA_SEARCHSPACE,                                                   NULL,        0,        iptr:&ra_SearchSpace,                                                   defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESETID,                                NULL,        0,        iptr:&PDCCH_common_controlResourceSetId,                                defintval:5,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESET_DURATION,                         NULL,        0,        iptr:&PDCCH_common_ControlResourceSet_duration,                         defintval:2,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_CCE_REG_MAPPINGTYPE,                                        NULL,        0,        strptr:&PDCCH_cce_REG_MappingType,                                      defstrval:"nonInterleaved",     TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PDCCH_REG_BUNDLESIZE,                                             NULL,        0,        iptr:&PDCCH_reg_BundleSize,                                             defintval:3,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_INTERLEAVERSIZE,                                            NULL,        0,        iptr:&PDCCH_interleaverSize,                                            defintval:3,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_SHIFTINDEX,                                                 NULL,        0,        iptr:&PDCCH_shiftIndex,                                                 defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_PRECODERGRANULARITY,                                        NULL,        0,        strptr:&PDCCH_precoderGranularity,                                      defstrval:"sameAsREG-bundle",   TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PDCCH_TCI_STATEID,                                                NULL,        0,        iptr:&PDCCH_TCI_StateId,                                                defintval:32,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_TCI_PRESENTINDCI,                                                 NULL,        0,        strptr:&tci_PresentInDCI,                                               defstrval:"ENABLE",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_PDCCH_DMRS_SCRAMBLINGID,                                          NULL,        0,        iptr:&PDCCH_DMRS_ScramblingID,                                          defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACEID,                                                    NULL,        0,        iptr:&SearchSpaceId,                                                    defintval:10,                   TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMONSEARCHSPACES_CONTROLRESOURCESETID,                          NULL,        0,        iptr:&commonSearchSpaces_controlResourceSetId,                          defintval:5,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_CHOICE,            NULL,        0,        strptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_choice,          defstrval:"sl1",                TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_VALUE,             NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_value,             defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_DURATION,                                             NULL,        0,        iptr:&SearchSpace_duration,                                             defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL1,                     NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel1,                     defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL2,                     NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel2,                     defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL4,                     NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel4,                     defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL8,                     NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel8,                     defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL16,                    NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel16,                    defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_SEARCHSPACETYPE,                                      NULL,        0,        strptr:&SearchSpace_searchSpaceType,                                    defstrval:"common",             TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL1,        NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel1,        defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL2,        NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel2,        defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL4,        NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel4,        defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL8,        NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel8,        defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL16,       NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel16,       defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_3_MONITORINGPERIODICITY,                       NULL,        0,        iptr:&Common_dci_Format2_3_monitoringPeriodicity,                       defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_3_NROFPDCCH_CANDIDATES,                        NULL,        0,        iptr:&Common_dci_Format2_3_nrofPDCCH_Candidates,                        defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UE_SPECIFIC__DCI_FORMATS,                                         NULL,        0,        strptr:&ue_Specific__dci_Formats,                                       defstrval:"formats0-0-And-1-0", TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_CARRIERFREQDL,                            NULL,        0,        iptr:&RateMatchPatternLTE_CRS_carrierFreqDL,                            defintval:6,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_CARRIERBANDWIDTHDL,                       NULL,        0,        iptr:&RateMatchPatternLTE_CRS_carrierBandwidthDL,                       defintval:6,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_NROFCRS_PORTS,                            NULL,        0,        iptr:&RateMatchPatternLTE_CRS_nrofCRS_Ports,                            defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_V_SHIFT,                                  NULL,        0,        iptr:&RateMatchPatternLTE_CRS_v_Shift,                                  defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_RADIOFRAMEALLOCATIONPERIOD,               NULL,        0,        iptr:&RateMatchPatternLTE_CRS_radioframeAllocationPeriod,               defintval:1,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_RADIOFRAMEALLOCATIONOFFSET,               NULL,        0,        iptr:&RateMatchPatternLTE_CRS_radioframeAllocationOffset,               defintval:0,                    TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNLTE_CRS_SUBFRAMEALLOCATION_CHOICE,                NULL,        0,        strptr:&RateMatchPatternLTE_CRS_subframeAllocation_choice,              defstrval:"oneFrame",           TYPE_STRING,     0},  \
}


/* component carries configuration parameters name */
#define GNB_CONFIG_FRAME_TYPE_IDX                                                   0
#define GNB_CONFIG_DL_PREFIX_TYPE_IDX                                               1
#define GNB_CONFIG_UL_PREFIX_TYPE_IDX                                               2
#define GNB_CONFIG_EUTRA_BAND_IDX                                                   3
#define GNB_CONFIG_DOWNLINK_FREQUENCY_IDX                                           4
#define GNB_CONFIG_UPLINK_FREQUENCY_OFFSET_IDX                                      5
#define GNB_CONFIG_NID_CELL_IDX                                                     6
#define GNB_CONFIG_N_RB_DL_IDX                                                      7
#define GNB_CONFIG_NB_ANT_PORTS_IDX                                                 8
#define GNB_CONFIG_NB_ANT_TX_IDX                                                    9
#define GNB_CONFIG_NB_ANT_RX_IDX                                                    10
#define GNB_CONFIG_TX_GAIN_IDX                                                      11
#define GNB_CONFIG_RX_GAIN_IDX                                                      12
#define GNB_CONFIG_MIB_SUBCARRIERSPACINGCOMMON_IDX                                  13
#define GNB_CONFIG_MIB_SSB_SUBCARRIEROFFSET_IDX                                     14
#define GNB_CONFIG_MIB_DMRS_TYPEA_POSITION_IDX                                      15
#define GNB_CONFIG_PDCCH_CONFIGSIB1_IDX                                             16
#define GNB_CONFIG_SIB1_FREQUENCYOFFSETSSB_IDX                                      17
#define GNB_CONFIG_SIB1_SSB_PERIODICITYSERVINGCELL_IDX                              18
#define GNB_CONFIG_SIB1_SS_PBCH_BLOCKPOWER_IDX                                      19
#define GNB_CONFIG_ABSOLUTEFREQUENCYSSB_IDX                                         20
#define GNB_CONFIG_DL_FREQBANDINDICATORNR_IDX                                       22
#define GNB_CONFIG_DL_ABSOLUTEFREQUENCYPOINTA_IDX                                   23
#define GNB_CONFIG_DL_OFFSETTOCARRIER_IDX                                           24
#define GNB_CONFIG_DL_SUBCARRIERSPACING_IDX                                         25
#define GNB_CONFIG_DL_CARRIERBANDWIDTH_IDX                                          27
#define GNB_CONFIG_DL_LOCATIONANDBANDWIDTH_IDX                                      28
#define GNB_CONFIG_DL_BWP_SUBCARRIERSPACING_IDX                                     29
#define GNB_CONFIG_DL_BWP_PREFIX_TYPE_IDX                                           30
#define GNB_CONFIG_UL_FREQBANDINDICATORNR_IDX                                       31
#define GNB_CONFIG_UL_ABSOLUTEFREQUENCYPOINTA_IDX                                   32
#define GNB_CONFIG_UL_ADDITIONALSPECTRUMEMISSION_IDX                                33
#define GNB_CONFIG_UL_P_MAX_IDX                                                     34
#define GNB_CONFIG_UL_FREQUENCYSHIFT7P5KHZ_IDX                                      35
#define GNB_CONFIG_UL_OFFSETTOCARRIER_IDX                                           36
#define GNB_CONFIG_UL_SCS_SUBCARRIERSPACING_IDX                                     37
#define GNB_CONFIG_UL_CARRIERBANDWIDTH_IDX                                          39
#define GNB_CONFIG_UL_LOCATIONANDBANDWIDTH_IDX                                      41
#define GNB_CONFIG_UL_BWP_SUBCARRIERSPACING_IDX                                     42
#define GNB_CONFIG_UL_BWP_PREFIX_TYPE_IDX                                           43
#define GNB_CONFIG_TIMEALIGNMENTTIMERCOMMON_IDX                                     44
#define GNB_CONFIG_SERVINGCELLCONFIGCOMMON_N_TIMINGADVANCEOFFSET_IDX
#define GNB_CONFIG_SERVINGCELLCONFIGCOMMON_SSB_POSITIONSINBURST_PR_IDX              45
#define GNB_CONFIG_SERVINGCELLCONFIGCOMMON_SSB_PERIODICITYSERVINGCELL_IDX           46
#define GNB_CONFIG_SERVINGCELLCONFIGCOMMON_DMRS_TYPEA_POSITION_IDX                  47
#define GNB_CONFIG_NIA_SUBCARRIERSPACING_IDX                                        48
#define GNB_CONFIG_SERVINGCELLCONFIGCOMMON_SS_PBCH_BLOCKPOWER_IDX                   49
#define GNB_CONFIG_REFERENCESUBCARRIERSPACING_IDX                                   50
#define GNB_CONFIG_DL_UL_TRANSMISSIONPERIODICITY_IDX                                51
#define GNB_CONFIG_NROFDOWNLINKSLOTS_IDX                                            52
#define GNB_CONFIG_NROFDOWNLINKSYMBOLS_IDX                                          53
#define GNB_CONFIG_NROFUPLINKSLOTS_IDX                                              54
#define GNB_CONFIG_NROFUPLINKSYMBOLS_IDX                                            55
#define GNB_CONFIG_RACH_TOTALNUMBEROFRA_PREAMBLES_IDX                               56
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_CHOICE_IDX        57
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEEIGHTH_IDX     58
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEFOURTH_IDX     59
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEHALF_IDX       60
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONE_IDX           61
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_TWO_IDX           62
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_FOUR_IDX          63
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_EIGHT_IDX         64
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_SIXTEEN_IDX       65
#define GNB_CONFIG_RACH_GROUPBCONFIGURED_IDX                                        66
#define GNB_CONFIG_RACH_RA_MSG3SIZEGROUPA_IDX                                       67
#define GNB_CONFIG_RACH_MESSAGEPOWEROFFSETGROUPB_IDX                                68
#define GNB_CONFIG_RACH_NUMBEROFRA_PREAMBLESGROUPA_IDX                              69
#define GNB_CONFIG_RACH_RA_CONTENTIONRESOLUTIONTIMER_IDX                            70
#define GNB_CONFIG_RSRP_THRESHOLDSSB_IDX                                            71
#define GNB_CONFIG_RSRP_THRESHOLDSSB_SUL_IDX                                        72
#define GNB_CONFIG_PRACH_ROOTSEQUENCEINDEX_CHOICE_IDX                               73
#define GNB_CONFIG_PRACH_ROOTSEQUENCEINDEX_L839_IDX                                 74
#define GNB_CONFIG_PRACH_ROOTSEQUENCEINDEX_L139_IDX                                 75
#define GNB_CONFIG_PRACH_MSG1_SUBCARRIERSPACING_IDX                                 76
#define GNB_CONFIG_RESTRICTEDSETCONFIG_IDX                                          77
#define GNB_CONFIG_MSG3_TRANSFORMPRECODING_IDX                                      78
#define GNB_CONFIG_PRACH_CONFIGURATIONINDEX_IDX                                     79
#define GNB_CONFIG_PRACH_MSG1_FDM_IDX                                               80
#define GNB_CONFIG_PRACH_MSG1_FREQUENCYSTART_IDX                                    81
#define GNB_CONFIG_ZEROCORRELATIONZONECONFIG_IDX                                    82
#define GNB_CONFIG_PREAMBLERECEIVEDTARGETPOWER_IDX                                  83
#define GNB_CONFIG_PREAMBLETRANSMAX_IDX                                             84
#define GNB_CONFIG_POWERRAMPINGSTEP_IDX                                             85
#define GNB_CONFIG_RA_RESPONSEWINDOW_IDX                                            86
#define GNB_CONFIG_GROUPHOPPINGENABLEDTRANSFORMPRECODING_IDX                        87
#define GNB_CONFIG_MSG3_DELTAPREAMBLE_IDX                                           88
#define GNB_CONFIG_P0_NOMINALWITHGRANT_IDX                                          89
#define GNB_CONFIG_PUSCH_TIMEDOMAINRESOURCEALLOCATION_K2_IDX                        90
#define GNB_CONFIG_PUSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE_IDX               91
#define GNB_CONFIG_PUSCH_TIMEDOMAINRESOURCEALLOCATION_STARTSYMBOLANDLENGTH_IDX      92
#define GNB_CONFIG_PUCCH_RESOURCECOMMON_IDX                                         93
#define GNB_CONFIG_PUCCH_GROUPHOPPING_IDX                                           94
#define GNB_CONFIG_HOPPINGID_IDX                                                    95
#define GNB_CONFIG_P0_NOMINAL_IDX                                                   96
#define GNB_CONFIG_PDSCH_TIMEDOMAINRESOURCEALLOCATION_K0_IDX                        97
#define GNB_CONFIG_PDSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE_IDX               98
#define GNB_CONFIG_PDSCH_TIMEDOMAINRESOURCEALLOCATION_STARTSYMBOLANDLENGTH          
#define GNB_CONFIG_RATEMATCHPATTERNID_IDX                                           99
#define GNB_CONFIG_RATEMATCHPATTERN_PATTERNTYPE_IDX                                 100
#define GNB_CONFIG_SYMBOLSINRESOURCEBLOCK_IDX                                       101
#define GNB_CONFIG_PERIODICITYANDPATTERN_IDX                                        102
#define GNB_CONFIG_RATEMATCHPATTERN_CONTROLRESOURCESET_IDX                          103
#define GNB_CONFIG_RATEMATCHPATTERN_SUBCARRIERSPACING_IDX                           104
#define GNB_CONFIG_RATEMATCHPATTERN_MODE_IDX                                        105
#define GNB_CONFIG_CONTROLRESOURCESETZERO_IDX                                       106
#define GNB_CONFIG_SEARCHSPACEZERO_IDX                                              107
#define GNB_CONFIG_SEARCHSPACESIB1_IDX                                              108
#define GNB_CONFIG_SEARCHSPACEOTHERSYSTEMINFORMATION_IDX                            109
#define GNB_CONFIG_PAGINGSEARCHSPACE_IDX                                            110
#define GNB_CONFIG_RA_SEARCHSPACE_IDX                                               111
#define GNB_CONFIG_PDCCH_COMMON_CONTROLRESOURCESETID_IDX                            112
#define GNB_CONFIG_PDCCH_COMMON_CONTROLRESOURCESET_DURATION_IDX                     113
#define GNB_CONFIG_PDCCH_CCE_REG_MAPPINGTYPE_IDX                                    114
#define GNB_CONFIG_PDCCH_REG_BUNDLESIZE_IDX                                         115
#define GNB_CONFIG_PDCCH_INTERLEAVERSIZE_IDX                                        116
#define GNB_CONFIG_PDCCH_SHIFTINDEX_IDX                                             117
#define GNB_CONFIG_PDCCH_PRECODERGRANULARITY_IDX                                    118
#define GNB_CONFIG_PDCCH_TCI_STATEID_IDX                                            119
#define GNB_CONFIG_TCI_PRESENTINDCI_IDX                                             120
#define GNB_CONFIG_PDCCH_DMRS_SCRAMBLINGID_IDX                                      121
#define GNB_CONFIG_SEARCHSPACEID_IDX                                                122
#define GNB_CONFIG_COMMONSEARCHSPACES_CONTROLRESOURCESETID_IDX                      123
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_CHOICE_IDX        124
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_VALUE_IDX         125
#define GNB_CONFIG_SEARCHSPACE_DURATION_IDX                                         126
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL1_IDX                 127
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL2_IDX                 128
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL4_IDX                 129
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL8_IDX                 130
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL16_IDX                131
#define GNB_CONFIG_SEARCHSPACE_SEARCHSPACETYPE_IDX                                  132
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL1_IDX    133
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL2_IDX    134
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL4_IDX    135
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL8_IDX    136
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL16_IDX   137
#define GNB_CONFIG_COMMON_DCI_FORMAT2_3_MONITORINGPERIODICITY_IDX                   138
#define GNB_CONFIG_COMMON_DCI_FORMAT2_3_NROFPDCCH_CANDIDATES_IDX                    139
#define GNB_CONFIG_UE_SPECIFIC__DCI_FORMATS_IDX                                     140
#define GNB_CONFIG_RATEMATCHPATTERNLTE_CRS_CARRIERFREQDL_IDX                        141
#define GNB_CONFIG_RATEMATCHPATTERNLTE_CRS_CARRIERBANDWIDTHDL_IDX                   142
#define GNB_CONFIG_RATEMATCHPATTERNLTE_CRS_NROFCRS_PORTS_IDX                        143
#define GNB_CONFIG_RATEMATCHPATTERNLTE_CRS_V_SHIFT_IDX                              144
#define GNB_CONFIG_RATEMATCHPATTERNLTE_CRS_RADIOFRAMEALLOCATIONPERIOD_IDX           145
#define GNB_CONFIG_RATEMATCHPATTERNLTE_CRS_RADIOFRAMEALLOCATIONOFFSET_IDX           146
#define GNB_CONFIG_RATEMATCHPATTERNLTE_CRS_SUBFRAMEALLOCATION_CHOICE_IDX            147


/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* SRB1 configuration parameters section name */
#define GNB_CONFIG_STRING_SRB1                                          "srb1_parameters"

/* SRB1 configuration parameters names   */
#define GNB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT                    "timer_poll_retransmit"
#define GNB_CONFIG_STRING_SRB1_TIMER_REORDERING                         "timer_reordering"
#define GNB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT                    "timer_status_prohibit"
#define GNB_CONFIG_STRING_SRB1_POLL_PDU                                 "poll_pdu"
#define GNB_CONFIG_STRING_SRB1_POLL_BYTE                                "poll_byte"
#define GNB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD                       "max_retx_threshold"

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            SRB1 configuration parameters                                                                                  */
/*   optname                                          helpstr   paramflags    XXXptr                             defXXXval         type           numelt     */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define SRB1PARAMS_DESC {                                                                                                \
{GNB_CONFIG_STRING_SRB1_TIMER_POLL_RETRANSMIT,         NULL,   0,            iptr:&srb1_timer_poll_retransmit,   defintval:80,     TYPE_UINT,      0},       \
{GNB_CONFIG_STRING_SRB1_TIMER_REORDERING,              NULL,   0,            iptr:&srb1_timer_reordering,        defintval:35,     TYPE_UINT,      0},       \
{GNB_CONFIG_STRING_SRB1_TIMER_STATUS_PROHIBIT,         NULL,   0,            iptr:&srb1_timer_status_prohibit,   defintval:0,      TYPE_UINT,      0},       \
{GNB_CONFIG_STRING_SRB1_POLL_PDU,                      NULL,   0,            iptr:&srb1_poll_pdu,                defintval:4,      TYPE_UINT,      0},       \
{GNB_CONFIG_STRING_SRB1_POLL_BYTE,                     NULL,   0,            iptr:&srb1_poll_byte,               defintval:99999,  TYPE_UINT,      0},       \
{GNB_CONFIG_STRING_SRB1_MAX_RETX_THRESHOLD,            NULL,   0,            iptr:&srb1_max_retx_threshold,      defintval:4,      TYPE_UINT,      0}        \
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* MME configuration parameters section name */
#define GNB_CONFIG_STRING_MME_IP_ADDRESS                "mme_ip_address"

/* SRB1 configuration parameters names   */


#define GNB_CONFIG_STRING_MME_IPV4_ADDRESS              "ipv4"
#define GNB_CONFIG_STRING_MME_IPV6_ADDRESS              "ipv6"
#define GNB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE         "active"
#define GNB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE     "preference"


/*-------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            MME configuration parameters                                                             */
/*   optname                                          helpstr   paramflags    XXXptr       defXXXval         type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------*/
#define S1PARAMS_DESC {  \
{GNB_CONFIG_STRING_MME_IPV4_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{GNB_CONFIG_STRING_MME_IPV6_ADDRESS,                   NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{GNB_CONFIG_STRING_MME_IP_ADDRESS_ACTIVE,              NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
{GNB_CONFIG_STRING_MME_IP_ADDRESS_PREFERENCE,          NULL,      0,         uptr:NULL,   defstrval:NULL,   TYPE_STRING,   0},          \
}

#define GNB_MME_IPV4_ADDRESS_IDX          0
#define GNB_MME_IPV6_ADDRESS_IDX          1
#define GNB_MME_IP_ADDRESS_ACTIVE_IDX     2
#define GNB_MME_IP_ADDRESS_PREFERENCE_IDX 3
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
#define SCTPPARAMS_DESC {  \
{GNB_CONFIG_STRING_SCTP_INSTREAMS,                       NULL,   0,   uptr:NULL,   defintval:-1,    TYPE_UINT,   0},       \
{GNB_CONFIG_STRING_SCTP_OUTSTREAMS,                      NULL,   0,   uptr:NULL,   defintval:-1,    TYPE_UINT,   0}        \
}

#define GNB_SCTP_INSTREAMS_IDX          0
#define GNB_SCTP_OUTSTREAMS_IDX         1
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* S1 interface configuration parameters section name */
#define GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG     "NETWORK_INTERFACES"

#define GNB_INTERFACE_NAME_FOR_S1_MME_IDX          0
#define GNB_IPV4_ADDRESS_FOR_S1_MME_IDX            1
#define GNB_INTERFACE_NAME_FOR_S1U_IDX             2
#define GNB_IPV4_ADDR_FOR_S1U_IDX                  3
#define GNB_PORT_FOR_S1U_IDX                       4

/* S1 interface configuration parameters names   */
#define GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_S1_MME "GNB_INTERFACE_NAME_FOR_S1_MME"
#define GNB_CONFIG_STRING_GNB_IPV4_ADDRESS_FOR_S1_MME   "GNB_IPV4_ADDRESS_FOR_S1_MME"
#define GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_S1U    "GNB_INTERFACE_NAME_FOR_S1U"
#define GNB_CONFIG_STRING_GNB_IPV4_ADDR_FOR_S1U         "GNB_IPV4_ADDRESS_FOR_S1U"
#define GNB_CONFIG_STRING_GNB_PORT_FOR_S1U              "GNB_PORT_FOR_S1U"

/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            S1 interface configuration parameters                                                                 */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval             type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
#define NETPARAMS_DESC {  \
{GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_S1_MME,        NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_GNB_IPV4_ADDRESS_FOR_S1_MME,          NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_S1U,           NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_GNB_IPV4_ADDR_FOR_S1U,                NULL,      0,         strptr:NULL,         defstrval:NULL,      TYPE_STRING,      0},      \
{GNB_CONFIG_STRING_GNB_PORT_FOR_S1U,                     NULL,      0,         uptr:NULL,           defintval:2152L,     TYPE_UINT,        0}       \
}   



/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            GTPU  configuration parameters                                                                                                      */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval                                           type           numelt     */
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define GTPUPARAMS_DESC { \
{GNB_CONFIG_STRING_GNB_INTERFACE_NAME_FOR_S1U,           NULL,    0,            strptr:&gnb_interface_name_for_S1U,      defstrval:"lo",                TYPE_STRING,   0},        \
{GNB_CONFIG_STRING_GNB_IPV4_ADDR_FOR_S1U,                NULL,    0,            strptr:&gnb_ipv4_address_for_S1U,        defstrval:"127.0.0.1",         TYPE_STRING,   0},        \
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
#define CONFIG_STRING_MACRLC_PHY_TEST_MODE                 "phy_test_mode"


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
#define MACRLC_PHY_TEST_IDX                                    18
