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
#include "common/utils/LOG/log_extern.h"
#include "assertions.h"
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
// #include "SystemInformationBlockType2.h"
// #include "LAYER2/MAC/extern.h"
// #include "LAYER2/MAC/proto.h"
#include "PHY/phy_extern.h"
#include "PHY/INIT/phy_init.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"
#include "nfapi_vnf.h"
#include "nfapi_pnf.h"

//#include "L1_paramdef.h"
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

#include "RRC/NR/MESSAGES/asn1_msg.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp.h"

extern uint16_t sf_ahead;
int macrlc_has_f1 = 0;
extern ngran_node_t node_type;

extern int config_check_band_frequencies(int ind, int16_t band, uint64_t downlink_frequency,
                                         int32_t uplink_frequency_offset, uint32_t  frame_type);

void prepare_scc(NR_ServingCellConfigCommon_t *scc) {

  NR_FreqBandIndicatorNR_t                        *dl_frequencyBandList,*ul_frequencyBandList;
  struct NR_SCS_SpecificCarrier                   *dl_scs_SpecificCarrierList,*ul_scs_SpecificCarrierList;
  struct NR_PDSCH_TimeDomainResourceAllocation    *bwp_dl_timedomainresourceallocation;
  struct NR_PUSCH_TimeDomainResourceAllocation    *pusch_configcommontimedomainresourceallocation;
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

  ASN_SEQUENCE_ADD(&scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list,dl_frequencyBandList);  
  ASN_SEQUENCE_ADD(&scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list,dl_scs_SpecificCarrierList);		   		   
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
  //
  for (int i=0;i<NR_maxNrofDL_Allocations;i++) {
    bwp_dl_timedomainresourceallocation = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
    bwp_dl_timedomainresourceallocation->k0  = CALLOC(1,sizeof(long));
    *bwp_dl_timedomainresourceallocation->k0=0;
    ASN_SEQUENCE_ADD(&scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,
		     bwp_dl_timedomainresourceallocation);
  }

  ul_frequencyBandList              = CALLOC(1,sizeof(NR_FreqBandIndicatorNR_t));
  scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList          = CALLOC(1,sizeof(struct NR_MultiFrequencyBandListNR));
  ASN_SEQUENCE_ADD(&scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list,ul_frequencyBandList);

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

  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->groupHoppingEnabledTransformPrecoding = CALLOC(1,sizeof(long));
  
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
  ASN_SEQUENCE_ADD(&scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list,ul_scs_SpecificCarrierList);

  for (int i=0;i<NR_maxNrofUL_Allocations;i++) {
    pusch_configcommontimedomainresourceallocation      = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
    pusch_configcommontimedomainresourceallocation->k2  = CALLOC(1,sizeof(long));
    *pusch_configcommontimedomainresourceallocation->k2=0;
    ASN_SEQUENCE_ADD(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,pusch_configcommontimedomainresourceallocation); 
  }
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


void fix_scc(NR_ServingCellConfigCommon_t *scc,uint64_t ssbmap) {

  int ssbmaplen = (int)scc->ssb_PositionsInBurst->present;
  uint8_t curr_bit;

  AssertFatal(ssbmaplen==NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_shortBitmap || ssbmaplen==NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap || ssbmaplen==NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_longBitmap, "illegal ssbmaplen %d\n",ssbmaplen);

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

  // fix UL absolute frequency
  if ((int)*scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA==-1) {
     free(scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA);
     scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA = NULL;
  }

  // default value for msg3 precoder is NULL (0 means enabled)
  if (*scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder!=0)
    scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder = NULL;

  // fix DL and UL Allocation lists
  
  for (int i=scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count-1;i>=0;i--) {
    printf("Checking element %d : %ld\n",i,*scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0);
    if (*scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0>32) {
      printf("removing pdsch_TimeDomainAllocationList element %d\n",i);
      free(scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0);
      asn_sequence_del(&scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,i,1); 
      printf("List size now %d\n",scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count);
    }
  }

  for (int i=scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.count-1;i>=0;i--) {
    if (*scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[i]->k2>32) {
      free(scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[i]->k2);
      asn_sequence_del(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,i,1); 
    }
  }
  if (scc->tdd_UL_DL_ConfigurationCommon->pattern2->dl_UL_TransmissionPeriodicity > 320 ) {
    free(scc->tdd_UL_DL_ConfigurationCommon->pattern2);
    scc->tdd_UL_DL_ConfigurationCommon->pattern2=NULL;
  }

  if ((int)*scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing == -1) {
    free(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing);
    scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing=NULL;
  }

  // check pucch_ResourceConfig
  AssertFatal(*scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon < 2,
	      "pucch_ResourceConfig should be 0 or 1 for now\n");
}

/* Function to allocate dedicated serving cell config strutures */
void prepare_scd(NR_ServingCellConfig_t *scd) {
  // Allocate downlink structures

  scd->downlinkBWP_ToAddModList = CALLOC(1, sizeof(*scd->downlinkBWP_ToAddModList));

  // Downlink bandwidth part
  NR_BWP_Downlink_t *bwp = calloc(1, sizeof(*bwp));
  bwp->bwp_Id = 1;

  // Allocate downlink dedicated bandwidth part and PDSCH structures
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
    ASN_SEQUENCE_ADD(&NR_PTRS_DownlinkCfg->frequencyDensity->list, &dl_rbs[i]);
  }
  NR_PTRS_DownlinkCfg->timeDensity = CALLOC(1, sizeof(*NR_PTRS_DownlinkCfg->timeDensity));
  long *dl_mcs = CALLOC(3, sizeof(long));
  for (int i=0;i<3;i++) {
    ASN_SEQUENCE_ADD(&NR_PTRS_DownlinkCfg->timeDensity->list, &dl_mcs[i]);
  }
  NR_PTRS_DownlinkCfg->epre_Ratio = CALLOC(1, sizeof(*NR_PTRS_DownlinkCfg->epre_Ratio));
  NR_PTRS_DownlinkCfg->resourceElementOffset = CALLOC(1, sizeof(*NR_PTRS_DownlinkCfg->resourceElementOffset));
  *NR_PTRS_DownlinkCfg->resourceElementOffset = 0;
  ASN_SEQUENCE_ADD(&scd->downlinkBWP_ToAddModList->list,bwp);

  // Allocate uplink structures

  scd->uplinkConfig = CALLOC(1, sizeof(*scd->uplinkConfig));
  scd->uplinkConfig->uplinkBWP_ToAddModList = CALLOC(1, sizeof(*scd->uplinkConfig->uplinkBWP_ToAddModList));

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
    ASN_SEQUENCE_ADD(&NR_PTRS_UplinkConfig->transformPrecoderDisabled->frequencyDensity->list, &n_rbs[i]);
  }
  NR_PTRS_UplinkConfig->transformPrecoderDisabled->timeDensity = CALLOC(1, sizeof(*NR_PTRS_UplinkConfig->transformPrecoderDisabled->timeDensity));
  long *ptrs_mcs = CALLOC(3, sizeof(long));
  for (int i = 0; i < 3; i++) {
    ASN_SEQUENCE_ADD(&NR_PTRS_UplinkConfig->transformPrecoderDisabled->timeDensity->list, &ptrs_mcs[i]);
  }
  NR_PTRS_UplinkConfig->transformPrecoderDisabled->resourceElementOffset = CALLOC(1, sizeof(*NR_PTRS_UplinkConfig->transformPrecoderDisabled->resourceElementOffset));
  *NR_PTRS_UplinkConfig->transformPrecoderDisabled->resourceElementOffset = 0;

  // UL bandwidth part
  NR_BWP_Uplink_t *ubwp = CALLOC(1, sizeof(*ubwp));
  ubwp->bwp_Id = 1;
  ubwp->bwp_Dedicated = CALLOC(1, sizeof(*ubwp->bwp_Dedicated));

  ubwp->bwp_Dedicated->pusch_Config = CALLOC(1, sizeof(*ubwp->bwp_Dedicated->pusch_Config));
  ubwp->bwp_Dedicated->pusch_Config->present = NR_SetupRelease_PUSCH_Config_PR_setup;
  ubwp->bwp_Dedicated->pusch_Config->choice.setup = pusch_Config;

  ASN_SEQUENCE_ADD(&scd->uplinkConfig->uplinkBWP_ToAddModList->list,ubwp);
}

/* This function checks dedicated serving cell configuration and performs fixes as needed */ 
void fix_scd(NR_ServingCellConfig_t *scd) {
  // Check for DL PTRS parameters validity
  if (scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS) {
    // If any of the frequencyDensity values are not set or are out of bounds, PTRS is assumed to be not present
    for (int i = scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity->list.count-1; i >= 0; i--) {
      if ((*scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity->list.array[i] < 1)
          || (*scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->frequencyDensity->list.array[i] > 276)) {
        LOG_I(RRC, "DL PTRS frequencyDensity %d not set. Assuming PTRS not present! \n", i);
        free(scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS);
        scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS = NULL;
        break;
      }
    }
  }
  if (scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS) {
    // If any of the timeDensity values are not set or are out of bounds, PTRS is assumed to be not present
    for (int i = scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity->list.count-1; i >= 0; i--) {
      if ((*scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity->list.array[i] < 0)
          || (*scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->timeDensity->list.array[i] > 29)) {
        LOG_I(RRC, "DL PTRS timeDensity %d not set. Assuming PTRS not present! \n", i);
        free(scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS);
        scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS = NULL;
        break;
      }
    }
  }
  if (scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS) {
    if (*scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->resourceElementOffset > 2) {
      LOG_I(RRC, "Freeing DL PTRS resourceElementOffset \n");
      free(scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->resourceElementOffset);
      scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->resourceElementOffset = NULL;
    }
    if (*scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->epre_Ratio > 1) {
      LOG_I(RRC, "Freeing DL PTRS epre_Ratio \n");
      free(scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->epre_Ratio);
      scd->downlinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->phaseTrackingRS->choice.setup->epre_Ratio = NULL;
    }
  }

  if (scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS) {
    // If any of the frequencyDensity values are not set or are out of bounds, PTRS is assumed to be not present
    for (int i = scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.count-1; i >= 0; i--) {
      if ((*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.array[i] < 1) 
          || (*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.array[i] > 276)) {
        LOG_I(RRC, "UL PTRS frequencyDensity %d not set. Assuming PTRS not present! \n", i);
        free(scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS);
        scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS = NULL;
        break;
      }
    }
  }

  if (scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS) {
    // If any of the timeDensity values are not set or are out of bounds, PTRS is assumed to be not present
    for (int i = scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.count-1; i >= 0; i--) {
      if ((*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[i] < 0)
          || (*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[i] > 29)) {
        LOG_I(RRC, "UL PTRS timeDensity %d not set. Assuming PTRS not present! \n", i);
        free(scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS);
        scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS = NULL;
        break;
      }
    }
  }
  if (scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS) {
    // Check for UL PTRS parameters validity
    if (*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->resourceElementOffset > 2) {
      LOG_I(RRC, "Freeing UL PTRS resourceElementOffset \n");
      free(scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->resourceElementOffset);
      scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->resourceElementOffset = NULL;
    }
  }
}

