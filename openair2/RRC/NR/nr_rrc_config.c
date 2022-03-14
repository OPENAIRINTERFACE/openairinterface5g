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

/*! \file nr_rrc_config.c
 * \brief rrc config for gNB
 * \author Raymond Knopp, WEI-TAI CHEN
 * \date 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
 */

#include "nr_rrc_config.h"
#include "common/utils/nr/nr_common.h"

void prepare_sim_uecap(NR_UE_NR_Capability_t *cap,
                       NR_ServingCellConfigCommon_t *scc,
                       int numerology,
                       int rbsize,
                       int mcs_table) {

  int band = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
  NR_BandNR_t *nr_bandnr = CALLOC(1,sizeof(NR_BandNR_t));
  nr_bandnr->bandNR = band;
  ASN_SEQUENCE_ADD(&cap->rf_Parameters.supportedBandListNR.list,
                   nr_bandnr);
  if (mcs_table == 1) {
    int bw = get_supported_band_index(numerology, band, rbsize);
    if (band>256) {
      NR_BandNR_t *bandNRinfo = cap->rf_Parameters.supportedBandListNR.list.array[0];
      bandNRinfo->pdsch_256QAM_FR2 = CALLOC(1,sizeof(*bandNRinfo->pdsch_256QAM_FR2));
      *bandNRinfo->pdsch_256QAM_FR2 = NR_BandNR__pdsch_256QAM_FR2_supported;
    }
    else{
      cap->phy_Parameters.phy_ParametersFR1 = CALLOC(1,sizeof(*cap->phy_Parameters.phy_ParametersFR1));
      NR_Phy_ParametersFR1_t *phy_fr1 = cap->phy_Parameters.phy_ParametersFR1;
      phy_fr1->pdsch_256QAM_FR1 = CALLOC(1,sizeof(*phy_fr1->pdsch_256QAM_FR1));
      *phy_fr1->pdsch_256QAM_FR1 = NR_Phy_ParametersFR1__pdsch_256QAM_FR1_supported;
    }
    cap->featureSets = CALLOC(1,sizeof(*cap->featureSets));
    NR_FeatureSets_t *fs=cap->featureSets;
    fs->featureSetsDownlinkPerCC = CALLOC(1,sizeof(*fs->featureSetsDownlinkPerCC));
    NR_FeatureSetDownlinkPerCC_t *fs_cc = CALLOC(1,sizeof(NR_FeatureSetDownlinkPerCC_t));
    fs_cc->supportedSubcarrierSpacingDL = numerology;
    if(band>256) {
      fs_cc->supportedBandwidthDL.present = NR_SupportedBandwidth_PR_fr2;
      fs_cc->supportedBandwidthDL.choice.fr2 = bw;
    }
    else{
      fs_cc->supportedBandwidthDL.present = NR_SupportedBandwidth_PR_fr1;
      fs_cc->supportedBandwidthDL.choice.fr1 = bw;
    }
    fs_cc->supportedModulationOrderDL = CALLOC(1,sizeof(*fs_cc->supportedModulationOrderDL));
    *fs_cc->supportedModulationOrderDL = NR_ModulationOrder_qam256;
    ASN_SEQUENCE_ADD(&fs->featureSetsDownlinkPerCC->list,
                     fs_cc);
  }
}

