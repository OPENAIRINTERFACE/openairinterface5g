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
#include "executables/softmodem-common.h"

const uint8_t slotsperframe[5] = {10, 20, 40, 80, 160};


void rrc_coreset_config(NR_ControlResourceSet_t *coreset,
                        int curr_bwp,
                        uint64_t ssb_bitmap) {

  // frequency domain resources depends on BWP size
  coreset->frequencyDomainResources.buf = calloc(1,6);
  coreset->frequencyDomainResources.buf[0] = (curr_bwp < 48) ? 0xf0 : 0xff;
  coreset->frequencyDomainResources.buf[1] = (curr_bwp < 96) ? 0x00 : 0xff;
  coreset->frequencyDomainResources.buf[2] = (curr_bwp < 144) ? 0x00 : 0xff;
  coreset->frequencyDomainResources.buf[3] = (curr_bwp < 192) ? 0x00 : 0xff;
  coreset->frequencyDomainResources.buf[4] = 0;
  coreset->frequencyDomainResources.buf[5] = 0;
  coreset->frequencyDomainResources.size = 6;
  coreset->frequencyDomainResources.bits_unused = 3;
  coreset->duration = (curr_bwp < 48) ? 2 : 1;
  coreset->cce_REG_MappingType.present = NR_ControlResourceSet__cce_REG_MappingType_PR_nonInterleaved;
  coreset->precoderGranularity = NR_ControlResourceSet__precoderGranularity_sameAsREG_bundle;

  // The ID space is used across the BWPs of a Serving Cell as per 38.331
  coreset->controlResourceSetId = (coreset->frequencyDomainResources.buf[0] > 0) +
                                  (coreset->frequencyDomainResources.buf[1] > 0) +
                                  (coreset->frequencyDomainResources.buf[2] > 0) +
                                  (coreset->frequencyDomainResources.buf[3] > 0);

  coreset->tci_StatesPDCCH_ToAddList=calloc(1,sizeof(*coreset->tci_StatesPDCCH_ToAddList));
  NR_TCI_StateId_t *tci[64];
  for (int i=0;i<64;i++) {
    if ((ssb_bitmap>>(63-i))&0x01){
      tci[i]=calloc(1,sizeof(*tci[i]));
      *tci[i] = i;
      ASN_SEQUENCE_ADD(&coreset->tci_StatesPDCCH_ToAddList->list,tci[i]);
    }
  }
  coreset->tci_StatesPDCCH_ToReleaseList = NULL;
  coreset->tci_PresentInDCI = NULL;
  coreset->pdcch_DMRS_ScramblingID = NULL;
}

uint64_t get_ssb_bitmap(NR_ServingCellConfigCommon_t *scc) {
  uint64_t bitmap=0;
  switch (scc->ssb_PositionsInBurst->present) {
    case 1 :
      bitmap = ((uint64_t) scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0])<<56;
      break;
    case 2 :
      bitmap = ((uint64_t) scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0])<<56;
      break;
    case 3 :
      for (int i=0; i<8; i++) {
        bitmap |= (((uint64_t) scc->ssb_PositionsInBurst->choice.longBitmap.buf[i])<<((7-i)*8));
      }
      break;
    default:
      AssertFatal(1==0,"SSB bitmap size value %d undefined (allowed values 1,2,3) \n", scc->ssb_PositionsInBurst->present);
  }
  return bitmap;
}