void RCconfig_nr_flexran()
{
  uint16_t  i;
  uint16_t  num_gnbs;
  char      aprefix[MAX_OPTNAME_SIZE*2 + 8];
  /* this will possibly truncate the cell id (RRC assumes int32_t).
   * Both Nid_cell and gnb_id are signed in RRC case, but we use unsigned for
   * the bitshifting to work properly */
  uint16_t  Nid_cell_tr = 0;
  uint32_t  gnb_id = 0;


  /* get number of gNBs */
  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  config_get(GNBSParams, sizeof(GNBSParams)/sizeof(paramdef_t), NULL);
  num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;

  /* for gNB ID */
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST, NULL, 0};

  paramdef_t flexranParams[] = FLEXRANPARAMS_DESC;
  config_get(flexranParams, sizeof(flexranParams)/sizeof(paramdef_t), CONFIG_STRING_NETWORK_CONTROLLER_CONFIG);

  if (!RC.flexran) {
    RC.flexran = calloc(num_gnbs, sizeof(flexran_agent_info_t*));
    AssertFatal(RC.flexran,
                "can't ALLOCATE %zu Bytes for %d flexran agent info with size %zu\n",
                num_gnbs * sizeof(flexran_agent_info_t*),
                num_gnbs, sizeof(flexran_agent_info_t*));
  }

  for (i = 0; i < num_gnbs; i++) {
    RC.flexran[i] = calloc(1, sizeof(flexran_agent_info_t));
    AssertFatal(RC.flexran[i],
                "can't ALLOCATE %zu Bytes for flexran agent info (iteration %d/%d)\n",
                sizeof(flexran_agent_info_t), i + 1, num_gnbs);
    /* if config says "yes", enable Agent, in all other cases it's like "no" */
    RC.flexran[i]->enabled          = strcasecmp(*(flexranParams[FLEXRAN_ENABLED].strptr), "yes") == 0;
    /* if not enabled, simply skip the rest, it is not needed anyway */
    if (!RC.flexran[i]->enabled)
      continue;
    RC.flexran[i]->interface_name   = strdup(*(flexranParams[FLEXRAN_INTERFACE_NAME_IDX].strptr));
    //inet_ntop(AF_INET, &(enb_properties->properties[mod_id]->flexran_agent_ipv4_address), in_ip, INET_ADDRSTRLEN);
    RC.flexran[i]->remote_ipv4_addr = strdup(*(flexranParams[FLEXRAN_IPV4_ADDRESS_IDX].strptr));
    RC.flexran[i]->remote_port      = *(flexranParams[FLEXRAN_PORT_IDX].uptr);
    RC.flexran[i]->cache_name       = strdup(*(flexranParams[FLEXRAN_CACHE_IDX].strptr));
    RC.flexran[i]->node_ctrl_state  = strcasecmp(*(flexranParams[FLEXRAN_AWAIT_RECONF_IDX].strptr), "yes") == 0 ? ENB_WAIT : ENB_NORMAL_OPERATION;

    config_getlist(&GNBParamList, GNBParams, sizeof(GNBParams)/sizeof(paramdef_t),NULL);
    /* gNB ID from configuration, as read in by RCconfig_RRC() */
    if (!GNBParamList.paramarray[i][GNB_GNB_ID_IDX].uptr) {
      // Calculate a default gNB ID
    if (AMF_MODE_ENABLED) 
      gnb_id = i + (ngap_generate_gNB_id () & 0xFFFFFF8);
    else
      gnb_id = i;
    } else {
        gnb_id = *(GNBParamList.paramarray[i][GNB_GNB_ID_IDX].uptr);
    }

    /* cell ID */
    sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, i);

    RC.flexran[i]->mod_id   = i;
    RC.flexran[i]->agent_id = (((uint64_t)i) << 48) | (((uint64_t)gnb_id) << 16) | ((uint64_t)Nid_cell_tr);

    /*
     * Assume for the moment the monolithic case, i.e. agent can provide information for all layers
     * Consider using uint16_t flexran_get_capabilities_mask(mid_t mod_id),
     *                    with RC.rrc[mod_id]->node_type = ngran_gNB
     */
    RC.flexran[i]->capability_mask = (1 << PROTOCOL__FLEX_BS_CAPABILITY__LOPHY)
    		                       | (1 << PROTOCOL__FLEX_BS_CAPABILITY__HIPHY)
								   | (1 << PROTOCOL__FLEX_BS_CAPABILITY__LOMAC)
								   | (1 << PROTOCOL__FLEX_BS_CAPABILITY__HIMAC)
								   | (1 << PROTOCOL__FLEX_BS_CAPABILITY__RLC)
								   | (1 << PROTOCOL__FLEX_BS_CAPABILITY__PDCP)
								   | (1 << PROTOCOL__FLEX_BS_CAPABILITY__SDAP)
								   | (1 << PROTOCOL__FLEX_BS_CAPABILITY__RRC);

  }
}

void RCconfig_NR_L1(void) {
  int j;
  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  ////////// Identification parameters
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;

  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};

  config_get( GNBSParams,sizeof(GNBSParams)/sizeof(paramdef_t),NULL); 
  int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal (num_gnbs > 0,"Failed to parse config file no gnbs %s \n",GNB_CONFIG_STRING_ACTIVE_GNBS);

  config_getlist( &GNBParamList,GNBParams,sizeof(GNBParams)/sizeof(paramdef_t),NULL); 
  char *ulprbbl = *GNBParamList.paramarray[0][GNB_ULPRBBLACKLIST_IDX].strptr; 
  if (ulprbbl) LOG_I(NR_PHY,"PRB blacklist %s\n",ulprbbl);
  char *pt = strtok(ulprbbl,",");
  int prbbl[275];
  int num_prbbl=0;
  memset(prbbl,0,275*sizeof(int));

  while (pt) {
    prbbl[atoi(pt)] = 1;
    LOG_I(NR_PHY,"Blacklisting prb %d\n",atoi(pt));
    pt = strtok(NULL,",");
    num_prbbl++;
  }

  
  paramdef_t L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t L1_ParamList = {CONFIG_STRING_L1_LIST,NULL,0};


  if (RC.gNB == NULL) {
    RC.gNB                       = (PHY_VARS_gNB **)malloc((1+NUMBER_OF_gNB_MAX)*sizeof(PHY_VARS_gNB*));
    LOG_I(NR_PHY,"RC.gNB = %p\n",RC.gNB);
    memset(RC.gNB,0,(1+NUMBER_OF_gNB_MAX)*sizeof(PHY_VARS_gNB*));
  }

  config_getlist( &L1_ParamList,L1_Params,sizeof(L1_Params)/sizeof(paramdef_t), NULL);

  if (L1_ParamList.numelt > 0) {

    for (j = 0; j < RC.nb_nr_L1_inst; j++) {

      if (RC.gNB[j] == NULL) {
        RC.gNB[j]                       = (PHY_VARS_gNB *)malloc(sizeof(PHY_VARS_gNB));
        LOG_I(NR_PHY,"RC.gNB[%d] = %p\n",j,RC.gNB[j]);
        memset(RC.gNB[j],0,sizeof(PHY_VARS_gNB));
	      RC.gNB[j]->Mod_id  = j;
      }

      RC.gNB[j]->pusch_proc_threads = *(L1_ParamList.paramarray[j][L1_PUSCH_PROC_THREADS].uptr);
      RC.gNB[j]->ofdm_offset_divisor = *(L1_ParamList.paramarray[j][L1_OFDM_OFFSET_DIVISOR].uptr);
      RC.gNB[j]->pucch0_thres       = *(L1_ParamList.paramarray[j][L1_PUCCH0_DTX_THRESHOLD].uptr);
      RC.gNB[j]->prach_thres        = *(L1_ParamList.paramarray[j][L1_PRACH_DTX_THRESHOLD].uptr);
      RC.gNB[j]->pusch_thres        = *(L1_ParamList.paramarray[j][L1_PUSCH_DTX_THRESHOLD].uptr);
      RC.gNB[j]->num_ulprbbl        = num_prbbl;
      LOG_I(NR_PHY,"Copying %d blacklisted PRB to L1 context\n",num_prbbl);
      memcpy(RC.gNB[j]->ulprbbl,prbbl,275*sizeof(int));
      if(strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_mac") == 0) {
        //sf_ahead = 2; // Need 4 subframe gap between RX and TX
      }else if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        RC.gNB[j]->eth_params_n.local_if_name            = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
        RC.gNB[j]->eth_params_n.my_addr                  = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
        RC.gNB[j]->eth_params_n.remote_addr              = strdup(*(L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
        RC.gNB[j]->eth_params_n.my_portc                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
        RC.gNB[j]->eth_params_n.remote_portc             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
        RC.gNB[j]->eth_params_n.my_portd                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
        RC.gNB[j]->eth_params_n.remote_portd             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
        RC.gNB[j]->eth_params_n.transp_preference        = ETH_UDP_MODE;

        //sf_ahead = 2; // Cannot cope with 4 subframes betweem RX and TX - set it to 2

        RC.nb_nr_macrlc_inst = 1;  // This is used by mac_top_init_gNB()

        // This is used by init_gNB_afterRU()
        RC.nb_nr_CC = (int *)malloc((1+RC.nb_nr_inst)*sizeof(int));
        RC.nb_nr_CC[0]=1;

        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_nr_inst=1 this is because phy_init_RU() uses that to index and not RC.num_gNB - why the 2 similar variables?\n", __FUNCTION__);
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_nr_CC[0]=%d for init_gNB_afterRU()\n", __FUNCTION__, RC.nb_nr_CC[0]);
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_nr_macrlc_inst:%d because used by mac_top_init_gNB()\n", __FUNCTION__, RC.nb_nr_macrlc_inst);

        mac_top_init_gNB();

        configure_nr_nfapi_pnf(RC.gNB[j]->eth_params_n.remote_addr,
                               RC.gNB[j]->eth_params_n.remote_portc,
                               RC.gNB[j]->eth_params_n.my_addr,
                               RC.gNB[j]->eth_params_n.my_portd,
                               RC.gNB[j]->eth_params_n.remote_portd);
      }else { // other midhaul
      } 
    }// for (j = 0; j < RC.nb_nr_L1_inst; j++)
    printf("Initializing northbound interface for L1\n");
    l1_north_init_gNB();
  }else{
    LOG_I(PHY,"No " CONFIG_STRING_L1_LIST " configuration found");    

    // DJP need to create some structures for VNF

    j = 0;


    if (RC.gNB[j] == NULL) {
      RC.gNB[j] = (PHY_VARS_gNB *)malloc(sizeof(PHY_VARS_gNB));
      memset((void*)RC.gNB[j],0,sizeof(PHY_VARS_gNB));
      LOG_I(PHY,"RC.gNB[%d] = %p\n",j,RC.gNB[j]);
      RC.gNB[j]->Mod_id  = j;
    }   
  }
}

