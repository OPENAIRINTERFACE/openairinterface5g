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


#define GNB_CONFIG_STRING_CC_NODE_FUNCTION                              "node_function"
#define GNB_CONFIG_STRING_CC_NODE_TIMING                                "node_timing"   
#define GNB_CONFIG_STRING_CC_NODE_SYNCH_REF                             "node_synch_ref"   


// OTG config per GNB-UE DL
#define GNB_CONF_STRING_OTG_CONFIG                         "otg_config"
#define GNB_CONF_STRING_OTG_UE_ID                          "ue_id"
#define GNB_CONF_STRING_OTG_APP_TYPE                       "app_type"
#define GNB_CONF_STRING_OTG_BG_TRAFFIC                     "bg_traffic"

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
{CONFIG_STRING_RU_NBIOTRRC_LIST,                 NULL,       0,       uptr:NULL,       defintarrayval:DEFGNBS,  TYPE_INTARRAY,    1}, \
}

/*---------------------------------------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------------------------------------*/
/* value definitions for ASN1 verbosity parameter */
#define GNB_CONFIG_STRING_ASN1_VERBOSITY_NONE              "none"
#define GNB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING          "annoying"
#define GNB_CONFIG_STRING_ASN1_VERBOSITY_INFO              "info"
 

/* global parameters, not under a specific section   */
#define GNB_CONFIG_STRING_ASN1_VERBOSITY           "Asn1_verbosity"
#define GNB_CONFIG_STRING_ACTIVE_GNBS              "Active_gNBs"
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
{GNB_CONFIG_STRING_GNB_NAME,                     NULL,   0,            strptr:NULL, defstrval:"OAIeNodeB",       TYPE_STRING,    0},  \
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

#define GNB_CONFIG_STRING_NB_ANT_PORTS													"nb_antenna_ports"
#define GNB_CONFIG_STRING_NB_ANT_TX														"nb_antennas_tx"
#define GNB_CONFIG_STRING_NB_ANT_RX														"nb_antennas_rx"
#define GNB_CONFIG_STRING_TX_GAIN														"tx_gain"
#define GNB_CONFIG_STRING_RX_GAIN														"rx_gain"

  ///NR
  //NR FREQUENCYINFODL
#define GNB_CONFIG_STRING_ABSOLUTEFREQUENCYSSB											"absoluteFrequencySSB"
#define GNB_CONFIG_STRING_SSB_SUBCARRIEROFFSET											"SubcarrierSpacing"
#define GNB_CONFIG_STRING_DL_FREQBANDINDICATORNR										"DL_FreqBandIndicatorNR"
#define GNB_CONFIG_STRING_DL_ABSOLUTEFREQUENCYPOINTA									"DL_absoluteFrequencyPointA"

  //NR DL SCS-SPECIFICCARRIER
#define GNB_CONFIG_STRING_DL_OFFSETTOCARRIER											"DL_offsetToCarrier"
#define GNB_CONFIG_STRING_DL_SUBCARRIERSPACING											"DL_SubcarrierSpacing"
#define GNB_CONFIG_STRING_DL_SCS_SPECIFICCARRIER_K0										"DL_SCS_SpecificCarrier_k0"
#define GNB_CONFIG_STRING_DL_CARRIERBANDWIDTH											"DL_carrierBandwidth"

  // NR BWP-DOWNLINKCOMMON
#define GNB_CONFIG_STRING_DL_LOCATIONANDBANDWIDTH										"DL_locationAndBandwidth"
///#define GNB_CONFIG_STRING_DL_PREFIX_TYPE;     

  //NR FREQUENCYINFOUL
#define GNB_CONFIG_STRING_UL_FREQBANDINDICATORNR										"UL_FreqBandIndicatorNR"
#define GNB_CONFIG_STRING_UL_ABSOLUTEFREQUENCYPOINTA									"UL_absoluteFrequencyPointA"
#define GNB_CONFIG_STRING_FREQUENCYINFOUL_P_MAX											"FrequencyInfoUL_p_Max"
#define GNB_CONFIG_STRING_FREQUENCYSHIFT7P5KHZ											"frequencyShift7p5khz"

  //NR UL SCS-SPECIFICCARRIER
