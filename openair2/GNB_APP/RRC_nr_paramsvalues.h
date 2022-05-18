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

/*! \file openair2/GNB_APP/RRC_nr_paramsvalues.h
 * \brief macro definitions for RRC authorized and asn1 parameters values, to be used in paramdef_t/chechedparam_t structure initializations 
 * \author Francois TABURET, WEI-TAI CHEN, Turker Yilmaz
 * \date 2018
 * \version 0.1
 * \company NOKIA BellLabs France, NTUST, EURECOM
 * \email: francois.taburet@nokia-bell-labs.com, kroempa@gmail.com, turker.yilmaz@eurecom.fr
 * \note
 * \warning
 */

#ifndef __NR_RRC_PARAMSVALUES__H__
#define __NR_RRC_PARAMSVALUES__H__

#include "common/config/config_paramdesc.h"
#include "NR_ServingCellConfigCommon.h"

/*    cell configuration section name */
#define GNB_CONFIG_STRING_GNB_LIST                              "gNBs"

#define GNB_CONFIG_STRING_PDCCH_CONFIGSIB1                      "pdcch_ConfigSIB1"
#define GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON               "servingCellConfigCommon"
#define GNB_CONFIG_STRING_PHYSCELLID                            "physCellId"
#define GNB_CONFIG_STRING_NTIMINGADVANCEOFFSET                  "n_TimingAdvanceOffset"
#define GNB_CONFIG_STRING_SUBCARRIERSPACING                     "subcarrierSpacing"
#define GNB_CONFIG_STRING_ABSOLUTEFREQUENCYSSB                  "absoluteFrequencySSB"
#define GNB_CONFIG_STRING_DLFREQUENCYBAND                       "dl_frequencyBand"
#define GNB_CONFIG_STRING_DLABSOLUEFREQUENCYPOINTA              "dl_absoluteFrequencyPointA"
#define GNB_CONFIG_STRING_DLOFFSETTOCARRIER                     "dl_offstToCarrier"
#define GNB_CONFIG_STRING_DLSUBCARRIERSPACING                   "dl_subcarrierSpacing"
#define GNB_CONFIG_STRING_DLCARRIERBANDWIDTH                    "dl_carrierBandwidth"

#define GNB_CONFIG_STRING_INITIALDLBWPLOCATIONANDBANDWIDTH      "initialDLBWPlocationAndBandwidth"
#define GNB_CONFIG_STRING_INITIALDLBWPSUBCARRIERSPACING         "initialDLBWPsubcarrierSpacing"
#define GNB_CONFIG_STRING_INITIALDLBWPCYCLICPREFIX              "initialDLBWPcyclicPrefix"

#define GNB_CONFIG_STRING_INITIALDLBWPCONTROLRESOURCESETZERO    "initialDLBWPcontrolResourceSetZero"
#define GNB_CONFIG_STRING_INITIALDLBWPSEARCHSPACEZERO           "initialDLBWPsearchSpaceZero"

#define GNB_CONFIG_STRING_ULFREQUENCYBAND                       "ul_frequencyBand"
#define GNB_CONFIG_STRING_ULABSOLUEFREQUENCYPOINTA              "ul_absoluteFrequencyPointA"
#define GNB_CONFIG_STRING_ULOFFSETTOCARRIER                     "ul_offstToCarrier"
#define GNB_CONFIG_STRING_ULSUBCARRIERSPACING                   "ul_subcarrierSpacing"
#define GNB_CONFIG_STRING_ULCARRIERBANDWIDTH                    "ul_carrierBandwidth"

#define GNB_CONFIG_STRING_INITIALULBWPLOCATIONANDBANDWIDTH      "initialULBWPlocationAndBandwidth"
#define GNB_CONFIG_STRING_INITIALULBWPSUBCARRIERSPACING         "initialULBWPsubcarrierSpacing"