void RCconfig_nr_macrlc() {
  int               j;

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  ////////// Identification parameters
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;

  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};

  config_get( GNBSParams,sizeof(GNBSParams)/sizeof(paramdef_t),NULL); 
  int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal (num_gnbs > 0,"Failed to parse config file no gnbs %s \n",GNB_CONFIG_STRING_ACTIVE_GNBS);
  
  config_getlist( &GNBParamList,GNBParams,sizeof(GNBParams)/sizeof(paramdef_t),NULL); 
  char *ulprbbl = *GNBParamList.paramarray[0][GNB_ULPRBBLACKLIST_IDX].strptr; 
  char *pt = strtok(ulprbbl,",");
  int prbbl[275];
  int num_prbbl=0;
  int prb;
  memset(prbbl,0,275*sizeof(int));
  while (pt) {
    prb=atoi(pt); 
    prbbl[prb] = 1;
    pt = strtok(NULL,",");
    num_prbbl++;
  }
  
  paramdef_t MacRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t MacRLC_ParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};

  config_getlist( &MacRLC_ParamList,MacRLC_Params,sizeof(MacRLC_Params)/sizeof(paramdef_t), NULL);    
  
  if ( MacRLC_ParamList.numelt > 0) {

    RC.nb_nr_macrlc_inst=MacRLC_ParamList.numelt; 
    mac_top_init_gNB();   
    RC.nb_nr_mac_CC = (int*)malloc(RC.nb_nr_macrlc_inst*sizeof(int));

    for (j=0;j<RC.nb_nr_macrlc_inst;j++) {
      RC.nb_nr_mac_CC[j] = *(MacRLC_ParamList.paramarray[j][MACRLC_CC_IDX].iptr);
      RC.nrmac[j]->pusch_target_snrx10                   = *(MacRLC_ParamList.paramarray[j][MACRLC_PUSCHTARGETSNRX10_IDX].iptr);
      RC.nrmac[j]->pucch_target_snrx10                   = *(MacRLC_ParamList.paramarray[j][MACRLC_PUCCHTARGETSNRX10_IDX].iptr);
      RC.nrmac[j]->pucch_failure_thres                   = *(MacRLC_ParamList.paramarray[j][MACRLC_PUCCHFAILURETHRES_IDX].iptr);
      RC.nrmac[j]->pusch_failure_thres                   = *(MacRLC_ParamList.paramarray[j][MACRLC_PUSCHFAILURETHRES_IDX].iptr);
      
      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_RRC") == 0) {
  // check number of instances is same as RRC/PDCP
  
      }else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "f1") == 0) {
        printf("Configuring F1 interfaces for MACRLC\n");
        RC.nrmac[j]->eth_params_n.local_if_name            = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_IF_NAME_IDX].strptr));
        RC.nrmac[j]->eth_params_n.my_addr                  = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_n.remote_addr              = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_n.my_portc                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_n.remote_portc             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_n.my_portd                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTD_IDX].iptr);
        RC.nrmac[j]->eth_params_n.remote_portd             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTD_IDX].iptr);;
        RC.nrmac[j]->eth_params_n.transp_preference        = ETH_UDP_MODE;
        macrlc_has_f1                                      = 1;
      }else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "cudu") == 0) {
        RC.nrmac[j]->eth_params_n.local_if_name            = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_IF_NAME_IDX].strptr));
        RC.nrmac[j]->eth_params_n.my_addr                  = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_n.remote_addr              = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_n.my_portc                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_n.remote_portc             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_n.my_portd                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTD_IDX].iptr);
        RC.nrmac[j]->eth_params_n.remote_portd             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTD_IDX].iptr);;
        RC.nrmac[j]->eth_params_n.transp_preference        = ETH_UDP_MODE;
      }else { // other midhaul
        AssertFatal(1==0,"MACRLC %d: %s unknown northbound midhaul\n",j, *(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr));
      } 

      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_L1") == 0) {

      }else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        RC.nrmac[j]->eth_params_s.local_if_name            = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_IF_NAME_IDX].strptr));
        RC.nrmac[j]->eth_params_s.my_addr                  = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_s.remote_addr              = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_ADDRESS_IDX].strptr));
        RC.nrmac[j]->eth_params_s.my_portc                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_s.remote_portc             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTC_IDX].iptr);
        RC.nrmac[j]->eth_params_s.my_portd                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTD_IDX].iptr);
        RC.nrmac[j]->eth_params_s.remote_portd             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTD_IDX].iptr);
        RC.nrmac[j]->eth_params_s.transp_preference        = ETH_UDP_MODE;

        printf("**************** vnf_port:%d\n", RC.nrmac[j]->eth_params_s.my_portc);
        configure_nr_nfapi_vnf(RC.nrmac[j]->eth_params_s.my_addr, RC.nrmac[j]->eth_params_s.my_portc);
        printf("**************** RETURNED FROM configure_nfapi_vnf() vnf_port:%d\n", RC.nrmac[j]->eth_params_s.my_portc);
      }else { // other midhaul
        AssertFatal(1==0,"MACRLC %d: %s unknown southbound midhaul\n",j,*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr));
      } 
      RC.nrmac[j]->ulsch_max_slots_inactivity = *(MacRLC_ParamList.paramarray[j][MACRLC_ULSCH_MAX_SLOTS_INACTIVITY].uptr);
      RC.nrmac[j]->num_ulprbbl = num_prbbl;
      LOG_I(NR_MAC,"Blacklisted PRBS %d\n",num_prbbl);
      memcpy(RC.nrmac[j]->ulprbbl,prbbl,275*sizeof(prbbl[0]));

    }//  for (j=0;j<RC.nb_nr_macrlc_inst;j++)
  }else {// MacRLC_ParamList.numelt > 0
    LOG_E(PHY,"No %s configuration found\n", CONFIG_STRING_MACRLC_LIST);
    // AssertFatal (0,"No " CONFIG_STRING_MACRLC_LIST " configuration found");     
  }

}

void config_security(gNB_RRC_INST *rrc)
{
  paramdef_t logparams_defaults[] = SECURITY_GLOBALPARAMS_DESC;
  int ret = config_get(logparams_defaults,
                       sizeof(logparams_defaults) / sizeof(paramdef_t),
                       CONFIG_STRING_SECURITY);
  int i;

  if (ret < 0) {
    LOG_W(RRC, "configuration file does not contain a \"security\" section, applying default parameters (no security)\n");
    rrc->security.ciphering_algorithms[0]    = 0;  /* nea0 = no ciphering */
    rrc->security.ciphering_algorithms_count = 1;
    rrc->security.integrity_algorithms[0]    = 0;  /* nia0 = no integrity */
    rrc->security.integrity_algorithms_count = 1;
    return;
  }

  if (logparams_defaults[SECURITY_CONFIG_CIPHERING_IDX].numelt > 4) {
    LOG_E(RRC, "too much ciphering algorithms in section \"security\" of the configuration file, maximum is 4\n");
    exit(1);
  }
  if (logparams_defaults[SECURITY_CONFIG_INTEGRITY_IDX].numelt > 4) {
    LOG_E(RRC, "too much integrity algorithms in section \"security\" of the configuration file, maximum is 4\n");
    exit(1);
  }

  /* get ciphering algorithms */
  rrc->security.ciphering_algorithms_count = 0;
  for (i = 0; i < logparams_defaults[SECURITY_CONFIG_CIPHERING_IDX].numelt; i++) {
    if (!strcmp(logparams_defaults[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i], "nea0")) {
      rrc->security.ciphering_algorithms[rrc->security.ciphering_algorithms_count] = 0;
      rrc->security.ciphering_algorithms_count++;
      continue;
    }
    if (!strcmp(logparams_defaults[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i], "nea1")) {
      rrc->security.ciphering_algorithms[rrc->security.ciphering_algorithms_count] = 1;
      rrc->security.ciphering_algorithms_count++;
      continue;
    }
    if (!strcmp(logparams_defaults[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i], "nea2")) {
      rrc->security.ciphering_algorithms[rrc->security.ciphering_algorithms_count] = 2;
      rrc->security.ciphering_algorithms_count++;
      continue;
    }
    if (!strcmp(logparams_defaults[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i], "nea3")) {
      rrc->security.ciphering_algorithms[rrc->security.ciphering_algorithms_count] = 3;
      rrc->security.ciphering_algorithms_count++;
      continue;
    }
    LOG_E(RRC, "unknown ciphering algorithm \"%s\" in section \"security\" of the configuration file\n",
          logparams_defaults[SECURITY_CONFIG_CIPHERING_IDX].strlistptr[i]);
    exit(1);
  }

  /* get integrity algorithms */
  rrc->security.integrity_algorithms_count = 0;
  for (i = 0; i < logparams_defaults[SECURITY_CONFIG_INTEGRITY_IDX].numelt; i++) {
    if (!strcmp(logparams_defaults[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i], "nia0")) {
      rrc->security.integrity_algorithms[rrc->security.integrity_algorithms_count] = 0;
      rrc->security.integrity_algorithms_count++;
      continue;
    }
    if (!strcmp(logparams_defaults[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i], "nia1")) {
      rrc->security.integrity_algorithms[rrc->security.integrity_algorithms_count] = 1;
      rrc->security.integrity_algorithms_count++;
      continue;
    }
    if (!strcmp(logparams_defaults[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i], "nia2")) {
      rrc->security.integrity_algorithms[rrc->security.integrity_algorithms_count] = 2;
      rrc->security.integrity_algorithms_count++;
      continue;
    }
    if (!strcmp(logparams_defaults[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i], "nia3")) {
      rrc->security.integrity_algorithms[rrc->security.integrity_algorithms_count] = 3;
      rrc->security.integrity_algorithms_count++;
      continue;
    }
    LOG_E(RRC, "unknown integrity algorithm \"%s\" in section \"security\" of the configuration file\n",
          logparams_defaults[SECURITY_CONFIG_INTEGRITY_IDX].strlistptr[i]);
    exit(1);
  }

  if (rrc->security.ciphering_algorithms_count == 0) {
    LOG_W(RRC, "no preferred ciphering algorithm set in configuration file, applying default parameters (no security)\n");
    rrc->security.ciphering_algorithms[0]    = 0;  /* nea0 = no ciphering */
    rrc->security.ciphering_algorithms_count = 1;
  }

  if (rrc->security.integrity_algorithms_count == 0) {
    LOG_W(RRC, "no preferred integrity algorithm set in configuration file, applying default parameters (no security)\n");
    rrc->security.integrity_algorithms[0]    = 0;  /* nia0 = no integrity */
    rrc->security.integrity_algorithms_count = 1;
  }
}