void nr_rrc_config_dl_tda(NR_ServingCellConfigCommon_t *scc){

  lte_frame_type_t frame_type = get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);
  int curr_bwp = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
  // coreset duration setting to be improved in the framework of RRC harmonization, potentially using a common function
  int len_coreset = 1;
  if (curr_bwp < 48)
    len_coreset = 2;
  // setting default TDA for DL with
  struct NR_PDSCH_TimeDomainResourceAllocation *timedomainresourceallocation = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
  timedomainresourceallocation->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  timedomainresourceallocation->startSymbolAndLength = get_SLIV(len_coreset,14-len_coreset); // basic slot configuration starting in symbol 1 til the end of the slot
  ASN_SEQUENCE_ADD(&scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,
                   timedomainresourceallocation);
  if(frame_type==TDD) {
    // TDD
    if(scc->tdd_UL_DL_ConfigurationCommon) {
      int dl_symb = scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols;
      if(dl_symb > 1) {
        timedomainresourceallocation = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
        timedomainresourceallocation->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
        timedomainresourceallocation->startSymbolAndLength = get_SLIV(len_coreset,dl_symb-len_coreset); // mixed slot configuration starting in symbol 1 til the end of the dl allocation
        ASN_SEQUENCE_ADD(&scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,
                         timedomainresourceallocation);
      }
    }
  }
}


void nr_rrc_config_ul_tda(NR_ServingCellConfigCommon_t *scc, int min_fb_delay){

  //TODO change to accomodate for SRS

  int temp_min_delay = 6; // k2 = 2 or 3 won'r work as well as higher values
  int k2 = (min_fb_delay<temp_min_delay)?temp_min_delay:min_fb_delay;

  uint8_t DELTA[4]= {2,3,4,6}; // Delta parameter for Msg3
  int mu = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
  lte_frame_type_t frame_type = get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);

  struct NR_PUSCH_TimeDomainResourceAllocation *pusch_timedomainresourceallocation = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
  pusch_timedomainresourceallocation->k2  = CALLOC(1,sizeof(long));
  *pusch_timedomainresourceallocation->k2 = k2;
  pusch_timedomainresourceallocation->mappingType = NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  pusch_timedomainresourceallocation->startSymbolAndLength = get_SLIV(0,13); // basic slot configuration starting in symbol 0 til the last but one symbol
  ASN_SEQUENCE_ADD(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,pusch_timedomainresourceallocation); 

  if(frame_type==TDD) {
    // TDD
    if(scc->tdd_UL_DL_ConfigurationCommon) {
      int ul_symb = scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols;
      pusch_timedomainresourceallocation = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
      pusch_timedomainresourceallocation->k2  = CALLOC(1,sizeof(long));
      *pusch_timedomainresourceallocation->k2 = k2;
      pusch_timedomainresourceallocation->mappingType = NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
      pusch_timedomainresourceallocation->startSymbolAndLength = get_SLIV(14-ul_symb,ul_symb-1); // starting in fist ul symbol til the last but one
      ASN_SEQUENCE_ADD(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,pusch_timedomainresourceallocation);

      // for msg3 in the mixed slot
      int nb_periods_per_frame = get_nb_periods_per_frame(scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity);
      int nb_slots_per_period = ((1<<mu) * 10)/nb_periods_per_frame;
      struct NR_PUSCH_TimeDomainResourceAllocation *pusch_timedomainresourceallocation_msg3 = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
      pusch_timedomainresourceallocation_msg3->k2  = CALLOC(1,sizeof(long));
      *pusch_timedomainresourceallocation_msg3->k2 = nb_slots_per_period - DELTA[mu];
      if(*pusch_timedomainresourceallocation_msg3->k2 < min_fb_delay)
        *pusch_timedomainresourceallocation_msg3->k2 += nb_slots_per_period;
      AssertFatal(*pusch_timedomainresourceallocation_msg3->k2<33,"Computed k2 for msg3 %ld is larger than the range allowed by RRC (0..32)\n",
                  *pusch_timedomainresourceallocation_msg3->k2);
      pusch_timedomainresourceallocation_msg3->mappingType = NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
      pusch_timedomainresourceallocation_msg3->startSymbolAndLength = get_SLIV(14-ul_symb,ul_symb-1); // starting in fist ul symbol til the last but one
      ASN_SEQUENCE_ADD(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,pusch_timedomainresourceallocation_msg3);
    }
  }
}