#define GNB_CONFIG_STRING_PMAX                                  "pMax"
#define GNB_CONFIG_STRING_PRACHCONFIGURATIONINDEX               "prach_ConfigurationIndex"
#define GNB_CONFIG_STRING_PRACHMSG1FDM                          "prach_msg1_FDM"
#define GNB_CONFIG_STRING_PRACHMSG1FREQUENCYSTART               "prach_msg1_FrequencyStart"
#define GNB_CONFIG_STRING_ZEROCORRELATIONZONECONFIG             "zeroCorrelationZoneConfig"
#define GNB_CONFIG_STRING_PREAMBLERECEIVEDTARGETPOWER           "preambleReceivedTargetPower"
#define GNB_CONFIG_STRING_PREAMBLETRANSMAX                      "preambleTransMax"
#define GNB_CONFIG_STRING_POWERRAMPINGSTEP                      "powerRampingStep"
#define GNB_CONFIG_STRING_RARESPONSEWINDOW                      "ra_ResponseWindow"
#define GNB_CONFIG_STRING_SSBPERRACHOCCASIONANDCBPREAMBLESPERSSBPR   "ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR"
#define GNB_CONFIG_STRING_SSBPERRACHOCCASIONANDCBPREAMBLESPERSSB     "ssb_perRACH_OccasionAndCB_PreamblesPerSSB"
#define GNB_CONFIG_STRING_RACONTENTIONRESOLUTIONTIMER           "ra_ContentionResolutionTimer"                     
#define GNB_CONFIG_STRING_RSRPTHRESHOLDSSB                      "rsrp_ThresholdSSB"                                
#define GNB_CONFIG_STRING_PRACHROOTSEQUENCEINDEXPR              "prach_RootSequenceIndex_PR"                     
#define GNB_CONFIG_STRING_PRACHROOTSEQUENCEINDEX                "prach_RootSequenceIndex"                     
#define GNB_CONFIG_STRING_MSG1SUBCARRIERSPACING                 "msg1_SubcarrierSpacing"                           
#define GNB_CONFIG_STRING_RESTRICTEDSETCONFIG                   "restrictedSetConfig"
#define GNB_CONFIG_STRING_MSG3TRANSFPREC                        "msg3_transformPrecoder"
#define GNB_CONFIG_STRING_PUSCHTIMEDOMAINALLOCATIONLIST         "puschTimeDomainAllocationList"
#define GNB_CONFIG_STRING_MSG3DELTAPREABMLE                     "msg3_DeltaPreamble"
#define GNB_CONFIG_STRING_P0NOMINALWITHGRANT                    "p0_NominalWithGrant"
#define GNB_CONFIG_STRING_PUCCHGROUPHOPPING                     "pucchGroupHopping"
#define GNB_CONFIG_STRING_HOPPINGID                             "hoppingId"
#define GNB_CONFIG_STRING_P0NOMINAL                             "p0_nominal"
#define GNB_CONFIG_STRING_PUCCHRES                              "pucch_ResourceCommon"


#define GNB_CONFIG_STRING_SSBPOSITIONSINBURSTPR                          "ssb_PositionsInBurst_PR"
#define GNB_CONFIG_STRING_SSBPOSITIONSINBURST                            "ssb_PositionsInBurst_Bitmap"
#define GNB_CONFIG_STRING_SSBPERIODICITYSERVINGCELL                      "ssb_periodicityServingCell"
#define GNB_CONFIG_STRING_DMRSTYPEAPOSITION                              "dmrs_TypeA_Position"
#define GNB_CONFIG_STRING_REFERENCESUBCARRIERSPACING                     "referenceSubcarrierSpacing"
#define GNB_CONFIG_STRING_DLULTRANSMISSIONPERIODICITY                    "dl_UL_TransmissionPeriodicity"
#define GNB_CONFIG_STRING_NROFDOWNLINKSLOTS                              "nrofDownlinkSlots"
#define GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS                            "nrofDownlinkSymbols"
#define GNB_CONFIG_STRING_NROFUPLINKSLOTS                                "nrofUplinkSlots"
#define GNB_CONFIG_STRING_NROFUPLINKSYMBOLS                              "nrofUplinkSymbols"
#define GNB_CONFIG_STRING_DLULTRANSMISSIONPERIODICITY2                   "dl_UL_TransmissionPeriodicity2"
#define GNB_CONFIG_STRING_NROFDOWNLINKSLOTS2                             "nrofDownlinkSlots2"
#define GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS2                           "nrofDownlinkSymbols2"
#define GNB_CONFIG_STRING_NROFUPLINKSLOTS2                               "nrofUplinkSlots2"
#define GNB_CONFIG_STRING_NROFUPLINKSYMBOLS2                             "nrofUplinkSymbols2"
#define GNB_CONFIG_STRING_SSPBCHBLOCKPOWER                               "ssPBCH_BlockPower"