void RCconfig_NRRRC(MessageDef *msg_p, uint32_t i, gNB_RRC_INST *rrc) {

  int                    num_gnbs                                                      = 0;
  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  int32_t                gnb_id                                                        = 0;
  int                    k                                                             = 0;

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  ////////// Identification parameters
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};

  NR_ServingCellConfigCommon_t *scc = calloc(1,sizeof(NR_ServingCellConfigCommon_t));
  uint64_t ssb_bitmap=0xff;
  memset((void*)scc,0,sizeof(NR_ServingCellConfigCommon_t));
  prepare_scc(scc);
  paramdef_t SCCsParams[] = SCCPARAMS_DESC(scc);
  paramlist_def_t SCCsParamList = {GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON, NULL, 0};

  // Serving Cell Config Dedicated
  NR_ServingCellConfig_t *scd = calloc(1,sizeof(NR_ServingCellConfig_t));
  memset((void*)scd,0,sizeof(NR_ServingCellConfig_t));
  prepare_scd(scd);
  paramdef_t SCDsParams[] = SCDPARAMS_DESC(scd);
  paramlist_def_t SCDsParamList = {GNB_CONFIG_STRING_SERVINGCELLCONFIGDEDICATED, NULL, 0};
   ////////// Physical parameters


  /* get global parameters, defined outside any section in the config file */
 
  config_get( GNBSParams,sizeof(GNBSParams)/sizeof(paramdef_t),NULL); 
  num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal (i<num_gnbs,"Failed to parse config file no %ith element in %s \n",i, GNB_CONFIG_STRING_ACTIVE_GNBS);

  /*
  if (AMF_MODE_ENABLED) {
    if (strcasecmp( *(GNBSParams[GNB_ASN1_VERBOSITY_IDX].strptr), GNB_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0) {
      asn_debug      = 0;
      asn1_xer_print = 0;
    } else if (strcasecmp( *(GNBSParams[GNB_ASN1_VERBOSITY_IDX].strptr), GNB_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0) {
      asn_debug      = 1;
      asn1_xer_print = 1;
    } else if (strcasecmp(*(GNBSParams[GNB_ASN1_VERBOSITY_IDX].strptr) , GNB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0) {
      asn_debug      = 1;
      asn1_xer_print = 2;
    } else {
      asn_debug      = 0;
      asn1_xer_print = 0;
    }
  }
  */

  if (num_gnbs>0) {



    // Output a list of all gNBs. ////////// Identification parameters
    config_getlist( &GNBParamList,GNBParams,sizeof(GNBParams)/sizeof(paramdef_t),NULL); 
    
    if (GNBParamList.paramarray[i][GNB_GNB_ID_IDX].uptr == NULL) {
    // Calculate a default gNB ID
      if (AMF_MODE_ENABLED) { 
        uint32_t hash;
        hash = ngap_generate_gNB_id ();
        gnb_id = i + (hash & 0xFFFFFF8);
      } else {
        gnb_id = i;
      }
    } else {
      gnb_id = *(GNBParamList.paramarray[i][GNB_GNB_ID_IDX].uptr);
    }

    // pdcch_ConfigSIB1
    rrc->carrier.pdcch_ConfigSIB1 = calloc(1,sizeof(NR_PDCCH_ConfigSIB1_t));
    paramdef_t pdcch_ConfigSIB1[] = PDCCH_CONFIGSIB1PARAMS_DESC(rrc->carrier.pdcch_ConfigSIB1);
    paramlist_def_t pdcch_ConfigSIB1ParamList = {GNB_CONFIG_STRING_PDCCH_CONFIGSIB1, NULL, 0};
    sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);
    config_getlist(&pdcch_ConfigSIB1ParamList, NULL, 0, aprefix);
    if (pdcch_ConfigSIB1ParamList.numelt > 0) {
        sprintf(aprefix, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST,0,GNB_CONFIG_STRING_PDCCH_CONFIGSIB1, 0);
        config_get(pdcch_ConfigSIB1,sizeof(pdcch_ConfigSIB1)/sizeof(paramdef_t),aprefix);
    }

    sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);

    config_getlist(&SCCsParamList, NULL, 0, aprefix);
    if (SCCsParamList.numelt > 0) {    
      sprintf(aprefix, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST,0,GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON, 0);
      config_get( SCCsParams,sizeof(SCCsParams)/sizeof(paramdef_t),aprefix);  
      LOG_I(RRC,"Read in ServingCellConfigCommon (PhysCellId %d, ABSFREQSSB %d, DLBand %d, ABSFREQPOINTA %d, DLBW %d,RACH_TargetReceivedPower %d\n",
	    (int)*scc->physCellId,
	    (int)*scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB,
	    (int)*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0],
	    (int)scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
	    (int)scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,
	    (int)scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.preambleReceivedTargetPower);
      fix_scc(scc,ssb_bitmap);
    }

    sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);

    config_getlist(&SCDsParamList, NULL, 0, aprefix);
    if (SCDsParamList.numelt > 0) {    
      sprintf(aprefix, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST,0,GNB_CONFIG_STRING_SERVINGCELLCONFIGDEDICATED, 0);
      config_get( SCDsParams,sizeof(SCDsParams)/sizeof(paramdef_t),aprefix);  
      LOG_I(RRC,"Read in ServingCellConfigDedicated UL (FreqDensity_0 %d, FreqDensity_1 %d, TimeDensity_0 %d, TimeDensity_1 %d, TimeDensity_2 %d, RE offset %d \n",
      (int)*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.array[0],
      (int)*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->frequencyDensity->list.array[1],
      (int)*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[0],
      (int)*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[1],
      (int)*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->timeDensity->list.array[2],
      (int)*scd->uplinkConfig->uplinkBWP_ToAddModList->list.array[0]->bwp_Dedicated->pusch_Config->choice.setup->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->phaseTrackingRS->choice.setup->transformPrecoderDisabled->resourceElementOffset);
    }
    fix_scd(scd);

    printf("NRRRC %d: Southbound Transport %s\n",i,*(GNBParamList.paramarray[i][GNB_TRANSPORT_S_PREFERENCE_IDX].strptr));

    if (strcmp(*(GNBParamList.paramarray[i][GNB_TRANSPORT_S_PREFERENCE_IDX].strptr), "f1") == 0) {
      paramdef_t SCTPParams[]  = GNBSCTPPARAMS_DESC;
      char aprefix[MAX_OPTNAME_SIZE*2 + 8];
      sprintf(aprefix,"%s.[%u].%s",GNB_CONFIG_STRING_GNB_LIST,i,GNB_CONFIG_STRING_SCTP_CONFIG);
      config_get(SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
      rrc->node_id        = *(GNBParamList.paramarray[0][GNB_GNB_ID_IDX].uptr);
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
      rrc->node_type                             = ngran_gNB_CU;
      rrc->sctp_in_streams                       = (uint16_t)*(SCTPParams[GNB_SCTP_INSTREAMS_IDX].uptr);
      rrc->sctp_out_streams                      = (uint16_t)*(SCTPParams[GNB_SCTP_OUTSTREAMS_IDX].uptr);
    } else {
      // set to ngran_gNB for now, it will get set to ngran_gNB_DU if macrlc entity which uses F1 is present
      // Note: we will have to handle the case of ngran_ng_gNB_DU
      if (macrlc_has_f1 == 0) {
        rrc->node_type = ngran_gNB;
        LOG_I(NR_RRC,"Setting node_type to ngran_gNB\n");
      } else {
        rrc->node_type = ngran_gNB_DU;
        LOG_I(NR_RRC,"Setting node_type to ngran_gNB_DU\n");
      }
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
    
    for (k=0; k <num_gnbs ; k++) {
      if (strcmp(GNBSParams[GNB_ACTIVE_GNBS_IDX].strlistptr[k], *(GNBParamList.paramarray[i][GNB_GNB_NAME_IDX].strptr) )== 0) {
	
        char gnbpath[MAX_OPTNAME_SIZE + 8];
        sprintf(gnbpath,"%s.[%i]",GNB_CONFIG_STRING_GNB_LIST,k);

	
        paramdef_t PLMNParams[] = GNBPLMNPARAMS_DESC;

        paramlist_def_t PLMNParamList = {GNB_CONFIG_STRING_PLMN_LIST, NULL, 0};
        /* map parameter checking array instances to parameter definition array instances */
        checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

        for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
          PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

        NRRRC_CONFIGURATION_REQ (msg_p).cell_identity     = gnb_id;
        NRRRC_CONFIGURATION_REQ (msg_p).tac               = *GNBParamList.paramarray[i][GNB_TRACKING_AREA_CODE_IDX].uptr;
        AssertFatal(!GNBParamList.paramarray[i][GNB_MOBILE_COUNTRY_CODE_IDX_OLD].strptr
                    && !GNBParamList.paramarray[i][GNB_MOBILE_NETWORK_CODE_IDX_OLD].strptr,
                    "It seems that you use an old configuration file. Please change the existing\n"
                    "    tracking_area_code  =  \"1\";\n"
                    "    mobile_country_code =  \"208\";\n"
                    "    mobile_network_code =  \"93\";\n"
                    "to\n"
                    "    tracking_area_code  =  1; // no string!!\n"
                    "    plmn_list = ( { mcc = 208; mnc = 93; mnc_length = 2; } )\n");
        config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), gnbpath);

        if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
          AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                      PLMNParamList.numelt);

        NRRRC_CONFIGURATION_REQ(msg_p).num_plmn = PLMNParamList.numelt;

        for (int l = 0; l < PLMNParamList.numelt; ++l) {
	
	  NRRRC_CONFIGURATION_REQ (msg_p).mcc[l]               = *PLMNParamList.paramarray[l][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
	  NRRRC_CONFIGURATION_REQ (msg_p).mnc[l]               = *PLMNParamList.paramarray[l][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
	  NRRRC_CONFIGURATION_REQ (msg_p).mnc_digit_length[l]  = *PLMNParamList.paramarray[l][GNB_MNC_DIGIT_LENGTH].u8ptr;
	  AssertFatal((NRRRC_CONFIGURATION_REQ (msg_p).mnc_digit_length[l] == 2) ||
		      (NRRRC_CONFIGURATION_REQ (msg_p).mnc_digit_length[l] == 3),"BAD MNC DIGIT LENGTH %d",
		      NRRRC_CONFIGURATION_REQ (msg_p).mnc_digit_length[l]);
	}
        printf("SSB SCO %d\n",*GNBParamList.paramarray[i][GNB_SSB_SUBCARRIEROFFSET_IDX].iptr);
        NRRRC_CONFIGURATION_REQ (msg_p).ssb_SubcarrierOffset = *GNBParamList.paramarray[i][GNB_SSB_SUBCARRIEROFFSET_IDX].iptr;
        printf("pdsch_AntennaPorts %d\n",*GNBParamList.paramarray[i][GNB_PDSCH_ANTENNAPORTS_IDX].iptr);
        NRRRC_CONFIGURATION_REQ (msg_p).pdsch_AntennaPorts = *GNBParamList.paramarray[i][GNB_PDSCH_ANTENNAPORTS_IDX].iptr;
        printf("pusch_AntennaPorts %d\n",*GNBParamList.paramarray[i][GNB_PUSCH_ANTENNAPORTS_IDX].iptr);
        NRRRC_CONFIGURATION_REQ (msg_p).pusch_AntennaPorts = *GNBParamList.paramarray[i][GNB_PUSCH_ANTENNAPORTS_IDX].iptr;
        NRRRC_CONFIGURATION_REQ (msg_p).sib1_tda = *GNBParamList.paramarray[i][GNB_SIB1_TDA_IDX].iptr;
        printf("Do CSI-RS %d\n",*GNBParamList.paramarray[i][GNB_DO_CSIRS_IDX].iptr);
        NRRRC_CONFIGURATION_REQ (msg_p).do_CSIRS = *GNBParamList.paramarray[i][GNB_DO_CSIRS_IDX].iptr;
        NRRRC_CONFIGURATION_REQ (msg_p).scc = scc;
        NRRRC_CONFIGURATION_REQ (msg_p).scd = scd;

	  
      }//
    }//End for (k=0; k <num_gnbs ; k++)
    memcpy(&rrc->configuration, &NRRRC_CONFIGURATION_REQ(msg_p), sizeof(NRRRC_CONFIGURATION_REQ(msg_p)));
  }//End if (num_gnbs>0)

  config_security(rrc);
}//End RCconfig_NRRRC function

