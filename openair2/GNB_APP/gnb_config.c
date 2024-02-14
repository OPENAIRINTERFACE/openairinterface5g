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

/*
  gnb_config.c
  -------------------
  AUTHOR  : Lionel GAUTHIER, navid nikaein, Laurent Winckel, WEI-TAI CHEN
  COMPANY : EURECOM, NTUST
  EMAIL   : Lionel.Gauthier@eurecom.fr, navid.nikaein@eurecom.fr, kroempa@gmail.com
*/

#include <string.h>
#include <inttypes.h>

#include "common/utils/LOG/log.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/LOG/log_extern.h"
#include "assertions.h"
#include "oai_asn1.h"
#include "executables/softmodem-common.h"
#include "gnb_config.h"
#include "gnb_paramdef.h"
#include "enb_paramdef.h"
#include "UTIL/OTG/otg.h"
#include "UTIL/OTG/otg_externs.h"
#include "intertask_interface.h"
#include "s1ap_eNB.h"
#include "ngap_gNB.h"
#include "sctp_eNB_task.h"
#include "sctp_default_values.h"
#include "F1AP_CauseRadioNetwork.h"
#include "f1ap_common.h"
// #include "SystemInformationBlockType2.h"
// #include "LAYER2/MAC/extern.h"
// #include "LAYER2/MAC/proto.h"
#include "PHY/INIT/nr_phy_init.h"
#include "radio/ETHERNET/ethernet_lib.h"
#include "nfapi_vnf.h"
#include "nfapi_pnf.h"
#include "nr_pdcp/nr_pdcp_oai_api.h"

//#include "L1_paramdef.h"
#include "prs_nr_paramdef.h"
#include "L1_nr_paramdef.h"
#include "MACRLC_nr_paramdef.h"
#include "common/config/config_userapi.h"
//#include "RRC_config_tools.h"
#include "gnb_paramdef.h"
#include "NR_MAC_gNB/mac_proto.h"
#include <openair3/ocp-gtpu/gtp_itf.h>

#include "NR_asn_constant.h"
#include "executables/thread-common.h"
#include "NR_SCS-SpecificCarrier.h"
#include "NR_TDD-UL-DL-ConfigCommon.h"
#include "NR_FrequencyInfoUL.h"
#include "NR_RACH-ConfigGeneric.h"
#include "NR_RACH-ConfigCommon.h"
#include "NR_PUSCH-TimeDomainResourceAllocation.h"
#include "NR_PUSCH-ConfigCommon.h"
#include "NR_PUCCH-ConfigCommon.h"
#include "NR_PDSCH-TimeDomainResourceAllocation.h"
#include "NR_PDSCH-ConfigCommon.h"
#include "NR_RateMatchPattern.h"
#include "NR_RateMatchPatternLTE-CRS.h"
#include "NR_SearchSpace.h"
#include "NR_ControlResourceSet.h"
#include "NR_EUTRA-MBSFN-SubframeConfig.h"
#include "uper_decoder.h"
#include "uper_encoder.h"

#include "RRC/NR/MESSAGES/asn1_msg.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp.h"
#include "nfapi/oai_integration/vendor_ext.h"

extern uint16_t sf_ahead;

// synchronization raster per band tables (Rel.15)
// (38.101-1 Table 5.4.3.3-1 and 38.101-2 Table 5.4.3.3-1)
// band nb, sub-carrier spacing index, Range of gscn (First, Step size, Last)
const sync_raster_t sync_raster[] = {
  {1, 0, 5279, 1, 5419},
  {2, 0, 4829, 1, 4969},
  {3, 0, 4517, 1, 4693},
  {5, 0, 2177, 1, 2230},
  {5, 1, 2183, 1, 2224},
  {7, 0, 6554, 1, 6718},
  {8, 0, 2318, 1, 2395},
  {12, 0, 1828, 1, 1858},
  {20, 0, 1982, 1, 2047},
  {25, 0, 4829, 1, 4981},
  {28, 0, 1901, 1, 2002},
  {34, 0, 5030, 1, 5056},
  {38, 0, 6431, 1, 6544},
  {39, 0, 4706, 1, 4795},
  {40, 1, 5762, 1, 5989},
  {41, 0, 6246, 3, 6717},
  {41, 1, 6252, 3, 6714},
  {48, 1, 7884, 1, 7982},
  {50, 0, 3584, 1, 3787},
  {51, 0, 3572, 1, 3574},
  {66, 0, 5279, 1, 5494},
  {66, 1, 5285, 1, 5488},
  {70, 0, 4993, 1, 5044},
  {71, 0, 1547, 1, 1624},
  {74, 0, 3692, 1, 3790},
  {75, 0, 3584, 1, 3787},
  {76, 0, 3572, 1, 3574},
  {77, 1, 7711, 1, 8329},
  {78, 1, 7711, 1, 8051},
  {79, 1, 8480, 16, 8880},
  {257, 3, 22388, 1, 22558},
  {257, 4, 22390, 2, 22556},
  {258, 3, 22257, 1, 22443},
  {258, 4, 22258, 2, 22442},
  {260, 3, 22995, 1, 23166},
  {260, 4, 22996, 2, 23164},
  {261, 3, 22446, 1, 22492},
  {261, 4, 22446, 2, 22490},
};

extern int config_check_band_frequencies(int ind, int16_t band, uint64_t downlink_frequency,
                                         int32_t uplink_frequency_offset, uint32_t  frame_type);

void prepare_scc(NR_ServingCellConfigCommon_t *scc) {

  NR_FreqBandIndicatorNR_t                        *dl_frequencyBandList,*ul_frequencyBandList;
  struct NR_SCS_SpecificCarrier                   *dl_scs_SpecificCarrierList,*ul_scs_SpecificCarrierList;
  //  struct NR_RateMatchPattern                      *ratematchpattern;
  //  NR_RateMatchPatternId_t                         *ratematchpatternid;
  //  NR_TCI_StateId_t                                *TCI_StateId;
  //  struct NR_ControlResourceSet                    *bwp_dl_controlresourceset;
  //  NR_SearchSpace_t                                *bwp_dl_searchspace;

  scc->physCellId                                = CALLOC(1,sizeof(NR_PhysCellId_t));
  scc->downlinkConfigCommon                      = CALLOC(1,sizeof(struct NR_DownlinkConfigCommon));
  scc->downlinkConfigCommon->frequencyInfoDL     = CALLOC(1,sizeof(struct NR_FrequencyInfoDL));
  scc->downlinkConfigCommon->initialDownlinkBWP  = CALLOC(1,sizeof(struct NR_BWP_DownlinkCommon));
  scc->uplinkConfigCommon                        = CALLOC(1,sizeof(struct NR_UplinkConfigCommon));
  scc->uplinkConfigCommon->frequencyInfoUL       = CALLOC(1,sizeof(struct NR_FrequencyInfoUL));
  scc->uplinkConfigCommon->initialUplinkBWP      = CALLOC(1,sizeof(struct NR_BWP_UplinkCommon));
  //scc->supplementaryUplinkConfig       = CALLOC(1,sizeof(struct NR_UplinkConfigCommon));
  scc->n_TimingAdvanceOffset = CALLOC(1, sizeof(long));
  scc->ssb_PositionsInBurst                      = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__ssb_PositionsInBurst));
  scc->ssb_periodicityServingCell                = CALLOC(1,sizeof(long));
  //  scc->rateMatchPatternToAddModList              = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__rateMatchPatternToAddModList));
  //  scc->rateMatchPatternToReleaseList             = CALLOC(1,sizeof(struct NR_ServingCellConfigCommon__rateMatchPatternToReleaseList));
  scc->ssbSubcarrierSpacing                      = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
  scc->tdd_UL_DL_ConfigurationCommon             = CALLOC(1,sizeof(struct NR_TDD_UL_DL_ConfigCommon));
  scc->tdd_UL_DL_ConfigurationCommon->pattern2   = CALLOC(1,sizeof(struct NR_TDD_UL_DL_Pattern));
  
  scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB     = CALLOC(1,sizeof(NR_ARFCN_ValueNR_t));
  
  dl_frequencyBandList              = CALLOC(1,sizeof(NR_FreqBandIndicatorNR_t));
  dl_scs_SpecificCarrierList        = CALLOC(1,sizeof(struct NR_SCS_SpecificCarrier));

  asn1cSeqAdd(&scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list,dl_frequencyBandList);  
  asn1cSeqAdd(&scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list,dl_scs_SpecificCarrierList);		   		   
  //  scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.cyclicPrefix    = CALLOC(1,sizeof(long));
  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon                = CALLOC(1,sizeof(struct NR_SetupRelease_PDCCH_ConfigCommon));
  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->present=NR_SetupRelease_PDCCH_ConfigCommon_PR_setup; 
  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup  = CALLOC(1,sizeof(struct NR_PDCCH_ConfigCommon));
  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero    = CALLOC(1,sizeof(long));
  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero           = CALLOC(1,sizeof(long));

  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonControlResourceSet = NULL;

  //  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList     = CALLOC(1,sizeof(struct NR_PDCCH_ConfigCommon__commonSearchSpaceList));
  //  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceSIB1                    = CALLOC(1,sizeof(NR_SearchSpaceId_t));
  //  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation  = CALLOC(1,sizeof(NR_SearchSpaceId_t));
  //  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->pagingSearchSpace                  = CALLOC(1,sizeof(NR_SearchSpaceId_t));
  //  scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_SearchSpace                     = CALLOC(1,sizeof(NR_SearchSpaceId_t));
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon                 = CALLOC(1,sizeof(struct NR_SetupRelease_PDSCH_ConfigCommon));
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->present        = NR_SetupRelease_PDSCH_ConfigCommon_PR_setup;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup   = CALLOC(1,sizeof(struct NR_PDSCH_ConfigCommon));
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList = CALLOC(1,sizeof(struct NR_PDSCH_TimeDomainResourceAllocationList));

  ul_frequencyBandList              = CALLOC(1,sizeof(NR_FreqBandIndicatorNR_t));
  scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList          = CALLOC(1,sizeof(struct NR_MultiFrequencyBandListNR));
  asn1cSeqAdd(&scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list,ul_frequencyBandList);

  scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA    = CALLOC(1,sizeof(NR_ARFCN_ValueNR_t));
  //  scc->uplinkConfigCommon->frequencyInfoUL->additionalSpectrumEmission = CALLOC(1,sizeof(NR_AdditionalSpectrumEmission_t));
  scc->uplinkConfigCommon->frequencyInfoUL->p_Max                      = CALLOC(1,sizeof(NR_P_Max_t));
  //  scc->uplinkConfigCommon->frequencyInfoUL->frequencyShift7p5khz       = CALLOC(1,sizeof(long));
  
  //  scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.cyclicPrefix    = CALLOC(1,sizeof(long));
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon                 = CALLOC(1,sizeof(NR_SetupRelease_RACH_ConfigCommon_t));
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->present = NR_SetupRelease_RACH_ConfigCommon_PR_setup;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup   = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon));
  // scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles                  = CALLOC(1,sizeof(long));
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB  = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB));
  //  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->groupBconfigured                           = CALLOC(1,sizeof(struct NR_RACH_ConfigCommon__groupBconfigured));
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB            = CALLOC(1,sizeof(NR_RSRP_Range_t));
  //  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB_SUL        = CALLOC(1,sizeof(NR_RSRP_Range_t));
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing       = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder       = CALLOC(1,sizeof(long));
  // 0 - ENABLE, 1 - DISABLE, hence explicitly setting to DISABLED.
  *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder       = NR_PUSCH_Config__transformPrecoder_disabled;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon                 = CALLOC(1,sizeof(NR_SetupRelease_PUSCH_ConfigCommon_t)); 
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->present        = NR_SetupRelease_PUSCH_ConfigCommon_PR_setup;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup   = CALLOC(1,sizeof(struct NR_PUSCH_ConfigCommon));

  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->groupHoppingEnabledTransformPrecoding = NULL;
  
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList  = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocationList));
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble              = CALLOC(1,sizeof(long));
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant             = CALLOC(1,sizeof(long));
  
  scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon                                                = CALLOC(1,sizeof(struct NR_SetupRelease_PUCCH_ConfigCommon)); 
  scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->present= NR_SetupRelease_PUCCH_ConfigCommon_PR_setup;
  scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup                                  = CALLOC(1,sizeof(struct NR_PUCCH_ConfigCommon));
  scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal                      = CALLOC(1,sizeof(long));
  scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon            = CALLOC(1,sizeof(long));
  scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId                       = CALLOC(1,sizeof(long));

  //  scc->ssb_PositionsInBurst->choice.shortBitmap.buf  = MALLOC(1);
  //  scc->ssb_PositionsInBurst->choice.mediumBitmap.buf = MALLOC(1);
  //  scc->ssb_PositionsInBurst->choice.longBitmap.buf   = MALLOC(8);
  

  ul_scs_SpecificCarrierList  = CALLOC(1,sizeof(struct NR_SCS_SpecificCarrier));
  asn1cSeqAdd(&scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list,ul_scs_SpecificCarrierList);

  //ratematchpattern                              = CALLOC(1,sizeof(struct NR_RateMatchPattern));
  //ratematchpattern->patternType.choice.bitmaps  = CALLOC(1,sizeof(struct NR_RateMatchPattern__patternType__bitmaps));
  //ratematchpattern->patternType.choice.bitmaps->resourceBlocks.buf = MALLOC(35);
  //ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.oneSlot.buf   = MALLOC(2);
  //ratematchpattern->patternType.choice.bitmaps->symbolsInResourceBlock.choice.twoSlots.buf  = MALLOC(4);
  //ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern                       = CALLOC(1,sizeof(struct NR_RateMatchPattern__patternType__bitmaps__periodicityAndPattern));
  //ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n2.buf        = MALLOC(1);
  //ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n4.buf        = MALLOC(1);
  //ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n5.buf        = MALLOC(1);
  //ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n8.buf        = MALLOC(1);
  //ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n10.buf       = MALLOC(2);
  //ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n20.buf       = MALLOC(3);
  //ratematchpattern->patternType.choice.bitmaps->periodicityAndPattern->choice.n40.buf       = MALLOC(5);
  //ratematchpattern->subcarrierSpacing           = CALLOC(1,sizeof(NR_SubcarrierSpacing_t));
  //ratematchpatternid                            = CALLOC(1,sizeof(NR_RateMatchPatternId_t));
  
}

void fill_scc_sim(NR_ServingCellConfigCommon_t *scc,uint64_t *ssb_bitmap,int N_RB_DL,int N_RB_UL,int mu_dl,int mu_ul) {

  *scc->physCellId=0;							\
  //  *scc->n_TimingAdvanceOffset=NR_ServingCellConfigCommon__n_TimingAdvanceOffset_n0;
  *scc->ssb_periodicityServingCell=NR_ServingCellConfigCommon__ssb_periodicityServingCell_ms20;
  scc->dmrs_TypeA_Position=NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2;
  *scc->ssbSubcarrierSpacing=mu_dl;
  if (mu_dl == 0) {
    *scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB=520432;
    *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]=38;
    scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA=520000;
  } else {
    *scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB=641032;
    *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]=78;
    scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA=640000;
  }
  scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier=0;
  scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing=mu_dl;
  scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth=N_RB_DL;
  scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth =
      PRBalloc_to_locationandbandwidth(N_RB_DL, 0);
  scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing=mu_dl;//NR_SubcarrierSpacing_kHz30;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero=12;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero=0;
  struct NR_PDSCH_TimeDomainResourceAllocation *timedomainresourceallocation0 = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
  timedomainresourceallocation0->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  timedomainresourceallocation0->startSymbolAndLength=54;
  asn1cSeqAdd(&scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,
                   timedomainresourceallocation0);
  struct NR_PDSCH_TimeDomainResourceAllocation *timedomainresourceallocation1 = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
  timedomainresourceallocation1->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  timedomainresourceallocation1->startSymbolAndLength=57;
  asn1cSeqAdd(&scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,
                   timedomainresourceallocation1);
  *scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0]=mu_ul?78:38;
  *scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA=-1;
  scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier=0;
  scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing=mu_ul;
  scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth=N_RB_UL;
  *scc->uplinkConfigCommon->frequencyInfoUL->p_Max=20;
  scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth = PRBalloc_to_locationandbandwidth(N_RB_UL, 0);
  scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing=mu_ul;//NR_SubcarrierSpacing_kHz30;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex=98;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM=NR_RACH_ConfigGeneric__msg1_FDM_one;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FrequencyStart=0;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig=13;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleReceivedTargetPower=-118;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleTransMax=NR_RACH_ConfigGeneric__preambleTransMax_n10;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.powerRampingStep=NR_RACH_ConfigGeneric__powerRampingStep_dB2;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.ra_ResponseWindow=NR_RACH_ConfigGeneric__ra_ResponseWindow_sl20;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present=NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.one=NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB__one_n64;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ra_ContentionResolutionTimer=NR_RACH_ConfigCommon__ra_ContentionResolutionTimer_sf64;
  *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rsrp_ThresholdSSB=19;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present=NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l139;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.choice.l139=0;
  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig=NR_RACH_ConfigCommon__restrictedSetConfig_unrestrictedSet;
  struct NR_PUSCH_TimeDomainResourceAllocation *pusch_timedomainresourceallocation0 = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
  pusch_timedomainresourceallocation0->k2  = CALLOC(1,sizeof(long));
  *pusch_timedomainresourceallocation0->k2=6;
  pusch_timedomainresourceallocation0->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  pusch_timedomainresourceallocation0->startSymbolAndLength=55;
  asn1cSeqAdd(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,pusch_timedomainresourceallocation0);
  struct NR_PUSCH_TimeDomainResourceAllocation *pusch_timedomainresourceallocation1 = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
  pusch_timedomainresourceallocation1->k2  = CALLOC(1,sizeof(long));
  *pusch_timedomainresourceallocation1->k2=6;
  pusch_timedomainresourceallocation1->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  pusch_timedomainresourceallocation1->startSymbolAndLength=38;
  asn1cSeqAdd(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,pusch_timedomainresourceallocation1);
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble=1;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant=-90;
 scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_GroupHopping=NR_PUCCH_ConfigCommon__pucch_GroupHopping_neither; 
 *scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId=40;
 *scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal=-90;
 scc->ssb_PositionsInBurst->present=NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap;
 *ssb_bitmap=0xff;
 scc->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing=mu_dl;
 if (mu_dl == 0)
   scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity=NR_TDD_UL_DL_Pattern__dl_UL_TransmissionPeriodicity_ms10;
 else
   scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity=NR_TDD_UL_DL_Pattern__dl_UL_TransmissionPeriodicity_ms5;
 scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots=7;
 scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols=6;
 scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots=2;
 scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols=4;
 scc->tdd_UL_DL_ConfigurationCommon->pattern2->dl_UL_TransmissionPeriodicity=321;

 scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofDownlinkSlots=-1;
 scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofDownlinkSymbols=-1;
 scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofUplinkSlots=-1;
 scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofUplinkSymbols=-1;
 scc->ss_PBCH_BlockPower=20;
 *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing=-1;
}


