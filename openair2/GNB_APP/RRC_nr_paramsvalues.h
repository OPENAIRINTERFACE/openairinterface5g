/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief macro definitions for RRC authorized and asn1 parameters values, to be used in paramdef_t/chechedparam_t structure initializations 
 */

#ifndef __NR_RRC_PARAMSVALUES__H__
#define __NR_RRC_PARAMSVALUES__H__

#include "common/config/config_paramdesc.h"

/*    cell configuration section name */
#define GNB_CONFIG_STRING_GNB_LIST                              "gNBs"

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
#define GNB_CONFIG_STRING_MSG3DELTAPREABMLE                     "msg3_DeltaPreamble"
#define GNB_CONFIG_STRING_P0NOMINALWITHGRANT                    "p0_NominalWithGrant"
#define GNB_CONFIG_STRING_PUCCHGROUPHOPPING                     "pucchGroupHopping"
#define GNB_CONFIG_STRING_HOPPINGID                             "hoppingId"
#define GNB_CONFIG_STRING_P0NOMINAL                             "p0_nominal"
#define GNB_CONFIG_STRING_PUCCHRES                              "pucch_ResourceCommon"

#define GNB_CONFIG_STRING_MSGBRESPONSEWINDOW_R16                         "msgB_ResponseWindow_r16"
#define GNB_CONFIG_STRING_MSGARSRPTHRESHOLD_R16                          "msgA_RSRP_Threshold_r16"
#define GNB_CONFIG_STRING_MSGAPRACHCONFIGURATIONINDEX_R16                "msgA_prach_ConfigurationIndex_r16"
#define GNB_CONFIG_STRING_MSGASSBPERRACHOCCASIONANDCBPREAMBLESPERSSBPR   "msgA_ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR"
#define GNB_CONFIG_STRING_MSGASSBPERRACHOCCASIONANDCBPREAMBLESPERSSB     "msgA_ssb_perRACH_OccasionAndCB_PreamblesPerSSB"
#define GNB_CONFIG_STRING_MSGAMCS_R16                                    "msgA_MCS_r16"
#define GNB_CONFIG_STRING_NROFSLOTSMSGAPUSCH_R16                         "nrofSlotsMsgA_PUSCH_r16"
#define GNB_CONFIG_STRING_NROFMSGAPOPERSLOT_R16                          "nrofMsgA_PO_PerSlot_r16"
#define GNB_CONFIG_STRING_MSGAPUSCHTIMEDOMAINOFFSET_R16                  "msgA_PUSCH_TimeDomainOffset_r16"
#define GNB_CONFIG_STRING_STARTSYMBOLANDLENGTHMSGA_PO_R16                "startSymbolAndLengthMsgA_PO_r16"
#define GNB_CONFIG_STRING_MAPPINGTYPEMSGAPUSCH_R16                       "mappingTypeMsgA_PUSCH_r16"
#define GNB_CONFIG_STRING_GUARDBANDMSGAPUSCH_R16                         "guardBandMsgA_PUSCH_r16"
#define GNB_CONFIG_STRING_FREQUENCYSTARTMSGAPUSCH_R16                    "frequencyStartMsgA_PUSCH_r16"
#define GNB_CONFIG_STRING_NROFPRBSPERMSGAPO_R16                          "nrofPRBs_PerMsgA_PO_r16"
#define GNB_CONFIG_STRING_NROFMSGAPOFDM_R16                              "nrofMsgA_PO_FDM_r16"
#define GNB_CONFIG_STRING_MSGAPUSCHNROFPORTS_R16                         "msgA_PUSCH_NrofPorts_r16"
#define GNB_CONFIG_STRING_NROFDMRSSEQUENCES_R16                          "nrofDMRS_Sequences_r16"
#define GNB_CONFIG_STRING_MSGATRANSFORMPRECODER_R16                      "msgA_TransformPrecoder_r16"

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