void set_dl_DataToUL_ACK(NR_PUCCH_Config_t *pucch_Config, int min_feedback_time) {

  pucch_Config->dl_DataToUL_ACK = calloc(1,sizeof(*pucch_Config->dl_DataToUL_ACK));
  long *delay[8];
  for (int i=0;i<8;i++) {
    delay[i] = calloc(1,sizeof(*delay[i]));
    *delay[i] = i+min_feedback_time;
    ASN_SEQUENCE_ADD(&pucch_Config->dl_DataToUL_ACK->list,delay[i]);
  }
}

// PUCCH resource set 0 for configuration with O_uci <= 2 bits and/or a positive or negative SR (section 9.2.1 of 38.213)
void config_pucch_resset0(NR_PUCCH_Config_t *pucch_Config, int uid, int curr_bwp, NR_UE_NR_Capability_t *uecap) {

  NR_PUCCH_ResourceSet_t *pucchresset = calloc(1,sizeof(*pucchresset));
  pucchresset->pucch_ResourceSetId = 0;
  NR_PUCCH_ResourceId_t *pucchid=calloc(1,sizeof(*pucchid));
  *pucchid=0;
  ASN_SEQUENCE_ADD(&pucchresset->resourceList.list,pucchid);
  pucchresset->maxPayloadSize=NULL;

  if(uecap) {
    long *pucch_F0_2WithoutFH = uecap->phy_Parameters.phy_ParametersFRX_Diff->pucch_F0_2WithoutFH;
    AssertFatal(pucch_F0_2WithoutFH == NULL,"UE does not support PUCCH F0 without frequency hopping. Current configuration is without FH\n");
  }

  NR_PUCCH_Resource_t *pucchres0=calloc(1,sizeof(*pucchres0));
  pucchres0->pucch_ResourceId=*pucchid;
  pucchres0->startingPRB= (8 + uid) % curr_bwp;
  pucchres0->intraSlotFrequencyHopping=NULL;
  pucchres0->secondHopPRB=NULL;
  pucchres0->format.present= NR_PUCCH_Resource__format_PR_format0;
  pucchres0->format.choice.format0=calloc(1,sizeof(*pucchres0->format.choice.format0));
  pucchres0->format.choice.format0->initialCyclicShift=0;
  pucchres0->format.choice.format0->nrofSymbols=1;
  pucchres0->format.choice.format0->startingSymbolIndex=13;
  ASN_SEQUENCE_ADD(&pucch_Config->resourceToAddModList->list,pucchres0);

  ASN_SEQUENCE_ADD(&pucch_Config->resourceSetToAddModList->list,pucchresset);
}


