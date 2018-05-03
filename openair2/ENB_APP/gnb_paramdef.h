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
#include "NRRRC_paramsvalues.h"


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
#define GNB_CONFIG_STRING_MIB_DMRS_TYPEA_POSITION                                      "MIB_dmrs_TypeA_Position"
#define GNB_CONFIG_STRING_PDCCH_CONFIGSIB1                                             "pdcch_ConfigSIB1"

  //SIB1
#define GNB_CONFIG_STRING_SIB1_FREQUENCYOFFSETSSB                                      "SIB1_frequencyOffsetSSB"
#define GNB_CONFIG_STRING_SIB1_SSB_PERIODICITYSERVINGCELL                              "SIB1_ssb_PeriodicityServingCell"
#define GNB_CONFIG_STRING_SIB1_SS_PBCH_BLOCKPOWER                                      "SIB1_ss_PBCH_BlockPower"
  //NR FREQUENCYINFODL
#define GNB_CONFIG_STRING_ABSOLUTEFREQUENCYSSB                                         "absoluteFrequencySSB"
#define GNB_CONFIG_STRING_SSB_SUBCARRIEROFFSET                                         "SubcarrierSpacing"
#define GNB_CONFIG_STRING_DL_FREQBANDINDICATORNR                                       "DL_FreqBandIndicatorNR"
#define GNB_CONFIG_STRING_DL_ABSOLUTEFREQUENCYPOINTA                                   "DL_absoluteFrequencyPointA"

  //NR DL SCS-SPECIFICCARRIER
#define GNB_CONFIG_STRING_DL_OFFSETTOCARRIER                                           "DL_offsetToCarrier"
#define GNB_CONFIG_STRING_DL_SUBCARRIERSPACING                                         "DL_SubcarrierSpacing"
#define GNB_CONFIG_STRING_DL_SCS_SPECIFICCARRIER_K0                                    "DL_SCS_SpecificCarrier_k0"
#define GNB_CONFIG_STRING_DL_CARRIERBANDWIDTH                                          "DL_carrierBandwidth"

  // NR BWP-DOWNLINKCOMMON
#define GNB_CONFIG_STRING_DL_LOCATIONANDBANDWIDTH                                      "DL_locationAndBandwidth"
   
  //NR FREQUENCYINFOUL
#define GNB_CONFIG_STRING_UL_FREQBANDINDICATORNR                                       "UL_FreqBandIndicatorNR"
#define GNB_CONFIG_STRING_UL_ABSOLUTEFREQUENCYPOINTA                                   "UL_absoluteFrequencyPointA"
#define GNB_CONFIG_STRING_FREQUENCYINFOUL_P_MAX                                        "FrequencyInfoUL_p_Max"
#define GNB_CONFIG_STRING_FREQUENCYSHIFT7P5KHZ                                         "frequencyShift7p5khz"

  //NR UL SCS-SPECIFICCARRIER
#define GNB_CONFIG_STRING_UL_OFFSETTOCARRIER                                           "UL_offsetToCarrier"
#define GNB_CONFIG_STRING_UL_SUBCARRIERSPACING                                         "UL_SubcarrierSpacing"
#define GNB_CONFIG_STRING_UL_SCS_SPECIFICCARRIER_K0                                    "UL_SCS_SpecificCarrier_k0"
#define GNB_CONFIG_STRING_UL_CARRIERBANDWIDTH                                          "UL_carrierBandwidth"

  // NR BWP-UPLINKCOMMON
#define GNB_CONFIG_STRING_UL_LOCATIONANDBANDWIDTH                                      "UL_locationAndBandwidth"
 
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

  //PUCCH-CONFIGCOMMON
#define GNB_CONFIG_STRING_PUCCH_GROUPHOPPING                                           "pucch_GroupHopping"
#define GNB_CONFIG_STRING_P0_NOMINAL                                                   "p0_nominal"

  //PDSCH-CONFIGCOMMON
  //PDSCH-TIMEDOMAINRESOURCEALLOCATION
#define GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_K0                        "PDSCH_TimeDomainResourceAllocation_k0"
#define GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE               "PDSCH_TimeDomainResourceAllocation_mappingType"

  //RATEMATCHPATTERN  IS USED TO CONFIGURE ONE RATE MATCHING PATTERN FOR PDSCH