void fix_scc(NR_ServingCellConfigCommon_t *scc,uint64_t ssbmap) {

  int ssbmaplen = (int)scc->ssb_PositionsInBurst->present;
  uint8_t curr_bit;

  AssertFatal(ssbmaplen==NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_shortBitmap ||
              ssbmaplen==NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap ||
              ssbmaplen==NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_longBitmap, "illegal ssbmaplen %d\n",ssbmaplen);

  // changing endianicity of ssbmap and filling the ssb_PositionsInBurst buffers
  if(ssbmaplen==NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_shortBitmap){
    scc->ssb_PositionsInBurst->choice.shortBitmap.size = 1;
    scc->ssb_PositionsInBurst->choice.shortBitmap.bits_unused = 4;
    scc->ssb_PositionsInBurst->choice.shortBitmap.buf = CALLOC(1,1);
    scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0] = 0;
    for (int i=0; i<8; i++) {
      if (i<scc->ssb_PositionsInBurst->choice.shortBitmap.bits_unused)
        curr_bit = 0;
      else
        curr_bit = (ssbmap>>(7-i))&0x01;
      scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0] |= curr_bit<<i;   
    }
  }else if(ssbmaplen==NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap){
    scc->ssb_PositionsInBurst->choice.mediumBitmap.size = 1;
    scc->ssb_PositionsInBurst->choice.mediumBitmap.bits_unused = 0;
    scc->ssb_PositionsInBurst->choice.mediumBitmap.buf = CALLOC(1,1);
    scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0] = 0;
    for (int i=0; i<8; i++)
      scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0] |= (((ssbmap>>(7-i))&0x01)<<i); 
  }else {
    scc->ssb_PositionsInBurst->choice.longBitmap.size = 8;
    scc->ssb_PositionsInBurst->choice.longBitmap.bits_unused = 0;
    scc->ssb_PositionsInBurst->choice.longBitmap.buf = CALLOC(1,8);
    for (int j=0; j<8; j++) {
       scc->ssb_PositionsInBurst->choice.longBitmap.buf[j] = 0;
       curr_bit = (ssbmap>>(j<<3))&(0xff);
       for (int i=0; i<8; i++)
         scc->ssb_PositionsInBurst->choice.longBitmap.buf[j] |= (((curr_bit>>(7-i))&0x01)<<i);
    }
  }

  // fix SS0 and Coreset0
  NR_PDCCH_ConfigCommon_t *pdcch_cc = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup;
  if((int)*pdcch_cc->searchSpaceZero == -1) {
    free(pdcch_cc->searchSpaceZero);
    pdcch_cc->searchSpaceZero = NULL;
  }
  if((int)*pdcch_cc->controlResourceSetZero == -1) {
    free(pdcch_cc->controlResourceSetZero);
    pdcch_cc->controlResourceSetZero = NULL;
  }

  // fix UL absolute frequency
  if ((int)*scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA==-1) {
     free(scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA);
     scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA = NULL;
  }

  // default value for msg3 precoder is NULL (0 means enabled)
  if (*scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder!=0)
    scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder = NULL;

  frame_type_t frame_type = get_frame_type((int)*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);

  // prepare DL Allocation lists
  nr_rrc_config_dl_tda(scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList,
                       frame_type, scc->tdd_UL_DL_ConfigurationCommon,
                       scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);

  if (frame_type == FDD) {
    ASN_STRUCT_FREE(asn_DEF_NR_TDD_UL_DL_ConfigCommon, scc->tdd_UL_DL_ConfigurationCommon);
    scc->tdd_UL_DL_ConfigurationCommon = NULL;
  } else { // TDD
    if (scc->tdd_UL_DL_ConfigurationCommon->pattern2->dl_UL_TransmissionPeriodicity > 320 ) {
      free(scc->tdd_UL_DL_ConfigurationCommon->pattern2);
      scc->tdd_UL_DL_ConfigurationCommon->pattern2 = NULL;
    }

  }

  if ((int)*scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing == -1) {
    free(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing);
    scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing=NULL;
  }

  if ((int)*scc->n_TimingAdvanceOffset == -1) {
    free(scc->n_TimingAdvanceOffset);
    scc->n_TimingAdvanceOffset = NULL;
  }

  // check pucch_ResourceConfig
  AssertFatal(*scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon < 2,
	      "pucch_ResourceConfig should be 0 or 1 for now\n");
}

/* Function to allocate dedicated serving cell config strutures */
void prepare_scd(NR_ServingCellConfig_t *scd) {
  // Allocate downlink structures
  scd->downlinkBWP_ToAddModList = CALLOC(1, sizeof(*scd->downlinkBWP_ToAddModList));
  scd->uplinkConfig = CALLOC(1, sizeof(*scd->uplinkConfig));
  scd->uplinkConfig->uplinkBWP_ToAddModList = CALLOC(1, sizeof(*scd->uplinkConfig->uplinkBWP_ToAddModList));
  scd->bwp_InactivityTimer = CALLOC(1, sizeof(*scd->bwp_InactivityTimer));
  scd->uplinkConfig->firstActiveUplinkBWP_Id  = CALLOC(1, sizeof(*scd->uplinkConfig->firstActiveUplinkBWP_Id));
  scd->firstActiveDownlinkBWP_Id = CALLOC(1, sizeof(*scd->firstActiveDownlinkBWP_Id));
  *scd->firstActiveDownlinkBWP_Id = 1;
  *scd->uplinkConfig->firstActiveUplinkBWP_Id = 1;
  scd->defaultDownlinkBWP_Id = CALLOC(1, sizeof(*scd->defaultDownlinkBWP_Id));
  *scd->defaultDownlinkBWP_Id = 0;

  for (int j = 0; j < NR_MAX_NUM_BWP; j++) {

    // Downlink bandwidth part
    NR_BWP_Downlink_t *bwp = calloc(1, sizeof(*bwp));
    bwp->bwp_Id = j+1;

    // Allocate downlink dedicated bandwidth part and PDSCH structures
    bwp->bwp_Common = calloc(1, sizeof(*bwp->bwp_Common));
    bwp->bwp_Common->pdcch_ConfigCommon = calloc(1, sizeof(*bwp->bwp_Common->pdcch_ConfigCommon));
    bwp->bwp_Common->pdsch_ConfigCommon = calloc(1, sizeof(*bwp->bwp_Common->pdsch_ConfigCommon));
    bwp->bwp_Dedicated = calloc(1, sizeof(*bwp->bwp_Dedicated));
    bwp->bwp_Dedicated->pdsch_Config = calloc(1, sizeof(*bwp->bwp_Dedicated->pdsch_Config));
    bwp->bwp_Dedicated->pdsch_Config->present = NR_SetupRelease_PDSCH_Config_PR_setup;
    bwp->bwp_Dedicated->pdsch_Config->choice.setup = calloc(1, sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA = calloc(1, sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA));
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->present = NR_SetupRelease_DMRS_DownlinkConfig_PR_setup;

    // Allocate DL DMRS and PTRS configuration
    bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup = calloc(1, sizeof(*bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup));
    NR_DMRS_DownlinkConfig_t *NR_DMRS_DownlinkCfg = bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup;
    NR_DMRS_DownlinkCfg->phaseTrackingRS=CALLOC(1, sizeof(*NR_DMRS_DownlinkCfg->phaseTrackingRS));
    NR_DMRS_DownlinkCfg->phaseTrackingRS->present = NR_SetupRelease_PTRS_DownlinkConfig_PR_setup;
    NR_DMRS_DownlinkCfg->phaseTrackingRS->choice.setup = CALLOC(1, sizeof(*NR_DMRS_DownlinkCfg->phaseTrackingRS->choice.setup));
    NR_PTRS_DownlinkConfig_t *NR_PTRS_DownlinkCfg = NR_DMRS_DownlinkCfg->phaseTrackingRS->choice.setup;
    NR_PTRS_DownlinkCfg->frequencyDensity = CALLOC(1, sizeof(*NR_PTRS_DownlinkCfg->frequencyDensity));
    long *dl_rbs = CALLOC(2, sizeof(long));
    for (int i=0;i<2;i++) {
      asn1cSeqAdd(&NR_PTRS_DownlinkCfg->frequencyDensity->list, &dl_rbs[i]);
    }
    NR_PTRS_DownlinkCfg->timeDensity = CALLOC(1, sizeof(*NR_PTRS_DownlinkCfg->timeDensity));
    long *dl_mcs = CALLOC(3, sizeof(long));
    for (int i=0;i<3;i++) {
      asn1cSeqAdd(&NR_PTRS_DownlinkCfg->timeDensity->list, &dl_mcs[i]);
    }
    NR_PTRS_DownlinkCfg->epre_Ratio = CALLOC(1, sizeof(*NR_PTRS_DownlinkCfg->epre_Ratio));
    NR_PTRS_DownlinkCfg->resourceElementOffset = CALLOC(1, sizeof(*NR_PTRS_DownlinkCfg->resourceElementOffset));
    *NR_PTRS_DownlinkCfg->resourceElementOffset = 0;
    asn1cSeqAdd(&scd->downlinkBWP_ToAddModList->list,bwp);

    // Allocate uplink structures

    NR_PUSCH_Config_t *pusch_Config = CALLOC(1, sizeof(*pusch_Config));

    // Allocate UL DMRS and PTRS structures
    pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB = CALLOC(1, sizeof(*pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB));
    pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->present = NR_SetupRelease_DMRS_UplinkConfig_PR_setup;
    pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup = CALLOC(1, sizeof(*pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup));
    NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup;
    NR_DMRS_UplinkConfig->phaseTrackingRS = CALLOC(1, sizeof(*NR_DMRS_UplinkConfig->phaseTrackingRS));
    NR_DMRS_UplinkConfig->phaseTrackingRS->present = NR_SetupRelease_PTRS_UplinkConfig_PR_setup;
    NR_DMRS_UplinkConfig->phaseTrackingRS->choice.setup = CALLOC(1, sizeof(*NR_DMRS_UplinkConfig->phaseTrackingRS->choice.setup));
    NR_PTRS_UplinkConfig_t *NR_PTRS_UplinkConfig = NR_DMRS_UplinkConfig->phaseTrackingRS->choice.setup;
    NR_PTRS_UplinkConfig->transformPrecoderDisabled = CALLOC(1, sizeof(*NR_PTRS_UplinkConfig->transformPrecoderDisabled));
    NR_PTRS_UplinkConfig->transformPrecoderDisabled->frequencyDensity = CALLOC(1, sizeof(*NR_PTRS_UplinkConfig->transformPrecoderDisabled->frequencyDensity));
    long *n_rbs = CALLOC(2, sizeof(long));
    for (int i=0;i<2;i++) {
      asn1cSeqAdd(&NR_PTRS_UplinkConfig->transformPrecoderDisabled->frequencyDensity->list, &n_rbs[i]);
    }
    NR_PTRS_UplinkConfig->transformPrecoderDisabled->timeDensity = CALLOC(1, sizeof(*NR_PTRS_UplinkConfig->transformPrecoderDisabled->timeDensity));
    long *ptrs_mcs = CALLOC(3, sizeof(long));
    for (int i = 0; i < 3; i++) {
      asn1cSeqAdd(&NR_PTRS_UplinkConfig->transformPrecoderDisabled->timeDensity->list, &ptrs_mcs[i]);
    }
    NR_PTRS_UplinkConfig->transformPrecoderDisabled->resourceElementOffset = CALLOC(1, sizeof(*NR_PTRS_UplinkConfig->transformPrecoderDisabled->resourceElementOffset));
    *NR_PTRS_UplinkConfig->transformPrecoderDisabled->resourceElementOffset = 0;

    // UL bandwidth part
    NR_BWP_Uplink_t *ubwp = CALLOC(1, sizeof(*ubwp));
    ubwp->bwp_Id = j+1;
    ubwp->bwp_Common = CALLOC(1, sizeof(*ubwp->bwp_Common));
    ubwp->bwp_Dedicated = CALLOC(1, sizeof(*ubwp->bwp_Dedicated));

    ubwp->bwp_Dedicated->pusch_Config = CALLOC(1, sizeof(*ubwp->bwp_Dedicated->pusch_Config));
    ubwp->bwp_Dedicated->pusch_Config->present = NR_SetupRelease_PUSCH_Config_PR_setup;
    ubwp->bwp_Dedicated->pusch_Config->choice.setup = pusch_Config;

    asn1cSeqAdd(&scd->uplinkConfig->uplinkBWP_ToAddModList->list,ubwp);
  }
}