// PUCCH resource set 1 for configuration with O_uci > 2 bits (currently format2)
void config_pucch_resset1(NR_PUCCH_Config_t *pucch_Config, NR_UE_NR_Capability_t *uecap) {

  NR_PUCCH_ResourceSet_t *pucchresset=calloc(1,sizeof(*pucchresset));
  pucchresset->pucch_ResourceSetId = 1;
  NR_PUCCH_ResourceId_t *pucchressetid=calloc(1,sizeof(*pucchressetid));
  *pucchressetid=2;
  ASN_SEQUENCE_ADD(&pucchresset->resourceList.list,pucchressetid);
  pucchresset->maxPayloadSize=NULL;

  if(uecap) {
    long *pucch_F0_2WithoutFH = uecap->phy_Parameters.phy_ParametersFRX_Diff->pucch_F0_2WithoutFH;
    AssertFatal(pucch_F0_2WithoutFH == NULL,"UE does not support PUCCH F2 without frequency hopping. Current configuration is without FH\n");
  }

  NR_PUCCH_Resource_t *pucchres2=calloc(1,sizeof(*pucchres2));
  pucchres2->pucch_ResourceId=*pucchressetid;
  pucchres2->startingPRB=0;
  pucchres2->intraSlotFrequencyHopping=NULL;
  pucchres2->secondHopPRB=NULL;
  pucchres2->format.present= NR_PUCCH_Resource__format_PR_format2;
  pucchres2->format.choice.format2=calloc(1,sizeof(*pucchres2->format.choice.format2));
  pucchres2->format.choice.format2->nrofPRBs=8;
  pucchres2->format.choice.format2->nrofSymbols=1;
  pucchres2->format.choice.format2->startingSymbolIndex=13;
  ASN_SEQUENCE_ADD(&pucch_Config->resourceToAddModList->list,pucchres2);

  ASN_SEQUENCE_ADD(&pucch_Config->resourceSetToAddModList->list,pucchresset);

  pucch_Config->format2=calloc(1,sizeof(*pucch_Config->format2));
  pucch_Config->format2->present=NR_SetupRelease_PUCCH_FormatConfig_PR_setup;
  NR_PUCCH_FormatConfig_t *pucchfmt2 = calloc(1,sizeof(*pucchfmt2));
  pucch_Config->format2->choice.setup = pucchfmt2;
  pucchfmt2->interslotFrequencyHopping=NULL;
  pucchfmt2->additionalDMRS=NULL;
  pucchfmt2->maxCodeRate=calloc(1,sizeof(*pucchfmt2->maxCodeRate));
  *pucchfmt2->maxCodeRate=NR_PUCCH_MaxCodeRate_zeroDot35;
  pucchfmt2->nrofSlots=NULL;
  pucchfmt2->pi2BPSK=NULL;

  // to check UE capabilities for that in principle
  pucchfmt2->simultaneousHARQ_ACK_CSI=calloc(1,sizeof(*pucchfmt2->simultaneousHARQ_ACK_CSI));
  *pucchfmt2->simultaneousHARQ_ACK_CSI=NR_PUCCH_FormatConfig__simultaneousHARQ_ACK_CSI_true;

}

void set_pucch_power_config(NR_PUCCH_Config_t *pucch_Config, int do_csirs) {

  pucch_Config->pucch_PowerControl = calloc(1,sizeof(*pucch_Config->pucch_PowerControl));
  NR_P0_PUCCH_t *p00 = calloc(1,sizeof(*p00));
  p00->p0_PUCCH_Id = 1;
  p00->p0_PUCCH_Value = 0;
  pucch_Config->pucch_PowerControl->p0_Set = calloc(1,sizeof(*pucch_Config->pucch_PowerControl->p0_Set));
  ASN_SEQUENCE_ADD(&pucch_Config->pucch_PowerControl->p0_Set->list,p00);

  pucch_Config->pucch_PowerControl->pathlossReferenceRSs = calloc(1,sizeof(*pucch_Config->pucch_PowerControl->pathlossReferenceRSs));
  struct NR_PUCCH_PathlossReferenceRS *PL_ref_RS = calloc(1,sizeof(*PL_ref_RS));
  PL_ref_RS->pucch_PathlossReferenceRS_Id = 0;
  if(do_csirs) {
    PL_ref_RS->referenceSignal.present = NR_PUCCH_PathlossReferenceRS__referenceSignal_PR_csi_RS_Index;
    PL_ref_RS->referenceSignal.choice.csi_RS_Index = 0;
  }
  else {
    PL_ref_RS->referenceSignal.present = NR_PUCCH_PathlossReferenceRS__referenceSignal_PR_ssb_Index;
    PL_ref_RS->referenceSignal.choice.ssb_Index = 0;
  }
  ASN_SEQUENCE_ADD(&pucch_Config->pucch_PowerControl->pathlossReferenceRSs->list,PL_ref_RS);

  pucch_Config->pucch_PowerControl->deltaF_PUCCH_f0 = calloc(1,sizeof(*pucch_Config->pucch_PowerControl->deltaF_PUCCH_f0));
  *pucch_Config->pucch_PowerControl->deltaF_PUCCH_f0 = 0;
  pucch_Config->pucch_PowerControl->deltaF_PUCCH_f2 = calloc(1,sizeof(*pucch_Config->pucch_PowerControl->deltaF_PUCCH_f2));
  *pucch_Config->pucch_PowerControl->deltaF_PUCCH_f2 = 0;

  pucch_Config->spatialRelationInfoToAddModList = calloc(1,sizeof(*pucch_Config->spatialRelationInfoToAddModList));
  pucch_Config->spatialRelationInfoToReleaseList=NULL;
  NR_PUCCH_SpatialRelationInfo_t *pucchspatial = calloc(1,sizeof(*pucchspatial));
  pucchspatial->pucch_SpatialRelationInfoId = 1;
  pucchspatial->servingCellId = NULL;
  if(do_csirs) {
    pucchspatial->referenceSignal.present = NR_PUCCH_SpatialRelationInfo__referenceSignal_PR_csi_RS_Index;
    pucchspatial->referenceSignal.choice.csi_RS_Index = 0;
  }
  else {
    pucchspatial->referenceSignal.present = NR_PUCCH_SpatialRelationInfo__referenceSignal_PR_ssb_Index;
    pucchspatial->referenceSignal.choice.ssb_Index = 0;
  }

  pucchspatial->pucch_PathlossReferenceRS_Id = PL_ref_RS->pucch_PathlossReferenceRS_Id;
  pucchspatial->p0_PUCCH_Id = p00->p0_PUCCH_Id;
  pucchspatial->closedLoopIndex = NR_PUCCH_SpatialRelationInfo__closedLoopIndex_i0;
  ASN_SEQUENCE_ADD(&pucch_Config->spatialRelationInfoToAddModList->list,pucchspatial);
}