#define GNB_CONFIG_STRING_ULSYNCVALIDITYDURATION                         "ntn-UlSyncValidityDuration-r17"
#define GNB_CONFIG_STRING_CELLSPECIFICKOFFSET                            "cellSpecificKoffset_r17"
#define GNB_CONFIG_STRING_EPHEMERIS_POSITION_X                           "positionX-r17"
#define GNB_CONFIG_STRING_EPHEMERIS_POSITION_Y                           "positionY-r17"
#define GNB_CONFIG_STRING_EPHEMERIS_POSITION_Z                           "positionZ-r17"
#define GNB_CONFIG_STRING_EPHEMERIS_VELOCITY_VX                          "velocityVX-r17"
#define GNB_CONFIG_STRING_EPHEMERIS_VELOCITY_VY                          "velocityVY-r17"
#define GNB_CONFIG_STRING_EPHEMERIS_VELOCITY_VZ                          "velocityVZ-r17"
#define GNB_CONFIG_STRING_TA_COMMON                                      "ta-Common-r17"
#define GNB_CONFIG_STRING_TA_COMMONDRIFT                                 "ta-CommonDrift-r17"

/*--------------------------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     Serving Cell Config Common configuration parameters                                                                                                     */
/*   optname                                                   helpstr   paramflags    XXXptr                                        defXXXval                    type         numelt  */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/