int RCconfig_nr_gtpu(void ) {

  int               num_gnbs                      = 0;



  char*             gnb_interface_name_for_NGU    = NULL;
  char*             gnb_ipv4_address_for_NGU      = NULL;
  uint32_t          gnb_port_for_NGU              = 0;
  char*             gnb_interface_name_for_S1U    = NULL;
  char*             gnb_ipv4_address_for_S1U      = NULL;
  uint32_t          gnb_port_for_S1U              = 0;
  char             *address                       = NULL;
  char             *cidr                          = NULL;
  char gtpupath[MAX_OPTNAME_SIZE*2 + 8];
  uint8_t           gnb_mode                      = 0;

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  paramdef_t NETParams[]  =  GNBNETPARAMS_DESC;
  paramdef_t GTPUParams[] = GNBGTPUPARAMS_DESC;
  LOG_I(GTPU,"Configuring GTPu\n");

/* get number of active eNodeBs */
  config_get( GNBSParams,sizeof(GNBSParams)/sizeof(paramdef_t),NULL); 
  num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal (num_gnbs >0,
           "Failed to parse config file no active gNodeBs in %s \n", GNB_CONFIG_STRING_ACTIVE_GNBS);

  sprintf(gtpupath,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,0,GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  config_get(GTPUParams,sizeof(GTPUParams)/sizeof(paramdef_t),gtpupath);

  config_get(NETParams,sizeof(NETParams)/sizeof(paramdef_t),gtpupath); 

  if (NETParams[0].strptr != NULL) { // SA
    LOG_I(GTPU, "SA mode \n");
    cidr = gnb_ipv4_address_for_NGU;
    gnb_mode = 0;
  } else {// NSA
    LOG_I(GTPU, "NSA mode \n");
    cidr = gnb_ipv4_address_for_S1U;
    gnb_mode = 1;
  }

    address = strtok(cidr, "/");

    if (address) {
      MessageDef *message;

      if (gnb_mode == 1) { // NSA
        message = itti_alloc_new_message(TASK_GNB_APP, 0, GTPV1U_ENB_S1_REQ);
        AssertFatal(message!=NULL,"");
        // IPV4_STR_ADDR_TO_INT_NWBO ( address, RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
        // LOG_I(GTPU,"Configuring GTPu address : %s -> %x\n",address,RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up);
        IPV4_STR_ADDR_TO_INT_NWBO (address, GTPV1U_ENB_S1_REQ(message).enb_ip_address_for_S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
        LOG_I(GTPU,"Configuring GTPu address : %s -> %x\n",address,GTPV1U_ENB_S1_REQ(message).enb_ip_address_for_S1u_S12_S4_up);
        GTPV1U_ENB_S1_REQ(message).enb_port_for_S1u_S12_S4_up = gnb_port_for_S1U;
        strcpy(GTPV1U_ENB_S1_REQ(message).addrStr,address);
        sprintf(GTPV1U_ENB_S1_REQ(message).portStr,"%d", gnb_port_for_NGU);
      } else {// TODO SA
        message = itti_alloc_new_message(TASK_GNB_APP, 0, GTPV1U_GNB_NG_REQ);
        AssertFatal(message!=NULL,"");
        IPV4_STR_ADDR_TO_INT_NWBO (address, GTPV1U_GNB_NG_REQ(message).gnb_ip_address_for_NGu_up, "BAD IP ADDRESS FORMAT FOR gNB NG_U !\n" );
        LOG_I(GTPU,"Configuring GTPu address : %s -> %x\n",address,GTPV1U_GNB_NG_REQ(message).gnb_ip_address_for_NGu_up);
        GTPV1U_GNB_NG_REQ(message).gnb_port_for_NGu_up = gnb_port_for_NGU;
        strcpy(GTPV1U_GNB_NG_REQ(message).addrStr,address);
        sprintf(GTPV1U_GNB_NG_REQ(message).portStr,"%d", gnb_port_for_NGU);
      }
     itti_send_msg_to_task (TASK_VARIABLE, 0, message); // data model is wrong: gtpu doesn't have enb_id (or module_id)
    } else
    LOG_E(GTPU,"invalid address for NGU\n");


return 0;
}

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

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};

  /* get global parameters, defined outside any section in the config file */
  config_get( GNBSParams,sizeof(GNBSParams)/sizeof(paramdef_t),NULL); 

  /*
  if (AMF_MODE_ENABLED) {
    if (strcasecmp( *(GNBSParams[GNB_ASN1_VERBOSITY_IDX].strptr), GNB_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0) {
      asn_debug      = 0;
      asn1_xer_print = 0;
    } else if (strcasecmp( *(GNBSParams[GNB_ASN1_VERBOSITY_IDX].strptr), GNB_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0) {
      asn_debug      = 1;
      asn1_xer_print = 1;
    } else if (strcasecmp(*(GNBSParams[GNB_ASN1_VERBOSITY_IDX].strptr) , GNB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0) {
      asn_debug      = 1;
      asn1_xer_print = 2;
    } else {
      asn_debug      = 0;
      asn1_xer_print = 0;
    }
  }
  */
  
    AssertFatal (i<GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt,
     "Failed to parse config file %s, %uth attribute %s \n",
     RC.config_file_name, i, GNB_CONFIG_STRING_ACTIVE_GNBS);
    
  
  if (GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt>0) {
    // Output a list of all gNBs.
       config_getlist( &GNBParamList,GNBParams,sizeof(GNBParams)/sizeof(paramdef_t),NULL); 
    if (GNBParamList.numelt > 0) {
      for (k = 0; k < GNBParamList.numelt; k++) {
        if (GNBParamList.paramarray[k][GNB_GNB_ID_IDX].uptr == NULL) {
          // Calculate a default gNB ID
          if (AMF_MODE_ENABLED) {
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

            for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
              PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);
            for (int J = 0; J < sizeof(SNSSAIParams) / sizeof(paramdef_t); ++J)
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
              "Failed to parse gNB configuration file %s, gnb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_GNB or CELL_HOME_GNB !\n",
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
            config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), aprefix);

            if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
              AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                          PLMNParamList.numelt);

            NGAP_REGISTER_GNB_REQ(msg_p).num_plmn = PLMNParamList.numelt;

            for (int l = 0; l < PLMNParamList.numelt; ++l) {
              char snssaistr[MAX_OPTNAME_SIZE*2 + 8];
              sprintf(snssaistr, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, k, GNB_CONFIG_STRING_PLMN_LIST, l);
              config_getlist(&SNSSAIParamList, SNSSAIParams, sizeof(SNSSAIParams)/sizeof(paramdef_t), snssaistr);

              NGAP_REGISTER_GNB_REQ (msg_p).mcc[l]              = *PLMNParamList.paramarray[l][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
              NGAP_REGISTER_GNB_REQ (msg_p).mnc[l]              = *PLMNParamList.paramarray[l][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
              NGAP_REGISTER_GNB_REQ (msg_p).mnc_digit_length[l] = *PLMNParamList.paramarray[l][GNB_MNC_DIGIT_LENGTH].u8ptr;
              NGAP_REGISTER_GNB_REQ (msg_p).default_drx      = 0;
              AssertFatal((NGAP_REGISTER_GNB_REQ (msg_p).mnc_digit_length[l] == 2) ||
                          (NGAP_REGISTER_GNB_REQ (msg_p).mnc_digit_length[l] == 3),
                          "BAD MNC DIGIT LENGTH %d",
                          NGAP_REGISTER_GNB_REQ (msg_p).mnc_digit_length[l]);
              
              NGAP_REGISTER_GNB_REQ (msg_p).num_nssai[l] = SNSSAIParamList.numelt;
              for (int s = 0; s < SNSSAIParamList.numelt; ++s) {
              
                NGAP_REGISTER_GNB_REQ (msg_p).s_nssai[l][s].sST = *SNSSAIParamList.paramarray[s][GNB_SLICE_SERIVE_TYPE_IDX].uptr;
                NGAP_REGISTER_GNB_REQ (msg_p).s_nssai[l][s].sD_flag = 0;
                if(*SNSSAIParamList.paramarray[s][GNB_SLICE_DIFFERENTIATOR_IDX].uptr != 0) {
                  NGAP_REGISTER_GNB_REQ (msg_p).s_nssai[l][s].sD_flag = 1;
                  NGAP_REGISTER_GNB_REQ (msg_p).s_nssai[l][s].sD[0] = (*SNSSAIParamList.paramarray[s][GNB_SLICE_DIFFERENTIATOR_IDX].uptr & 0xFF0000) >> 16;
                  NGAP_REGISTER_GNB_REQ (msg_p).s_nssai[l][s].sD[1] = (*SNSSAIParamList.paramarray[s][GNB_SLICE_DIFFERENTIATOR_IDX].uptr & 0x00FF00) >> 8;
                  NGAP_REGISTER_GNB_REQ (msg_p).s_nssai[l][s].sD[2] = (*SNSSAIParamList.paramarray[s][GNB_SLICE_DIFFERENTIATOR_IDX].uptr & 0x0000FF);
                }
              }
            }
            sprintf(aprefix,"%s.[%i]",GNB_CONFIG_STRING_GNB_LIST,k);
            config_getlist( &NGParamList,NGParams,sizeof(NGParams)/sizeof(paramdef_t),aprefix); 
            
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
            if (AMF_MODE_ENABLED) {
              sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_SCTP_CONFIG);
              config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix); 
              NGAP_REGISTER_GNB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[GNB_SCTP_INSTREAMS_IDX].uptr);
              NGAP_REGISTER_GNB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[GNB_SCTP_OUTSTREAMS_IDX].uptr);
            }

            sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
            // NETWORK_INTERFACES
            config_get( NETParams,sizeof(NETParams)/sizeof(paramdef_t),aprefix); 
            
            //    NGAP_REGISTER_GNB_REQ (msg_p).enb_interface_name_for_NGU = strdup(enb_interface_name_for_NGU);
            cidr = *(NETParams[GNB_IPV4_ADDRESS_FOR_NG_AMF_IDX].strptr);
            address = strtok(cidr, "/");
            
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
  config_getlist( &THREADParamList,NULL,0,NULL);

  if(THREADParamList.numelt>0) {
    config_getlist( &THREADParamList,ThreadParams,sizeof(ThreadParams)/sizeof(paramdef_t),NULL);
    parallel_conf = strdup(*(THREADParamList.paramarray[0][THREAD_PARALLEL_IDX].strptr));
  } else {
    parallel_conf = strdup("PARALLEL_RU_L1_TRX_SPLIT");
  }

  if(THREADParamList.numelt>0) {
    config_getlist( &THREADParamList,ThreadParams,sizeof(ThreadParams)/sizeof(paramdef_t),NULL);
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
 
  config_get( GNBSParams,sizeof(GNBSParams)/sizeof(paramdef_t),NULL); 
  RC.nb_nr_inst = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;

	// Get num MACRLC instances
  config_getlist( &MACRLCParamList,NULL,0, NULL);
  RC.nb_nr_macrlc_inst  = MACRLCParamList.numelt;
  // Get num L1 instances
  config_getlist( &L1ParamList,NULL,0, NULL);
  RC.nb_nr_L1_inst = L1ParamList.numelt;
  
  // Get num RU instances
  config_getlist( &RUParamList,NULL,0, NULL);  
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
  config_get( GNBSParams,sizeof(GNBSParams)/sizeof(paramdef_t),NULL);
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
    config_getlist( &GNBParamList,GNBParams,sizeof(GNBParams)/sizeof(paramdef_t),NULL);

    if (GNBParamList.numelt > 0) {
      for (k = 0; k < GNBParamList.numelt; k++) {
        if (GNBParamList.paramarray[k][GNB_GNB_ID_IDX].uptr == NULL) {
          // Calculate a default eNB ID
          if (AMF_MODE_ENABLED) {
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

            for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
              PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

            paramdef_t X2Params[]  = X2PARAMS_DESC;
            paramlist_def_t X2ParamList = {ENB_CONFIG_STRING_TARGET_ENB_X2_IP_ADDRESS,NULL,0};
            paramdef_t SCTPParams[]  = GNBSCTPPARAMS_DESC;
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
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                           RC.config_file_name, i, *(GNBParamList.paramarray[k][GNB_CELL_TYPE_IDX].strptr));
            }

            X2AP_REGISTER_ENB_REQ (msg_p).eNB_name         = strdup(*(GNBParamList.paramarray[k][GNB_GNB_NAME_IDX].strptr));
            X2AP_REGISTER_ENB_REQ (msg_p).tac              = *GNBParamList.paramarray[k][GNB_TRACKING_AREA_CODE_IDX].uptr;
            config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), aprefix);

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

            config_getlist(&SCCsParamList, NULL, 0, aprefix);
            if (SCCsParamList.numelt > 0) {
              sprintf(aprefix, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST,0,GNB_CONFIG_STRING_SERVINGCELLCONFIGCOMMON, 0);
              config_get( SCCsParams,sizeof(SCCsParams)/sizeof(paramdef_t),aprefix);
              fix_scc(scc,ssb_bitmap);
            }
            X2AP_REGISTER_ENB_REQ (msg_p).num_cc = SCCsParamList.numelt;
            for (J = 0; J < SCCsParamList.numelt ; J++) {
              X2AP_REGISTER_ENB_REQ (msg_p).nr_band[J] = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]; //nr_band; //78
              X2AP_REGISTER_ENB_REQ (msg_p).nrARFCN[J] = scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;
              X2AP_REGISTER_ENB_REQ (msg_p).uplink_frequency_offset[J] = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier; //0
              X2AP_REGISTER_ENB_REQ (msg_p).Nid_cell[J]= *scc->physCellId; //0
              X2AP_REGISTER_ENB_REQ (msg_p).N_RB_DL[J]=  scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;//106
              X2AP_REGISTER_ENB_REQ (msg_p).frame_type[J] = TDD;
              LOG_I(X2AP, "gNB configuration parameters: nr_band: %d, nr_ARFCN: %d, DL_RBs: %d, num_cc: %d \n",
                  X2AP_REGISTER_ENB_REQ (msg_p).nr_band[J],
                  X2AP_REGISTER_ENB_REQ (msg_p).nrARFCN[J],
                  X2AP_REGISTER_ENB_REQ (msg_p).N_RB_DL[J],
                  X2AP_REGISTER_ENB_REQ (msg_p).num_cc);
            }

            sprintf(aprefix,"%s.[%i]",GNB_CONFIG_STRING_GNB_LIST,k);
            config_getlist( &X2ParamList,X2Params,sizeof(X2Params)/sizeof(paramdef_t),aprefix);
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
                { "t_reloc_prep", "t_reloc_prep", 0, iptr:&t_reloc_prep, defintval:0, TYPE_INT, 0 },
                { "tx2_reloc_overall", "tx2_reloc_overall", 0, iptr:&tx2_reloc_overall, defintval:0, TYPE_INT, 0 },
                { "t_dc_prep", "t_dc_prep", 0, iptr:&t_dc_prep, defintval:0, TYPE_INT, 0 },
                { "t_dc_overall", "t_dc_overall", 0, iptr:&t_dc_overall, defintval:0, TYPE_INT, 0 }
              };
              config_get(p, sizeof(p)/sizeof(paramdef_t), aprefix);

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

            if (AMF_MODE_ENABLED) {
              sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_SCTP_CONFIG);
              config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
              X2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[GNB_SCTP_INSTREAMS_IDX].uptr);
              X2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[GNB_SCTP_OUTSTREAMS_IDX].uptr);
            }

            sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
            // NETWORK_INTERFACES
            config_get( NETParams,sizeof(NETParams)/sizeof(paramdef_t),aprefix);
            X2AP_REGISTER_ENB_REQ (msg_p).enb_port_for_X2C = (uint32_t)*(NETParams[GNB_PORT_FOR_X2C_IDX].uptr);

            //temp out
            if ((NETParams[GNB_IPV4_ADDR_FOR_X2C_IDX].strptr == NULL) || (X2AP_REGISTER_ENB_REQ (msg_p).enb_port_for_X2C == 0)) {
              LOG_E(RRC,"Add eNB IPv4 address and/or port for X2C in the CONF file!\n");
              exit(1);
            }

            cidr = *(NETParams[ENB_IPV4_ADDR_FOR_X2C_IDX].strptr);
            address = strtok(cidr, "/");
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