#define CARRIERBANDWIDTH_OKVALUES {11,18,24,25,31,32,38,51,52,65,66,78,79,93,106,107,121,132,133,135,160,162,189,216,217,245,264,270,273}

/* Serving Cell Config Dedicated */
#define GNB_CONFIG_STRING_SERVINGCELLCONFIGDEDICATED                     "servingCellConfigDedicated"
#define GNB_CONFIG_STRING_DLPTRSFREQDENSITY0_0                           "dl_ptrsFreqDensity0_0"
#define GNB_CONFIG_STRING_DLPTRSFREQDENSITY1_0                           "dl_ptrsFreqDensity1_0"
#define GNB_CONFIG_STRING_DLPTRSTIMEDENSITY0_0                           "dl_ptrsTimeDensity0_0"
#define GNB_CONFIG_STRING_DLPTRSTIMEDENSITY1_0                           "dl_ptrsTimeDensity1_0"
#define GNB_CONFIG_STRING_DLPTRSTIMEDENSITY2_0                           "dl_ptrsTimeDensity2_0"
#define GNB_CONFIG_STRING_DLPTRSEPRERATIO_0                              "dl_ptrsEpreRatio_0"
#define GNB_CONFIG_STRING_DLPTRSREOFFSET_0                               "dl_ptrsReOffset_0"
#define GNB_CONFIG_STRING_ULPTRSFREQDENSITY0_0                           "ul_ptrsFreqDensity0_0"
#define GNB_CONFIG_STRING_ULPTRSFREQDENSITY1_0                           "ul_ptrsFreqDensity1_0"
#define GNB_CONFIG_STRING_ULPTRSTIMEDENSITY0_0                           "ul_ptrsTimeDensity0_0"
#define GNB_CONFIG_STRING_ULPTRSTIMEDENSITY1_0                           "ul_ptrsTimeDensity1_0"
#define GNB_CONFIG_STRING_ULPTRSTIMEDENSITY2_0                           "ul_ptrsTimeDensity2_0"
#define GNB_CONFIG_STRING_ULPTRSREOFFSET_0                               "ul_ptrsReOffset_0"
#define GNB_CONFIG_STRING_ULPTRSMAXPORTS_0                               "ul_ptrsMaxPorts_0"
#define GNB_CONFIG_STRING_ULPTRSPOWER_0                                  "ul_ptrsPower_0"

#define GNB_CONFIG_STRING_DLBWP1_ID                                      "dl_bwp-Id_1"
#define GNB_CONFIG_STRING_DLBWP2_ID                                      "dl_bwp-Id_2"
#define GNB_CONFIG_STRING_DLBWP3_ID                                      "dl_bwp-Id_3"
#define GNB_CONFIG_STRING_DLBWP4_ID                                      "dl_bwp-Id_4"
#define GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_DLBWP1                    "dl_bwp1_locationAndBandwidth"
#define GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_DLBWP2                    "dl_bwp2_locationAndBandwidth"
#define GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_DLBWP3                    "dl_bwp3_locationAndBandwidth"
#define GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_DLBWP4                    "dl_bwp4_locationAndBandwidth"
#define GNB_CONFIG_STRING_SCS_DLBWP1                                     "dl_bwp1_subcarrierSpacing"
#define GNB_CONFIG_STRING_SCS_DLBWP2                                     "dl_bwp2_subcarrierSpacing"
#define GNB_CONFIG_STRING_SCS_DLBWP3                                     "dl_bwp3_subcarrierSpacing"
#define GNB_CONFIG_STRING_SCS_DLBWP4                                     "dl_bwp4_subcarrierSpacing"
#define GNB_CONFIG_STRING_FIRSTACTIVEDLBWP_ID                            "firstActiveDownlinkBWP-Id"
#define GNB_CONFIG_STRING_DEFAULTDLBWP_ID                                "defaultDownlinkBWP-Id"
#define GNB_CONFIG_STRING_ULBWP1_ID                                      "ul_bwp-Id_1"
#define GNB_CONFIG_STRING_ULBWP2_ID                                      "ul_bwp-Id_2"
#define GNB_CONFIG_STRING_ULBWP3_ID                                      "ul_bwp-Id_3"
#define GNB_CONFIG_STRING_ULBWP4_ID                                      "ul_bwp-Id_4"
#define GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_ULBWP1                    "ul_bwp1_locationAndBandwidth"
#define GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_ULBWP2                    "ul_bwp2_locationAndBandwidth"
#define GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_ULBWP3                    "ul_bwp3_locationAndBandwidth"
#define GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_ULBWP4                    "ul_bwp4_locationAndBandwidth"
#define GNB_CONFIG_STRING_SCS_ULBWP1                                     "ul_bwp1_subcarrierSpacing"
#define GNB_CONFIG_STRING_SCS_ULBWP2                                     "ul_bwp2_subcarrierSpacing"
#define GNB_CONFIG_STRING_SCS_ULBWP3                                     "ul_bwp3_subcarrierSpacing"
#define GNB_CONFIG_STRING_SCS_ULBWP4                                     "ul_bwp4_subcarrierSpacing"
#define GNB_CONFIG_STRING_FIRSTACTIVEULBWP_ID                            "firstActiveUplinkBWP-Id"