void set_SR_periodandoffset(NR_SchedulingRequestResourceConfig_t *schedulingRequestResourceConfig,
                            NR_ServingCellConfigCommon_t *scc) {

  const NR_TDD_UL_DL_Pattern_t *tdd = scc->tdd_UL_DL_ConfigurationCommon ? &scc->tdd_UL_DL_ConfigurationCommon->pattern1 : NULL;
  int sr_slot = 0;
  if(tdd)
    sr_slot = tdd->nrofDownlinkSlots; // SR in the first uplink slot

  schedulingRequestResourceConfig->periodicityAndOffset = calloc(1,sizeof(*schedulingRequestResourceConfig->periodicityAndOffset));

  if(sr_slot<10){
    schedulingRequestResourceConfig->periodicityAndOffset->present = NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl10;
    schedulingRequestResourceConfig->periodicityAndOffset->choice.sl10 = sr_slot;
    return;
  }
  if(sr_slot<20){
    schedulingRequestResourceConfig->periodicityAndOffset->present = NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl20;
    schedulingRequestResourceConfig->periodicityAndOffset->choice.sl20 = sr_slot;
    return;
  }
  if(sr_slot<40){
    schedulingRequestResourceConfig->periodicityAndOffset->present = NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl40;
    schedulingRequestResourceConfig->periodicityAndOffset->choice.sl40 = sr_slot;
    return;
  }
  if(sr_slot<80){
    schedulingRequestResourceConfig->periodicityAndOffset->present = NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl80;
    schedulingRequestResourceConfig->periodicityAndOffset->choice.sl80 = sr_slot;
    return;
  }
  if(sr_slot<160){
    schedulingRequestResourceConfig->periodicityAndOffset->present = NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl160;
    schedulingRequestResourceConfig->periodicityAndOffset->choice.sl160 = sr_slot;
    return;
  }
  if(sr_slot<320){
    schedulingRequestResourceConfig->periodicityAndOffset->present = NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl320;
    schedulingRequestResourceConfig->periodicityAndOffset->choice.sl320 = sr_slot;
    return;
  }
  schedulingRequestResourceConfig->periodicityAndOffset->present = NR_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl640;
  schedulingRequestResourceConfig->periodicityAndOffset->choice.sl640 = sr_slot;
}