#define GNB_CONFIG_STRING_RATEMATCHPATTERNID                                           "rateMatchPatternId"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_PATTERNTYPE                                 "RateMatchPattern_patternType"
#define GNB_CONFIG_STRING_SYMBOLSINRESOURCEBLOCK                                       "symbolsInResourceBlock"
#define GNB_CONFIG_STRING_PERIODICITYANDPATTERN                                        "periodicityAndPattern"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_CONTROLRESOURCESET                          "RateMatchPattern_controlResourceSet"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_SUBCARRIERSPACING                           "RateMatchPattern_subcarrierSpacing"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_MODE                                        "RateMatchPattern_mode"

  //PDCCH-CONFIGCOMMON
#define GNB_CONFIG_STRING_SEARCHSPACESIB1                                              "searchSpaceSIB1"
#define GNB_CONFIG_STRING_SEARCHSPACEOTHERSYSTEMINFORMATION                            "searchSpaceOtherSystemInformation"
#define GNB_CONFIG_STRING_PAGINGSEARCHSPACE                                            "pagingSearchSpace"
#define GNB_CONFIG_STRING_RA_SEARCHSPACE                                               "ra_SearchSpace"
#define GNB_CONFIG_STRING_RACH_RA_CONTROLRESOURCESET                                   "rach_ra_ControlResourceSet"
  //NR PDCCH-CONFIGCOMMON COMMONCONTROLRESOURCESSETS
#define GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESETID                            "PDCCH_common_controlResourceSetId"
#define GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESET_DURATION                     "PDCCH_common_ControlResourceSet_duration"
#define GNB_CONFIG_STRING_PDCCH_CCE_REG_MAPPINGTYPE                                    "PDCCH_cce_REG_MappingType"
#define GNB_CONFIG_STRING_PDCCH_REG_BUNDLESIZE                                         "PDCCH_reg_BundleSize"
#define GNB_CONFIG_STRING_PDCCH_INTERLEAVERSIZE                                        "PDCCH_interleaverSize"
#define GNB_CONFIG_STRING_PDCCH_SHIFTINDEX                                             "PDCCH_shiftIndex"
#define GNB_CONFIG_STRING_PDCCH_PRECODERGRANULARITY                                    "PDCCH_precoderGranularity"
#define GNB_CONFIG_STRING_TCI_PRESENTINDCI                                             "tci_PresentInDCI"

  //NR PDCCH-ConfigCommon commonSearchSpaces