/*--------------------------------------------------------------------------------------------------------------------*/
/*                                            pdcch_ConfigSIB1 parameters                                             */
/*--------------------------------------------------------------------------------------------------------------------*/
#define CONTROL_RESOURCE_SET_ZERO                   "controlResourceSetZero"
#define SEARCH_SPACE_ZERO                           "searchSpaceZero"

#define PDCCH_CONFIGSIB1PARAMS_DESC(pdcch_ConfigSIB1) {                                                                                       \
{CONTROL_RESOURCE_SET_ZERO,        NULL,     0,         i64ptr:&pdcch_ConfigSIB1->controlResourceSetZero,      defintval:0,     TYPE_INT64,        0},   \
{SEARCH_SPACE_ZERO,                NULL,     0,         i64ptr:&pdcch_ConfigSIB1->searchSpaceZero,      defintval:0,     TYPE_INT64,        0}    \
}

#define CONTROL_RESOURCE_SET_ZERO_IDX                   0
#define SEARCH_SPACE_ZERO_IDX                           1
/*--------------------------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     Serving Cell Config Common configuration parameters                                                                                                     */
/*   optname                                                   helpstr   paramflags    XXXptr                                        defXXXval                    type         numelt  */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#define GNB_CONFIG_PHYSCELLID_IDX 0
#define GNB_CONFIG_ABSOLUTEFREQUENCYSSB_IDX 5
#define GNB_CONFIG_DLFREQUENCYBAND_IDX 6
#define GNB_CONFIG_ABSOLUTEFREQUENCYPOINTA_IDX 7
#define GNB_CONFIG_DLCARRIERBANDWIDTH_IDX 10