void set_csirs_periodicity(NR_NZP_CSI_RS_Resource_t *nzpcsi0, int uid, int nb_slots_per_period) {

  nzpcsi0->periodicityAndOffset = calloc(1,sizeof(*nzpcsi0->periodicityAndOffset));
  int ideal_period = nb_slots_per_period*MAX_MOBILES_PER_GNB;

  if (ideal_period<5) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots4;
    nzpcsi0->periodicityAndOffset->choice.slots4 = nb_slots_per_period*uid;
  }
  else if (ideal_period<6) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots5;
    nzpcsi0->periodicityAndOffset->choice.slots5 = nb_slots_per_period*uid;
  }
  else if (ideal_period<9) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots8;
    nzpcsi0->periodicityAndOffset->choice.slots8 = nb_slots_per_period*uid;
  }
  else if (ideal_period<11) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots10;
    nzpcsi0->periodicityAndOffset->choice.slots10 = nb_slots_per_period*uid;
  }
  else if (ideal_period<17) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots16;
    nzpcsi0->periodicityAndOffset->choice.slots16 = nb_slots_per_period*uid;
  }
  else if (ideal_period<21) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots20;
    nzpcsi0->periodicityAndOffset->choice.slots20 = nb_slots_per_period*uid;
  }
  else if (ideal_period<41) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots40;
    nzpcsi0->periodicityAndOffset->choice.slots40 = nb_slots_per_period*uid;
  }
  else if (ideal_period<81) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots80;
    nzpcsi0->periodicityAndOffset->choice.slots80 = nb_slots_per_period*uid;
  }
  else if (ideal_period<161) {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots160;
    nzpcsi0->periodicityAndOffset->choice.slots160 = nb_slots_per_period*uid;
  }
  else {
    nzpcsi0->periodicityAndOffset->present = NR_CSI_ResourcePeriodicityAndOffset_PR_slots320;
    nzpcsi0->periodicityAndOffset->choice.slots320 = (nb_slots_per_period*uid)%320 + (nb_slots_per_period*uid)/320;
  }
}