#define GNB_CONFIG_STRING_SEARCHSPACEID                                                "SearchSpaceId"
#define GNB_CONFIG_STRING_COMMONSEARCHSPACES_CONTROLRESOURCESETID                      "commonSearchSpaces_controlResourceSetId"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_CHOICE        "SearchSpace_monitoringSlotPeriodicityAndOffset_choice"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL1           "SearchSpace_monitoringSlotPeriodicityAndOffset_sl1"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL2           "SearchSpace_monitoringSlotPeriodicityAndOffset_sl2"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL4           "SearchSpace_monitoringSlotPeriodicityAndOffset_sl4"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL5           "SearchSpace_monitoringSlotPeriodicityAndOffset_sl5"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL8           "SearchSpace_monitoringSlotPeriodicityAndOffset_sl8"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL10          "SearchSpace_monitoringSlotPeriodicityAndOffset_sl10"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL16          "SearchSpace_monitoringSlotPeriodicityAndOffset_sl16"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL20          "SearchSpace_monitoringSlotPeriodicityAndOffset_sl20"
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


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                                                             component carriers configuration parameters                                                                                                                           */
/*   optname                                                                        helpstr   paramflags    XXXptr                                                                  defXXXval                    type         numelt  checked_param  */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define NRCCPARAMS_DESC { \
{GNB_CONFIG_STRING_FRAME_TYPE,                                                       NULL,        0,        strptr:&frame_type,                                                     defstrval:"FDD",           TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_DL_prefix_type,                                                   NULL,        0,        strptr:&DL_prefix_type,                                                 defstrval:"NORMAL",        TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_UL_prefix_type,                                                   NULL,        0,        strptr:&UL_prefix_type,                                                 defstrval:"NORMAL",        TYPE_STRING,     0},  \
{GNB_CONFIG_STRING_EUTRA_BAND,                                                       NULL,        0,        iptr:&eutra_band,                                                       defintval:7,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DOWNLINK_FREQUENCY,                                               NULL,        0,        i64ptr:(int64_t *)&downlink_frequency,                                  defint64val:2680000000,    TYPE_UINT64,     0},  \
{GNB_CONFIG_STRING_UPLINK_FREQUENCY_OFFSET,                                          NULL,        0,        iptr:&uplink_frequency_offset,                                          defintval:-120000000,      TYPE_INT,        0},  \
{GNB_CONFIG_STRING_NID_CELL,                                                         NULL,        0,        iptr:&Nid_cell,                                                         defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_N_RB_DL,                                                          NULL,        0,        iptr:&N_RB_DL,                                                          defintval:25,              TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NB_ANT_PORTS,                                                     NULL,        0,        iptr:&nb_antenna_ports,                                                 defintval:1,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_MIB_SUBCARRIERSPACINGCOMMON,                                      NULL,        0,        iptr:&MIB_subCarrierSpacingCommon,                                      defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_MIB_DMRS_TYPEA_POSITION,                                          NULL,        0,        iptr:&MIB_dmrs_TypeA_Position,                                          defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_CONFIGSIB1,                                                 NULL,        0,        iptr:&pdcch_ConfigSIB1,                                                 defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SIB1_FREQUENCYOFFSETSSB,                                          NULL,        0,        iptr:&SIB1_frequencyOffsetSSB,                                          defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SIB1_SSB_PERIODICITYSERVINGCELL,                                  NULL,        0,        iptr:&SIB1_ssb_PeriodicityServingCell,                                  defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SIB1_SS_PBCH_BLOCKPOWER,                                          NULL,        0,        iptr:&SIB1_ss_PBCH_BlockPower,                                          defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_ABSOLUTEFREQUENCYSSB,                                             NULL,        0,        iptr:&absoluteFrequencySSB,                                             defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SSB_SUBCARRIEROFFSET,                                             NULL,        0,        iptr:&ssb_SubcarrierOffset,                                             defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_FREQBANDINDICATORNR,                                           NULL,        0,        iptr:&DL_FreqBandIndicatorNR,                                           defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_ABSOLUTEFREQUENCYPOINTA,                                       NULL,        0,        iptr:&DL_absoluteFrequencyPointA,                                       defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_OFFSETTOCARRIER,                                               NULL,        0,        iptr:&DL_offsetToCarrier,                                               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_SUBCARRIERSPACING,                                             NULL,        0,        iptr:&DL_SubcarrierSpacing,                                             defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_SCS_SPECIFICCARRIER_K0,                                        NULL,        0,        iptr:&DL_SCS_SpecificCarrier_k0,                                        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_CARRIERBANDWIDTH,                                              NULL,        0,        iptr:&DL_carrierBandwidth,                                              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_LOCATIONANDBANDWIDTH,                                          NULL,        0,        iptr:&DL_locationAndBandwidth,                                          defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_FREQBANDINDICATORNR,                                           NULL,        0,        iptr:&UL_FreqBandIndicatorNR,                                           defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_ABSOLUTEFREQUENCYPOINTA,                                       NULL,        0,        iptr:&UL_absoluteFrequencyPointA,                                       defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_FREQUENCYINFOUL_P_MAX,                                            NULL,        0,        iptr:&FrequencyInfoUL_p_Max,                                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_FREQUENCYSHIFT7P5KHZ,                                             NULL,        0,        iptr:&frequencyShift7p5khz,                                             defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_OFFSETTOCARRIER,                                               NULL,        0,        iptr:&UL_offsetToCarrier,                                               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_SUBCARRIERSPACING,                                             NULL,        0,        iptr:&UL_SubcarrierSpacing,                                             defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_SCS_SPECIFICCARRIER_K0,                                        NULL,        0,        iptr:&UL_SCS_SpecificCarrier_k0,                                        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_CARRIERBANDWIDTH,                                              NULL,        0,        iptr:&UL_carrierBandwidth,                                              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UL_LOCATIONANDBANDWIDTH,                                          NULL,        0,        iptr:&UL_locationAndBandwidth,                                          defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_SSB_PERIODICITYSERVINGCELL,               NULL,        0,        iptr:&ServingCellConfigCommon_ssb_periodicityServingCell,               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_DMRS_TYPEA_POSITION,                      NULL,        0,        iptr:&ServingCellConfigCommon_dmrs_TypeA_Position,                      defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NIA_SUBCARRIERSPACING,                                            NULL,        0,        iptr:&NIA_SubcarrierSpacing,                                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON_SS_PBCH_BLOCKPOWER,                       NULL,        0,        iptr:&ServingCellConfigCommon_ss_PBCH_BlockPower,                       defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_REFERENCESUBCARRIERSPACING,                                       NULL,        0,        iptr:&referenceSubcarrierSpacing,                                       defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_DL_UL_TRANSMISSIONPERIODICITY,                                    NULL,        0,        iptr:&dl_UL_TransmissionPeriodicity,                                    defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NROFDOWNLINKSLOTS,                                                NULL,        0,        iptr:&nrofDownlinkSlots,                                                defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS,                                              NULL,        0,        iptr:&nrofDownlinkSymbols,                                              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NROFUPLINKSLOTS,                                                  NULL,        0,        iptr:&nrofUplinkSlots,                                                  defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_NROFUPLINKSYMBOLS,                                                NULL,        0,        iptr:&nrofUplinkSymbols,                                                defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_TOTALNUMBEROFRA_PREAMBLES,                                   NULL,        0,        iptr:&rach_totalNumberOfRA_Preambles,                                   defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_CHOICE,            NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_choice,            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEEIGHTH,         NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneEighth,         defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEFOURTH,         NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneFourth,         defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEHALF,           NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneHalf,           defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONE,               NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_one,               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_TWO,               NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_two,               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_FOUR,              NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_four,              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_EIGHT,             NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_eight,             defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_SIXTEEN,           NULL,        0,        iptr:&rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_sixteen,           defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_GROUPBCONFIGURED,                                            NULL,        0,        iptr:&rach_groupBconfigured,                                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_RA_MSG3SIZEGROUPA,                                           NULL,        0,        iptr:&rach_ra_Msg3SizeGroupA,                                           defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB,                                    NULL,        0,        iptr:&rach_messagePowerOffsetGroupB,                                    defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_NUMBEROFRA_PREAMBLESGROUPA,                                  NULL,        0,        iptr:&rach_numberOfRA_PreamblesGroupA,                                  defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_RA_CONTENTIONRESOLUTIONTIMER,                                NULL,        0,        iptr:&rach_ra_ContentionResolutionTimer,                                defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RSRP_THRESHOLDSSB,                                                NULL,        0,        iptr:&rsrp_ThresholdSSB,                                                defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RSRP_THRESHOLDSSB_SUL,                                            NULL,        0,        iptr:&rsrp_ThresholdSSB_SUL,                                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_CHOICE,                                   NULL,        0,        iptr:&prach_RootSequenceIndex_choice,                                   defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_L839,                                     NULL,        0,        iptr:&prach_RootSequenceIndex_l839,                                     defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_L139,                                     NULL,        0,        iptr:&prach_RootSequenceIndex_l139,                                     defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_MSG1_SUBCARRIERSPACING,                                     NULL,        0,        iptr:&prach_msg1_SubcarrierSpacing,                                     defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RESTRICTEDSETCONFIG,                                              NULL,        0,        iptr:&restrictedSetConfig,                                              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_MSG3_TRANSFORMPRECODING,                                          NULL,        0,        iptr:&msg3_transformPrecoding,                                          defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_CONFIGURATIONINDEX,                                         NULL,        0,        iptr:&prach_ConfigurationIndex,                                         defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_MSG1_FDM,                                                   NULL,        0,        iptr:&prach_msg1_FDM,                                                   defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PRACH_MSG1_FREQUENCYSTART,                                        NULL,        0,        iptr:&prach_msg1_FrequencyStart,                                        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_ZEROCORRELATIONZONECONFIG,                                        NULL,        0,        iptr:&zeroCorrelationZoneConfig,                                        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PREAMBLERECEIVEDTARGETPOWER,                                      NULL,        0,        iptr:&preambleReceivedTargetPower,                                      defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PREAMBLETRANSMAX,                                                 NULL,        0,        iptr:&preambleTransMax,                                                 defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_POWERRAMPINGSTEP,                                                 NULL,        0,        iptr:&powerRampingStep,                                                 defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RA_RESPONSEWINDOW,                                                NULL,        0,        iptr:&ra_ResponseWindow,                                                defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_GROUPHOPPINGENABLEDTRANSFORMPRECODING,                            NULL,        0,        iptr:&groupHoppingEnabledTransformPrecoding,                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_MSG3_DELTAPREAMBLE,                                               NULL,        0,        iptr:&msg3_DeltaPreamble,                                               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_P0_NOMINALWITHGRANT,                                              NULL,        0,        iptr:&p0_NominalWithGrant,                                              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_K2,                            NULL,        0,        iptr:&PUSCH_TimeDomainResourceAllocation_k2,                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE,                   NULL,        0,        iptr:&PUSCH_TimeDomainResourceAllocation_mappingType,                   defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PUCCH_GROUPHOPPING,                                               NULL,        0,        iptr:&pucch_GroupHopping,                                               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_P0_NOMINAL,                                                       NULL,        0,        iptr:&p0_nominal,                                                       defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_K0,                            NULL,        0,        iptr:&PDSCH_TimeDomainResourceAllocation_k0,                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE,                   NULL,        0,        iptr:&PDSCH_TimeDomainResourceAllocation_mappingType,                   defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERNID,                                               NULL,        0,        iptr:&rateMatchPatternId,                                               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERN_PATTERNTYPE,                                     NULL,        0,        iptr:&RateMatchPattern_patternType,                                     defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SYMBOLSINRESOURCEBLOCK,                                           NULL,        0,        iptr:&symbolsInResourceBlock,                                           defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PERIODICITYANDPATTERN,                                            NULL,        0,        iptr:&periodicityAndPattern,                                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERN_CONTROLRESOURCESET,                              NULL,        0,        iptr:&RateMatchPattern_controlResourceSet,                              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERN_SUBCARRIERSPACING,                               NULL,        0,        iptr:&RateMatchPattern_subcarrierSpacing,                               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RATEMATCHPATTERN_MODE,                                            NULL,        0,        iptr:&RateMatchPattern_mode,                                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACESIB1,                                                  NULL,        0,        iptr:&searchSpaceSIB1,                                                  defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACEOTHERSYSTEMINFORMATION,                                NULL,        0,        iptr:&searchSpaceOtherSystemInformation,                                defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PAGINGSEARCHSPACE,                                                NULL,        0,        iptr:&pagingSearchSpace,                                                defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RA_SEARCHSPACE,                                                   NULL,        0,        iptr:&ra_SearchSpace,                                                   defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_RACH_RA_CONTROLRESOURCESET,                                       NULL,        0,        iptr:&rach_ra_ControlResourceSet,                                       defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESETID,                                NULL,        0,        iptr:&PDCCH_common_controlResourceSetId,                                defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESET_DURATION,                         NULL,        0,        iptr:&PDCCH_common_ControlResourceSet_duration,                         defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_CCE_REG_MAPPINGTYPE,                                        NULL,        0,        iptr:&PDCCH_cce_REG_MappingType,                                        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_REG_BUNDLESIZE,                                             NULL,        0,        iptr:&PDCCH_reg_BundleSize,                                             defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_INTERLEAVERSIZE,                                            NULL,        0,        iptr:&PDCCH_interleaverSize,                                            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_SHIFTINDEX,                                                 NULL,        0,        iptr:&PDCCH_shiftIndex,                                                 defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_PDCCH_PRECODERGRANULARITY,                                        NULL,        0,        iptr:&PDCCH_precoderGranularity,                                        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_TCI_PRESENTINDCI,                                                 NULL,        0,        iptr:&tci_PresentInDCI,                                                 defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACEID,                                                    NULL,        0,        iptr:&SearchSpaceId,                                                    defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMONSEARCHSPACES_CONTROLRESOURCESETID,                          NULL,        0,        iptr:&commonSearchSpaces_controlResourceSetId,                          defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_CHOICE,            NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_choice,            defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL1,               NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_sl1,               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL2,               NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_sl2,               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL4,               NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_sl4,               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL5,               NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_sl5,               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL8,               NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_sl8,               defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL10,              NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_sl10,              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL16,              NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_sl16,              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL20,              NULL,        0,        iptr:&SearchSpace_monitoringSlotPeriodicityAndOffset_sl20,              defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL1,                     NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel1,                     defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL2,                     NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel2,                     defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL4,                     NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel4,                     defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL8,                     NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel8,                     defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL16,                    NULL,        0,        iptr:&SearchSpace_nrofCandidates_aggregationLevel16,                    defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_SEARCHSPACE_SEARCHSPACETYPE,                                      NULL,        0,        iptr:&SearchSpace_searchSpaceType,                                      defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL1,        NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel1,        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL2,        NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel2,        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL4,        NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel4,        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL8,        NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel8,        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL16,       NULL,        0,        iptr:&Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel16,       defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_3_MONITORINGPERIODICITY,                       NULL,        0,        iptr:&Common_dci_Format2_3_monitoringPeriodicity,                       defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_3_NROFPDCCH_CANDIDATES,                        NULL,        0,        iptr:&Common_dci_Format2_3_nrofPDCCH_Candidates,                        defintval:0,               TYPE_UINT,       0},  \
{GNB_CONFIG_STRING_UE_SPECIFIC__DCI_FORMATS,                                         NULL,        0,        iptr:&ue_Specific__dci_Formats,                                         defintval:0,               TYPE_UINT,       0},  \
}