/* This function checks dedicated serving cell configuration and performs fixes as needed */
void fix_scd(NR_ServingCellConfig_t *scd) {

  // Remove unused BWPs
  int b = 0;
  while (b<scd->downlinkBWP_ToAddModList->list.count) {
    if (scd->downlinkBWP_ToAddModList->list.array[b]->bwp_Common->genericParameters.locationAndBandwidth == 0) {
      asn_sequence_del(&scd->downlinkBWP_ToAddModList->list,b,1);
    } else {
      b++;
    }
  }

  b = 0;
  while (b<scd->uplinkConfig->uplinkBWP_ToAddModList->list.count) {
    if (scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[b]->bwp_Common->genericParameters.locationAndBandwidth == 0) {
      asn_sequence_del(&scd->uplinkConfig->uplinkBWP_ToAddModList->list,b,1);
    } else {
      b++;
    }
  }

  // Check for DL PTRS parameters validity
  for (int bwp_i = 0 ; bwp_i<scd->downlinkBWP_ToAddModList->list.count; bwp_i++) {

    NR_DMRS_DownlinkConfig_t *dmrs_dl_config = scd->downlinkBWP_ToAddModList->list.array[bwp_i]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup;
    
    if (dmrs_dl_config->phaseTrackingRS) {
      // If any of the frequencyDensity values are not set or are out of bounds, PTRS is assumed to be not present
      for (int i = dmrs_dl_config->phaseTrackingRS->choice.setup->frequencyDensity->list.count - 1; i >= 0; i--) {
        if ((*dmrs_dl_config->phaseTrackingRS->choice.setup->frequencyDensity->list.array[i] < 1)
            || (*dmrs_dl_config->phaseTrackingRS->choice.setup->frequencyDensity->list.array[i] > 276)) {
          LOG_I(GNB_APP, "DL PTRS frequencyDensity %d not set. Assuming PTRS not present! \n", i);
          free(dmrs_dl_config->phaseTrackingRS);
          dmrs_dl_config->phaseTrackingRS = NULL;
          break;
        }
      }
    }

    if (dmrs_dl_config->phaseTrackingRS) {
      // If any of the timeDensity values are not set or are out of bounds, PTRS is assumed to be not present
      for (int i = dmrs_dl_config->phaseTrackingRS->choice.setup->timeDensity->list.count - 1; i >= 0; i--) {
        if ((*dmrs_dl_config->phaseTrackingRS->choice.setup->timeDensity->list.array[i] < 0)
            || (*dmrs_dl_config->phaseTrackingRS->choice.setup->timeDensity->list.array[i] > 29)) {
          LOG_I(GNB_APP, "DL PTRS timeDensity %d not set. Assuming PTRS not present! \n", i);
          free(dmrs_dl_config->phaseTrackingRS);
          dmrs_dl_config->phaseTrackingRS = NULL;
          break;
        }
      }
    }

    if (dmrs_dl_config->phaseTrackingRS) {
      if (*dmrs_dl_config->phaseTrackingRS->choice.setup->resourceElementOffset > 2) {
        LOG_I(GNB_APP, "Freeing DL PTRS resourceElementOffset \n");
        free(dmrs_dl_config->phaseTrackingRS->choice.setup->resourceElementOffset);
        dmrs_dl_config->phaseTrackingRS->choice.setup->resourceElementOffset = NULL;
      }
      if (*dmrs_dl_config->phaseTrackingRS->choice.setup->epre_Ratio > 1) {
        LOG_I(GNB_APP, "Freeing DL PTRS epre_Ratio \n");
        free(dmrs_dl_config->phaseTrackingRS->choice.setup->epre_Ratio);
        dmrs_dl_config->phaseTrackingRS->choice.setup->epre_Ratio = NULL;
      }
    }
  }

  // Check for UL PTRS parameters validity
  for (int bwp_i = 0 ; bwp_i<scd->uplinkConfig->uplinkBWP_ToAddModList->list.count; bwp_i++) {

    NR_DMRS_UplinkConfig_t *dmrs_ul_config = scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_i]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup;
    
    if (dmrs_ul_config->phaseTrackingRS) {
      // If any of the frequencyDensity values are not set or are out of bounds, PTRS is assumed to be not present
      for (int i = dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.count-1; i >= 0; i--) {
        if ((*dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.array[i] < 1)
            || (*dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.array[i] > 276)) {
          LOG_I(GNB_APP, "UL PTRS frequencyDensity %d not set. Assuming PTRS not present! \n", i);
          free(dmrs_ul_config->phaseTrackingRS);
          dmrs_ul_config->phaseTrackingRS = NULL;
          break;
        }
      }
    }

    if (dmrs_ul_config->phaseTrackingRS) {
      // If any of the timeDensity values are not set or are out of bounds, PTRS is assumed to be not present
      for (int i = dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.count-1; i >= 0; i--) {
        if ((*dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[i] < 0)
            || (*dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[i] > 29)) {
          LOG_I(GNB_APP, "UL PTRS timeDensity %d not set. Assuming PTRS not present! \n", i);
          free(dmrs_ul_config->phaseTrackingRS);
          dmrs_ul_config->phaseTrackingRS = NULL;
          break;
        }
      }
    }

    if (dmrs_ul_config->phaseTrackingRS) {
      // Check for UL PTRS parameters validity
      if (*dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->resourceElementOffset > 2) {
        LOG_I(GNB_APP, "Freeing UL PTRS resourceElementOffset \n");
        free(dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->resourceElementOffset);
        dmrs_ul_config->phaseTrackingRS->choice.setup->transformPrecoderDisabled->resourceElementOffset = NULL;
      }
    }

  }
}

static void verify_gnb_param_notset(paramdef_t *params, int paramidx, const char *paramname)
{
  char aprefix[MAX_OPTNAME_SIZE * 2 + 8];
  sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);
  AssertFatal(!config_isparamset(params, paramidx),
              "Option \"%s." GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON
              ".%s\" is not allowed in this config, please remove it\n",
              aprefix,
              paramname);
}
static void verify_section_notset(configmodule_interface_t *cfg, char *aprefix, const char *secname)
{
  paramlist_def_t pl = {0};
  strncpy(pl.listname, secname, sizeof(pl.listname) - 1);
  config_getlist(cfg, &pl, NULL, 0, aprefix);
  AssertFatal(pl.numelt == 0, "Section \"%s.%s\" not allowed in this config, please remove it\n", aprefix ? aprefix : "", secname);
}
void RCconfig_verify(configmodule_interface_t *cfg, ngran_node_t node_type)
{
  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  config_get(cfg, GNBSParams, sizeof(GNBSParams) / sizeof(paramdef_t), NULL);
  int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal(num_gnbs == 1, "need to have a " GNB_CONFIG_STRING_GNB_LIST " section, but %d found\n", num_gnbs);
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST, NULL, 0};
  paramdef_t GNBParams[] = GNBPARAMS_DESC;
  config_getlist(cfg, &GNBParamList, GNBParams, sizeof(GNBParams) / sizeof(paramdef_t), NULL);
  paramdef_t *gnbp = GNBParamList.paramarray[0];

  if (NODE_IS_CU(node_type)) {
    // verify that there is no SCC and radio config in the case of CU
    verify_section_notset(cfg, GNB_CONFIG_STRING_GNB_LIST ".[0]", GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON);

    verify_gnb_param_notset(gnbp, GNB_PDSCH_ANTENNAPORTS_N1_IDX, GNB_CONFIG_STRING_PDSCHANTENNAPORTS_N1);
    verify_gnb_param_notset(gnbp, GNB_PDSCH_ANTENNAPORTS_N2_IDX, GNB_CONFIG_STRING_PDSCHANTENNAPORTS_N2);
    verify_gnb_param_notset(gnbp, GNB_PDSCH_ANTENNAPORTS_XP_IDX, GNB_CONFIG_STRING_PDSCHANTENNAPORTS_XP);
    verify_gnb_param_notset(gnbp, GNB_PUSCH_ANTENNAPORTS_IDX, GNB_CONFIG_STRING_PUSCHANTENNAPORTS);
    verify_gnb_param_notset(gnbp, GNB_MINRXTXTIME_IDX, GNB_CONFIG_STRING_MINRXTXTIME);
    verify_gnb_param_notset(gnbp, GNB_SIB1_TDA_IDX, GNB_CONFIG_STRING_SIB1TDA);
    verify_gnb_param_notset(gnbp, GNB_DO_CSIRS_IDX, GNB_CONFIG_STRING_DOCSIRS);
    verify_gnb_param_notset(gnbp, GNB_DO_SRS_IDX, GNB_CONFIG_STRING_DOSRS);
    verify_gnb_param_notset(gnbp, GNB_FORCE256QAMOFF_IDX, GNB_CONFIG_STRING_FORCE256QAMOFF);

    // check for some general sections
    verify_section_notset(cfg, NULL, CONFIG_STRING_L1_LIST);
    verify_section_notset(cfg, NULL, CONFIG_STRING_RU_LIST);
    verify_section_notset(cfg, NULL, CONFIG_STRING_MACRLC_LIST);
  } else if (NODE_IS_DU(node_type)) {
    // verify that there is no bearer config
    verify_gnb_param_notset(gnbp, GNB_ENABLE_SDAP_IDX, GNB_CONFIG_STRING_ENABLE_SDAP);
    verify_gnb_param_notset(gnbp, GNB_DRBS, GNB_CONFIG_STRING_DRBS);

    verify_section_notset(cfg, GNB_CONFIG_STRING_GNB_LIST ".", GNB_CONFIG_STRING_AMF_IP_ADDRESS);
    verify_section_notset(cfg, NULL, CONFIG_STRING_SECURITY);
  } // else nothing to be checked

  /* other possible verifications: PNF, VNF, CU-CP, CU-UP, ...? */
}

void RCconfig_nr_prs(void)
{
  uint16_t  j = 0, k = 0;
  prs_config_t *prs_config = NULL;
  char str[7][100] = {0};

  paramdef_t PRS_Params[] = PRS_PARAMS_DESC;
  paramlist_def_t PRS_ParamList = {CONFIG_STRING_PRS_CONFIG,NULL,0};
  if (RC.gNB == NULL) {
    RC.gNB                       = (PHY_VARS_gNB **)malloc((1+NUMBER_OF_gNB_MAX)*sizeof(PHY_VARS_gNB*));
    LOG_I(NR_PHY,"RC.gNB = %p\n",RC.gNB);
    memset(RC.gNB,0,(1+NUMBER_OF_gNB_MAX)*sizeof(PHY_VARS_gNB*));
  }

  config_getlist(config_get_if(), &PRS_ParamList, PRS_Params, sizeofArray(PRS_Params), NULL);

  if (PRS_ParamList.numelt > 0) {
    for (j = 0; j < RC.nb_nr_L1_inst; j++) {

      if (RC.gNB[j] == NULL) {
        RC.gNB[j]                       = (PHY_VARS_gNB *)malloc(sizeof(PHY_VARS_gNB));
        LOG_I(NR_PHY,"RC.gNB[%d] = %p\n",j,RC.gNB[j]);
        memset(RC.gNB[j],0,sizeof(PHY_VARS_gNB));
	      RC.gNB[j]->Mod_id  = j;
      }

      RC.gNB[j]->prs_vars.NumPRSResources = *(PRS_ParamList.paramarray[j][NUM_PRS_RESOURCES].uptr);
      for (k = 0; k < RC.gNB[j]->prs_vars.NumPRSResources; k++)
      {
        prs_config = &RC.gNB[j]->prs_vars.prs_cfg[k];
        prs_config->PRSResourceSetPeriod[0]  = PRS_ParamList.paramarray[j][PRS_RESOURCE_SET_PERIOD_LIST].uptr[0];
        prs_config->PRSResourceSetPeriod[1]  = PRS_ParamList.paramarray[j][PRS_RESOURCE_SET_PERIOD_LIST].uptr[1];
        // per PRS resources parameters
        prs_config->SymbolStart              = PRS_ParamList.paramarray[j][PRS_SYMBOL_START_LIST].uptr[k];
        prs_config->NumPRSSymbols            = PRS_ParamList.paramarray[j][PRS_NUM_SYMBOLS_LIST].uptr[k];
        prs_config->REOffset                 = PRS_ParamList.paramarray[j][PRS_RE_OFFSET_LIST].uptr[k];
        prs_config->PRSResourceOffset        = PRS_ParamList.paramarray[j][PRS_RESOURCE_OFFSET_LIST].uptr[k];
        prs_config->NPRSID                   = PRS_ParamList.paramarray[j][PRS_ID_LIST].uptr[k];
        // Common parameters to all PRS resources
        prs_config->NumRB                    = *(PRS_ParamList.paramarray[j][PRS_NUM_RB].uptr);
        prs_config->RBOffset                 = *(PRS_ParamList.paramarray[j][PRS_RB_OFFSET].uptr);
        prs_config->CombSize                 = *(PRS_ParamList.paramarray[j][PRS_COMB_SIZE].uptr);
        prs_config->PRSResourceRepetition    = *(PRS_ParamList.paramarray[j][PRS_RESOURCE_REPETITION].uptr);
        prs_config->PRSResourceTimeGap       = *(PRS_ParamList.paramarray[j][PRS_RESOURCE_TIME_GAP].uptr);
        prs_config->MutingBitRepetition      = *(PRS_ParamList.paramarray[j][PRS_MUTING_BIT_REPETITION].uptr);
        for (int l = 0; l < PRS_ParamList.paramarray[j][PRS_MUTING_PATTERN1_LIST].numelt; l++)
        {
          prs_config->MutingPattern1[l]      = PRS_ParamList.paramarray[j][PRS_MUTING_PATTERN1_LIST].uptr[l];
          if (k == 0) // print only for 0th resource 
            snprintf(str[5]+strlen(str[5]),sizeof(str[5])-strlen(str[5]),"%d, ",prs_config->MutingPattern1[l]);
        }
        for (int l = 0; l < PRS_ParamList.paramarray[j][PRS_MUTING_PATTERN2_LIST].numelt; l++)
        {
          prs_config->MutingPattern2[l]      = PRS_ParamList.paramarray[j][PRS_MUTING_PATTERN2_LIST].uptr[l];
          if (k == 0) // print only for 0th resource
            snprintf(str[6]+strlen(str[6]),sizeof(str[6])-strlen(str[6]),"%d, ",prs_config->MutingPattern2[l]);
        }

        // print to buffer
        snprintf(str[0]+strlen(str[0]),sizeof(str[0])-strlen(str[0]),"%d, ",prs_config->SymbolStart);
        snprintf(str[1]+strlen(str[1]),sizeof(str[1])-strlen(str[1]),"%d, ",prs_config->NumPRSSymbols);
        snprintf(str[2]+strlen(str[2]),sizeof(str[2])-strlen(str[2]),"%d, ",prs_config->REOffset);
        snprintf(str[3]+strlen(str[3]),sizeof(str[3])-strlen(str[3]),"%d, ",prs_config->PRSResourceOffset);
        snprintf(str[4]+strlen(str[4]),sizeof(str[4])-strlen(str[4]),"%d, ",prs_config->NPRSID);
      } // for k

      prs_config = &RC.gNB[j]->prs_vars.prs_cfg[0];
      LOG_I(PHY, "-----------------------------------------\n");
      LOG_I(PHY, "PRS Config for gNB_id %d @ %p\n", j, prs_config);
      LOG_I(PHY, "-----------------------------------------\n");
      LOG_I(PHY, "NumPRSResources \t%d\n", RC.gNB[j]->prs_vars.NumPRSResources);
      LOG_I(PHY, "PRSResourceSetPeriod \t[%d, %d]\n", prs_config->PRSResourceSetPeriod[0], prs_config->PRSResourceSetPeriod[1]);
      LOG_I(PHY, "NumRB \t\t\t%d\n", prs_config->NumRB);
      LOG_I(PHY, "RBOffset \t\t%d\n", prs_config->RBOffset);
      LOG_I(PHY, "CombSize \t\t%d\n", prs_config->CombSize);
      LOG_I(PHY, "PRSResourceRepetition \t%d\n", prs_config->PRSResourceRepetition);
      LOG_I(PHY, "PRSResourceTimeGap \t%d\n", prs_config->PRSResourceTimeGap);
      LOG_I(PHY, "MutingBitRepetition \t%d\n", prs_config->MutingBitRepetition);
      LOG_I(PHY, "SymbolStart \t\t[%s\b\b]\n", str[0]);
      LOG_I(PHY, "NumPRSSymbols \t\t[%s\b\b]\n", str[1]);
      LOG_I(PHY, "REOffset \t\t[%s\b\b]\n", str[2]);
      LOG_I(PHY, "PRSResourceOffset \t[%s\b\b]\n", str[3]);
      LOG_I(PHY, "NPRS_ID \t\t[%s\b\b]\n", str[4]);
      LOG_I(PHY, "MutingPattern1 \t\t[%s\b\b]\n", str[5]);
      LOG_I(PHY, "MutingPattern2 \t\t[%s\b\b]\n", str[6]);
      LOG_I(PHY, "-----------------------------------------\n");
    } // for j
  }
  else
  {
    LOG_I(PHY,"No " CONFIG_STRING_PRS_CONFIG " configuration found..!!\n");
  }
}