#define SCCPARAMS_DESC(scc) { \
{GNB_CONFIG_STRING_PHYSCELLID,NULL,0,i64ptr:scc->physCellId,defint64val:0,TYPE_INT64,0/*0*/}, \
{GNB_CONFIG_STRING_NTIMINGADVANCEOFFSET,NULL,0,i64ptr:scc->n_TimingAdvanceOffset,defint64val:NR_ServingCellConfigCommon__n_TimingAdvanceOffset_n0,TYPE_INT64,0/*1*/},\
{GNB_CONFIG_STRING_SSBPERIODICITYSERVINGCELL,NULL,0,i64ptr:scc->ssb_periodicityServingCell,defint64val:NR_ServingCellConfigCommon__ssb_periodicityServingCell_ms20,TYPE_INT64,0/*2*/},\
{GNB_CONFIG_STRING_DMRSTYPEAPOSITION,NULL,0,i64ptr:&scc->dmrs_TypeA_Position,defint64val:NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2,TYPE_INT64,0/*3*/},\
{GNB_CONFIG_STRING_SUBCARRIERSPACING,NULL,0,i64ptr:scc->ssbSubcarrierSpacing,defint64val:NR_SubcarrierSpacing_kHz30,TYPE_INT64,0/*4*/},\
{GNB_CONFIG_STRING_ABSOLUTEFREQUENCYSSB,NULL,0,i64ptr:scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB,defint64val:660960,TYPE_INT64,0/*5*/},\
{GNB_CONFIG_STRING_DLFREQUENCYBAND,NULL,0,i64ptr:scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0],defint64val:78,TYPE_INT64,0/*6*/},\
{GNB_CONFIG_STRING_DLABSOLUEFREQUENCYPOINTA,NULL,0,i64ptr:&scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,defint64val:660000,TYPE_INT64,0/*7*/},\
{GNB_CONFIG_STRING_DLOFFSETTOCARRIER,NULL,0,i64ptr:&scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier,defint64val:0,TYPE_INT64,0/*8*/},\
{GNB_CONFIG_STRING_DLSUBCARRIERSPACING,NULL,0,i64ptr:&scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,defint64val:NR_SubcarrierSpacing_kHz30,TYPE_INT64,0/*9*/},\
{GNB_CONFIG_STRING_DLCARRIERBANDWIDTH,NULL,0,i64ptr:&scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,defint64val:217,TYPE_INT64,0 /*10*/}, \
{GNB_CONFIG_STRING_INITIALDLBWPLOCATIONANDBANDWIDTH,NULL,0,i64ptr:&scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,defint64val:13036,TYPE_INT64,0/*11*/},\
{GNB_CONFIG_STRING_INITIALDLBWPSUBCARRIERSPACING,NULL,0,i64ptr:&scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing,defint64val:NR_SubcarrierSpacing_kHz30,TYPE_INT64,0/*12*/},\
{GNB_CONFIG_STRING_INITIALDLBWPCONTROLRESOURCESETZERO,NULL,0,i64ptr:scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero,defint64val:12,TYPE_INT64,0/*13*/},\
{GNB_CONFIG_STRING_INITIALDLBWPSEARCHSPACEZERO,NULL,0,i64ptr:scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero,defint64val:0,TYPE_INT64,0/*14*/},\
{GNB_CONFIG_STRING_ULFREQUENCYBAND,NULL,0,i64ptr:scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0],defint64val:-1,TYPE_INT64,0/*63*/},\
{GNB_CONFIG_STRING_ULABSOLUEFREQUENCYPOINTA,NULL,0,i64ptr:scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA,defint64val:-1,TYPE_INT64,0/*64*/},\
{GNB_CONFIG_STRING_ULOFFSETTOCARRIER,NULL,0,i64ptr:&scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier,defint64val:0,TYPE_INT64,0/*65*/},\
{GNB_CONFIG_STRING_ULSUBCARRIERSPACING,NULL,0,i64ptr:&scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,defint64val:NR_SubcarrierSpacing_kHz30,TYPE_INT64,0/*66*/},\
{GNB_CONFIG_STRING_ULCARRIERBANDWIDTH,NULL,0,i64ptr:&scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,defint64val:217,TYPE_INT64,0/*67*/},\
{GNB_CONFIG_STRING_PMAX,NULL,0,i64ptr:scc->uplinkConfigCommon->frequencyInfoUL->p_Max,defint64val:20,TYPE_INT64,0/*68*/},\
{GNB_CONFIG_STRING_INITIALULBWPLOCATIONANDBANDWIDTH,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,defint64val:13036,TYPE_INT64,0/*69*/},\
{GNB_CONFIG_STRING_INITIALULBWPSUBCARRIERSPACING,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing,defint64val:NR_SubcarrierSpacing_kHz30,TYPE_INT64,0 /*70*/}, \
{GNB_CONFIG_STRING_PRACHCONFIGURATIONINDEX,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex,defint64val:98,TYPE_INT64,0/*71*/},\
{GNB_CONFIG_STRING_PRACHMSG1FDM,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM,defint64val:NR_RACH_ConfigGeneric__msg1_FDM_one,TYPE_INT64,0/*72*/},\
{GNB_CONFIG_STRING_PRACHMSG1FREQUENCYSTART,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart,defint64val:0,TYPE_INT64,0/*73*/},\
{GNB_CONFIG_STRING_ZEROCORRELATIONZONECONFIG,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig,defint64val:13,TYPE_INT64,0/*74*/},\
{GNB_CONFIG_STRING_PREAMBLERECEIVEDTARGETPOWER,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleReceivedTargetPower,defintval:-118,TYPE_INT64,0/*75*/},\
{GNB_CONFIG_STRING_PREAMBLETRANSMAX,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleTransMax,defint64val:NR_RACH_ConfigGeneric__preambleTransMax_n10,TYPE_INT64,0/*76*/},\
{GNB_CONFIG_STRING_POWERRAMPINGSTEP,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.powerRampingStep,defint64val:NR_RACH_ConfigGeneric__powerRampingStep_dB2,TYPE_INT64,0/*77*/},\
{GNB_CONFIG_STRING_RARESPONSEWINDOW,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.ra_ResponseWindow,defint64val:NR_RACH_ConfigGeneric__ra_ResponseWindow_sl20,TYPE_INT64,0/*78*/},\
{GNB_CONFIG_STRING_SSBPERRACHOCCASIONANDCBPREAMBLESPERSSBPR,NULL,0,uptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present,defuintval:NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one,TYPE_UINT,0/*79*/},\
{GNB_CONFIG_STRING_SSBPERRACHOCCASIONANDCBPREAMBLESPERSSB,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.one,defint64val:NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB__one_n64,TYPE_INT64,0 /*80*/}, \
{GNB_CONFIG_STRING_RACONTENTIONRESOLUTIONTIMER,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ra_ContentionResolutionTimer,defint64val:NR_RACH_ConfigCommon__ra_ContentionResolutionTimer_sf64,TYPE_INT64,0/*81*/},\
{GNB_CONFIG_STRING_RSRPTHRESHOLDSSB,NULL,0,i64ptr:scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB,defint64val:19,TYPE_INT64,0/*82*/},\
{GNB_CONFIG_STRING_PRACHROOTSEQUENCEINDEXPR,NULL,0,uptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present,defuintval:NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l139,TYPE_UINT,0/*83*/},\
{GNB_CONFIG_STRING_PRACHROOTSEQUENCEINDEX,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139,defint64val:0,TYPE_INT64,0/*84*/},\
{GNB_CONFIG_STRING_RESTRICTEDSETCONFIG,NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig,defintval:NR_RACH_ConfigCommon__restrictedSetConfig_unrestrictedSet,TYPE_INT64,0/*85*/}, \
{GNB_CONFIG_STRING_MSG3TRANSFPREC,NULL,0,i64ptr:scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder,defintval:1,TYPE_INT64,0/*86*/}, \
{GNB_CONFIG_STRING_MSG3DELTAPREABMLE, NULL,0,i64ptr:scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble,defint64val:1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_P0NOMINALWITHGRANT, NULL,0,i64ptr:scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant,defint64val:1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_PUCCHGROUPHOPPING, NULL,0,i64ptr:&scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_GroupHopping,defint64val:NR_PUCCH_ConfigCommon__pucch_GroupHopping_neither,TYPE_INT64,0},\
{GNB_CONFIG_STRING_HOPPINGID, NULL,0,i64ptr:scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId,defint64val:40,TYPE_INT64,0},\
{GNB_CONFIG_STRING_P0NOMINAL, NULL,0,i64ptr:scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal,defint64val:1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_PUCCHRES, NULL,0,i64ptr:scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon,defint64val:0,TYPE_INT64,0},\
{GNB_CONFIG_STRING_SSBPOSITIONSINBURSTPR,NULL,0,uptr:&scc->ssb_PositionsInBurst->present,defuintval:NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap,TYPE_UINT,0/*140*/}, \
{GNB_CONFIG_STRING_SSBPOSITIONSINBURST,NULL,0,u64ptr:&ssb_bitmap,defintval:0xff,TYPE_UINT64,0}, \
{GNB_CONFIG_STRING_REFERENCESUBCARRIERSPACING,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing,defint64val:NR_SubcarrierSpacing_kHz30,TYPE_INT64,0},\
{GNB_CONFIG_STRING_DLULTRANSMISSIONPERIODICITY,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity,defint64val:NR_TDD_UL_DL_Pattern__dl_UL_TransmissionPeriodicity_ms0p5,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDOWNLINKSLOTS,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots,defint64val:7,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols,defint64val:6,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFUPLINKSLOTS,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots,defint64val:2,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFUPLINKSYMBOLS,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols,defint64val:4,TYPE_INT64,0},\
{GNB_CONFIG_STRING_DLULTRANSMISSIONPERIODICITY2,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern2->dl_UL_TransmissionPeriodicity,defintval:-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDOWNLINKSLOTS2,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofDownlinkSlots,defint64val:-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS2,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofDownlinkSymbols,defint64val:-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFUPLINKSLOTS2,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofUplinkSlots,defint64val:-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFUPLINKSYMBOLS2,NULL,0,i64ptr:&scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofUplinkSymbols,defint64val:-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_SSPBCHBLOCKPOWER,NULL,0,i64ptr:&scc->ss_PBCH_BlockPower,defint64val:20,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_MSG1SUBCARRIERSPACING,NULL,0,i64ptr:scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing,defintval:-1,TYPE_INT64,0}}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     Serving Cell Config Dedicated configuration parameters                                                                                                     */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#define SCDPARAMS_DESC(scd) { \
{GNB_CONFIG_STRING_DLPTRSFREQDENSITY0_0,NULL,0,i64ptr:scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity->list.array[0],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLPTRSFREQDENSITY1_0,NULL,0,i64ptr:scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity->list.array[1],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLPTRSTIMEDENSITY0_0,NULL,0,i64ptr:scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity->list.array[0],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLPTRSTIMEDENSITY1_0,NULL,0,i64ptr:scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity->list.array[1],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLPTRSTIMEDENSITY2_0,NULL,0,i64ptr:scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity->list.array[2],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLPTRSEPRERATIO_0,NULL,0,i64ptr:scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->epre_Ratio,defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLPTRSREOFFSET_0,NULL,0,i64ptr:scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->resourceElementOffset,defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULPTRSFREQDENSITY0_0,NULL,0,i64ptr:scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.array[0],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULPTRSFREQDENSITY1_0,NULL,0,i64ptr:scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.array[1],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULPTRSTIMEDENSITY0_0,NULL,0,i64ptr:scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[0],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULPTRSTIMEDENSITY1_0,NULL,0,i64ptr:scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[1],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULPTRSTIMEDENSITY2_0,NULL,0,i64ptr:scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[2],defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULPTRSREOFFSET_0,NULL,0,i64ptr:scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->resourceElementOffset,defint64val:-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULPTRSMAXPORTS_0,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->maxNrofPorts,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULPTRSPOWER_0,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->ptrs_Power,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_FIRSTACTIVEDLBWP_ID,NULL,0,i64ptr:scd->firstActiveDownlinkBWP_Id,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_FIRSTACTIVEULBWP_ID,NULL,0,i64ptr:scd->uplinkConfig->firstActiveUplinkBWP_Id,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLBWP1_ID,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Id,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLBWP2_ID,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[1]->bwp_Id,defint64val:2,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLBWP3_ID,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[2]->bwp_Id,defint64val:3,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DLBWP4_ID,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[3]->bwp_Id,defint64val:4,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_SCS_DLBWP1,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Common->genericParameters.subcarrierSpacing,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_SCS_DLBWP2,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[1]->bwp_Common->genericParameters.subcarrierSpacing,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_SCS_DLBWP3,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[2]->bwp_Common->genericParameters.subcarrierSpacing,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_SCS_DLBWP4,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[3]->bwp_Common->genericParameters.subcarrierSpacing,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_DLBWP1,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Common->genericParameters.locationAndBandwidth,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_DLBWP2,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[1]->bwp_Common->genericParameters.locationAndBandwidth,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_DLBWP3,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[2]->bwp_Common->genericParameters.locationAndBandwidth,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_DLBWP4,NULL,0,i64ptr:&scd->downlinkBWP_ToAddModList->list.array[3]->bwp_Common->genericParameters.locationAndBandwidth,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULBWP1_ID,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Id,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULBWP2_ID,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[1]->bwp_Id,defint64val:2,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULBWP3_ID,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[2]->bwp_Id,defint64val:3,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULBWP4_ID,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[3]->bwp_Id,defint64val:4,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_SCS_ULBWP1,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Common->genericParameters.subcarrierSpacing,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_SCS_ULBWP2,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[1]->bwp_Common->genericParameters.subcarrierSpacing,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_SCS_ULBWP3,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[2]->bwp_Common->genericParameters.subcarrierSpacing,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_SCS_ULBWP4,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[3]->bwp_Common->genericParameters.subcarrierSpacing,defint64val:1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_ULBWP1,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Common->genericParameters.locationAndBandwidth,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_ULBWP2,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[1]->bwp_Common->genericParameters.locationAndBandwidth,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_ULBWP3,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[2]->bwp_Common->genericParameters.locationAndBandwidth,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_LOCATIONANDBANDWIDTH_ULBWP4,NULL,0,i64ptr:&scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[3]->bwp_Common->genericParameters.locationAndBandwidth,defint64val:0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_DEFAULTDLBWP_ID,NULL,0,i64ptr:scd->defaultDownlinkBWP_Id,defint64val:0,TYPE_INT64,0}}

#endif