int RCconfig_NR_DU_F1(MessageDef *msg_p, uint32_t i) {
  int k;
  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  paramdef_t GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};
  config_get( GNBSParams,sizeof(GNBSParams)/sizeof(paramdef_t),NULL);
  int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal (i < num_gnbs,
               "Failed to parse config file no %uth element in %s \n",i, GNB_CONFIG_STRING_ACTIVE_GNBS);

  if (num_gnbs > 0) {
    // Output a list of all eNBs.
    config_getlist( &GNBParamList,GNBParams,sizeof(GNBParams)/sizeof(paramdef_t),NULL);
    AssertFatal(GNBParamList.paramarray[i][GNB_GNB_ID_IDX].uptr != NULL,
                "gNB id %u is not defined in configuration file\n",i);
    F1AP_SETUP_REQ (msg_p).num_cells_available = 0;

    for (k=0; k <num_gnbs ; k++) {
      if (strcmp(GNBSParams[GNB_ACTIVE_GNBS_IDX].strlistptr[k], *(GNBParamList.paramarray[i][GNB_GNB_NAME_IDX].strptr)) == 0) {
        char aprefix[MAX_OPTNAME_SIZE*2 + 8];
        sprintf(aprefix,"%s.[%i]",GNB_CONFIG_STRING_GNB_LIST,k);
        paramdef_t PLMNParams[] = GNBPLMNPARAMS_DESC;
        paramlist_def_t PLMNParamList = {GNB_CONFIG_STRING_PLMN_LIST, NULL, 0};
        /* map parameter checking array instances to parameter definition array instances */
        checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

        for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
          PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

        config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), aprefix);
        paramdef_t SCTPParams[]  = SCTPPARAMS_DESC;
        F1AP_SETUP_REQ (msg_p).num_cells_available++;
        F1AP_SETUP_REQ (msg_p).gNB_DU_id        = *(GNBParamList.paramarray[0][GNB_GNB_ID_IDX].uptr);
        LOG_I(GNB_APP,"F1AP: gNB_DU_id[%d] %ld\n",k,F1AP_SETUP_REQ (msg_p).gNB_DU_id);
        F1AP_SETUP_REQ (msg_p).gNB_DU_name      = strdup(*(GNBParamList.paramarray[0][GNB_GNB_NAME_IDX].strptr));
        LOG_I(GNB_APP,"F1AP: gNB_DU_name[%d] %s\n",k,F1AP_SETUP_REQ (msg_p).gNB_DU_name);
        F1AP_SETUP_REQ (msg_p).tac[k]              = *GNBParamList.paramarray[i][GNB_TRACKING_AREA_CODE_IDX].uptr;
        LOG_I(GNB_APP,"F1AP: tac[%d] %d\n",k,F1AP_SETUP_REQ (msg_p).tac[k]);
        F1AP_SETUP_REQ (msg_p).mcc[k]              = *PLMNParamList.paramarray[0][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
        LOG_I(GNB_APP,"F1AP: mcc[%d] %d\n",k,F1AP_SETUP_REQ (msg_p).mcc[k]);
        F1AP_SETUP_REQ (msg_p).mnc[k]              = *PLMNParamList.paramarray[0][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
        LOG_I(GNB_APP,"F1AP: mnc[%d] %d\n",k,F1AP_SETUP_REQ (msg_p).mnc[k]);
        F1AP_SETUP_REQ (msg_p).mnc_digit_length[k] = *PLMNParamList.paramarray[0][GNB_MNC_DIGIT_LENGTH].u8ptr;
        LOG_I(GNB_APP,"F1AP: mnc_digit_length[%d] %d\n",k,F1AP_SETUP_REQ (msg_p).mnc_digit_length[k]);
        AssertFatal((F1AP_SETUP_REQ (msg_p).mnc_digit_length[k] == 2) ||
                    (F1AP_SETUP_REQ (msg_p).mnc_digit_length[k] == 3),
                    "BAD MNC DIGIT LENGTH %d",
                    F1AP_SETUP_REQ (msg_p).mnc_digit_length[k]);
        F1AP_SETUP_REQ (msg_p).nr_cellid[k] = (uint64_t)*(GNBParamList.paramarray[i][GNB_NRCELLID_IDX].u64ptr);
        LOG_I(GNB_APP,"F1AP: nr_cellid[%d] %ld\n",k,F1AP_SETUP_REQ (msg_p).nr_cellid[k]);
        LOG_I(GNB_APP,"F1AP: CU_ip4_address in DU %s\n",RC.nrmac[k]->eth_params_n.remote_addr);
        LOG_I(GNB_APP,"FIAP: CU_ip4_address in DU %p, strlen %d\n",F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv4_address,(int)strlen(RC.nrmac[k]->eth_params_n.remote_addr));
        F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv6 = 0;
        F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv4 = 1;
        //strcpy(F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv6_address, "");
        strcpy(F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv4_address, RC.nrmac[k]->eth_params_n.remote_addr);
        LOG_I(GNB_APP,"F1AP: DU_ip4_address in DU %s\n",RC.nrmac[k]->eth_params_n.my_addr);
        LOG_I(GNB_APP,"FIAP: DU_ip4_address in DU %p, strlen %d\n",F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv4_address,(int)strlen(RC.nrmac[k]->eth_params_n.my_addr));
        F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv6 = 0;
        F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv4 = 1;
        //strcpy(F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv6_address, "");
        strcpy(F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv4_address, RC.nrmac[k]->eth_params_n.my_addr);
        //strcpy(F1AP_SETUP_REQ (msg_p).CU_ip_address[l].ipv6_address,*(F1ParamList.paramarray[l][ENB_CU_IPV6_ADDRESS_IDX].strptr));
        sprintf(aprefix,"%s.[%i].%s",GNB_CONFIG_STRING_GNB_LIST,k,GNB_CONFIG_STRING_SCTP_CONFIG);
        config_get(SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
        F1AP_SETUP_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[GNB_SCTP_INSTREAMS_IDX].uptr);
        F1AP_SETUP_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[GNB_SCTP_OUTSTREAMS_IDX].uptr);
        gNB_RRC_INST *rrc = RC.nrrrc[k];
        // wait until RRC cell information is configured
        int cell_info_configured = 0;

        do {
          LOG_I(GNB_APP,"ngran_gNB_DU: Waiting for basic cell configuration\n");
          usleep(100000);
          pthread_mutex_lock(&rrc->cell_info_mutex);
          cell_info_configured = rrc->cell_info_configured;
          pthread_mutex_unlock(&rrc->cell_info_mutex);
        } while (cell_info_configured == 0);

        rrc->configuration.mcc[0] = F1AP_SETUP_REQ (msg_p).mcc[k];
        rrc->configuration.mnc[0] = F1AP_SETUP_REQ (msg_p).mnc[k];
        rrc->configuration.tac    = F1AP_SETUP_REQ (msg_p).tac[k];
        rrc->nr_cellid = F1AP_SETUP_REQ (msg_p).nr_cellid[k];
        F1AP_SETUP_REQ (msg_p).nr_pci[k]    = *rrc->configuration.scc->physCellId;
        F1AP_SETUP_REQ (msg_p).num_ssi[k] = 0;

        if (rrc->configuration.scc->tdd_UL_DL_ConfigurationCommon) {
          LOG_I(GNB_APP,"ngran_DU: Configuring Cell %d for TDD\n",k);
          F1AP_SETUP_REQ (msg_p).fdd_flag = 0;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.nr_arfcn = rrc->configuration.scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.scs = rrc->configuration.scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.nrb = rrc->configuration.scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.num_frequency_bands = 1;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.nr_band[0] = *rrc->configuration.scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.sul_active              = 0;
        } else {
          /***************** for test *****************/
          LOG_I(GNB_APP,"ngran_DU: Configuring Cell %d for FDD\n",k);
          F1AP_SETUP_REQ (msg_p).fdd_flag = 1;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_nr_arfcn             = 26200UL;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nr_arfcn             = 26200UL;
          // For LTE use scs field to carry prefix type and number of antennas
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_scs                  = 0;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_scs                  = 0;
          // use nrb field to hold LTE N_RB_DL (0...5)
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nrb                  = 3;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nrb                  = 3;
          // RK: we need to check there value for FDD's frequency_bands DL/UL
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_num_frequency_bands  = 1;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nr_band[0]           = 7;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_num_frequency_bands  = 1;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_nr_band[0]           = 7;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_num_sul_frequency_bands  = 0;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nr_sul_band[0]           = 7;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_num_sul_frequency_bands  = 0;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_nr_sul_band[0]           = 7;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.sul_active              = 0;
          /***************** for test *****************/
        }

        F1AP_SETUP_REQ (msg_p).measurement_timing_information[k]             = "0";
        F1AP_SETUP_REQ (msg_p).ranac[k]                                      = 0;
        F1AP_SETUP_REQ (msg_p).mib[k]                                        = rrc->carrier.MIB;
        F1AP_SETUP_REQ (msg_p).sib1[k]                                       = rrc->carrier.SIB1;
        F1AP_SETUP_REQ (msg_p).mib_length[k]                                 = rrc->carrier.sizeof_MIB;
        F1AP_SETUP_REQ (msg_p).sib1_length[k]                                = rrc->carrier.sizeof_SIB1;
        break;
      }
    }
  }
  return 0;
}