void RCconfig_NR_L1(void)
{
  int j = 0;
  if (RC.gNB == NULL) {
    RC.gNB = (PHY_VARS_gNB **)malloc((1 + NUMBER_OF_gNB_MAX) * sizeof(PHY_VARS_gNB *));
    LOG_I(NR_PHY, "RC.gNB = %p\n", RC.gNB);
    memset(RC.gNB, 0, (1 + NUMBER_OF_gNB_MAX) * sizeof(PHY_VARS_gNB *));

    if (RC.gNB[j] == NULL) {
      RC.gNB[j] = calloc(1, sizeof(PHY_VARS_gNB));
    }
  }
  if (NFAPI_MODE != NFAPI_MODE_PNF) {
    paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
    ////////// Identification parameters
    paramdef_t GNBParams[] = GNBPARAMS_DESC;

    paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST, NULL, 0};

    config_get(config_get_if(), GNBSParams, sizeofArray(GNBSParams), NULL);
    int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
    AssertFatal(num_gnbs > 0, "Failed to parse config file no gnbs %s \n", GNB_CONFIG_STRING_ACTIVE_GNBS);

    config_getlist(config_get_if(), &GNBParamList, GNBParams, sizeofArray(GNBParams), NULL);
    int N1 = *GNBParamList.paramarray[0][GNB_PDSCH_ANTENNAPORTS_N1_IDX].iptr;
    int N2 = *GNBParamList.paramarray[0][GNB_PDSCH_ANTENNAPORTS_N2_IDX].iptr;
    int XP = *GNBParamList.paramarray[0][GNB_PDSCH_ANTENNAPORTS_XP_IDX].iptr;
    char *ulprbbl = *GNBParamList.paramarray[0][GNB_ULPRBBLACKLIST_IDX].strptr;
    if (ulprbbl)
      LOG_I(NR_PHY, "PRB blacklist %s\n", ulprbbl);
    char *save = NULL;
    char *pt = strtok_r(ulprbbl, ",", &save);
    int prbbl[275];
    int num_prbbl = 0;
    memset(prbbl, 0, 275 * sizeof(int));

    while (pt) {
      const int rb = atoi(pt);
      AssertFatal(rb < 275, "RB %d out of bounds (max 275)\n", rb);
      prbbl[rb] = 0x3FFF; // all symbols taken
      LOG_I(NR_PHY, "Blacklisting prb %d\n", atoi(pt));
      pt = strtok_r(NULL, ",", &save);
      num_prbbl++;
    }

    RC.gNB[j]->num_ulprbbl = num_prbbl;
    LOG_I(NR_PHY, "Copying %d blacklisted PRB to L1 context\n", num_prbbl);
    memcpy(RC.gNB[j]->ulprbbl, prbbl, 275 * sizeof(int));

    RC.gNB[j]->ap_N1 = N1;
    RC.gNB[j]->ap_N2 = N2;
    RC.gNB[j]->ap_XP = XP;
  }

  paramdef_t L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t L1_ParamList = {CONFIG_STRING_L1_LIST, NULL, 0};

  config_getlist(config_get_if(), &L1_ParamList, L1_Params, sizeofArray(L1_Params), NULL);

  if (L1_ParamList.numelt > 0) {
    for (j = 0; j < RC.nb_nr_L1_inst; j++) {
      if (RC.gNB[j] == NULL) {
        RC.gNB[j] = (PHY_VARS_gNB *)malloc(sizeof(PHY_VARS_gNB));
        LOG_I(NR_PHY, "RC.gNB[%d] = %p\n", j, RC.gNB[j]);
        memset(RC.gNB[j], 0, sizeof(PHY_VARS_gNB));
        RC.gNB[j]->Mod_id = j;
      }
      AssertFatal(*L1_ParamList.paramarray[j][L1_THREAD_POOL_SIZE].uptr == 2022, "thread_pool_size removed, please use --thread-pool\n");
      RC.gNB[j]->ofdm_offset_divisor = *(L1_ParamList.paramarray[j][L1_OFDM_OFFSET_DIVISOR].uptr);
      RC.gNB[j]->pucch0_thres = *(L1_ParamList.paramarray[j][L1_PUCCH0_DTX_THRESHOLD].uptr);
      RC.gNB[j]->prach_thres = *(L1_ParamList.paramarray[j][L1_PRACH_DTX_THRESHOLD].uptr);
      RC.gNB[j]->pusch_thres = *(L1_ParamList.paramarray[j][L1_PUSCH_DTX_THRESHOLD].uptr);
      RC.gNB[j]->srs_thres = *(L1_ParamList.paramarray[j][L1_SRS_DTX_THRESHOLD].uptr);
      RC.gNB[j]->max_ldpc_iterations = *(L1_ParamList.paramarray[j][L1_MAX_LDPC_ITERATIONS].uptr);
      RC.gNB[j]->L1_rx_thread_core = *(L1_ParamList.paramarray[j][L1_RX_THREAD_CORE].iptr);
      RC.gNB[j]->L1_tx_thread_core = *(L1_ParamList.paramarray[j][L1_TX_THREAD_CORE].iptr);
      LOG_I(PHY,"L1_RX_THREAD_CORE %d (%d)\n",*(L1_ParamList.paramarray[j][L1_RX_THREAD_CORE].iptr),L1_RX_THREAD_CORE);
      RC.gNB[j]->TX_AMP = (int16_t)(32767.0 / pow(10.0, .05 * (double)(*L1_ParamList.paramarray[j][L1_TX_AMP_BACKOFF_dB].uptr)));
      RC.gNB[j]->phase_comp = *L1_ParamList.paramarray[j][L1_PHASE_COMP].uptr;
      LOG_I(PHY, "TX_AMP = %d (-%d dBFS)\n", RC.gNB[j]->TX_AMP, *L1_ParamList.paramarray[j][L1_TX_AMP_BACKOFF_dB].uptr);
      AssertFatal(RC.gNB[j]->TX_AMP > 300, "TX_AMP is too small, must be larger than 300 (is %d)\n", RC.gNB[j]->TX_AMP);
      if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_mac") == 0) {
        // sf_ahead = 2; // Need 4 subframe gap between RX and TX
      } else if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        RC.gNB[j]->eth_params_n.local_if_name = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
        RC.gNB[j]->eth_params_n.my_addr = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
        RC.gNB[j]->eth_params_n.remote_addr = strdup(*(L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
        RC.gNB[j]->eth_params_n.my_portc = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
        RC.gNB[j]->eth_params_n.remote_portc = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
        RC.gNB[j]->eth_params_n.my_portd = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
        RC.gNB[j]->eth_params_n.remote_portd = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
        RC.gNB[j]->eth_params_n.transp_preference = ETH_UDP_MODE;

        // sf_ahead = 2; // Cannot cope with 4 subframes betweem RX and TX - set it to 2

        RC.nb_nr_macrlc_inst = 1; // This is used by mac_top_init_gNB()

        // This is used by init_gNB_afterRU()
        RC.nb_nr_CC = (int *)malloc((1 + RC.nb_nr_inst) * sizeof(int));
        RC.nb_nr_CC[0] = 1;

        LOG_I(PHY, "%s() NFAPI PNF mode - RC.nb_nr_inst=1 this is because phy_init_RU() uses that to index and not RC.num_gNB - why the 2 similar variables?\n", __FUNCTION__);
        LOG_I(PHY, "%s() NFAPI PNF mode - RC.nb_nr_CC[0]=%d for init_gNB_afterRU()\n", __FUNCTION__, RC.nb_nr_CC[0]);
        LOG_I(PHY, "%s() NFAPI PNF mode - RC.nb_nr_macrlc_inst:%d because used by mac_top_init_gNB()\n", __FUNCTION__, RC.nb_nr_macrlc_inst);

        configure_nr_nfapi_pnf(RC.gNB[j]->eth_params_n.remote_addr,
                               RC.gNB[j]->eth_params_n.remote_portc,
                               RC.gNB[j]->eth_params_n.my_addr,
                               RC.gNB[j]->eth_params_n.my_portd,
                               RC.gNB[j]->eth_params_n.remote_portd);
      } else { // other midhaul
      }
    } // for (j = 0; j < RC.nb_nr_L1_inst; j++)
    printf("Initializing northbound interface for L1\n");
    l1_north_init_gNB();
  } else {
    LOG_I(PHY, "No " CONFIG_STRING_L1_LIST " configuration found");

    // need to create some structures for VNF

    j = 0;

    if (RC.gNB[j] == NULL) {
      RC.gNB[j] = (PHY_VARS_gNB *)malloc(sizeof(PHY_VARS_gNB));
      memset((void *)RC.gNB[j], 0, sizeof(PHY_VARS_gNB));
      LOG_I(PHY, "RC.gNB[%d] = %p\n", j, RC.gNB[j]);
      RC.gNB[j]->Mod_id = j;
    }
  }
}

static void check_ssb_raster(uint64_t freq, int band, int scs);

static NR_ServingCellConfigCommon_t *get_scc_config(configmodule_interface_t *cfg, int minRXTXTIME)
{
  NR_ServingCellConfigCommon_t *scc = calloc(1,sizeof(*scc));
  uint64_t ssb_bitmap=0xff;
  prepare_scc(scc);
  paramdef_t SCCsParams[] = SCCPARAMS_DESC(scc);
  paramlist_def_t SCCsParamList = {GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON, NULL, 0};

  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);
  config_getlist(cfg, &SCCsParamList, NULL, 0, aprefix);
  if (SCCsParamList.numelt > 0) {
    sprintf(aprefix, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST,0,GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON, 0);
    config_get(cfg, SCCsParams, sizeofArray(SCCsParams), aprefix);
    struct NR_FrequencyInfoDL *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
    LOG_I(RRC,
          "Read in ServingCellConfigCommon (PhysCellId %d, ABSFREQSSB %d, DLBand %d, ABSFREQPOINTA %d, DLBW "
          "%d,RACH_TargetReceivedPower %d\n",
          (int)*scc->physCellId,
          (int)*frequencyInfoDL->absoluteFrequencySSB,
          (int)*frequencyInfoDL->frequencyBandList.list.array[0],
          (int)frequencyInfoDL->absoluteFrequencyPointA,
          (int)frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
          (int)scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric
              .preambleReceivedTargetPower);
    // SSB of the PCell is always on the sync raster
    uint64_t ssb_freq = from_nrarfcn(*frequencyInfoDL->frequencyBandList.list.array[0],
                                     *scc->ssbSubcarrierSpacing,
                                     *frequencyInfoDL->absoluteFrequencySSB);
    LOG_I(RRC, "absoluteFrequencySSB %ld corresponds to %lu Hz\n", *frequencyInfoDL->absoluteFrequencySSB, ssb_freq);
    if (get_softmodem_params()->sa)
      check_ssb_raster(ssb_freq, *frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);
    fix_scc(scc, ssb_bitmap);
  }
  nr_rrc_config_ul_tda(scc, minRXTXTIME);

  // the gNB uses the servingCellConfigCommon everywhere, even when it should use the servingCellConfigCommonSIB.
  // previously (before this commit), the following fields were indirectly populated through get_SIB1_NR().
  // since this might lead to memory problems (e.g., double frees), it has been moved here.
  // note that the "right solution" would be to not populate the servingCellConfigCommon here, and use
  // an "abstraction struct" that contains the corresponding values, from which SCC/SIB1/... is generated.
  NR_PDCCH_ConfigCommon_t *pcc = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup;
  AssertFatal(pcc != NULL && pcc->commonSearchSpaceList == NULL, "memory leak\n");
  pcc->commonSearchSpaceList = calloc_or_fail(1, sizeof(*pcc->commonSearchSpaceList));

  NR_SearchSpace_t *ss1 = rrc_searchspace_config(true, 1, 0);
  asn1cSeqAdd(&pcc->commonSearchSpaceList->list, ss1);
  NR_SearchSpace_t *ss2 = rrc_searchspace_config(true, 2, 0);
  asn1cSeqAdd(&pcc->commonSearchSpaceList->list, ss2);
  NR_SearchSpace_t *ss3 = rrc_searchspace_config(true, 3, 0);
  asn1cSeqAdd(&pcc->commonSearchSpaceList->list, ss3);

  asn1cCallocOne(pcc->searchSpaceSIB1,  0);
  asn1cCallocOne(pcc->ra_SearchSpace, 1);
  asn1cCallocOne(pcc->pagingSearchSpace, 2);
  asn1cCallocOne(pcc->searchSpaceOtherSystemInformation, 3);

  return scc;
}

static NR_ServingCellConfig_t *get_scd_config(configmodule_interface_t *cfg)
{
  NR_ServingCellConfig_t *scd = calloc(1, sizeof(*scd));
  prepare_scd(scd);
  paramdef_t SCDsParams[] = SCDPARAMS_DESC(scd);
  paramlist_def_t SCDsParamList = {GNB_CONFIG_STRING_SERVINGCELLCONFIGDEDICATED, NULL, 0};

  char aprefix[MAX_OPTNAME_SIZE * 2 + 8];
  snprintf(aprefix, sizeof(aprefix), "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);
  config_getlist(cfg, &SCDsParamList, NULL, 0, aprefix);
  if (SCDsParamList.numelt > 0) {
    sprintf(aprefix, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0, GNB_CONFIG_STRING_SERVINGCELLCONFIGDEDICATED, 0);
    config_get(cfg, SCDsParams, sizeof(SCDsParams) / sizeof(paramdef_t), aprefix);
    const NR_BWP_UplinkDedicated_t *bwp_Dedicated = scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated;
    const NR_PTRS_UplinkConfig_t *setup =
        bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup;
    LOG_I(RRC,
          "Read in ServingCellConfigDedicated UL (FreqDensity_0 %ld, FreqDensity_1 %ld, TimeDensity_0 %ld, TimeDensity_1 %ld, "
          "TimeDensity_2 %ld, RE offset %ld, First_active_BWP_ID %ld SCS %ld, LocationandBW %ld\n",
          *setup->transformPrecoderDisabled->frequencyDensity->list.array[0],
          *setup->transformPrecoderDisabled->frequencyDensity->list.array[1],
          *setup->transformPrecoderDisabled->timeDensity->list.array[0],
          *setup->transformPrecoderDisabled->timeDensity->list.array[1],
          *setup->transformPrecoderDisabled->timeDensity->list.array[2],
          *setup->transformPrecoderDisabled->resourceElementOffset,
          *scd->firstActiveDownlinkBWP_Id,
          scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Common->genericParameters.subcarrierSpacing,
          scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Common->genericParameters.locationAndBandwidth);
  }
  fix_scd(scd);

  return scd;
}

static int read_du_cell_info(configmodule_interface_t *cfg,
                             bool separate_du,
                             uint32_t *gnb_id,
                             uint64_t *gnb_du_id,
                             char **name,
                             f1ap_served_cell_info_t *info,
                             int max_cell_info)
{
  AssertFatal(max_cell_info == 1, "only one cell supported\n");
  memset(info, 0, sizeof(*info));

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};
  config_get(cfg, GNBSParams, sizeof(GNBSParams) / sizeof(paramdef_t), NULL);
  int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal(num_gnbs == 1, "cannot configure DU: required config section \"gNBs\" missing\n");

  // Output a list of all eNBs.
  config_getlist(cfg, &GNBParamList, GNBParams, sizeof(GNBParams) / sizeof(paramdef_t), NULL);

  // read the gNB-ID. The DU itself only needs the gNB-DU ID, but some (e.g.,
  // E2 agent) need the gNB-ID as well if it is running in a separate process
  AssertFatal(config_isparamset(GNBParamList.paramarray[0], GNB_GNB_ID_IDX), "%s is not defined in configuration file\n", GNB_CONFIG_STRING_GNB_ID);
  *gnb_id = *GNBParamList.paramarray[0][GNB_GNB_ID_IDX].uptr;

  AssertFatal(strcmp(GNBSParams[GNB_ACTIVE_GNBS_IDX].strlistptr[0], *GNBParamList.paramarray[0][GNB_GNB_NAME_IDX].strptr) == 0,
              "no active gNB found/mismatch of gNBs: %s vs %s\n",
              GNBSParams[GNB_ACTIVE_GNBS_IDX].strlistptr[0],
              *GNBParamList.paramarray[0][GNB_GNB_NAME_IDX].strptr);

  char aprefix[MAX_OPTNAME_SIZE * 2 + 8];
  sprintf(aprefix, "%s.[0]", GNB_CONFIG_STRING_GNB_LIST);
  paramdef_t PLMNParams[] = GNBPLMNPARAMS_DESC;
  /* map parameter checking array instances to parameter definition array instances */
  checkedparam_t config_check_PLMNParams[] = PLMNPARAMS_CHECK;
  for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
    PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);
  paramlist_def_t PLMNParamList = {GNB_CONFIG_STRING_PLMN_LIST, NULL, 0};
  config_getlist(cfg, &PLMNParamList, PLMNParams, sizeof(PLMNParams) / sizeof(paramdef_t), aprefix);

  // if fronthaul is F1, require gNB_DU_ID, else use gNB_ID
  if (separate_du) {
    AssertFatal(config_isparamset(GNBParamList.paramarray[0], GNB_GNB_DU_ID_IDX), "%s is not defined in configuration file\n", GNB_CONFIG_STRING_GNB_DU_ID);
    *gnb_du_id = *GNBParamList.paramarray[0][GNB_GNB_DU_ID_IDX].u64ptr;
  } else {
    AssertFatal(!config_isparamset(GNBParamList.paramarray[0], GNB_GNB_DU_ID_IDX),
                "%s should not be defined in configuration file for monolithic gNB\n",
                GNB_CONFIG_STRING_GNB_DU_ID);
    *gnb_du_id = *gnb_id; // the gNB-DU ID is equal to the gNB ID, since the config has no gNB-DU ID
  }

  *name = strdup(*(GNBParamList.paramarray[0][GNB_GNB_NAME_IDX].strptr));
  info->tac = malloc(sizeof(*info->tac));
  AssertFatal(info->tac != NULL, "out of memory\n");
  *info->tac = *GNBParamList.paramarray[0][GNB_TRACKING_AREA_CODE_IDX].uptr;
  info->plmn.mcc = *PLMNParamList.paramarray[0][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
  info->plmn.mnc = *PLMNParamList.paramarray[0][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
  info->plmn.mnc_digit_length = *PLMNParamList.paramarray[0][GNB_MNC_DIGIT_LENGTH].u8ptr;
  AssertFatal((info->plmn.mnc_digit_length == 2) || (info->plmn.mnc_digit_length == 3),
              "BAD MNC DIGIT LENGTH %d",
              info->plmn.mnc_digit_length);
  info->nr_cellid = (uint64_t) * (GNBParamList.paramarray[0][GNB_NRCELLID_IDX].u64ptr);

  LOG_W(GNB_APP, "no slices transported via F1 Setup Request!\n");
  info->num_ssi = 0;

  return 1;
}

static f1ap_setup_req_t *RC_read_F1Setup(uint64_t id,
                                         const char *name,
                                         const f1ap_served_cell_info_t *info,
                                         const NR_ServingCellConfigCommon_t *scc,
                                         NR_BCCH_BCH_Message_t *mib,
                                         const NR_BCCH_DL_SCH_Message_t *sib1)
{
  f1ap_setup_req_t *req = calloc(1, sizeof(*req));
  AssertFatal(req != NULL, "out of memory\n");
  req->gNB_DU_id = id;
  req->gNB_DU_name = strdup(name);
  req->num_cells_available = 1;
  req->cell[0].info = *info;
  LOG_I(GNB_APP,
        "F1AP: gNB idx %d gNB_DU_id %ld, gNB_DU_name %s, TAC %d MCC/MNC/length %d/%d/%d cellID %ld\n",
        0,
        req->gNB_DU_id,
        req->gNB_DU_name,
        *req->cell[0].info.tac,
        req->cell[0].info.plmn.mcc,
        req->cell[0].info.plmn.mnc,
        req->cell[0].info.plmn.mnc_digit_length,
        req->cell[0].info.nr_cellid);

  req->cell[0].info.nr_pci = *scc->physCellId;
  struct NR_FrequencyInfoDL *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  if (scc->tdd_UL_DL_ConfigurationCommon) {
    LOG_I(GNB_APP, "ngran_DU: Configuring Cell %d for TDD\n", 0);
    req->cell[0].info.mode = F1AP_MODE_TDD;
    f1ap_tdd_info_t *tdd = &req->cell[0].info.tdd;
    tdd->freqinfo.arfcn = frequencyInfoDL->absoluteFrequencyPointA;
    tdd->tbw.scs = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
    tdd->tbw.nrb = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
    tdd->freqinfo.band = *frequencyInfoDL->frequencyBandList.list.array[0];
  } else {
    LOG_I(GNB_APP, "ngran_DU: Configuring Cell %d for FDD\n", 0);
    req->cell[0].info.mode = F1AP_MODE_FDD;
    f1ap_fdd_info_t *fdd = &req->cell[0].info.fdd;
    fdd->dl_freqinfo.arfcn = frequencyInfoDL->absoluteFrequencyPointA;
    fdd->ul_freqinfo.arfcn = *scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA;
    fdd->dl_tbw.scs = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
    fdd->ul_tbw.scs = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
    fdd->dl_tbw.nrb = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
    fdd->ul_tbw.nrb = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
    fdd->dl_freqinfo.band = *frequencyInfoDL->frequencyBandList.list.array[0];
    fdd->ul_freqinfo.band = *scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0];
  }

  req->cell[0].info.measurement_timing_information = "0";

  if (get_softmodem_params()->sa) {
    // in NSA we don't transmit SIB1, so cannot fill DU system information
    // so cannot send MIB either

    int buf_len = 3; // this is what we assume in monolithic
    req->cell[0].sys_info = calloc(1, sizeof(*req->cell[0].sys_info));
    AssertFatal(req->cell[0].sys_info != NULL, "out of memory\n");
    f1ap_gnb_du_system_info_t *sys_info = req->cell[0].sys_info;
    sys_info->mib = calloc(buf_len, sizeof(*sys_info->mib));
    DevAssert(sys_info->mib != NULL);
    DevAssert(mib != NULL);
    sys_info->mib_length = encode_MIB_NR(mib, 0, sys_info->mib, buf_len);
    DevAssert(sys_info->mib_length == buf_len);

    DevAssert(sib1 != NULL);
    NR_SIB1_t *bcch_SIB1 = sib1->message.choice.c1->choice.systemInformationBlockType1;
    sys_info->sib1 = calloc(NR_MAX_SIB_LENGTH / 8, sizeof(*sys_info->sib1));
    asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_SIB1, NULL, (void *)bcch_SIB1, sys_info->sib1, NR_MAX_SIB_LENGTH / 8);
    AssertFatal(enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n", enc_rval.failed_type->name, enc_rval.encoded);
    sys_info->sib1_length = (enc_rval.encoded + 7) / 8;
  }

  int num = read_version(TO_STRING(NR_RRC_VERSION), &req->rrc_ver[0], &req->rrc_ver[1], &req->rrc_ver[2]);
  AssertFatal(num == 3, "could not read RRC version string %s\n", TO_STRING(NR_RRC_VERSION));

  return req;
}