void schedulingrequest_config(NR_ServingCellConfigCommon_t *scc,
                              NR_MAC_CellGroupConfig_t *mac_CellGroupConfig,
                              NR_PUCCH_Config_t *pucch_Config) {

  mac_CellGroupConfig->schedulingRequestConfig = calloc(1, sizeof(*mac_CellGroupConfig->schedulingRequestConfig));
  mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList = CALLOC(1,sizeof(*mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList));
  struct NR_SchedulingRequestToAddMod *schedulingrequestlist = CALLOC(1,sizeof(*schedulingrequestlist));
  schedulingrequestlist->schedulingRequestId  = 0;
  schedulingrequestlist->sr_ProhibitTimer = NULL;
  schedulingrequestlist->sr_TransMax      = NR_SchedulingRequestToAddMod__sr_TransMax_n64;
  ASN_SEQUENCE_ADD(&(mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList->list),schedulingrequestlist);

  // format with <=2 bits in pucch resource set 0
  NR_PUCCH_ResourceSet_t *pucchresset = pucch_Config->resourceSetToAddModList->list.array[0];
  // assigning the 1st pucch resource in the set to scheduling request
  NR_PUCCH_ResourceId_t *pucchressetid = pucchresset->resourceList.list.array[0];

  pucch_Config->schedulingRequestResourceToAddModList = calloc(1,sizeof(*pucch_Config->schedulingRequestResourceToAddModList));
  NR_SchedulingRequestResourceConfig_t *schedulingRequestResourceConfig = calloc(1,sizeof(*schedulingRequestResourceConfig));
  schedulingRequestResourceConfig->schedulingRequestResourceId = 1;
  schedulingRequestResourceConfig->schedulingRequestID = 0;

  set_SR_periodandoffset(schedulingRequestResourceConfig, scc);

  schedulingRequestResourceConfig->resource = calloc(1,sizeof(*schedulingRequestResourceConfig->resource));
  *schedulingRequestResourceConfig->resource = *pucchressetid;
  ASN_SEQUENCE_ADD(&pucch_Config->schedulingRequestResourceToAddModList->list,schedulingRequestResourceConfig);
}

void set_dl_mcs_table(int scs, NR_UE_NR_Capability_t *cap,
                      NR_BWP_DownlinkDedicated_t *bwp_Dedicated,
                      NR_ServingCellConfigCommon_t *scc) {

  if (cap == NULL){
    bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table = NULL;
    return;
  }

  int band = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
  struct NR_FrequencyInfoDL__scs_SpecificCarrierList scs_list = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList;
  int bw_rb = -1;
  for(int i=0; i<scs_list.list.count; i++){
    if(scs == scs_list.list.array[i]->subcarrierSpacing){
      bw_rb = scs_list.list.array[i]->carrierBandwidth;
      break;
    }
  }
  AssertFatal(bw_rb>0,"Could not find scs-SpecificCarrierList element for scs %d",scs);
  int bw = get_supported_band_index(scs, band, bw_rb);
  AssertFatal(bw>0,"Supported band corresponding to %d RBs not found\n", bw_rb);

  bool supported = false;
  if (band>256) {
    for (int i=0;i<cap->rf_Parameters.supportedBandListNR.list.count;i++) {
      NR_BandNR_t *bandNRinfo = cap->rf_Parameters.supportedBandListNR.list.array[i];
      if(bandNRinfo->bandNR == band && bandNRinfo->pdsch_256QAM_FR2) {
        supported = true;
        break;
      }
    }
  }
  else if (cap->phy_Parameters.phy_ParametersFR1 && cap->phy_Parameters.phy_ParametersFR1->pdsch_256QAM_FR1)
    supported = true;

  if (supported) {
    if(bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table == NULL)
      bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table = calloc(1, sizeof(*bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table));
    *bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table = NR_PDSCH_Config__mcs_Table_qam256;
  }
  else
    bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table = NULL;
}