int du_check_plmn_identity(rrc_gNB_carrier_data_t *carrier,uint16_t mcc,uint16_t mnc,uint8_t mnc_digit_length) {
  NR_SIB1_t *sib1 = carrier->siblock1->message.choice.c1->choice.systemInformationBlockType1;
  AssertFatal(sib1->cellAccessRelatedInfo.plmn_IdentityList.list.array[0]->plmn_IdentityList.list.count > 0,
              "plmn info isn't there\n");
  AssertFatal(mnc_digit_length == 2 || mnc_digit_length == 3,
              "impossible mnc_digit_length %d\n", mnc_digit_length);
  NR_PLMN_Identity_t *plmn_Identity = sib1->cellAccessRelatedInfo.plmn_IdentityList.list.array[0]
                                            ->plmn_IdentityList.list.array[0];

  // check if mcc is different and return failure if so
  if (mcc !=
      ((*plmn_Identity->mcc->list.array[0])*100)+
      ((*plmn_Identity->mcc->list.array[1])*10) +
      (*plmn_Identity->mcc->list.array[2])) {
    LOG_E(GNB_APP, "mcc in F1AP_SETUP_RESP message is different from mcc in DU \n");
    return(0);
  }

  // check that mnc digit length is different and return failure if so
  if (mnc_digit_length != plmn_Identity->mnc.list.count) {
    LOG_E(GNB_APP, "mnc(length: %d) in F1AP_SETUP_RESP message is different from mnc(length: %d) in DU \n",
                    mnc_digit_length, plmn_Identity->mnc.list.count);
    return 0;
  }

  // check that 2 digit mnc is different and return failure if so
  if (mnc_digit_length == 2 &&
      (mnc !=
       (*plmn_Identity->mnc.list.array[0]*10) +
       (*plmn_Identity->mnc.list.array[1]))) {
    LOG_E(GNB_APP, "mnc(%d) in F1AP_SETUP_RESP message is different from mnc(%ld%ld) in DU \n",
                    mnc, *plmn_Identity->mnc.list.array[0], *plmn_Identity->mnc.list.array[1]);
    return(0);
  }
  else if (mnc_digit_length == 3 &&
           (mnc !=
            (*plmn_Identity->mnc.list.array[0]*100) +
            (*plmn_Identity->mnc.list.array[1]*10) +
            (*plmn_Identity->mnc.list.array[2]))) {
    LOG_E(GNB_APP, "mnc(%d) in F1AP_SETUP_RESP message is different from mnc(%ld%ld%ld) in DU \n",
                    mnc, *plmn_Identity->mnc.list.array[0], *plmn_Identity->mnc.list.array[1], *plmn_Identity->mnc.list.array[2]);
    return(0);
  }

  // if we're here, the mcc/mnc match so return success
  return(1);
}