void RCconfig_nr_macrlc(configmodule_interface_t *cfg)
{
  int j = 0;
  uint16_t prbbl[275] = {0};
  int num_prbbl = 0;

  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST, NULL, 0};
  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  config_get(cfg, GNBSParams, sizeofArray(GNBSParams), NULL);
  int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal(num_gnbs == 1,
              "Failed to parse config file: number of gnbs for gNB %s is %d != 1\n",
              GNB_CONFIG_STRING_ACTIVE_GNBS,
              num_gnbs);
  paramdef_t GNBParams[] = GNBPARAMS_DESC;
  config_getlist(cfg, &GNBParamList, GNBParams, sizeofArray(GNBParams), NULL);

  if (NFAPI_MODE != NFAPI_MODE_PNF) {
    ////////// Identification parameters

    char *ulprbbl = *GNBParamList.paramarray[0][GNB_ULPRBBLACKLIST_IDX].strptr;
    char *save = NULL;
    char *pt = strtok_r(ulprbbl, ",", &save);
    memset(prbbl, 0, sizeof(prbbl));
    while (pt) {
      const int prb = atoi(pt);
      AssertFatal(prb < 275, "RB %d out of bounds (max 275)\n", prb);
      prbbl[prb] = 0x3FFF; // all symbols taken
      pt = strtok_r(NULL, ",", &save);
      num_prbbl++;
    }
  }
  paramdef_t MacRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t MacRLC_ParamList = {CONFIG_STRING_MACRLC_LIST, NULL, 0};
  /* map parameter checking array instances to parameter definition array instances */
  checkedparam_t config_check_MacRLCParams[] = MACRLCPARAMS_CHECK;
  for (int i = 0; i < sizeofArray(MacRLC_Params); ++i)
    MacRLC_Params[i].chkPptr = &(config_check_MacRLCParams[i]);
  config_getlist(config_get_if(), &MacRLC_ParamList, MacRLC_Params, sizeofArray(MacRLC_Params), NULL);

  nr_mac_config_t config = {0};
  config.pdsch_AntennaPorts.N1 = *GNBParamList.paramarray[0][GNB_PDSCH_ANTENNAPORTS_N1_IDX].iptr;
  config.pdsch_AntennaPorts.N2 = *GNBParamList.paramarray[0][GNB_PDSCH_ANTENNAPORTS_N2_IDX].iptr;
  config.pdsch_AntennaPorts.XP = *GNBParamList.paramarray[0][GNB_PDSCH_ANTENNAPORTS_XP_IDX].iptr;
  config.pusch_AntennaPorts = *GNBParamList.paramarray[0][GNB_PUSCH_ANTENNAPORTS_IDX].iptr;
  LOG_I(GNB_APP,
        "pdsch_AntennaPorts N1 %d N2 %d XP %d pusch_AntennaPorts %d\n",
        config.pdsch_AntennaPorts.N1,
        config.pdsch_AntennaPorts.N2,
        config.pdsch_AntennaPorts.XP,
        config.pusch_AntennaPorts);

  config.minRXTXTIME = *GNBParamList.paramarray[0][GNB_MINRXTXTIME_IDX].iptr;
  LOG_I(GNB_APP, "minTXRXTIME %d\n", config.minRXTXTIME);
  config.sib1_tda = *GNBParamList.paramarray[0][GNB_SIB1_TDA_IDX].iptr;
  LOG_I(GNB_APP, "SIB1 TDA %d\n", config.sib1_tda);
  config.do_CSIRS = *GNBParamList.paramarray[0][GNB_DO_CSIRS_IDX].iptr;
  config.do_SRS = *GNBParamList.paramarray[0][GNB_DO_SRS_IDX].iptr;
  config.force_256qam_off = *GNBParamList.paramarray[0][GNB_FORCE256QAMOFF_IDX].iptr;
  LOG_I(GNB_APP,
        "CSI-RS %d, SRS %d, 256 QAM %s\n",
        config.do_CSIRS,
        config.do_SRS,
        config.force_256qam_off ? "force off" : "may be on");

  NR_ServingCellConfigCommon_t *scc = get_scc_config(cfg, config.minRXTXTIME);
  //xer_fprint(stdout, &asn_DEF_NR_ServingCellConfigCommon, scc);
  NR_ServingCellConfig_t *scd = get_scd_config(cfg);

  if (MacRLC_ParamList.numelt > 0) {
    RC.nb_nr_macrlc_inst = MacRLC_ParamList.numelt;
    ngran_node_t node_type = get_node_type();
    mac_top_init_gNB(node_type, scc, scd, &config);
    RC.nb_nr_mac_CC = (int *)malloc(RC.nb_nr_macrlc_inst * sizeof(int));

    for (j = 0; j < RC.nb_nr_macrlc_inst; j++) {
      RC.nb_nr_mac_CC[j] = *(MacRLC_ParamList.paramarray[j][MACRLC_CC_IDX].iptr);
      RC.nrmac[j]->pusch_target_snrx10 = *(MacRLC_ParamList.paramarray[j][MACRLC_PUSCHTARGETSNRX10_IDX].iptr);
      RC.nrmac[j]->pucch_target_snrx10 = *(MacRLC_ParamList.paramarray[j][MACRLC_PUCCHTARGETSNRX10_IDX].iptr);
      RC.nrmac[j]->ul_prbblack_SNR_threshold = *(MacRLC_ParamList.paramarray[j][MACRLC_UL_PRBBLACK_SNR_THRESHOLD_IDX].iptr);
      RC.nrmac[j]->pucch_failure_thres = *(MacRLC_ParamList.paramarray[j][MACRLC_PUCCHFAILURETHRES_IDX].iptr);
      RC.nrmac[j]->pusch_failure_thres = *(MacRLC_ParamList.paramarray[j][MACRLC_PUSCHFAILURETHRES_IDX].iptr);

      LOG_I(NR_MAC,
            "PUSCH Target %d, PUCCH Target %d, PUCCH Failure %d, PUSCH Failure %d\n",
            RC.nrmac[j]->pusch_target_snrx10,
            RC.nrmac[j]->pucch_target_snrx10,
            RC.nrmac[j]->pucch_failure_thres,
            RC.nrmac[j]->pusch_failure_thres);
      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_RRC") == 0) {
        // check number of instances is same as RRC/PDCP

      } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "f1") == 0
                 || strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "cudu") == 0) {
        printf("Configuring F1 interfaces for MACRLC\n");
        RC.nrmac[j]->eth_params_n.local_if_name = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_IF_NAME_IDX].strptr));
        RC.nrmac[j]->eth_params_n.my_addr = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_n.remote_addr = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_n.my_portc = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_n.remote_portc = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_n.my_portd = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTD_IDX].iptr);
        RC.nrmac[j]->eth_params_n.remote_portd = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTD_IDX].iptr);
        RC.nrmac[j]->eth_params_n.transp_preference = ETH_UDP_MODE;
      } else { // other midhaul
        AssertFatal(1 == 0, "MACRLC %d: %s unknown northbound midhaul\n", j, *(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr));
      }

      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_L1") == 0) {
      } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        RC.nrmac[j]->eth_params_s.local_if_name = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_IF_NAME_IDX].strptr));
        RC.nrmac[j]->eth_params_s.my_addr = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_s.remote_addr = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_s.my_portc = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_s.remote_portc = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_s.my_portd = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTD_IDX].iptr);
        RC.nrmac[j]->eth_params_s.remote_portd = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTD_IDX].iptr);
        RC.nrmac[j]->eth_params_s.transp_preference = ETH_UDP_MODE;

        printf("**************** vnf_port:%d\n", RC.nrmac[j]->eth_params_s.my_portc);
        configure_nr_nfapi_vnf(
            RC.nrmac[j]->eth_params_s.my_addr, RC.nrmac[j]->eth_params_s.my_portc, RC.nrmac[j]->eth_params_s.remote_addr, RC.nrmac[j]->eth_params_s.remote_portd, RC.nrmac[j]->eth_params_s.my_portd);
        printf("**************** RETURNED FROM configure_nfapi_vnf() vnf_port:%d\n", RC.nrmac[j]->eth_params_s.my_portc);
      } else { // other midhaul
        AssertFatal(1 == 0, "MACRLC %d: %s unknown southbound midhaul\n", j, *(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr));
      }
      RC.nrmac[j]->ulsch_max_frame_inactivity = *(MacRLC_ParamList.paramarray[j][MACRLC_ULSCH_MAX_FRAME_INACTIVITY].uptr);
      NR_bler_options_t *dl_bler_options = &RC.nrmac[j]->dl_bler;
      dl_bler_options->upper = *(MacRLC_ParamList.paramarray[j][MACRLC_DL_BLER_TARGET_UPPER_IDX].dblptr);
      dl_bler_options->lower = *(MacRLC_ParamList.paramarray[j][MACRLC_DL_BLER_TARGET_LOWER_IDX].dblptr);
      dl_bler_options->max_mcs = *(MacRLC_ParamList.paramarray[j][MACRLC_DL_MAX_MCS_IDX].u8ptr);
      dl_bler_options->harq_round_max = *(MacRLC_ParamList.paramarray[j][MACRLC_DL_HARQ_ROUND_MAX_IDX].u8ptr);
      NR_bler_options_t *ul_bler_options = &RC.nrmac[j]->ul_bler;
      ul_bler_options->upper = *(MacRLC_ParamList.paramarray[j][MACRLC_UL_BLER_TARGET_UPPER_IDX].dblptr);
      ul_bler_options->lower = *(MacRLC_ParamList.paramarray[j][MACRLC_UL_BLER_TARGET_LOWER_IDX].dblptr);
      ul_bler_options->max_mcs = *(MacRLC_ParamList.paramarray[j][MACRLC_UL_MAX_MCS_IDX].u8ptr);
      ul_bler_options->harq_round_max = *(MacRLC_ParamList.paramarray[j][MACRLC_UL_HARQ_ROUND_MAX_IDX].u8ptr);
      RC.nrmac[j]->min_grant_prb = *(MacRLC_ParamList.paramarray[j][MACRLC_MIN_GRANT_PRB_IDX].u8ptr);
      RC.nrmac[j]->min_grant_mcs = *(MacRLC_ParamList.paramarray[j][MACRLC_MIN_GRANT_MCS_IDX].u8ptr);
      RC.nrmac[j]->identity_pm = *(MacRLC_ParamList.paramarray[j][MACRLC_IDENTITY_PM_IDX].u8ptr);
      RC.nrmac[j]->num_ulprbbl = num_prbbl;
      memcpy(RC.nrmac[j]->ulprbbl, prbbl, 275 * sizeof(prbbl[0]));
    } //  for (j=0;j<RC.nb_nr_macrlc_inst;j++)

    uint64_t gnb_du_id = 0;
    uint32_t gnb_id = 0;
    char *name = NULL;
    f1ap_served_cell_info_t info;
    read_du_cell_info(cfg, NODE_IS_DU(node_type), &gnb_id, &gnb_du_id, &name, &info, 1);

    if (get_softmodem_params()->sa)
      nr_mac_configure_sib1(RC.nrmac[0], &info.plmn, info.nr_cellid, *info.tac);

    // read F1 Setup information from config and generated MIB/SIB1
    // and store it at MAC for sending later
    NR_BCCH_BCH_Message_t *mib = RC.nrmac[0]->common_channels[0].mib;
    const NR_BCCH_DL_SCH_Message_t *sib1 = RC.nrmac[0]->common_channels[0].sib1;
    f1ap_setup_req_t *req = RC_read_F1Setup(gnb_du_id, name, &info, scc, mib, sib1);
    AssertFatal(req != NULL, "could not read F1 Setup information\n");
    RC.nrmac[0]->f1_config.setup_req = req;
    RC.nrmac[0]->f1_config.gnb_id = gnb_id;

    free(name); /* read_du_cell_info() allocated memory */

  } else { // MacRLC_ParamList.numelt > 0
    LOG_E(PHY, "No %s configuration found\n", CONFIG_STRING_MACRLC_LIST);
    // AssertFatal (0,"No " CONFIG_STRING_MACRLC_LIST " configuration found");
  }
}