void config_csirs(NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                  NR_CSI_MeasConfig_t *csi_MeasConfig,
                  int uid,
                  int num_dl_antenna_ports,
                  int curr_bwp,
                  int do_csirs) {

  if (do_csirs) {

    csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList  = calloc(1,sizeof(*csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList));
    NR_NZP_CSI_RS_ResourceSet_t *nzpcsirs0 = calloc(1,sizeof(*nzpcsirs0));
    nzpcsirs0->nzp_CSI_ResourceSetId = 0;
    NR_NZP_CSI_RS_ResourceId_t *nzpid0 = calloc(1,sizeof(*nzpid0));
    *nzpid0 = 0;
    ASN_SEQUENCE_ADD(&nzpcsirs0->nzp_CSI_RS_Resources,nzpid0);
    nzpcsirs0->repetition = NULL;
    nzpcsirs0->aperiodicTriggeringOffset = NULL;
    nzpcsirs0->trs_Info = NULL;
    ASN_SEQUENCE_ADD(&csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list,nzpcsirs0);

    const NR_TDD_UL_DL_Pattern_t *tdd = servingcellconfigcommon->tdd_UL_DL_ConfigurationCommon ?
                                        &servingcellconfigcommon->tdd_UL_DL_ConfigurationCommon->pattern1 : NULL;

    const int n_slots_frame = slotsperframe[*servingcellconfigcommon->ssbSubcarrierSpacing];

    int nb_slots_per_period = n_slots_frame;
    if (tdd)
      nb_slots_per_period = n_slots_frame/get_nb_periods_per_frame(tdd->dl_UL_TransmissionPeriodicity);

    csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList = calloc(1,sizeof(*csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList));
    NR_NZP_CSI_RS_Resource_t *nzpcsi0 = calloc(1,sizeof(*nzpcsi0));
    nzpcsi0->nzp_CSI_RS_ResourceId = 0;
    NR_CSI_RS_ResourceMapping_t resourceMapping;
    switch (num_dl_antenna_ports) {
      case 1:
        resourceMapping.frequencyDomainAllocation.present = NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row2;
        resourceMapping.frequencyDomainAllocation.choice.row2.buf = calloc(2, sizeof(uint8_t));
        resourceMapping.frequencyDomainAllocation.choice.row2.size = 2;
        resourceMapping.frequencyDomainAllocation.choice.row2.bits_unused = 4;
        resourceMapping.frequencyDomainAllocation.choice.row2.buf[0] = 0;
        resourceMapping.frequencyDomainAllocation.choice.row2.buf[1] = 16;
        resourceMapping.nrofPorts = NR_CSI_RS_ResourceMapping__nrofPorts_p1;
        resourceMapping.cdm_Type = NR_CSI_RS_ResourceMapping__cdm_Type_noCDM;
        break;
      case 2:
        resourceMapping.frequencyDomainAllocation.present = NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_other;
        resourceMapping.frequencyDomainAllocation.choice.other.buf = calloc(2, sizeof(uint8_t));
        resourceMapping.frequencyDomainAllocation.choice.other.size = 1;
        resourceMapping.frequencyDomainAllocation.choice.other.bits_unused = 2;
        resourceMapping.frequencyDomainAllocation.choice.other.buf[0] = 4;
        resourceMapping.nrofPorts = NR_CSI_RS_ResourceMapping__nrofPorts_p2;
        resourceMapping.cdm_Type = NR_CSI_RS_ResourceMapping__cdm_Type_fd_CDM2;
        break;
      case 4:
        resourceMapping.frequencyDomainAllocation.present = NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row4;
        resourceMapping.frequencyDomainAllocation.choice.row4.buf = calloc(2, sizeof(uint8_t));
        resourceMapping.frequencyDomainAllocation.choice.row4.size = 1;
        resourceMapping.frequencyDomainAllocation.choice.row4.bits_unused = 5;
        resourceMapping.frequencyDomainAllocation.choice.row4.buf[0] = 32;
        resourceMapping.nrofPorts = NR_CSI_RS_ResourceMapping__nrofPorts_p4;
        resourceMapping.cdm_Type = NR_CSI_RS_ResourceMapping__cdm_Type_fd_CDM2;
        break;
      default:
        AssertFatal(1==0,"Number of ports not yet supported\n");
    }
    resourceMapping.firstOFDMSymbolInTimeDomain = 13;  // last symbol of slot
    resourceMapping.firstOFDMSymbolInTimeDomain2 = NULL;
    resourceMapping.density.present = NR_CSI_RS_ResourceMapping__density_PR_one;
    resourceMapping.density.choice.one = (NULL_t)0;
    resourceMapping.freqBand.startingRB = 0;
    resourceMapping.freqBand.nrofRBs = ((curr_bwp>>2)+(curr_bwp%4>0))<<2;
    nzpcsi0->resourceMapping = resourceMapping;
    nzpcsi0->powerControlOffset = 0;
    nzpcsi0->powerControlOffsetSS=calloc(1,sizeof(*nzpcsi0->powerControlOffsetSS));
    *nzpcsi0->powerControlOffsetSS = NR_NZP_CSI_RS_Resource__powerControlOffsetSS_db0;
    nzpcsi0->scramblingID = *servingcellconfigcommon->physCellId;
    set_csirs_periodicity(nzpcsi0, uid, nb_slots_per_period);
    nzpcsi0->qcl_InfoPeriodicCSI_RS = NULL;
    ASN_SEQUENCE_ADD(&csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list,nzpcsi0);
  }
  else {
    csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList = NULL;
    csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList  = NULL;
  }
  csi_MeasConfig->nzp_CSI_RS_ResourceSetToReleaseList = NULL;
  csi_MeasConfig->nzp_CSI_RS_ResourceToReleaseList = NULL;
}

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