#define GNB_CONFIG_STRING_UL_OFFSETTOCARRIER											"UL_offsetToCarrier"
#define GNB_CONFIG_STRING_UL_SUBCARRIERSPACING											"UL_SubcarrierSpacing"
#define GNB_CONFIG_STRING_UL_SCS_SPECIFICCARRIER_K0										"UL_SCS_SpecificCarrier_k0"
#define GNB_CONFIG_STRING_UL_CARRIERBANDWIDTH											"UL_carrierBandwidth"

  // NR BWP-UPLINKCOMMON
#define GNB_CONFIG_STRING_UL_LOCATIONANDBANDWIDTH										"UL_locationAndBandwidth"
  //NR_PREFIX_TYPE_T      UL_PREFIX_TYPE[MAX_NUM_CCS]; 


#define GNB_CONFIG_STRING_SSB_PERIODICITYSERVINGCELL									"ssb_periodicityServingCell"
#define GNB_CONFIG_STRING_DMRS_TYPEA_POSITION											"dmrs_TypeA_Position"
#define GNB_CONFIG_STRING_NIA_SUBCARRIERSPACING											"NIA_SubcarrierSpacing"
#define GNB_CONFIG_STRING_SS_PBCH_BLOCKPOWER											"ss_PBCH_BlockPower"


  //NR TDD-UL-DL-CONFIGCOMMON
#define GNB_CONFIG_STRING_REFERENCESUBCARRIERSPACING									"referenceSubcarrierSpacing"
#define GNB_CONFIG_STRING_DL_UL_TRANSMISSIONPERIODICITY									"dl_UL_TransmissionPeriodicity"
#define GNB_CONFIG_STRING_NROFDOWNLINKSLOTS												"nrofDownlinkSlots"
#define GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS											"nrofDownlinkSymbols"
#define GNB_CONFIG_STRING_NROFUPLINKSLOTS												"nrofUplinkSlots"
#define GNB_CONFIG_STRING_NROFUPLINKSYMBOLS												"nrofUplinkSymbols"

  //NR RACH-CONFIGCOMMON
#define GNB_CONFIG_STRING_RACH_TOTALNUMBEROFRA_PREAMBLES								"rach_totalNumberOfRA_Preambles"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_CHOICE			"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_choice"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEEIGHTH		"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneEighth"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEFOURTH		"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneFourth"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONEHALF		"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_oneHalf"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_ONE			"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_one"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_TWO			"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_two"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_FOUR			"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_four"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_EIGHT			"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_eight"
#define GNB_CONFIG_STRING_RACH_SSB_PERRACH_OCCASIONANDCB_PREAMBLESPERSSB_SIXTEEN		"rach_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_sixteen"
#define GNB_CONFIG_STRING_RACH_GROUPBCONFIGURED											"rach_groupBconfigured "
#define GNB_CONFIG_STRING_RACH_RA_MSG3SIZEGROUPA										"rach_ra_Msg3SizeGroupA"
#define GNB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB									"rach_messagePowerOffsetGroupB"
#define GNB_CONFIG_STRING_RACH_NUMBEROFRA_PREAMBLESGROUPA								"rach_numberOfRA_PreamblesGroupA"
#define GNB_CONFIG_STRING_RACH_RA_CONTENTIONRESOLUTIONTIMER								"rach_ra_ContentionResolutionTimer"
#define GNB_CONFIG_STRING_RSRP_THRESHOLDSSB												"rsrp_ThresholdSSB"
#define GNB_CONFIG_STRING_RSRP_THRESHOLDSSB_SUL											"rsrp_ThresholdSSB_SUL"
#define GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_CHOICE								"prach_RootSequenceIndex_choice"
#define GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_L839									"prach_RootSequenceIndex_l839"
#define GNB_CONFIG_STRING_PRACH_ROOTSEQUENCEINDEX_L139									"prach_RootSequenceIndex_l139"
#define GNB_CONFIG_STRING_PRACH_MSG1_SUBCARRIERSPACING									"prach_msg1_SubcarrierSpacing"
#define GNB_CONFIG_STRING_RESTRICTEDSETCONFIG											"restrictedSetConfig"
#define GNB_CONFIG_STRING_MSG3_TRANSFORMPRECODING										"msg3_transformPrecoding"
  //SSB-PERRACH-OCCASIONANDCB-PREAMBLESPERSSB NOT SURE

  //NR RACH-CONFIGGENERIC