void config_security(gNB_RRC_INST *rrc)
{
  paramdef_t sec_params[] = SECURITY_GLOBALPARAMS_DESC;
  int ret = config_get(config_get_if(), sec_params, sizeofArray(sec_params), CONFIG_STRING_SECURITY);
  int i;

  if (ret < 0) {
    LOG_W(RRC, "configuration file does not contain a \"security\" section, applying default parameters (nia2 nea0, integrity disabled for DRBs)\n");
    rrc->security.ciphering_algorithms[0]    = 0;  /* nea0 = no ciphering */
    rrc->security.ciphering_algorithms_count = 1;
    rrc->security.integrity_algorithms[0]    = 2;  /* nia2 */
    rrc->security.integrity_algorithms[1]    = 0;  /* nia0 = no integrity, as a fallback (but nia2 should be supported by all UEs) */
    rrc->security.integrity_algorithms_count = 2;
    rrc->security.do_drb_ciphering           = 1;  /* even if nea0 let's activate so that we don't generate cipheringDisabled in pdcp_Config */
    rrc->security.do_drb_integrity           = 0;
    return;
  }

  if (sec_params[SECURITY_CONFIG_CIPHERING_IDX].numelt > 4) {
    LOG_E(RRC, "too much ciphering algorithms in section \"security\" of the configuration file, maximum is 4\n");
    exit(1);
  }
  if (sec_params[SECURITY_CONFIG_INTEGRITY_IDX].numelt > 4) {
    LOG_E(RRC, "too much integrity algorithms in section \"security\" of the configuration file, maximum is 4\n");
    exit(1);
  }

  /* get ciphering algorithms */
  rrc->security.ciphering_algorithms_count = 0;
  for (i = 0; i < sec_params[SECURITY_CONFIG_CIPHERING_IDX].numelt; i++) {
    if (!strcmp(sec_params[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i], "nea0")) {
      rrc->security.ciphering_algorithms[rrc->security.ciphering_algorithms_count] = 0;
      rrc->security.ciphering_algorithms_count++;
      continue;
    }
    if (!strcmp(sec_params[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i], "nea1")) {
      rrc->security.ciphering_algorithms[rrc->security.ciphering_algorithms_count] = 1;
      rrc->security.ciphering_algorithms_count++;
      continue;
    }
    if (!strcmp(sec_params[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i], "nea2")) {
      rrc->security.ciphering_algorithms[rrc->security.ciphering_algorithms_count] = 2;
      rrc->security.ciphering_algorithms_count++;
      continue;
    }
    if (!strcmp(sec_params[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i], "nea3")) {
      rrc->security.ciphering_algorithms[rrc->security.ciphering_algorithms_count] = 3;
      rrc->security.ciphering_algorithms_count++;
      continue;
    }
    LOG_E(RRC, "unknown ciphering algorithm \"%s\" in section \"security\" of the configuration file\n",
          sec_params[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i]);
    exit(1);
  }

  /* get integrity algorithms */
  rrc->security.integrity_algorithms_count = 0;
  for (i = 0; i < sec_params[SECURITY_CONFIG_INTEGRITY_IDX].numelt; i++) {
    if (!strcmp(sec_params[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i], "nia0")) {
      rrc->security.integrity_algorithms[rrc->security.integrity_algorithms_count] = 0;
      rrc->security.integrity_algorithms_count++;
      continue;
    }
    if (!strcmp(sec_params[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i], "nia1")) {
      rrc->security.integrity_algorithms[rrc->security.integrity_algorithms_count] = 1;
      rrc->security.integrity_algorithms_count++;
      continue;
    }
    if (!strcmp(sec_params[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i], "nia2")) {
      rrc->security.integrity_algorithms[rrc->security.integrity_algorithms_count] = 2;
      rrc->security.integrity_algorithms_count++;
      continue;
    }
    if (!strcmp(sec_params[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i], "nia3")) {
      rrc->security.integrity_algorithms[rrc->security.integrity_algorithms_count] = 3;
      rrc->security.integrity_algorithms_count++;
      continue;
    }
    LOG_E(RRC, "unknown integrity algorithm \"%s\" in section \"security\" of the configuration file\n",
          sec_params[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i]);
    exit(1);
  }

  if (rrc->security.ciphering_algorithms_count == 0) {
    LOG_W(RRC, "no preferred ciphering algorithm set in configuration file, applying default parameters (no security)\n");
    rrc->security.ciphering_algorithms[0]    = 0;  /* nea0 = no ciphering */
    rrc->security.ciphering_algorithms_count = 1;
  }

  if (rrc->security.integrity_algorithms_count == 0) {
    LOG_W(RRC, "no preferred integrity algorithm set in configuration file, applying default parameters (nia2)\n");
    rrc->security.integrity_algorithms[0]    = 2;  /* nia2 */
    rrc->security.integrity_algorithms[1]    = 0;  /* nia0 = no integrity */
    rrc->security.integrity_algorithms_count = 2;
  }

  if (!strcmp(*sec_params[SECURITY_CONFIG_DO_DRB_CIPHERING_IDX].strptr, "yes")) {
    rrc->security.do_drb_ciphering = 1;
  } else if (!strcmp(*sec_params[SECURITY_CONFIG_DO_DRB_CIPHERING_IDX].strptr, "no")) {
    rrc->security.do_drb_ciphering = 0;
  } else {
    LOG_E(RRC, "in configuration file, bad drb_ciphering value '%s', only 'yes' and 'no' allowed\n",
          *sec_params[SECURITY_CONFIG_DO_DRB_CIPHERING_IDX].strptr);
    exit(1);
  }

  if (!strcmp(*sec_params[SECURITY_CONFIG_DO_DRB_INTEGRITY_IDX].strptr, "yes")) {
    rrc->security.do_drb_integrity = 1;
  } else if (!strcmp(*sec_params[SECURITY_CONFIG_DO_DRB_INTEGRITY_IDX].strptr, "no")) {
    rrc->security.do_drb_integrity = 0;
  } else {
    LOG_E(RRC, "in configuration file, bad drb_integrity value '%s', only 'yes' and 'no' allowed\n",
          *sec_params[SECURITY_CONFIG_DO_DRB_INTEGRITY_IDX].strptr);
    exit(1);
  }
}

// Section 5.4.3 of 38.101-1 and -2
static void check_ssb_raster(uint64_t freq, int band, int scs)
{
  int start_gscn = 0, step_gscn = 0, end_gscn = 0;
  for (int i = 0; i < sizeof(sync_raster) / sizeof(sync_raster_t); i++) {
    if (sync_raster[i].band == band &&
        sync_raster[i].scs_index == scs) {
      start_gscn = sync_raster[i].first_gscn;
      step_gscn = sync_raster[i].step_gscn;
      end_gscn = sync_raster[i].last_gscn;
      break;
    }
  }
  AssertFatal(start_gscn != 0, "Couldn't find band %d with SCS %d\n", band, scs);
  int gscn;
  if (freq < 3000000000) {
    int N = 0;
    int M = 0;
    for (int k = 0; k < 3; k++) {
      M = (k << 1) + 1;
      if ((freq - M * 50000) % 1200000 == 0) {
        N = (freq - M * 50000) / 1200000;
        break;
      }
    }
    AssertFatal(N != 0, "SSB frequency %lu Hz not on the synchronization raster (N * 1200kHz + M * 50 kHz)\n",
                freq);
    gscn = (3 * N) + (M - 3) / 2;
  }
  else if (freq < 24250000000) {
    AssertFatal((freq - 3000000000) % 1440000 == 0,
                "SSB frequency %lu Hz not on the synchronization raster (3000 MHz + N * 1.44 MHz)\n",
                freq);
    gscn = ((freq - 3000000000) / 1440000) + 7499;
  }
  else {
    AssertFatal((freq - 24250080000) % 17280000 == 0,
                "SSB frequency %lu Hz not on the synchronization raster (24250.08 MHz + N * 17.28 MHz)\n",
                freq);
    gscn = ((freq - 24250080000) / 17280000) + 22256;
  }
  AssertFatal(gscn >= start_gscn && gscn <= end_gscn,
              "GSCN %d corresponding to SSB frequency %lu does not belong to GSCN range for band %d\n",
              gscn, freq, band);
  int rel_gscn = gscn - start_gscn;
  AssertFatal(rel_gscn % step_gscn == 0,
              "GSCN %d corresponding to SSB frequency %lu not in accordance with GSCN step for band %d\n",
               gscn, freq, band);
}