#define SCCPARAMS_DESC(scc) { \
{GNB_CONFIG_STRING_PHYSCELLID,NULL,0,.i64ptr=scc->physCellId,.defint64val=0,TYPE_INT64,0/*0*/}, \
{GNB_CONFIG_STRING_NTIMINGADVANCEOFFSET,NULL,0,.i64ptr=scc->n_TimingAdvanceOffset,.defint64val=-1,TYPE_INT64,0/*1*/},\
{GNB_CONFIG_STRING_SSBPERIODICITYSERVINGCELL,NULL,0,.i64ptr=scc->ssb_periodicityServingCell,.defint64val=NR_ServingCellConfigCommon__ssb_periodicityServingCell_ms20,TYPE_INT64,0/*2*/},\
{GNB_CONFIG_STRING_DMRSTYPEAPOSITION,NULL,0,.i64ptr=&scc->dmrs_TypeA_Position,.defint64val=NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2,TYPE_INT64,0/*3*/},\
{GNB_CONFIG_STRING_SUBCARRIERSPACING,NULL,0,.i64ptr=scc->ssbSubcarrierSpacing,.defint64val=NR_SubcarrierSpacing_kHz30,TYPE_INT64,0/*4*/},\
{GNB_CONFIG_STRING_ABSOLUTEFREQUENCYSSB,NULL,0,.i64ptr=scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB,.defint64val=660960,TYPE_INT64,0/*5*/},\
{GNB_CONFIG_STRING_DLFREQUENCYBAND,NULL,0,.i64ptr=scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0],.defint64val=78,TYPE_INT64,0/*6*/},\
{GNB_CONFIG_STRING_DLABSOLUEFREQUENCYPOINTA,NULL,0,.i64ptr=&scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,.defint64val=660000,TYPE_INT64,0/*7*/},\
{GNB_CONFIG_STRING_DLOFFSETTOCARRIER,NULL,0,.i64ptr=&scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier,.defint64val=0,TYPE_INT64,0/*8*/},\
{GNB_CONFIG_STRING_DLSUBCARRIERSPACING,NULL,0,.i64ptr=&scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,.defint64val=NR_SubcarrierSpacing_kHz30,TYPE_INT64,0/*9*/},\
{GNB_CONFIG_STRING_DLCARRIERBANDWIDTH,NULL,0,.i64ptr=&scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,.defint64val=217,TYPE_INT64,0 /*10*/}, \
{GNB_CONFIG_STRING_INITIALDLBWPLOCATIONANDBANDWIDTH,NULL,0,.i64ptr=&scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,.defint64val=13036,TYPE_INT64,0/*11*/},\
{GNB_CONFIG_STRING_INITIALDLBWPSUBCARRIERSPACING,NULL,0,.i64ptr=&scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing,.defint64val=NR_SubcarrierSpacing_kHz30,TYPE_INT64,0/*12*/},\
{GNB_CONFIG_STRING_INITIALDLBWPCONTROLRESOURCESETZERO,NULL,0,.i64ptr=scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero,.defint64val=-1,TYPE_INT64,0/*13*/},\
{GNB_CONFIG_STRING_INITIALDLBWPSEARCHSPACEZERO,NULL,0,.i64ptr=scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero,.defint64val=-1,TYPE_INT64,0/*14*/},\
{GNB_CONFIG_STRING_ULFREQUENCYBAND,NULL,0,.i64ptr=scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0],.defint64val=-1,TYPE_INT64,0/*63*/},\
{GNB_CONFIG_STRING_ULABSOLUEFREQUENCYPOINTA,NULL,0,.i64ptr=scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA,.defint64val=-1,TYPE_INT64,0/*64*/},\
{GNB_CONFIG_STRING_ULOFFSETTOCARRIER,NULL,0,.i64ptr=&scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier,.defint64val=0,TYPE_INT64,0/*65*/},\
{GNB_CONFIG_STRING_ULSUBCARRIERSPACING,NULL,0,.i64ptr=&scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,.defint64val=NR_SubcarrierSpacing_kHz30,TYPE_INT64,0/*66*/},\
{GNB_CONFIG_STRING_ULCARRIERBANDWIDTH,NULL,0,.i64ptr=&scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,.defint64val=217,TYPE_INT64,0/*67*/},\
{GNB_CONFIG_STRING_PMAX,NULL,0,.i64ptr=scc->uplinkConfigCommon->frequencyInfoUL->p_Max,.defint64val=20,TYPE_INT64,0/*68*/},\
{GNB_CONFIG_STRING_INITIALULBWPLOCATIONANDBANDWIDTH,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,.defint64val=13036,TYPE_INT64,0/*69*/},\
{GNB_CONFIG_STRING_INITIALULBWPSUBCARRIERSPACING,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing,.defint64val=NR_SubcarrierSpacing_kHz30,TYPE_INT64,0 /*70*/}, \
{GNB_CONFIG_STRING_PRACHCONFIGURATIONINDEX,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex,.defint64val=98,TYPE_INT64,0/*71*/},\
{GNB_CONFIG_STRING_PRACHMSG1FDM,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM,.defint64val=NR_RACH_ConfigGeneric__msg1_FDM_one,TYPE_INT64,0/*72*/},\
{GNB_CONFIG_STRING_PRACHMSG1FREQUENCYSTART,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart,.defint64val=0,TYPE_INT64,0/*73*/},\
{GNB_CONFIG_STRING_ZEROCORRELATIONZONECONFIG,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig,.defint64val=13,TYPE_INT64,0/*74*/},\
{GNB_CONFIG_STRING_PREAMBLERECEIVEDTARGETPOWER,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleReceivedTargetPower,.defint64val=-118,TYPE_INT64,0/*75*/},\
{GNB_CONFIG_STRING_PREAMBLETRANSMAX,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleTransMax,.defint64val=NR_RACH_ConfigGeneric__preambleTransMax_n10,TYPE_INT64,0/*76*/},\
{GNB_CONFIG_STRING_POWERRAMPINGSTEP,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.powerRampingStep,.defint64val=NR_RACH_ConfigGeneric__powerRampingStep_dB2,TYPE_INT64,0/*77*/},\
{GNB_CONFIG_STRING_RARESPONSEWINDOW,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.ra_ResponseWindow,.defint64val=-1,TYPE_INT64,0/*78*/},\
{GNB_CONFIG_STRING_SSBPERRACHOCCASIONANDCBPREAMBLESPERSSBPR,NULL,0,.uptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present,.defuintval=NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one,TYPE_UINT,0/*79*/},\
{GNB_CONFIG_STRING_SSBPERRACHOCCASIONANDCBPREAMBLESPERSSB,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.one,.defint64val=NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB__one_n64,TYPE_INT64,0 /*80*/}, \
{GNB_CONFIG_STRING_RACONTENTIONRESOLUTIONTIMER,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ra_ContentionResolutionTimer,.defint64val=NR_RACH_ConfigCommon__ra_ContentionResolutionTimer_sf64,TYPE_INT64,0/*81*/},\
{GNB_CONFIG_STRING_RSRPTHRESHOLDSSB,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB,.defint64val=19,TYPE_INT64,0/*82*/},\
{GNB_CONFIG_STRING_PRACHROOTSEQUENCEINDEXPR,NULL,0,.uptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present,.defuintval=NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l139,TYPE_UINT,0/*83*/},\
{GNB_CONFIG_STRING_PRACHROOTSEQUENCEINDEX,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139,.defint64val=0,TYPE_INT64,0/*84*/},\
{GNB_CONFIG_STRING_RESTRICTEDSETCONFIG,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig,.defint64val=NR_RACH_ConfigCommon__restrictedSetConfig_unrestrictedSet,TYPE_INT64,0/*85*/}, \
{GNB_CONFIG_STRING_MSG3TRANSFPREC,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder,.defint64val=1,TYPE_INT64,0/*86*/}, \
{GNB_CONFIG_STRING_MSG3DELTAPREABMLE, NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble,.defint64val=1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_P0NOMINALWITHGRANT, NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant,.defint64val=1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_PUCCHGROUPHOPPING, NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_GroupHopping,.defint64val=NR_PUCCH_ConfigCommon__pucch_GroupHopping_neither,TYPE_INT64,0},\
{GNB_CONFIG_STRING_HOPPINGID, NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId,.defint64val=40,TYPE_INT64,0},\
{GNB_CONFIG_STRING_P0NOMINAL, NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal,.defint64val=1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_PUCCHRES, NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon,.defint64val=0,TYPE_INT64,0},\
{GNB_CONFIG_STRING_SSBPOSITIONSINBURST,NULL,0,.u64ptr=&ssb_bitmap,.defint64val=0xff,TYPE_UINT64,0}, \
{GNB_CONFIG_STRING_REFERENCESUBCARRIERSPACING,NULL,0,.i64ptr=&scc->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing,.defint64val=NR_SubcarrierSpacing_kHz30,TYPE_INT64,0},\
{GNB_CONFIG_STRING_DLULTRANSMISSIONPERIODICITY,NULL,0,.i64ptr=&scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity,.defint64val=NR_TDD_UL_DL_Pattern__dl_UL_TransmissionPeriodicity_ms0p5,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDOWNLINKSLOTS,NULL,0,.i64ptr=&scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots,.defint64val=7,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS,NULL,0,.i64ptr=&scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols,.defint64val=6,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFUPLINKSLOTS,NULL,0,.i64ptr=&scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots,.defint64val=2,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFUPLINKSYMBOLS,NULL,0,.i64ptr=&scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols,.defint64val=4,TYPE_INT64,0},\
{GNB_CONFIG_STRING_SSPBCHBLOCKPOWER,NULL,0,.i64ptr=&scc->ss_PBCH_BlockPower,.defint64val=20,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_ULSYNCVALIDITYDURATION,NULL,0,.i64ptr=scc->ext2->ntn_Config_r17->ntn_UlSyncValidityDuration_r17,.defint64val=0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_CELLSPECIFICKOFFSET,NULL,0,.i64ptr=scc->ext2->ntn_Config_r17->cellSpecificKoffset_r17,.defint64val=0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_EPHEMERIS_POSITION_X,NULL,0,.i64ptr=&scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17->positionX_r17,.defint64val=LONG_MAX,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_EPHEMERIS_POSITION_Y,NULL,0,.i64ptr=&scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17->positionY_r17,.defint64val=LONG_MAX,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_EPHEMERIS_POSITION_Z,NULL,0,.i64ptr=&scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17->positionZ_r17,.defint64val=LONG_MAX,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_EPHEMERIS_VELOCITY_VX,NULL,0,.i64ptr=&scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17->velocityVX_r17,.defint64val=LONG_MAX,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_EPHEMERIS_VELOCITY_VY,NULL,0,.i64ptr=&scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17->velocityVY_r17,.defint64val=LONG_MAX,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_EPHEMERIS_VELOCITY_VZ,NULL,0,.i64ptr=&scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17->velocityVZ_r17,.defint64val=LONG_MAX,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_TA_COMMON,NULL,0,.i64ptr=&scc->ext2->ntn_Config_r17->ta_Info_r17->ta_Common_r17,.defint64val=-1,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_TA_COMMONDRIFT,NULL,0,.i64ptr=scc->ext2->ntn_Config_r17->ta_Info_r17->ta_CommonDrift_r17,.defint64val=0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_MSG1SUBCARRIERSPACING,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing,.defint64val=-1,TYPE_INT64,0}}