void du_extract_and_decode_SI(int inst, int si_ind, uint8_t *si_container, int si_container_length) {
  gNB_RRC_INST *rrc = RC.nrrrc[inst];
  rrc_gNB_carrier_data_t *carrier = &rrc->carrier;
  NR_BCCH_DL_SCH_Message_t *bcch_message ;
  AssertFatal(si_ind == 0, "Can only handle a single SI block for now\n");
  LOG_I(GNB_APP, "rrc inst %d: Trying to decode SI block %d @ %p, length %d\n", inst, si_ind, si_container, si_container_length);
  // point to first SI block
  bcch_message = &carrier->systemInformation;
  asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                            &asn_DEF_NR_BCCH_DL_SCH_Message,
                            (void **)&bcch_message,
                            (const void *)si_container,
                            si_container_length);

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    AssertFatal(1==0, "[GNB_APP][NR_RRC inst %"PRIu8"] Failed to decode BCCH_DLSCH_MESSAGE (%zu bits)\n",
                inst,
                dec_rval.consumed );
  }

  if (bcch_message->message.present == NR_BCCH_DL_SCH_MessageType_PR_c1) {
    switch (bcch_message->message.choice.c1->present) {
      case NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1:
        AssertFatal(1 == 0, "Should have received SIB1 from CU\n");
        break;

      case NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformation:
      {
        NR_SystemInformation_t *si = bcch_message->message.choice.c1->choice.systemInformation;

        if (si->criticalExtensions.present == NR_SystemInformation__criticalExtensions_PR_systemInformation) {
          for (int i = 0; i < si->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.count; i++) {
            LOG_I(GNB_APP, "Extracting SI %d/%d\n", i, si->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.count);
            SystemInformation_IEs__sib_TypeAndInfo__Member *typeAndInfo;
            typeAndInfo = si->criticalExtensions.choice.systemInformation->sib_TypeAndInfo.list.array[i];

            switch(typeAndInfo->present) {
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_NOTHING:
                AssertFatal(0, "Should have received SIB2 SIB3 from CU\n");
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib2:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB2 in CU F1AP_SETUP_RESP message\n", inst);
                carrier->sib2 = typeAndInfo->choice.sib2;
                carrier->SIB23 = (uint8_t *)malloc(64);
                memcpy((void *)carrier->SIB23, (void *)si_container, si_container_length);
                carrier->sizeof_SIB23 = si_container_length;
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib3:
                carrier->sib3 = typeAndInfo->choice.sib3;
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB3 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib4:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB4 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib5:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB5 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib6:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB6 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib7:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB7 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib8:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB8 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib9:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB9 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib10_v1610:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB10 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib11_v1610:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB11 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib12_v1610:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB12 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib13_v1610:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB13 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              case NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib14_v1610:
                LOG_I(GNB_APP, "[NR_RRC %"PRIu8"] Found SIB14 in CU F1AP_SETUP_RESP message\n", inst);
                break;
              default:
                AssertFatal(1 == 0,"Shouldn't have received this SI %d\n", typeAndInfo->present);
                break;
            }
          }
        }

        break;
      }

      case NR_BCCH_DL_SCH_MessageType__c1_PR_NOTHING:
        AssertFatal(0, "Should have received SIB1 from CU\n");
        break;
    }
  } else AssertFatal(1 == 0, "No SI messages\n");
}

void configure_gnb_du_mac(int inst) {
  gNB_RRC_INST *rrc = RC.nrrrc[inst];
  // LOG_I(GNB_APP,"Configuring MAC/L1 %d, carrier->sib2 %p\n", inst, &carrier->sib2->radioResourceConfigCommon);
  LOG_I(GNB_APP,"Configuring gNB DU MAC/L1 %d \n", inst);
  rrc_mac_config_req_gNB(rrc->module_id,
                        rrc->configuration.ssb_SubcarrierOffset,
                        rrc->configuration.pdsch_AntennaPorts,
                        rrc->configuration.pusch_AntennaPorts,
                        rrc->configuration.sib1_tda,
                        rrc->configuration.scc,
                        0,
                        0, // rnti
                        (NR_CellGroupConfig_t *)NULL
                        );
}


int gNB_app_handle_f1ap_setup_resp(f1ap_setup_resp_t *resp) {
  int i, j, si_ind;
  int ret=0;
  LOG_I(GNB_APP, "cells_to_activate %d, RRC instances %d\n",
        resp->num_cells_to_activate, RC.nb_nr_inst);

  for (j = 0; j < resp->num_cells_to_activate; j++) {
    for (i = 0; i < RC.nb_nr_inst; i++) {
      rrc_gNB_carrier_data_t *carrier =  &RC.nrrrc[i]->carrier;
      // identify local index of cell j by nr_cellid, plmn identity and physical cell ID
      LOG_I(GNB_APP, "Checking cell %d, rrc inst %d : rrc->nr_cellid %lx, resp->nr_cellid %lx\n",
            j, i, RC.nrrrc[i]->nr_cellid, resp->cells_to_activate[j].nr_cellid);

      if (RC.nrrrc[i]->nr_cellid == resp->cells_to_activate[j].nr_cellid &&
          (du_check_plmn_identity(carrier, resp->cells_to_activate[j].mcc, resp->cells_to_activate[j].mnc, resp->cells_to_activate[j].mnc_digit_length)>0 &&
           resp->cells_to_activate[j].nrpci == carrier->physCellId)) {
        // copy system information and decode it
        for (si_ind=0; si_ind<resp->cells_to_activate[j].num_SI; si_ind++)  {

          du_extract_and_decode_SI(i,
                                   si_ind,
                                   resp->cells_to_activate[j].SI_container[2+si_ind],
                                   resp->cells_to_activate[j].SI_container_length[2+si_ind]);
        }

        // perform MAC/L1 common configuration
        configure_gnb_du_mac(i);
	ret++;
      } else {
        LOG_E(GNB_APP, "F1 Setup Response not matching\n");
      }
    }
  }
  return(ret);
}

int gNB_app_handle_f1ap_gnb_cu_configuration_update(f1ap_gnb_cu_configuration_update_t *gnb_cu_cfg_update) {
  int i, j, si_ind, ret=0;
  LOG_I(GNB_APP, "cells_to_activate %d, RRC instances %d\n",
        gnb_cu_cfg_update->num_cells_to_activate, RC.nb_nr_inst);

  for (j = 0; j < gnb_cu_cfg_update->num_cells_to_activate; j++) {
    for (i = 0; i < RC.nb_nr_inst; i++) {
      rrc_gNB_carrier_data_t *carrier =  &RC.nrrrc[i]->carrier;
      // identify local index of cell j by nr_cellid, plmn identity and physical cell ID
      LOG_I(GNB_APP, "Checking cell %d, rrc inst %d : rrc->nr_cellid %lx, gnb_cu_cfg_updatenr_cellid %lx\n",
            j, i, RC.nrrrc[i]->nr_cellid, gnb_cu_cfg_update->cells_to_activate[j].nr_cellid);

      if (RC.nrrrc[i]->nr_cellid == gnb_cu_cfg_update->cells_to_activate[j].nr_cellid &&
          (du_check_plmn_identity(carrier, gnb_cu_cfg_update->cells_to_activate[j].mcc, gnb_cu_cfg_update->cells_to_activate[j].mnc, gnb_cu_cfg_update->cells_to_activate[j].mnc_digit_length)>0 &&
           gnb_cu_cfg_update->cells_to_activate[j].nrpci == carrier->physCellId)) {
        // copy system information and decode it
        for (si_ind=0; si_ind<gnb_cu_cfg_update->cells_to_activate[j].num_SI; si_ind++)  {

          du_extract_and_decode_SI(i,
                                   si_ind,
                                   gnb_cu_cfg_update->cells_to_activate[j].SI_container[2+si_ind],
                                   gnb_cu_cfg_update->cells_to_activate[j].SI_container_length[2+si_ind]);
        }

        // perform MAC/L1 common configuration
        configure_gnb_du_mac(i);
	ret++;
      } else {
        LOG_E(GNB_APP, "GNB_CU_CONFIGURATION_UPDATE not matching\n");
      }
    }
  }
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

void set_node_type(void) {
  int               j;
  paramdef_t        MacRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t   MacRLC_ParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};
  paramdef_t        GNBParams[]  = GNBPARAMS_DESC;
  paramlist_def_t   GNBParamList = {GNB_CONFIG_STRING_GNB_LIST,NULL,0};

  config_getlist( &MacRLC_ParamList,MacRLC_Params,sizeof(MacRLC_Params)/sizeof(paramdef_t), NULL);   
  config_getlist( &GNBParamList,GNBParams,sizeof(GNBParams)/sizeof(paramdef_t),NULL);  

  if ( MacRLC_ParamList.numelt > 0) {
    RC.nb_nr_macrlc_inst = MacRLC_ParamList.numelt; 
    for (j=0;j<RC.nb_nr_macrlc_inst;j++) {
      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "f1") == 0) {
        macrlc_has_f1 = 1;
      }
    }
  }

  if (strcmp(*(GNBParamList.paramarray[0][GNB_TRANSPORT_S_PREFERENCE_IDX].strptr), "f1") == 0) {
      node_type = ngran_gNB_CU;
    } else {
      if (macrlc_has_f1 == 0) {
        node_type = ngran_gNB;
        LOG_I(NR_RRC,"Setting node_type to ngran_gNB\n");
      } else {
        node_type = ngran_gNB_DU;
        LOG_I(NR_RRC,"Setting node_type to ngran_gNB_DU\n");
      }
    }
}

void nr_read_config_and_init(void) {
  MessageDef *msg_p = NULL;
  uint32_t    gnb_id;
  uint32_t    gnb_nb = RC.nb_nr_inst;

  RCconfig_NR_L1();
  set_node_type();
  RCconfig_nr_macrlc();

  LOG_I(PHY, "%s() RC.nb_nr_L1_inst:%d\n", __FUNCTION__, RC.nb_nr_L1_inst);

  if (RC.nb_nr_L1_inst>0) AssertFatal(l1_north_init_gNB()==0,"could not initialize L1 north interface\n");

  AssertFatal (gnb_nb <= RC.nb_nr_inst,
               "Number of gNB is greater than gNB defined in configuration file (%d/%d)!",
               gnb_nb, RC.nb_nr_inst);

  LOG_I(GNB_APP,"Allocating gNB_RRC_INST for %d instances\n",RC.nb_nr_inst);

  RC.nrrrc = (gNB_RRC_INST **)malloc(RC.nb_nr_inst*sizeof(gNB_RRC_INST *));
  LOG_I(PHY, "%s() RC.nb_nr_inst:%d RC.nrrrc:%p\n", __FUNCTION__, RC.nb_nr_inst, RC.nrrrc);

  for (gnb_id = 0; gnb_id < RC.nb_nr_inst ; gnb_id++) {
    RC.nrrrc[gnb_id] = (gNB_RRC_INST*)malloc(sizeof(gNB_RRC_INST));
    LOG_I(PHY, "%s() Creating RRC instance RC.nrrrc[%d]:%p (%d of %d)\n", __FUNCTION__, gnb_id, RC.nrrrc[gnb_id], gnb_id+1, RC.nb_nr_inst);
    memset((void *)RC.nrrrc[gnb_id],0,sizeof(gNB_RRC_INST));
    msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, NRRRC_CONFIGURATION_REQ);
    RCconfig_NRRRC(msg_p,gnb_id, RC.nrrrc[gnb_id]);
  }

  if (NODE_IS_CU(RC.nrrrc[0]->node_type)) {
    pdcp_layer_init();
//    nr_DRB_preconfiguration(0x1234);
    rrc_init_nr_global_param();
  }
}