void RCconfig_NRRRC(gNB_RRC_INST *rrc)
{

  int num_gnbs = 0;
  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  int32_t gnb_id = 0;
  int k = 0;
  int i = 0;

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  ////////// Identification parameters
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};

  ////////// Physical parameters

  /* get global parameters, defined outside any section in the config file */

  config_get(config_get_if(), GNBSParams, sizeofArray(GNBSParams), NULL);
  num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal (i<num_gnbs,"Failed to parse config file no %uth element in %s \n",i, GNB_CONFIG_STRING_ACTIVE_GNBS);
  AssertFatal(num_gnbs == 1, "required section \"gNBs\" not in config!\n");

  if (num_gnbs > 0) {

    // Output a list of all gNBs. ////////// Identification parameters
    config_getlist(config_get_if(), &GNBParamList, GNBParams, sizeofArray(GNBParams), NULL);
    if (GNBParamList.paramarray[i][GNB_GNB_ID_IDX].uptr == NULL) {
    // Calculate a default gNB ID
      if (get_softmodem_params()->sa) { 
        uint32_t hash;
        hash = ngap_generate_gNB_id ();
        gnb_id = i + (hash & 0xFFFFFF8);
      } else {
        gnb_id = i;
      }
    } else {
      gnb_id = *(GNBParamList.paramarray[i][GNB_GNB_ID_IDX].uptr);
    }

    sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);

    printf("NRRRC %u: Southbound Transport %s\n",i,*(GNBParamList.paramarray[i][GNB_TRANSPORT_S_PREFERENCE_IDX].strptr));

    rrc->node_type = get_node_type();
    rrc->node_id        = gnb_id;
    if (NODE_IS_CU(rrc->node_type)) {
      paramdef_t SCTPParams[]  = GNBSCTPPARAMS_DESC;
      char aprefix[MAX_OPTNAME_SIZE*2 + 8];
      sprintf(aprefix,"%s.[%u].%s",GNB_CONFIG_STRING_GNB_LIST,i,GNB_CONFIG_STRING_SCTP_CONFIG);
      config_get(config_get_if(), SCTPParams, sizeofArray(SCTPParams), aprefix);
      LOG_I(GNB_APP,"F1AP: gNB_CU_id[%d] %d\n",k,rrc->node_id);
      rrc->node_name = strdup(*(GNBParamList.paramarray[0][GNB_GNB_NAME_IDX].strptr));
      LOG_I(GNB_APP,"F1AP: gNB_CU_name[%d] %s\n",k,rrc->node_name);
      rrc->eth_params_s.local_if_name            = strdup(*(GNBParamList.paramarray[i][GNB_LOCAL_S_IF_NAME_IDX].strptr));
      rrc->eth_params_s.my_addr                  = strdup(*(GNBParamList.paramarray[i][GNB_LOCAL_S_ADDRESS_IDX].strptr));
      rrc->eth_params_s.remote_addr              = strdup(*(GNBParamList.paramarray[i][GNB_REMOTE_S_ADDRESS_IDX].strptr));
      rrc->eth_params_s.my_portc                 = *(GNBParamList.paramarray[i][GNB_LOCAL_S_PORTC_IDX].uptr);
      rrc->eth_params_s.remote_portc             = *(GNBParamList.paramarray[i][GNB_REMOTE_S_PORTC_IDX].uptr);
      rrc->eth_params_s.my_portd                 = *(GNBParamList.paramarray[i][GNB_LOCAL_S_PORTD_IDX].uptr);
      rrc->eth_params_s.remote_portd             = *(GNBParamList.paramarray[i][GNB_REMOTE_S_PORTD_IDX].uptr);
      rrc->eth_params_s.transp_preference        = ETH_UDP_MODE;
    }

   

    rrc->nr_cellid        = (uint64_t)*(GNBParamList.paramarray[i][GNB_NRCELLID_IDX].u64ptr);

    if (strcmp(*(GNBParamList.paramarray[i][GNB_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_mac") == 0) {
      
    } else if (strcmp(*(GNBParamList.paramarray[i][GNB_TRANSPORT_S_PREFERENCE_IDX].strptr), "cudu") == 0) {
      rrc->eth_params_s.local_if_name            = strdup(*(GNBParamList.paramarray[i][GNB_LOCAL_S_IF_NAME_IDX].strptr));
      rrc->eth_params_s.my_addr                  = strdup(*(GNBParamList.paramarray[i][GNB_LOCAL_S_ADDRESS_IDX].strptr));
      rrc->eth_params_s.remote_addr              = strdup(*(GNBParamList.paramarray[i][GNB_REMOTE_S_ADDRESS_IDX].strptr));
      rrc->eth_params_s.my_portc                 = *(GNBParamList.paramarray[i][GNB_LOCAL_S_PORTC_IDX].uptr);
      rrc->eth_params_s.remote_portc             = *(GNBParamList.paramarray[i][GNB_REMOTE_S_PORTC_IDX].uptr);
      rrc->eth_params_s.my_portd                 = *(GNBParamList.paramarray[i][GNB_LOCAL_S_PORTD_IDX].uptr);
      rrc->eth_params_s.remote_portd             = *(GNBParamList.paramarray[i][GNB_REMOTE_S_PORTD_IDX].uptr);
      rrc->eth_params_s.transp_preference        = ETH_UDP_MODE;
    } else { // other midhaul
    }       
    
    // search if in active list
    
    gNB_RrcConfigurationReq nrrrc_config = {0};
    for (k=0; k <num_gnbs ; k++) {
      if (strcmp(GNBSParams[GNB_ACTIVE_GNBS_IDX].strlistptr[k], *(GNBParamList.paramarray[i][GNB_GNB_NAME_IDX].strptr) )== 0) {
	
        char gnbpath[MAX_OPTNAME_SIZE + 8];
        sprintf(gnbpath,"%s.[%i]",GNB_CONFIG_STRING_GNB_LIST,k);

	
        paramdef_t PLMNParams[] = GNBPLMNPARAMS_DESC;

        paramlist_def_t PLMNParamList = {GNB_CONFIG_STRING_PLMN_LIST, NULL, 0};
        /* map parameter checking array instances to parameter definition array instances */
        checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

        for (int I = 0; I < sizeofArray(PLMNParams); ++I)
          PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

        nrrrc_config.tac               = *GNBParamList.paramarray[i][GNB_TRACKING_AREA_CODE_IDX].uptr;
        AssertFatal(!GNBParamList.paramarray[i][GNB_MOBILE_COUNTRY_CODE_IDX_OLD].strptr
                    && !GNBParamList.paramarray[i][GNB_MOBILE_NETWORK_CODE_IDX_OLD].strptr,
                    "It seems that you use an old configuration file. Please change the existing\n"
                    "    tracking_area_code  =  \"1\";\n"
                    "    mobile_country_code =  \"208\";\n"
                    "    mobile_network_code =  \"93\";\n"
                    "to\n"
                    "    tracking_area_code  =  1; // no string!!\n"
                    "    plmn_list = ( { mcc = 208; mnc = 93; mnc_length = 2; } )\n");
        config_getlist(config_get_if(), &PLMNParamList, PLMNParams, sizeofArray(PLMNParams), gnbpath);

        if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
          AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                      PLMNParamList.numelt);

        nrrrc_config.num_plmn = PLMNParamList.numelt;

        for (int l = 0; l < PLMNParamList.numelt; ++l) {
	
	  nrrrc_config.mcc[l]               = *PLMNParamList.paramarray[l][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
	  nrrrc_config.mnc[l]               = *PLMNParamList.paramarray[l][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
	  nrrrc_config.mnc_digit_length[l]  = *PLMNParamList.paramarray[l][GNB_MNC_DIGIT_LENGTH].u8ptr;
	  AssertFatal((nrrrc_config.mnc_digit_length[l] == 2) ||
		      (nrrrc_config.mnc_digit_length[l] == 3),"BAD MNC DIGIT LENGTH %d",
		      nrrrc_config.mnc_digit_length[l]);
        }
        nrrrc_config.enable_sdap = *GNBParamList.paramarray[i][GNB_ENABLE_SDAP_IDX].iptr;
        LOG_I(GNB_APP, "SDAP layer is %s\n", nrrrc_config.enable_sdap ? "enabled" : "disabled");
        nrrrc_config.drbs = *GNBParamList.paramarray[i][GNB_DRBS].iptr;
        nrrrc_config.um_on_default_drb = *(GNBParamList.paramarray[i][GNB_UMONDEFAULTDRB_IDX].uptr);
        LOG_I(GNB_APP, "Data Radio Bearer count %d\n", nrrrc_config.drbs);

      }//
    }//End for (k=0; k <num_gnbs ; k++)
    openair_rrc_gNB_configuration(rrc, &nrrrc_config);
  }//End if (num_gnbs>0)

  config_security(rrc);
}//End RCconfig_NRRRC function

int RCconfig_NR_NG(MessageDef *msg_p, uint32_t i) {

  int               j,k = 0;
  int               gnb_id;
  int32_t           my_int;
  const char*       active_gnb[MAX_GNB];
  char             *address                       = NULL;
  char             *cidr                          = NULL;

  // for no gcc warnings 

  (void)  my_int;

  memset((char*)active_gnb,0,MAX_GNB* sizeof(char*));
  char*             gnb_ipv4_address_for_NGU      = NULL;
  uint32_t          gnb_port_for_NGU              = 0;
  char*             gnb_ipv4_address_for_S1U      = NULL;
  uint32_t          gnb_port_for_S1U              = 0;

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};

  /* get global parameters, defined outside any section in the config file */
  config_get(config_get_if(), GNBSParams, sizeofArray(GNBSParams), NULL);

  AssertFatal (i<GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt,
     "Failed to parse config file %s, %uth attribute %s \n",
     RC.config_file_name, i, GNB_CONFIG_STRING_ACTIVE_GNBS);
    
  
  if (GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt>0) {
    // Output a list of all gNBs.
    config_getlist(config_get_if(), &GNBParamList, GNBParams, sizeofArray(GNBParams), NULL);
    if (GNBParamList.numelt > 0) {
      for (k = 0; k < GNBParamList.numelt; k++) {
        if (GNBParamList.paramarray[k][GNB_GNB_ID_IDX].uptr == NULL) {
          // Calculate a default gNB ID
          if (get_softmodem_params()->sa) {
            uint32_t hash;
          
          hash = ngap_generate_gNB_id ();
          gnb_id = k + (hash & 0xFFFFFF8);
          } else {
            gnb_id = k;
          }
        } else {
          gnb_id = *(GNBParamList.paramarray[k][GNB_GNB_ID_IDX].uptr);
        }
  
  
        // search if in active list
        for (j=0; j < GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt; j++) {
          if (strcmp(GNBSParams[GNB_ACTIVE_GNBS_IDX].strlistptr[j], *(GNBParamList.paramarray[k][GNB_GNB_NAME_IDX].strptr)) == 0) {
            paramdef_t PLMNParams[] = GNBPLMNPARAMS_DESC;
            paramlist_def_t PLMNParamList = {GNB_CONFIG_STRING_PLMN_LIST, NULL, 0};
            paramdef_t SNSSAIParams[] = GNBSNSSAIPARAMS_DESC;
            paramlist_def_t SNSSAIParamList = {GNB_CONFIG_STRING_SNSSAI_LIST, NULL, 0};
            /* map parameter checking array instances to parameter definition array instances */
            checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;
            checkedparam_t config_check_SNSSAIParams [] = SNSSAIPARAMS_CHECK;

            for (int I = 0; I < sizeofArray(PLMNParams); ++I)
              PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);
            for (int J = 0; J < sizeofArray(SNSSAIParams); ++J)
              SNSSAIParams[J].chkPptr = &(config_check_SNSSAIParams[J]);

            paramdef_t NGParams[]  = GNBNGPARAMS_DESC;
            paramlist_def_t NGParamList = {GNB_CONFIG_STRING_AMF_IP_ADDRESS,NULL,0};
            
            paramdef_t SCTPParams[]  = GNBSCTPPARAMS_DESC;
            paramdef_t NETParams[]  =  GNBNETPARAMS_DESC;
            char aprefix[MAX_OPTNAME_SIZE*2 + 8];
            sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, k);
            
            NGAP_REGISTER_GNB_REQ (msg_p).gNB_id = gnb_id;
            
            if (strcmp(*(GNBParamList.paramarray[k][GNB_CELL_TYPE_IDX].strptr), "CELL_MACRO_GNB") == 0) {
              NGAP_REGISTER_GNB_REQ (msg_p).cell_type = CELL_MACRO_GNB;
            } else  if (strcmp(*(GNBParamList.paramarray[k][GNB_CELL_TYPE_IDX].strptr), "CELL_HOME_GNB") == 0) {
              NGAP_REGISTER_GNB_REQ (msg_p).cell_type = CELL_HOME_ENB;
            } else {
              AssertFatal (0,
              "Failed to parse gNB configuration file %s, gnb %u unknown value \"%s\" for cell_type choice: CELL_MACRO_GNB or CELL_HOME_GNB !\n",
              RC.config_file_name, i, *(GNBParamList.paramarray[k][GNB_CELL_TYPE_IDX].strptr));
            }
            
            NGAP_REGISTER_GNB_REQ (msg_p).gNB_name         = strdup(*(GNBParamList.paramarray[k][GNB_GNB_NAME_IDX].strptr));
            NGAP_REGISTER_GNB_REQ (msg_p).tac              = *GNBParamList.paramarray[k][GNB_TRACKING_AREA_CODE_IDX].uptr;
            AssertFatal(!GNBParamList.paramarray[k][GNB_MOBILE_COUNTRY_CODE_IDX_OLD].strptr
                        && !GNBParamList.paramarray[k][GNB_MOBILE_NETWORK_CODE_IDX_OLD].strptr,
                        "It seems that you use an old configuration file. Please change the existing\n"
                        "    tracking_area_code  =  \"1\";\n"
                        "    mobile_country_code =  \"208\";\n"
                        "    mobile_network_code =  \"93\";\n"
                        "to\n"
                        "    tracking_area_code  =  1; // no string!!\n"
                        "    plmn_list = ( { mcc = 208; mnc = 93; mnc_length = 2; } )\n");
            config_getlist(config_get_if(), &PLMNParamList, PLMNParams, sizeofArray(PLMNParams), aprefix);

            if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
              AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                          PLMNParamList.numelt);

            NGAP_REGISTER_GNB_REQ(msg_p).num_plmn = PLMNParamList.numelt;

            for (int l = 0; l < PLMNParamList.numelt; ++l) {
              char snssaistr[MAX_OPTNAME_SIZE*2 + 8];
              sprintf(snssaistr, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, k, GNB_CONFIG_STRING_PLMN_LIST, l);
              config_getlist(config_get_if(), &SNSSAIParamList, SNSSAIParams, sizeofArray(SNSSAIParams), snssaistr);

              NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].mcc = *PLMNParamList.paramarray[l][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
              NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].mnc = *PLMNParamList.paramarray[l][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
              NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].mnc_digit_length = *PLMNParamList.paramarray[l][GNB_MNC_DIGIT_LENGTH].u8ptr;
              NGAP_REGISTER_GNB_REQ (msg_p).default_drx      = 0;
              AssertFatal((NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].mnc_digit_length == 2)
                              || (NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].mnc_digit_length == 3),
                          "BAD MNC DIGIT LENGTH %d",
                          NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].mnc_digit_length);

              NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].num_nssai = SNSSAIParamList.numelt;
              for (int s = 0; s < SNSSAIParamList.numelt; ++s) {
                NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].s_nssai[s].sst =
                    *SNSSAIParamList.paramarray[s][GNB_SLICE_SERVICE_TYPE_IDX].uptr;
                // SD is optional
                // 0xffffff is "no SD", see 23.003 Sec 28.4.2
                NGAP_REGISTER_GNB_REQ(msg_p).plmn[l].s_nssai[s].sd =
                    (*SNSSAIParamList.paramarray[s][GNB_SLICE_DIFFERENTIATOR_IDX].uptr & 0xffffff);
              }
            }
            sprintf(aprefix,"%s.[%i]",GNB_CONFIG_STRING_GNB_LIST,k);
            config_getlist(config_get_if(), &NGParamList, NGParams, sizeofArray(NGParams), aprefix);

            NGAP_REGISTER_GNB_REQ (msg_p).nb_amf = 0;
            
            for (int l = 0; l < NGParamList.numelt; l++) {
              
              NGAP_REGISTER_GNB_REQ (msg_p).nb_amf += 1;
              
              strcpy(NGAP_REGISTER_GNB_REQ (msg_p).amf_ip_address[l].ipv4_address,*(NGParamList.paramarray[l][GNB_AMF_IPV4_ADDRESS_IDX].strptr));
              strcpy(NGAP_REGISTER_GNB_REQ (msg_p).amf_ip_address[l].ipv6_address,*(NGParamList.paramarray[l][GNB_AMF_IPV6_ADDRESS_IDX].strptr));

              if (strcmp(*(NGParamList.paramarray[l][GNB_AMF_IP_ADDRESS_ACTIVE_IDX].strptr), "yes") == 0) {
              } 
              if (strcmp(*(NGParamList.paramarray[l][GNB_AMF_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
                NGAP_REGISTER_GNB_REQ (msg_p).amf_ip_address[j].ipv4 = 1;
              } else if (strcmp(*(NGParamList.paramarray[l][GNB_AMF_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
                NGAP_REGISTER_GNB_REQ (msg_p).amf_ip_address[j].ipv6 = 1;
              } else if (strcmp(*(NGParamList.paramarray[l][GNB_AMF_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
                NGAP_REGISTER_GNB_REQ (msg_p).amf_ip_address[j].ipv4 = 1;
                NGAP_REGISTER_GNB_REQ (msg_p).amf_ip_address[j].ipv6 = 1;
              }

              /* not in configuration yet ...
              if (NGParamList.paramarray[l][GNB_AMF_BROADCAST_PLMN_INDEX].iptr)
                NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_num[l] = NGParamList.paramarray[l][GNB_AMF_BROADCAST_PLMN_INDEX].numelt;
              else
                NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_num[l] = 0;
              */

              AssertFatal(NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_num[l] <= NGAP_REGISTER_GNB_REQ(msg_p).num_plmn,
                          "List of broadcast PLMN to be sent to AMF can not be longer than actual "
                          "PLMN list (max %d, but is %d)\n",
                          NGAP_REGISTER_GNB_REQ(msg_p).num_plmn,
                          NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_num[l]);

              for (int el = 0; el < NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_num[l]; ++el) {
                /* UINTARRAY gets mapped to int, see config_libconfig.c:223 */
                NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_index[l][el] = NGParamList.paramarray[l][GNB_AMF_BROADCAST_PLMN_INDEX].iptr[el];
                AssertFatal(NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_index[l][el] >= 0
                            && NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_index[l][el] < NGAP_REGISTER_GNB_REQ(msg_p).num_plmn,
                            "index for AMF's MCC/MNC (%d) is an invalid index for the registered PLMN IDs (%d)\n",
                            NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_index[l][el],
                            NGAP_REGISTER_GNB_REQ(msg_p).num_plmn);
              }

              /* if no broadcasst_plmn array is defined, fill default values */
              if (NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_num[l] == 0) {
                NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_num[l] = NGAP_REGISTER_GNB_REQ(msg_p).num_plmn;

                for (int el = 0; el < NGAP_REGISTER_GNB_REQ(msg_p).num_plmn; ++el)
                  NGAP_REGISTER_GNB_REQ(msg_p).broadcast_plmn_index[l][el] = el;
              }
              
            }

          
            // SCTP SETTING
            NGAP_REGISTER_GNB_REQ (msg_p).sctp_out_streams = SCTP_OUT_STREAMS;
            NGAP_REGISTER_GNB_REQ (msg_p).sctp_in_streams  = SCTP_IN_STREAMS;
            if (get_softmodem_params()->sa) {
              sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_SCTP_CONFIG);
              config_get(config_get_if(), SCTPParams, sizeofArray(SCTPParams), aprefix);
              NGAP_REGISTER_GNB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[GNB_SCTP_INSTREAMS_IDX].uptr);
              NGAP_REGISTER_GNB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[GNB_SCTP_OUTSTREAMS_IDX].uptr);
            }

            sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
            // NETWORK_INTERFACES
            config_get(config_get_if(), NETParams, sizeofArray(NETParams), aprefix);

            //    NGAP_REGISTER_GNB_REQ (msg_p).enb_interface_name_for_NGU = strdup(enb_interface_name_for_NGU);
            cidr = *(NETParams[GNB_IPV4_ADDRESS_FOR_NG_AMF_IDX].strptr);
            char *save = NULL;
            address = strtok_r(cidr, "/", &save);
            
            NGAP_REGISTER_GNB_REQ (msg_p).gnb_ip_address.ipv6 = 0;
            NGAP_REGISTER_GNB_REQ (msg_p).gnb_ip_address.ipv4 = 1;
            
            strcpy(NGAP_REGISTER_GNB_REQ (msg_p).gnb_ip_address.ipv4_address, address);
            
            break;
          }
        }
      }
    }
  }
  return 0;
}

int RCconfig_nr_parallel(void) {
  char *parallel_conf = NULL;
  char *worker_conf   = NULL;
  extern char *parallel_config;
  extern char *worker_config;
  paramdef_t ThreadParams[]  = THREAD_CONF_DESC;
  paramlist_def_t THREADParamList = {THREAD_CONFIG_STRING_THREAD_STRUCT,NULL,0};
  config_getlist(config_get_if(), &THREADParamList, NULL, 0, NULL);

  if(THREADParamList.numelt>0) {
    config_getlist(config_get_if(), &THREADParamList, ThreadParams, sizeofArray(ThreadParams), NULL);
    parallel_conf = strdup(*(THREADParamList.paramarray[0][THREAD_PARALLEL_IDX].strptr));
  } else {
    parallel_conf = strdup("PARALLEL_RU_L1_TRX_SPLIT");
  }

  if(THREADParamList.numelt>0) {
    config_getlist(config_get_if(), &THREADParamList, ThreadParams, sizeofArray(ThreadParams), NULL);
    worker_conf   = strdup(*(THREADParamList.paramarray[0][THREAD_WORKER_IDX].strptr));
  } else {
    worker_conf   = strdup("WORKER_ENABLE");
  }

  if(parallel_config == NULL) set_parallel_conf(parallel_conf);
  if(worker_config == NULL)   set_worker_conf(worker_conf);

  return 0;
}

void NRRCConfig(void) {

  paramlist_def_t MACRLCParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};
  paramlist_def_t L1ParamList     = {CONFIG_STRING_L1_LIST,NULL,0};
  paramlist_def_t RUParamList     = {CONFIG_STRING_RU_LIST,NULL,0};
  paramdef_t GNBSParams[]         = GNBSPARAMS_DESC;
  
/* get global parameters, defined outside any section in the config file */

  LOG_I(GNB_APP, "Getting GNBSParams\n");

  config_get(config_get_if(), GNBSParams, sizeofArray(GNBSParams), NULL);
  RC.nb_nr_inst = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;

  // Get num MACRLC instances
  config_getlist(config_get_if(), &MACRLCParamList, NULL, 0, NULL);
  RC.nb_nr_macrlc_inst  = MACRLCParamList.numelt;
  // Get num L1 instances
  config_getlist(config_get_if(), &L1ParamList, NULL, 0, NULL);
  RC.nb_nr_L1_inst = L1ParamList.numelt;
  
  // Get num RU instances
  config_getlist(config_get_if(), &RUParamList, NULL, 0, NULL);
  RC.nb_RU     = RUParamList.numelt; 
  
  RCconfig_nr_parallel();
    

}