#define SCC_PATTERN2_STRING_CONFIG     "pattern2"

#define SCC_PATTERN2_PARAMS_DESC(pattern2) { \
{GNB_CONFIG_STRING_DLULTRANSMISSIONPERIODICITY2, NULL,0,.i64ptr=&pattern2.dl_UL_TransmissionPeriodicity,.defint64val=-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDOWNLINKSLOTS2, NULL,0,.i64ptr=&pattern2.nrofDownlinkSlots,.defint64val=-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDOWNLINKSYMBOLS2, NULL,0,.i64ptr=&pattern2.nrofDownlinkSymbols,.defint64val=-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFUPLINKSLOTS2,NULL, 0,.i64ptr=&pattern2.nrofUplinkSlots,.defint64val=-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFUPLINKSYMBOLS2,NULL, 0,.i64ptr=&pattern2.nrofUplinkSymbols,.defint64val=-1,TYPE_INT64,0}}


/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                     Serving Cell Config Common configuration parameters to apply in RA 2-Step                                                                                                    */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#define MSGASCCPARAMS_DESC(scc) { \
{GNB_CONFIG_STRING_MSGBRESPONSEWINDOW_R16,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->rach_ConfigCommonTwoStepRA_r16.rach_ConfigGenericTwoStepRA_r16.msgB_ResponseWindow_r16,.defint64val=-1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_MSGARSRPTHRESHOLD_R16,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->rach_ConfigCommonTwoStepRA_r16.msgA_RSRP_Threshold_r16,.defint64val=19,TYPE_INT64,0},\
{GNB_CONFIG_STRING_MSGAPRACHCONFIGURATIONINDEX_R16,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->rach_ConfigCommonTwoStepRA_r16.rach_ConfigGenericTwoStepRA_r16.msgA_PRACH_ConfigurationIndex_r16,.defint64val=98,TYPE_INT64,0},\
{GNB_CONFIG_STRING_MSGASSBPERRACHOCCASIONANDCBPREAMBLESPERSSBPR,NULL,0,.uptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->rach_ConfigCommonTwoStepRA_r16.msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16->present,.defuintval=0,TYPE_UINT,0},\
{GNB_CONFIG_STRING_MSGASSBPERRACHOCCASIONANDCBPREAMBLESPERSSB,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->rach_ConfigCommonTwoStepRA_r16.msgA_SSB_PerRACH_OccasionAndCB_PreamblesPerSSB_r16->choice.one,.defint64val=0,TYPE_INT64,0},\
{GNB_CONFIG_STRING_MSGAMCS_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->msgA_MCS_r16,.defint64val=2,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFSLOTSMSGAPUSCH_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->nrofSlotsMsgA_PUSCH_r16,.defint64val=1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFMSGAPOPERSLOT_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->nrofMsgA_PO_PerSlot_r16,.defint64val=NR_MsgA_PUSCH_Resource_r16__nrofMsgA_PO_PerSlot_r16_one,TYPE_INT64,0},\
{GNB_CONFIG_STRING_MSGAPUSCHTIMEDOMAINOFFSET_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->msgA_PUSCH_TimeDomainOffset_r16,.defint64val=0,TYPE_INT64,0},\
{GNB_CONFIG_STRING_STARTSYMBOLANDLENGTHMSGA_PO_R16,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->startSymbolAndLengthMsgA_PO_r16,.defint64val=38,TYPE_INT64,0},\
{GNB_CONFIG_STRING_MAPPINGTYPEMSGAPUSCH_R16,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->mappingTypeMsgA_PUSCH_r16,.defint64val=0,TYPE_INT64,0},\
{GNB_CONFIG_STRING_GUARDBANDMSGAPUSCH_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->guardBandMsgA_PUSCH_r16,.defint64val=0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_FREQUENCYSTARTMSGAPUSCH_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->frequencyStartMsgA_PUSCH_r16,.defint64val=0,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_NROFPRBSPERMSGAPO_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->nrofPRBs_PerMsgA_PO_r16,.defint64val=8,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_NROFMSGAPOFDM_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->nrofMsgA_PO_FDM_r16,.defint64val=NR_MsgA_PUSCH_Resource_r16__nrofMsgA_PO_FDM_r16_one,TYPE_INT64,0}, \
{GNB_CONFIG_STRING_MSGAPUSCHNROFPORTS_R16,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->msgA_DMRS_Config_r16.msgA_PUSCH_NrofPorts_r16,.defint64val=1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_NROFDMRSSEQUENCES_R16,NULL,0,.i64ptr=&scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_PUSCH_ResourceGroupA_r16->nrofDMRS_Sequences_r16,.defint64val=1,TYPE_INT64,0},\
{GNB_CONFIG_STRING_MSGATRANSFORMPRECODER_R16,NULL,0,.i64ptr=scc->uplinkConfigCommon->initialUplinkBWP->ext1->msgA_ConfigCommon_r16->choice.setup->msgA_PUSCH_Config_r16->msgA_TransformPrecoder_r16,.defint64val=NR_MsgA_PUSCH_Config_r16__msgA_TransformPrecoder_r16_disabled,TYPE_INT64,0}}
#endif