void nr_rrc_config_dl_tda(struct NR_PDSCH_TimeDomainResourceAllocationList *pdsch_TimeDomainAllocationList,
                          lte_frame_type_t frame_type,
                          NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon,
                          int curr_bwp) {

  // coreset duration setting to be improved in the framework of RRC harmonization, potentially using a common function
  int len_coreset = 1;
  if (curr_bwp < 48)
    len_coreset = 2;
  // setting default TDA for DL with
  struct NR_PDSCH_TimeDomainResourceAllocation *timedomainresourceallocation = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
  timedomainresourceallocation->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  timedomainresourceallocation->startSymbolAndLength = get_SLIV(len_coreset,14-len_coreset); // basic slot configuration starting in symbol 1 til the end of the slot
  ASN_SEQUENCE_ADD(&pdsch_TimeDomainAllocationList->list, timedomainresourceallocation);
  if(frame_type==TDD) {
    // TDD
    if(tdd_UL_DL_ConfigurationCommon) {
      int dl_symb = tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols;
      if(dl_symb > 1) {
        timedomainresourceallocation = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
        timedomainresourceallocation->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
        timedomainresourceallocation->startSymbolAndLength = get_SLIV(len_coreset,dl_symb-len_coreset); // mixed slot configuration starting in symbol 1 til the end of the dl allocation
        ASN_SEQUENCE_ADD(&pdsch_TimeDomainAllocationList->list, timedomainresourceallocation);
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


void config_downlinkBWP(NR_BWP_Downlink_t *bwp,
                        NR_ServingCellConfigCommon_t *scc,
                        NR_ServingCellConfig_t *servingcellconfigdedicated,
                        NR_UE_NR_Capability_t *uecap,
                        int dl_antenna_ports,
                        bool force_256qam_off,
                        int bwp_loop, bool is_SA) {

  bwp->bwp_Common = calloc(1,sizeof(*bwp->bwp_Common));

  if(servingcellconfigdedicated->downlinkBWP_ToAddModList &&
     bwp_loop < servingcellconfigdedicated->downlinkBWP_ToAddModList->list.count) {
    bwp->bwp_Id = servingcellconfigdedicated->downlinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Id;
    bwp->bwp_Common->genericParameters.locationAndBandwidth = servingcellconfigdedicated->downlinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Common->genericParameters.locationAndBandwidth;
    bwp->bwp_Common->genericParameters.subcarrierSpacing = servingcellconfigdedicated->downlinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Common->genericParameters.subcarrierSpacing;
    bwp->bwp_Common->genericParameters.cyclicPrefix = servingcellconfigdedicated->downlinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Common->genericParameters.cyclicPrefix;
  } else {
    bwp->bwp_Id=bwp_loop+1;
    bwp->bwp_Common->genericParameters.locationAndBandwidth = PRBalloc_to_locationandbandwidth(scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,0);
    bwp->bwp_Common->genericParameters.subcarrierSpacing = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
    bwp->bwp_Common->genericParameters.cyclicPrefix = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.cyclicPrefix;
  }

  bwp->bwp_Common->pdcch_ConfigCommon=calloc(1,sizeof(*bwp->bwp_Common->pdcch_ConfigCommon));
  bwp->bwp_Common->pdcch_ConfigCommon->present = NR_SetupRelease_PDCCH_ConfigCommon_PR_setup;
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup = calloc(1,sizeof(*bwp->bwp_Common->pdcch_ConfigCommon->choice.setup));
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->controlResourceSetZero=NULL;
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonControlResourceSet=calloc(1,sizeof(*bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonControlResourceSet));

  int curr_bwp = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);

  NR_ControlResourceSet_t *coreset = calloc(1,sizeof(*coreset));
  uint64_t ssb_bitmap = get_ssb_bitmap(scc);
  rrc_coreset_config(coreset, curr_bwp, ssb_bitmap);
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonControlResourceSet = coreset;

  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->searchSpaceZero=NULL;
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList=NULL;
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList=calloc(1,sizeof(*bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList));

  NR_SearchSpace_t *ss=calloc(1,sizeof(*ss));
  ss->searchSpaceId = 10+bwp->bwp_Id;
  ss->controlResourceSetId=calloc(1,sizeof(*ss->controlResourceSetId));
  *ss->controlResourceSetId=coreset->controlResourceSetId;
  ss->monitoringSlotPeriodicityAndOffset = calloc(1,sizeof(*ss->monitoringSlotPeriodicityAndOffset));
  ss->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
  ss->duration=NULL;
  ss->monitoringSymbolsWithinSlot = calloc(1,sizeof(*ss->monitoringSymbolsWithinSlot));
  ss->monitoringSymbolsWithinSlot->buf = calloc(1,2);
  // should be '1100 0000 0000 00'B (LSB first!), first two symols in slot, adjust if needed
  ss->monitoringSymbolsWithinSlot->buf[1] = 0;
  ss->monitoringSymbolsWithinSlot->buf[0] = 0xc0;
  ss->monitoringSymbolsWithinSlot->size = 2;
  ss->monitoringSymbolsWithinSlot->bits_unused = 2;
  ss->nrofCandidates = calloc(1,sizeof(*ss->nrofCandidates));
  // TODO write a function to program nr of candidates and aggregation level
  ss->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
  ss->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n0;
  ss->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n1;
  ss->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n0;
  ss->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n0;
  ss->searchSpaceType = calloc(1,sizeof(*ss->searchSpaceType));
  ss->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_common;
  ss->searchSpaceType->choice.common=calloc(1,sizeof(*ss->searchSpaceType->choice.common));
  ss->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 = calloc(1,sizeof(*ss->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0));
  ASN_SEQUENCE_ADD(&bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list,ss);

  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->searchSpaceSIB1=NULL;
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->searchSpaceOtherSystemInformation=NULL;
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->pagingSearchSpace=NULL;
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace=NULL;
  if(is_SA == false) {
    bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace=calloc(1,sizeof(*bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace));
    *bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace=ss->searchSpaceId;
  }
  bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ext1=NULL;
  bwp->bwp_Common->pdsch_ConfigCommon=calloc(1,sizeof(*bwp->bwp_Common->pdsch_ConfigCommon));
  bwp->bwp_Common->pdsch_ConfigCommon->present = NR_SetupRelease_PDSCH_ConfigCommon_PR_setup;
  bwp->bwp_Common->pdsch_ConfigCommon->choice.setup = calloc(1,sizeof(*bwp->bwp_Common->pdsch_ConfigCommon->choice.setup));
  bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList = calloc(1,sizeof(*bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList));

  nr_rrc_config_dl_tda(bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList,
                       get_frame_type((int)*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing),
                       scc->tdd_UL_DL_ConfigurationCommon,
                       curr_bwp);

  if (!bwp->bwp_Dedicated) {
    bwp->bwp_Dedicated=calloc(1,sizeof(*bwp->bwp_Dedicated));
  }
  bwp->bwp_Dedicated->pdcch_Config=calloc(1,sizeof(*bwp->bwp_Dedicated->pdcch_Config));
  bwp->bwp_Dedicated->pdcch_Config->present = NR_SetupRelease_PDCCH_Config_PR_setup;
  bwp->bwp_Dedicated->pdcch_Config->choice.setup = calloc(1,sizeof(*bwp->bwp_Dedicated->pdcch_Config->choice.setup));
  bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList = calloc(1,sizeof(*bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList));
  bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList = calloc(1,sizeof(*bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList));

  ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list, coreset);

  bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList = calloc(1,sizeof(*bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList));
  NR_SearchSpace_t *ss2 = calloc(1,sizeof(*ss2));
  ss2->searchSpaceId= 20+bwp->bwp_Id;
  ss2->controlResourceSetId=calloc(1,sizeof(*ss2->controlResourceSetId));
  *ss2->controlResourceSetId=coreset->controlResourceSetId;
  ss2->monitoringSlotPeriodicityAndOffset=calloc(1,sizeof(*ss2->monitoringSlotPeriodicityAndOffset));
  ss2->monitoringSlotPeriodicityAndOffset->present = NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
  ss2->monitoringSlotPeriodicityAndOffset->choice.sl1=(NULL_t)0;
  ss2->duration=NULL;
  ss2->monitoringSymbolsWithinSlot = calloc(1,sizeof(*ss2->monitoringSymbolsWithinSlot));
  ss2->monitoringSymbolsWithinSlot->buf = calloc(1,2);
  ss2->monitoringSymbolsWithinSlot->size = 2;
  ss2->monitoringSymbolsWithinSlot->buf[0]=0xc0;
  ss2->monitoringSymbolsWithinSlot->buf[1]=0x0;
  ss2->monitoringSymbolsWithinSlot->bits_unused = 2;
  ss2->nrofCandidates=calloc(1,sizeof(*ss2->nrofCandidates));
  ss2->nrofCandidates->aggregationLevel1 = NR_SearchSpace__nrofCandidates__aggregationLevel1_n0;
  ss2->nrofCandidates->aggregationLevel2 = NR_SearchSpace__nrofCandidates__aggregationLevel2_n4;
  ss2->nrofCandidates->aggregationLevel4 = NR_SearchSpace__nrofCandidates__aggregationLevel4_n0;
  ss2->nrofCandidates->aggregationLevel8 = NR_SearchSpace__nrofCandidates__aggregationLevel8_n0;
  ss2->nrofCandidates->aggregationLevel16 = NR_SearchSpace__nrofCandidates__aggregationLevel16_n0;
  ss2->searchSpaceType=calloc(1,sizeof(*ss2->searchSpaceType));
  ss2->searchSpaceType->present = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  ss2->searchSpaceType->choice.ue_Specific = calloc(1,sizeof(*ss2->searchSpaceType->choice.ue_Specific));
  ss2->searchSpaceType->choice.ue_Specific->dci_Formats=NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_1_And_1_1;
  ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list, ss2);

  bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToReleaseList = NULL;

  struct NR_SetupRelease_PDSCH_Config *pdsch_Config = bwp->bwp_Dedicated->pdsch_Config;
  if (!pdsch_Config) {
    pdsch_Config = calloc(1,sizeof(*pdsch_Config));
    pdsch_Config->present = NR_SetupRelease_PDSCH_Config_PR_setup;
    pdsch_Config->choice.setup = calloc(1,sizeof(*pdsch_Config->choice.setup));
    pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA = calloc(1,sizeof(*pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA));
    pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->present= NR_SetupRelease_DMRS_DownlinkConfig_PR_setup;
    pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup = calloc(1,sizeof(*pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup));
  }
  pdsch_Config->choice.setup->dataScramblingIdentityPDSCH = NULL;
  struct NR_SetupRelease_DMRS_DownlinkConfig *dmrs_DownlinkForPDSCH_MappingTypeA = pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA;

  if ((get_softmodem_params()->do_ra || get_softmodem_params()->phy_test) && dl_antenna_ports > 1) // for MIMO, we use DMRS Config Type 2 but only with OAI UE
    dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type=calloc(1,sizeof(*dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type));
  else
    dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type=NULL;
  dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->maxLength=NULL;
  dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->scramblingID0=NULL;
  dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->scramblingID1=NULL;
  dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = calloc(1,sizeof(*dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition));
  *dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1;

  pdsch_Config->choice.setup->resourceAllocation = NR_PDSCH_Config__resourceAllocation_resourceAllocationType1;
  pdsch_Config->choice.setup->prb_BundlingType.present = NR_PDSCH_Config__prb_BundlingType_PR_staticBundling;
  pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling = calloc(1,sizeof(*pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling));
  pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling->bundleSize = calloc(1,sizeof(*pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling->bundleSize));
  *pdsch_Config->choice.setup->prb_BundlingType.choice.staticBundling->bundleSize = NR_PDSCH_Config__prb_BundlingType__staticBundling__bundleSize_wideband;

  set_dl_mcs_table(bwp->bwp_Common->genericParameters.subcarrierSpacing,
                   force_256qam_off ? NULL : uecap,
                   bwp->bwp_Dedicated,
                   scc);

  int n_ssb = 0;
  NR_TCI_State_t *tcid[64];
  for (int i=0;i<64;i++) {
    if ((ssb_bitmap>>(63-i))&0x01){
      tcid[i]=calloc(1,sizeof(*tcid[i]));
      tcid[i]->tci_StateId=n_ssb;
      tcid[i]->qcl_Type1.cell=NULL;
      tcid[i]->qcl_Type1.bwp_Id=calloc(1,sizeof(*tcid[i]->qcl_Type1.bwp_Id));
      *tcid[i]->qcl_Type1.bwp_Id=bwp->bwp_Id;
      tcid[i]->qcl_Type1.referenceSignal.present = NR_QCL_Info__referenceSignal_PR_ssb;
      tcid[i]->qcl_Type1.referenceSignal.choice.ssb = i;
      tcid[i]->qcl_Type1.qcl_Type=NR_QCL_Info__qcl_Type_typeC;
      ASN_SEQUENCE_ADD(&bwp->bwp_Dedicated->pdsch_Config->choice.setup->tci_StatesToAddModList->list,tcid[i]);
      n_ssb++;
    }
  }
}