int RCconfig_NR_X2(MessageDef *msg_p, uint32_t i) {
  int   J, l;
  char *address = NULL;
  char *cidr    = NULL;
  //int                    num_gnbs                                                      = 0;
  //int                    num_component_carriers                                        = 0;
  int                    j,k                                                           = 0;
  int32_t                gnb_id                                                        = 0;

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  ////////// Identification parameters
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};
  /* get global parameters, defined outside any section in the config file */
  config_get(config_get_if(), GNBSParams, sizeofArray(GNBSParams), NULL);
  NR_ServingCellConfigCommon_t *scc = calloc(1,sizeof(NR_ServingCellConfigCommon_t));
  uint64_t ssb_bitmap=0xff;
  memset((void*)scc,0,sizeof(NR_ServingCellConfigCommon_t));
  prepare_scc(scc);
  paramdef_t SCCsParams[] = SCCPARAMS_DESC(scc);
  paramlist_def_t SCCsParamList = {GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON, NULL, 0};

  AssertFatal(i < GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt,
              "Failed to parse config file %s, %uth attribute %s \n",
              RC.config_file_name, i, GNB_CONFIG_STRING_ACTIVE_GNBS);

  if (GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt > 0) {
    // Output a list of all gNBs.
    config_getlist(config_get_if(), &GNBParamList, GNBParams, sizeofArray(GNBParams), NULL);

    if (GNBParamList.numelt > 0) {
      for (k = 0; k < GNBParamList.numelt; k++) {
        if (GNBParamList.paramarray[k][GNB_GNB_ID_IDX].uptr == NULL) {
          // Calculate a default eNB ID
          if (get_softmodem_params()->sa) {
            uint32_t hash;
            hash = ngap_generate_gNB_id ();
            gnb_id = k + (hash & 0xFFFFFF8);
          } else {
            gnb_id = k;
          }
        } else {
          gnb_id = *(GNBParamList.paramarray[k][GNB_GNB_ID_IDX].uptr);
        }

        // search if in active list
        for (j = 0; j < GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt; j++) {
          if (strcmp(GNBSParams[GNB_ACTIVE_GNBS_IDX].strlistptr[j], *(GNBParamList.paramarray[k][GNB_GNB_NAME_IDX].strptr)) == 0) {
            paramdef_t PLMNParams[] = GNBPLMNPARAMS_DESC;
            paramlist_def_t PLMNParamList = {GNB_CONFIG_STRING_PLMN_LIST, NULL, 0};
            /* map parameter checking array instances to parameter definition array instances */
            checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

            for (int I = 0; I < sizeofArray(PLMNParams); ++I)
              PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

            paramdef_t X2Params[]  = X2PARAMS_DESC;
            paramlist_def_t X2ParamList = {ENB_CONFIG_STRING_TARGET_ENB_X2_IP_ADDRESS,NULL,0};
            paramdef_t SCTPParams[]  = GNBSCTPPARAMS_DESC;
	    char*             gnb_ipv4_address_for_NGU      = NULL;
	    uint32_t          gnb_port_for_NGU              = 0;
	    char*             gnb_ipv4_address_for_S1U      = NULL;
	    uint32_t          gnb_port_for_S1U              = 0;

            paramdef_t NETParams[]  =  GNBNETPARAMS_DESC;
            /* TODO: fix the size - if set lower we have a crash (MAX_OPTNAME_SIZE was 64 when this code was written) */
            /* this is most probably a problem with the config module */
            char aprefix[MAX_OPTNAME_SIZE*80 + 8];
            sprintf(aprefix,"%s.[%i]",GNB_CONFIG_STRING_GNB_LIST,k);
            /* Some default/random parameters */
            X2AP_REGISTER_ENB_REQ (msg_p).eNB_id = gnb_id;

            if (strcmp(*(GNBParamList.paramarray[k][GNB_CELL_TYPE_IDX].strptr), "CELL_MACRO_GNB") == 0) {
              X2AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_MACRO_GNB;
            }else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                           RC.config_file_name, i, *(GNBParamList.paramarray[k][GNB_CELL_TYPE_IDX].strptr));
            }

            X2AP_REGISTER_ENB_REQ (msg_p).eNB_name         = strdup(*(GNBParamList.paramarray[k][GNB_GNB_NAME_IDX].strptr));
            X2AP_REGISTER_ENB_REQ (msg_p).tac              = *GNBParamList.paramarray[k][GNB_TRACKING_AREA_CODE_IDX].uptr;
            config_getlist(config_get_if(), &PLMNParamList, PLMNParams, sizeofArray(PLMNParams), aprefix);

            if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
              AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                          PLMNParamList.numelt);

            if (PLMNParamList.numelt > 1)
              LOG_W(X2AP, "X2AP currently handles only one PLMN, ignoring the others!\n");

            X2AP_REGISTER_ENB_REQ (msg_p).mcc = *PLMNParamList.paramarray[0][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
            X2AP_REGISTER_ENB_REQ (msg_p).mnc = *PLMNParamList.paramarray[0][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
            X2AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length = *PLMNParamList.paramarray[0][GNB_MNC_DIGIT_LENGTH].u8ptr;
            AssertFatal(X2AP_REGISTER_ENB_REQ(msg_p).mnc_digit_length == 3
                        || X2AP_REGISTER_ENB_REQ(msg_p).mnc < 100,
                        "MNC %d cannot be encoded in two digits as requested (change mnc_digit_length to 3)\n",
                        X2AP_REGISTER_ENB_REQ(msg_p).mnc);

            sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);

            config_getlist(config_get_if(), &SCCsParamList, NULL, 0, aprefix);
            if (SCCsParamList.numelt > 0) {
              sprintf(aprefix, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST,0,GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON, 0);
              config_get(config_get_if(), SCCsParams, sizeofArray(SCCsParams), aprefix);
              fix_scc(scc,ssb_bitmap);
            }
            X2AP_REGISTER_ENB_REQ (msg_p).num_cc = SCCsParamList.numelt;
            for (J = 0; J < SCCsParamList.numelt ; J++) {
              struct NR_FrequencyInfoDL *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
              X2AP_REGISTER_ENB_REQ(msg_p).nr_band[J] = *frequencyInfoDL->frequencyBandList.list.array[0]; // nr_band; //78
              X2AP_REGISTER_ENB_REQ(msg_p).nrARFCN[J] = frequencyInfoDL->absoluteFrequencyPointA;
              X2AP_REGISTER_ENB_REQ (msg_p).uplink_frequency_offset[J] = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier; //0
              X2AP_REGISTER_ENB_REQ (msg_p).Nid_cell[J]= *scc->physCellId; //0
              X2AP_REGISTER_ENB_REQ(msg_p).N_RB_DL[J] =
                  frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth; // 106
              X2AP_REGISTER_ENB_REQ (msg_p).frame_type[J] = TDD;
              LOG_I(X2AP, "gNB configuration parameters: nr_band: %d, nr_ARFCN: %d, DL_RBs: %d, num_cc: %d \n",
                  X2AP_REGISTER_ENB_REQ (msg_p).nr_band[J],
                  X2AP_REGISTER_ENB_REQ (msg_p).nrARFCN[J],
                  X2AP_REGISTER_ENB_REQ (msg_p).N_RB_DL[J],
                  X2AP_REGISTER_ENB_REQ (msg_p).num_cc);
            }

            sprintf(aprefix,"%s.[%i]",GNB_CONFIG_STRING_GNB_LIST,k);
            config_getlist(config_get_if(), &X2ParamList, X2Params, sizeofArray(X2Params), aprefix);
            AssertFatal(X2ParamList.numelt <= X2AP_MAX_NB_ENB_IP_ADDRESS,
                        "value of X2ParamList.numelt %d must be lower than X2AP_MAX_NB_ENB_IP_ADDRESS %d value: reconsider to increase X2AP_MAX_NB_ENB_IP_ADDRESS\n",
                        X2ParamList.numelt,X2AP_MAX_NB_ENB_IP_ADDRESS);
            X2AP_REGISTER_ENB_REQ (msg_p).nb_x2 = 0;

            for (l = 0; l < X2ParamList.numelt; l++) {
              X2AP_REGISTER_ENB_REQ (msg_p).nb_x2 += 1;
              strcpy(X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4_address,*(X2ParamList.paramarray[l][ENB_X2_IPV4_ADDRESS_IDX].strptr));
              strcpy(X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6_address,*(X2ParamList.paramarray[l][ENB_X2_IPV6_ADDRESS_IDX].strptr));

              if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4 = 1;
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6 = 0;
              } else if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4 = 0;
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6 = 1;
              } else if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4 = 1;
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6 = 1;
              }
            }

            // timers
            {
              int t_reloc_prep = 0;
              int tx2_reloc_overall = 0;
              int t_dc_prep = 0;
              int t_dc_overall = 0;
              paramdef_t p[] = {
                { "t_reloc_prep", "t_reloc_prep", 0, .iptr=&t_reloc_prep, .defintval=0, TYPE_INT, 0 },
                { "tx2_reloc_overall", "tx2_reloc_overall", 0, .iptr=&tx2_reloc_overall, .defintval=0, TYPE_INT, 0 },
                { "t_dc_prep", "t_dc_prep", 0, .iptr=&t_dc_prep, .defintval=0, TYPE_INT, 0 },
                { "t_dc_overall", "t_dc_overall", 0, .iptr=&t_dc_overall, .defintval=0, TYPE_INT, 0 }
              };
              config_get(config_get_if(), p, sizeofArray(p), aprefix);

              if (t_reloc_prep <= 0 || t_reloc_prep > 10000 ||
                  tx2_reloc_overall <= 0 || tx2_reloc_overall > 20000 ||
                  t_dc_prep <= 0 || t_dc_prep > 10000 ||
                  t_dc_overall <= 0 || t_dc_overall > 20000) {
                LOG_E(X2AP, "timers in configuration file have wrong values. We must have [0 < t_reloc_prep <= 10000] and [0 < tx2_reloc_overall <= 20000] and [0 < t_dc_prep <= 10000] and [0 < t_dc_overall <= 20000]\n");
                exit(1);
              }

              X2AP_REGISTER_ENB_REQ (msg_p).t_reloc_prep = t_reloc_prep;
              X2AP_REGISTER_ENB_REQ (msg_p).tx2_reloc_overall = tx2_reloc_overall;
              X2AP_REGISTER_ENB_REQ (msg_p).t_dc_prep = t_dc_prep;
              X2AP_REGISTER_ENB_REQ (msg_p).t_dc_overall = t_dc_overall;
            }
            // SCTP SETTING
            X2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = SCTP_OUT_STREAMS;
            X2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams  = SCTP_IN_STREAMS;

            if (get_softmodem_params()->sa) {
              sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_SCTP_CONFIG);
              config_get(config_get_if(), SCTPParams, sizeofArray(SCTPParams), aprefix);
              X2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[GNB_SCTP_INSTREAMS_IDX].uptr);
              X2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[GNB_SCTP_OUTSTREAMS_IDX].uptr);
            }

            sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
            // NETWORK_INTERFACES
            config_get(config_get_if(), NETParams, sizeofArray(NETParams), aprefix);
            X2AP_REGISTER_ENB_REQ (msg_p).enb_port_for_X2C = (uint32_t)*(NETParams[GNB_PORT_FOR_X2C_IDX].uptr);

            //temp out
            if ((NETParams[GNB_IPV4_ADDR_FOR_X2C_IDX].strptr == NULL) || (X2AP_REGISTER_ENB_REQ (msg_p).enb_port_for_X2C == 0)) {
              LOG_E(RRC,"Add eNB IPv4 address and/or port for X2C in the CONF file!\n");
              exit(1);
            }

            cidr = *(NETParams[ENB_IPV4_ADDR_FOR_X2C_IDX].strptr);
            char *save = NULL;
            address = strtok_r(cidr, "/", &save);
            X2AP_REGISTER_ENB_REQ (msg_p).enb_x2_ip_address.ipv6 = 0;
            X2AP_REGISTER_ENB_REQ (msg_p).enb_x2_ip_address.ipv4 = 1;
            strcpy(X2AP_REGISTER_ENB_REQ (msg_p).enb_x2_ip_address.ipv4_address, address);
          }
        }
      }
    }
  }

  return 0;
}

void wait_f1_setup_response(void)
{
  gNB_MAC_INST *mac = RC.nrmac[0];
  NR_SCHED_LOCK(&mac->sched_lock);
  if (mac->f1_config.setup_resp != NULL) {
    NR_SCHED_UNLOCK(&mac->sched_lock);
    return;
  }

  LOG_W(GNB_APP, "waiting for F1 Setup Response before activating radio\n");

  /* for the moment, we keep it simple and just sleep to periodically check.
   * The actual check is protected by a mutex */
  while (mac->f1_config.setup_resp == NULL) {
    NR_SCHED_UNLOCK(&mac->sched_lock);
    sleep(1);
    NR_SCHED_LOCK(&mac->sched_lock);
  }
  NR_SCHED_UNLOCK(&mac->sched_lock);
}
static bool check_plmn_identity(const f1ap_plmn_t *check_plmn, const f1ap_plmn_t *plmn)
{
  return plmn->mcc == check_plmn->mcc && plmn->mnc_digit_length == check_plmn->mnc_digit_length && plmn->mnc == check_plmn->mnc;
}

int gNB_app_handle_f1ap_gnb_cu_configuration_update(f1ap_gnb_cu_configuration_update_t *gnb_cu_cfg_update) {
  int i, j, ret=0;
  LOG_I(GNB_APP, "cells_to_activate %d, RRC instances %d\n",
        gnb_cu_cfg_update->num_cells_to_activate, RC.nb_nr_inst);

  AssertFatal(gnb_cu_cfg_update->num_cells_to_activate == 1, "only one cell supported at the moment\n");
  AssertFatal(RC.nb_nr_inst == 1, "expected one instance\n");
  gNB_MAC_INST *mac = RC.nrmac[0];
  NR_SCHED_LOCK(&mac->sched_lock);
  for (j = 0; j < gnb_cu_cfg_update->num_cells_to_activate; j++) {
    for (i = 0; i < RC.nb_nr_inst; i++) {
      f1ap_setup_req_t *setup_req = RC.nrmac[i]->f1_config.setup_req;
      // identify local index of cell j by nr_cellid, plmn identity and physical cell ID

      if (setup_req->cell[0].info.nr_cellid == gnb_cu_cfg_update->cells_to_activate[j].nr_cellid
          && check_plmn_identity(&setup_req->cell[0].info.plmn, &gnb_cu_cfg_update->cells_to_activate[j].plmn) > 0
          && setup_req->cell[0].info.nr_pci == gnb_cu_cfg_update->cells_to_activate[j].nrpci) {
        // copy system information and decode it
        AssertFatal(gnb_cu_cfg_update->cells_to_activate[j].num_SI == 0,
                    "gNB-CU Configuration Update: handling of additional SIs not implemend\n");
        ret++;
        mac->f1_config.setup_resp = malloc(sizeof(*mac->f1_config.setup_resp));
        AssertFatal(mac->f1_config.setup_resp != NULL, "out of memory\n");
        mac->f1_config.setup_resp->gNB_CU_name = gnb_cu_cfg_update->gNB_CU_name;
        mac->f1_config.setup_resp->num_cells_to_activate = gnb_cu_cfg_update->num_cells_to_activate;
        mac->f1_config.setup_resp->cells_to_activate[0] = gnb_cu_cfg_update->cells_to_activate[0];
      } else {
        LOG_E(GNB_APP, "GNB_CU_CONFIGURATION_UPDATE not matching\n");
      }
    }
  }
  NR_SCHED_UNLOCK(&mac->sched_lock);
  MessageDef *msg_ack_p = NULL;
  if (ret > 0) {
    // generate gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE
    msg_ack_p = itti_alloc_new_message (TASK_GNB_APP, 0, F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE);
    F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(msg_ack_p).num_cells_failed_to_be_activated = 0;
    F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(msg_ack_p).have_criticality = 0; 
    F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(msg_ack_p).noofTNLAssociations_to_setup =0;
    F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(msg_ack_p).noofTNLAssociations_failed = 0;
    F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(msg_ack_p).noofDedicatedSIDeliveryNeededUEs = 0;
    itti_send_msg_to_task (TASK_DU_F1, INSTANCE_DEFAULT, msg_ack_p);

  }
  else {
    // generate gNB_CU_CONFIGURATION_UPDATE_FAILURE
    msg_ack_p = itti_alloc_new_message (TASK_GNB_APP, 0, F1AP_GNB_CU_CONFIGURATION_UPDATE_FAILURE);
    F1AP_GNB_CU_CONFIGURATION_UPDATE_FAILURE(msg_ack_p).cause = F1AP_CauseRadioNetwork_cell_not_available;

    itti_send_msg_to_task (TASK_DU_F1, INSTANCE_DEFAULT, msg_ack_p);

  }

  return(ret);
}

ngran_node_t get_node_type(void)
{
  paramdef_t        MacRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t   MacRLC_ParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};
  paramdef_t        GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t   GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};
  paramdef_t        GNBE1Params[] = GNBE1PARAMS_DESC;
  paramlist_def_t   GNBE1ParamList = {GNB_CONFIG_STRING_E1_PARAMETERS, NULL, 0};

  config_getlist(config_get_if(), &GNBParamList, GNBParams, sizeofArray(GNBParams), NULL);

  if (GNBParamList.numelt == 0) // We have no valid configuration, let's return a default 
    return ngran_gNB;

  config_getlist(config_get_if(), &MacRLC_ParamList, MacRLC_Params, sizeofArray(MacRLC_Params), NULL);
  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);
  config_getlist(config_get_if(), &GNBE1ParamList, GNBE1Params, sizeofArray(GNBE1Params), aprefix);
  if ( MacRLC_ParamList.numelt > 0) {
    RC.nb_nr_macrlc_inst = MacRLC_ParamList.numelt; 
    for (int j = 0; j < RC.nb_nr_macrlc_inst; j++) {
      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "f1") == 0) {
        return ngran_gNB_DU; // MACRLCs present in config: it must be a DU
      }
    }
  }

  if (strcmp(*(GNBParamList.paramarray[0][GNB_TRANSPORT_S_PREFERENCE_IDX].strptr), "f1") == 0) {
    if ( GNBE1ParamList.paramarray == NULL || GNBE1ParamList.numelt == 0 )
      return ngran_gNB_CU;
    else if (strcmp(*(GNBE1ParamList.paramarray[0][GNB_CONFIG_E1_CU_TYPE_IDX].strptr), "cp") == 0)
      return ngran_gNB_CUCP;
    else if (strcmp(*(GNBE1ParamList.paramarray[0][GNB_CONFIG_E1_CU_TYPE_IDX].strptr), "up") == 0)
      return ngran_gNB_CUUP;
    else
      return ngran_gNB_CU;
  } else {
    return ngran_gNB;
  }
}