#define GNB_CONFIG_STRING_PRACH_CONFIGURATIONINDEX										"prach_ConfigurationIndex"
#define GNB_CONFIG_STRING_PRACH_MSG1_FDM												"prach_msg1_FDM"
#define GNB_CONFIG_STRING_PRACH_MSG1_FREQUENCYSTART										"prach_msg1_FrequencyStart"
#define GNB_CONFIG_STRING_ZEROCORRELATIONZONECONFIG										"zeroCorrelationZoneConfig"
#define GNB_CONFIG_STRING_PREAMBLERECEIVEDTARGETPOWER									"preambleReceivedTargetPower"
#define GNB_CONFIG_STRING_PREAMBLETRANSMAX												"preambleTransMax"
#define GNB_CONFIG_STRING_POWERRAMPINGSTEP												"powerRampingStep"
#define GNB_CONFIG_STRING_RA_RESPONSEWINDOW												"ra_ResponseWindow"

  //PUSCH-CONFIGCOMMON
#define GNB_CONFIG_STRING_GROUPHOPPINGENABLEDTRANSFORMPRECODING							"groupHoppingEnabledTransformPrecoding"
#define GNB_CONFIG_STRING_MSG3_DELTAPREAMBLE											"msg3_DeltaPreamble"
#define GNB_CONFIG_STRING_P0_NOMINALWITHGRANT											"p0_NominalWithGrant"

  ///PUSCH-TIMEDOMAINRESOURCEALLOCATION
#define GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_K2							"PUSCH_TimeDomainResourceAllocation_k2"
#define GNB_CONFIG_STRING_PUSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE				"PUSCH_TimeDomainResourceAllocation_mappingType"

  //PUCCH-CONFIGCOMMON
#define GNB_CONFIG_STRING_PUCCH_GROUPHOPPING											"pucch_GroupHopping"
#define GNB_CONFIG_STRING_P0_NOMINAL													"p0_nominal"

  //PDSCH-CONFIGCOMMON
  //PDSCH-TIMEDOMAINRESOURCEALLOCATION
#define GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_K0							"PDSCH_TimeDomainResourceAllocation_k0"
#define GNB_CONFIG_STRING_PDSCH_TIMEDOMAINRESOURCEALLOCATION_MAPPINGTYPE				"PDSCH_TimeDomainResourceAllocation_mappingType"

  //RATEMATCHPATTERN  IS USED TO CONFIGURE ONE RATE MATCHING PATTERN FOR PDSCH
#define GNB_CONFIG_STRING_RATEMATCHPATTERNID											"rateMatchPatternId"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_PATTERNTYPE									"RateMatchPattern_patternType"
#define GNB_CONFIG_STRING_SYMBOLSINRESOURCEBLOCK										"symbolsInResourceBlock"
#define GNB_CONFIG_STRING_PERIODICITYANDPATTERN											"periodicityAndPattern"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_CONTROLRESOURCESET							"RateMatchPattern_controlResourceSet"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_SUBCARRIERSPACING							"RateMatchPattern_subcarrierSpacing"
#define GNB_CONFIG_STRING_RATEMATCHPATTERN_MODE											"RateMatchPattern_mode"

  //PDCCH-CONFIGCOMMON
#define GNB_CONFIG_STRING_SEARCHSPACESIB1												"searchSpaceSIB1"
#define GNB_CONFIG_STRING_SEARCHSPACEOTHERSYSTEMINFORMATION								"searchSpaceOtherSystemInformation"
#define GNB_CONFIG_STRING_PAGINGSEARCHSPACE												"pagingSearchSpace"
#define GNB_CONFIG_STRING_RA_SEARCHSPACE												"ra_SearchSpace"
#define GNB_CONFIG_STRING_RACH_RA_CONTROLRESOURCESET									"rach_ra_ControlResourceSet"
  //NR PDCCH-CONFIGCOMMON COMMONCONTROLRESOURCESSETS
#define GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESETID								"PDCCH_common_controlResourceSetId"
#define GNB_CONFIG_STRING_PDCCH_COMMON_CONTROLRESOURCESET_DURATION						"PDCCH_common_ControlResourceSet_duration"
#define GNB_CONFIG_STRING_PDCCH_CCE_REG_MAPPINGTYPE										"PDCCH_cce_REG_MappingType"
#define GNB_CONFIG_STRING_PDCCH_REG_BUNDLESIZE											"PDCCH_reg_BundleSize"
#define GNB_CONFIG_STRING_PDCCH_INTERLEAVERSIZE											"PDCCH_interleaverSize"
#define GNB_CONFIG_STRING_PDCCH_SHIFTINDEX												"PDCCH_shiftIndex"
#define GNB_CONFIG_STRING_PDCCH_PRECODERGRANULARITY										"PDCCH_precoderGranularity"
#define GNB_CONFIG_STRING_TCI_PRESENTINDCI												"tci_PresentInDCI"

  //NR PDCCH-ConfigCommon commonSearchSpaces