/* component carries configuration parameters name */
#define ENB_CONFIG_FRAME_TYPE_IDX                             0             
#define ENB_CONFIG_DL_PREFIX_TYPE_IDX                            1
#define ENB_CONFIG_UL_PREFIX_TYPE_IDX                            1
#define ENB_CONFIG_EUTRA_BAND_IDX                             2
#define ENB_CONFIG_DOWNLINK_FREQUENCY_IDX                     3
#define ENB_CONFIG_UPLINK_FREQUENCY_OFFSET_IDX                4
#define ENB_CONFIG_NID_CELL_IDX                               5
#define ENB_CONFIG_N_RB_DL_IDX                                6
#define GNB_CONFIG_NB_ANT_PORTS_IDX                                                 
#define GNB_CONFIG_NB_ANT_TX_IDX                                                    
#define GNB_CONFIG_NB_ANT_RX_IDX                                                   
#define GNB_CONFIG_TX_GAIN_IDX                                                      
#define GNB_CONFIG_RX_GAIN_IDX
#define GNB_CONFIG_MIB_SUBCARRIERSPACINGCOMMON_IDX
#define GNB_CONFIG_MIB_DMRS_TYPEA_POSITION_IDX
#define GNB_CONFIG_PDCCH_CONFIGSIB1_IDX
#define GNB_CONFIG_SIB1_FREQUENCYOFFSETSSB_IDX
#define GNB_CONFIG_SIB1_SSB_PERIODICITYSERVINGCELL_IDX
#define GNB_CONFIG_SIB1_SS_PBCH_BLOCKPOWER_IDX                                                   
#define GNB_CONFIG_ABSOLUTEFREQUENCYSSB_IDX                                         
#define GNB_CONFIG_SSB_SUBCARRIEROFFSET_IDX                                         
#define GNB_CONFIG_DL_FREQBANDINDICATORNR_IDX                                       
#define GNB_CONFIG_DL_ABSOLUTEFREQUENCYPOINTA_IDX                                   
#define GNB_CONFIG_DL_OFFSETTOCARRIER_IDX                                           
#define GNB_CONFIG_DL_SUBCARRIERSPACING_IDX                                         
#define GNB_CONFIG_DL_SCS_SPECIFICCARRIER_K0_IDX                                    
#define GNB_CONFIG_DL_CARRIERBANDWIDTH_IDX                                          
#define GNB_CONFIG_DL_LOCATIONANDBANDWIDTH_IDX                                      
#define GNB_CONFIG_UL_FREQBANDINDICATORNR_IDX                                       
#define GNB_CONFIG_UL_ABSOLUTEFREQUENCYPOINTA_IDX                                   
#define GNB_CONFIG_FREQUENCYINFOUL_P_MAX_IDX                                        
#define GNB_CONFIG_FREQUENCYSHIFT7P5KHZ_IDX                                         
#define GNB_CONFIG_UL_OFFSETTOCARRIER_IDX                                           
#define GNB_CONFIG_UL_SUBCARRIERSPACING_IDX                                         
#define GNB_CONFIG_UL_SCS_SPECIFICCARRIER_K0_IDX                                    
#define GNB_CONFIG_UL_CARRIERBANDWIDTH_IDX                                          
#define GNB_CONFIG_UL_LOCATIONANDBANDWIDTH_IDX
#define GNB_CONFIG_SUBCARRIERSPACINGCOMMON_IDX
#define GNB_CONFIG_PDCCH_CONFIGSIB1_IDX
#define GNB_CONFIG_FREQUENCYOFFSETSSB_IDX                                      
#define GNB_CONFIG_SERVINGCELLCONFIGCOMMON_SSB_PERIODICITYSERVINGCELL_IDX                                  
#define GNB_CONFIG_SERVINGCELLCONFIGCOMMON_DMRS_TYPEA_POSITION_IDX                                          
#define GNB_CONFIG_NIA_SUBCARRIERSPACING_IDX                                        
#define GNB_CONFIG_SERVINGCELLCONFIGCOMMON_SS_PBCH_BLOCKPOWER_IDX                                           
#define GNB_CONFIG_REFERENCESUBCARRIERSPACING_IDX                                   
#define GNB_CONFIG_DL_UL_TRANSMISSIONPERIODICITY_IDX                                
#define GNB_CONFIG_NROFDOWNLINKSLOTS_IDX                                            
#define GNB_CONFIG_NROFDOWNLINKSYMBOLS_IDX                                          
#define GNB_CONFIG_NROFUPLINKSLOTS_IDX                                              
#define GNB_CONFIG_NROFUPLINKSYMBOLS_IDX                                            
#define GNB_CONFIG_RACH_TOTALNUMBEROFRA_PREAMBLES_IDX                               
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_CHOICE_IDX        
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEEIGHTH_IDX     
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEFOURTH_IDX     
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEHALF_IDX       
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONE_IDX         
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_TWO_IDX           
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_FOUR_IDX          
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_EIGHT_IDX         
#define GNB_CONFIG_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_SIXTEEN_IDX       
#define GNB_CONFIG_RACH_GROUPBCONFIGURED_IDX                                        
#define GNB_CONFIG_RACH_RA_MSG3SIZEGROUPA_IDX                                       
#define GNB_CONFIG_RACH_MESSAGEPOWEROFFSETGROUPB_IDX                                
#define GNB_CONFIG_RACH_NUMBEROFRA_PREAMBLESGROUPA_IDX                              
#define GNB_CONFIG_RACH_RA_CONTENTIONRESOLUTIONTIMER_IDX                           
#define GNB_CONFIG_RSRP_THRESHOLDSSB_IDX                                            
#define GNB_CONFIG_RSRP_THRESHOLDSSB_SUL_IDX                                        
#define GNB_CONFIG_PRACH_ROOTSEQUENCEINDEX_CHOICE_IDX                               
#define GNB_CONFIG_PRACH_ROOTSEQUENCEINDEX_L839_IDX                                
#define GNB_CONFIG_PRACH_ROOTSEQUENCEINDEX_L139_IDX                                 
#define GNB_CONFIG_PRACH_MSG1_SUBCARRIERSPACING_IDX                                
#define GNB_CONFIG_RESTRICTEDSETCONFIG_IDX                                          
#define GNB_CONFIG_MSG3_TRANSFORMPRECODING_IDX                                      
#define GNB_CONFIG_PRACH_CONFIGURATIONINDEX_IDX                                    
#define GNB_CONFIG_PRACH_MSG1_FDM_IDX                                              
#define GNB_CONFIG_PRACH_MSG1_FREQUENCYSTART_IDX                                    
#define GNB_CONFIG_ZEROCORRELATIONZONECONFIG_IDX                                    
#define GNB_CONFIG_PREAMBLERECEIVEDTARGETPOWER_IDX                                  
#define GNB_CONFIG_PREAMBLETRANSMAX_IDX                                            
#define GNB_CONFIG_POWERRAMPINGSTEP_IDX                                             
#define GNB_CONFIG_RA_RESPONSEWINDOW_IDX                                            
#define GNB_CONFIG_GROUPHOPPINGENABLEDTRANSFORMPRECODING_IDX                        
#define GNB_CONFIG_MSG3_DELTAPREAMBLE_IDX                                          
#define GNB_CONFIG_P0_NOMINALWITHGRANT_IDX                                          
#define GNB_CONFIG_PUSCH_TIMEDOMAINRESOURCEALLOCATION_K2_IDX                        
#define GNB_CONFIG_PUSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE_IDX               
#define GNB_CONFIG_PUCCH_GROUPHOPPING_IDX                                           
#define GNB_CONFIG_P0_NOMINAL_IDX                                                 
#define GNB_CONFIG_PDSCH_TIMEDOMAINRESOURCEALLOCATION_K0_IDX                        
#define GNB_CONFIG_PDSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE_IDX               
#define GNB_CONFIG_RATEMATCHPATTERNID_IDX                                           
#define GNB_CONFIG_RATEMATCHPATTERN_PATTERNTYPE_IDX                                 
#define GNB_CONFIG_SYMBOLSINRESOURCEBLOCK_IDX                                       
#define GNB_CONFIG_PERIODICITYANDPATTERN_IDX                                        
#define GNB_CONFIG_RATEMATCHPATTERN_CONTROLRESOURCESET_IDX                          
#define GNB_CONFIG_RATEMATCHPATTERN_SUBCARRIERSPACING_IDX                           
#define GNB_CONFIG_RATEMATCHPATTERN_MODE_IDX                                        
#define GNB_CONFIG_SEARCHSPACESIB1_IDX                                              
#define GNB_CONFIG_SEARCHSPACEOTHERSYSTEMINFORMATION_IDX                            
#define GNB_CONFIG_PAGINGSEARCHSPACE_IDX                                            
#define GNB_CONFIG_RA_SEARCHSPACE_IDX                                               
#define GNB_CONFIG_RACH_RA_CONTROLRESOURCESET_IDX                                  
#define GNB_CONFIG_PDCCH_COMMON_CONTROLRESOURCESETID_IDX                            
#define GNB_CONFIG_PDCCH_COMMON_CONTROLRESOURCESET_DURATION_IDX                     
#define GNB_CONFIG_PDCCH_CCE_REG_MAPPINGTYPE_IDX                                    
#define GNB_CONFIG_PDCCH_REG_BUNDLESIZE_IDX                                        
#define GNB_CONFIG_PDCCH_INTERLEAVERSIZE_IDX                                        
#define GNB_CONFIG_PDCCH_SHIFTINDEX_IDX                                             
#define GNB_CONFIG_PDCCH_PRECODERGRANULARITY_IDX                                    
#define GNB_CONFIG_TCI_PRESENTINDCI_IDX                                             
#define GNB_CONFIG_SEARCHSPACEID_IDX                                                
#define GNB_CONFIG_COMMONSEARCHSPACES_CONTROLRESOURCESETID_IDX                      
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_CHOICE_IDX        
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL1_IDX           
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL2_IDX           
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL4_IDX           
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL5_IDX           
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL8_IDX           
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL10_IDX         
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL16_IDX          
#define GNB_CONFIG_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL20_IDX          
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL1_IDX                 
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL2_IDX                 
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL4_IDX                 
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL8_IDX                 
#define GNB_CONFIG_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL16_IDX                
#define GNB_CONFIG_SEARCHSPACE_SEARCHSPACETYPE_IDX                                  
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL1_IDX   
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL2_IDX   
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL4_IDX    
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL8_IDX    
#define GNB_CONFIG_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL16_IDX   
#define GNB_CONFIG_COMMON_DCI_FORMAT2_3_MONITORINGPERIODICITY_IDX                   
#define GNB_CONFIG_COMMON_DCI_FORMAT2_3_NROFPDCCH_CANDIDATES_IDX                   
#define GNB_CONFIG_UE_SPECIFIC__DCI_FORMATS_IDX                                     