#define GNB_CONFIG_STRING_SEARCHSPACEID													"SearchSpaceId"
#define GNB_CONFIG_STRING_COMMONSEARCHSPACES_CONTROLRESOURCESETID						"commonSearchSpaces_controlResourceSetId"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_CHOICE			"SearchSpace_monitoringSlotPeriodicityAndOffset_choice"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL1			"SearchSpace_monitoringSlotPeriodicityAndOffset_sl1"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL2			"SearchSpace_monitoringSlotPeriodicityAndOffset_sl2"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL4			"SearchSpace_monitoringSlotPeriodicityAndOffset_sl4"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL5			"SearchSpace_monitoringSlotPeriodicityAndOffset_sl5"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL8			"SearchSpace_monitoringSlotPeriodicityAndOffset_sl8"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL10			"SearchSpace_monitoringSlotPeriodicityAndOffset_sl10"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL16			"SearchSpace_monitoringSlotPeriodicityAndOffset_sl16"
#define GNB_CONFIG_STRING_SEARCHSPACE_MONITORINGSLOTPERIODICITYANDOFFSET_SL20			"SearchSpace_monitoringSlotPeriodicityAndOffset_sl20"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL1					"SearchSpace_nrofCandidates_aggregationLevel1"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL2					"SearchSpace_nrofCandidates_aggregationLevel2"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL4					"SearchSpace_nrofCandidates_aggregationLevel4"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL8					"SearchSpace_nrofCandidates_aggregationLevel8"
#define GNB_CONFIG_STRING_SEARCHSPACE_NROFCANDIDATES_AGGREGATIONLEVEL16					"SearchSpace_nrofCandidates_aggregationLevel16"
#define GNB_CONFIG_STRING_SEARCHSPACE_SEARCHSPACETYPE									"SearchSpace_searchSpaceType"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL1		"Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel1"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL2		"Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel2"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL4		"Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel4"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL8		"Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel8"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_0_NROFCANDIDATES_SFI_AGGREGATIONLEVEL16	"Common_dci_Format2_0_nrofCandidates_SFI_aggregationLevel16"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_3_MONITORINGPERIODICITY					"Common_dci_Format2_3_monitoringPeriodicity"
#define GNB_CONFIG_STRING_COMMON_DCI_FORMAT2_3_NROFPDCCH_CANDIDATES						"Common_dci_Format2_3_nrofPDCCH_Candidates"
#define GNB_CONFIG_STRING_UE_SPECIFIC__DCI_FORMATS										"ue_Specific__dci_Formats"


  //PDCCH-CONFIGCOMMON

  //CONTROLRESOURCESET
#define GNB_CONFIG_STRING_CONTROLRESOURCESETID                                    = 0;
#define GNB_CONFIG_STRING_FREQUENCYDOMAINRESOURCES                                = 0;
#define GNB_CONFIG_STRING_CONTROLRESOURCESET_DURATION                             = 0;
#define GNB_CONFIG_STRING_PRECODERGRANULARITY                                     = 0; //CORRESPONDS TO L1 PARAMETER 'CORESET-PRECODER-GRANUALITY'
#define GNB_CONFIG_STRING_TCI_PRESENTINDCI                                        = NULL;

  //SEARCHSPACE
#define GNB_CONFIG_STRING_SEARCHSPACEID                                           = 0;
#define GNB_CONFIG_STRING_DCI_FORMAT2_3_MONITORINGPERIODICITY                     = 0;
#define GNB_CONFIG_STRING_DCI_FORMAT2_3_NROFPDCCH_CANDIDATES                      = 0;


  // NR SYNCHRONIZATION SIGNAL BLOCK
  
#define GNB_CONFIG_STRING_SSB_PERIODICITYSERVINGCELL                              = 0;
#define GNB_CONFIG_STRING_SS_PBCH_BLOCKPOWER                                      = 0;


