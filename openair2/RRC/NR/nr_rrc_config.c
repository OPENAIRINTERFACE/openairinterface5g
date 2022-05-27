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

const uint8_t slotsperframe[5] = {10, 20, 40, 80, 160};


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

void set_csiim_offset(struct NR_CSI_ResourcePeriodicityAndOffset *periodicityAndOffset,
                      struct NR_CSI_ResourcePeriodicityAndOffset *target_periodicityAndOffset) {

  switch(periodicityAndOffset->present) {
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots4:
      periodicityAndOffset->choice.slots4 = target_periodicityAndOffset->choice.slots4;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots5:
      periodicityAndOffset->choice.slots5 = target_periodicityAndOffset->choice.slots5;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots8:
      periodicityAndOffset->choice.slots8 = target_periodicityAndOffset->choice.slots8;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots10:
      periodicityAndOffset->choice.slots10 = target_periodicityAndOffset->choice.slots10;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots16:
      periodicityAndOffset->choice.slots16 = target_periodicityAndOffset->choice.slots16;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots20:
      periodicityAndOffset->choice.slots20 = target_periodicityAndOffset->choice.slots20;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots32:
      periodicityAndOffset->choice.slots32 = target_periodicityAndOffset->choice.slots32;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots40:
      periodicityAndOffset->choice.slots40 = target_periodicityAndOffset->choice.slots40;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots64:
      periodicityAndOffset->choice.slots64 = target_periodicityAndOffset->choice.slots64;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots80:
      periodicityAndOffset->choice.slots80 = target_periodicityAndOffset->choice.slots80;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots160:
      periodicityAndOffset->choice.slots160 = target_periodicityAndOffset->choice.slots160;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots320:
      periodicityAndOffset->choice.slots320 = target_periodicityAndOffset->choice.slots320;
      break;
    case NR_CSI_ResourcePeriodicityAndOffset_PR_slots640:
      periodicityAndOffset->choice.slots640 = target_periodicityAndOffset->choice.slots640;
      break;
    default:
      AssertFatal(1==0,"CSI periodicity not among allowed values\n");
  }

}

void config_csiim(int do_csirs, int dl_antenna_ports, int curr_bwp,
                  NR_CSI_MeasConfig_t *csi_MeasConfig) {

 if (do_csirs && dl_antenna_ports > 1) {
   csi_MeasConfig->csi_IM_ResourceToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_IM_ResourceToAddModList));
   NR_CSI_IM_Resource_t *imres = calloc(1,sizeof(*imres));
   imres->csi_IM_ResourceId = 0;
   NR_NZP_CSI_RS_Resource_t *nzpcsi = NULL;
   for (int i=0; i<csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.count; i++){
     nzpcsi = csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList->list.array[i];
     if (nzpcsi->nzp_CSI_RS_ResourceId == imres->csi_IM_ResourceId)
       break;
   }
   AssertFatal(nzpcsi->nzp_CSI_RS_ResourceId == imres->csi_IM_ResourceId, "Couldn't find NZP CSI-RS corresponding to CSI-IM\n");
   imres->csi_IM_ResourceElementPattern = calloc(1,sizeof(*imres->csi_IM_ResourceElementPattern));
   imres->csi_IM_ResourceElementPattern->present = NR_CSI_IM_Resource__csi_IM_ResourceElementPattern_PR_pattern1;
   imres->csi_IM_ResourceElementPattern->choice.pattern1 = calloc(1,sizeof(*imres->csi_IM_ResourceElementPattern->choice.pattern1));
   // starting subcarrier is 4 in the following configuration
   // this is ok for current possible CSI-RS configurations (using only the first 4 symbols)
   // TODO needs a more dynamic setting if CSI-RS is changed
   imres->csi_IM_ResourceElementPattern->choice.pattern1->subcarrierLocation_p1 = NR_CSI_IM_Resource__csi_IM_ResourceElementPattern__pattern1__subcarrierLocation_p1_s4;
   imres->csi_IM_ResourceElementPattern->choice.pattern1->symbolLocation_p1 = nzpcsi->resourceMapping.firstOFDMSymbolInTimeDomain; // same symbol as CSI-RS
   imres->freqBand = calloc(1,sizeof(*imres->freqBand));
   imres->freqBand->startingRB = 0;
   imres->freqBand->nrofRBs = ((curr_bwp>>2)+(curr_bwp%4>0))<<2;
   imres->periodicityAndOffset = calloc(1,sizeof(*imres->periodicityAndOffset));
   // same period and offset of the associated CSI-RS
   imres->periodicityAndOffset->present = nzpcsi->periodicityAndOffset->present;
   set_csiim_offset(imres->periodicityAndOffset, nzpcsi->periodicityAndOffset);
   ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_IM_ResourceToAddModList->list,imres);
   csi_MeasConfig->csi_IM_ResourceSetToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_IM_ResourceSetToAddModList));
   NR_CSI_IM_ResourceSet_t *imset = calloc(1,sizeof(*imset));
   imset->csi_IM_ResourceSetId = 0;
   NR_CSI_IM_ResourceId_t *res = calloc(1,sizeof(*res));
   *res = imres->csi_IM_ResourceId;
   ASN_SEQUENCE_ADD(&imset->csi_IM_Resources,res);
   ASN_SEQUENCE_ADD(&csi_MeasConfig->csi_IM_ResourceSetToAddModList->list,imset);
 }
 else {
   csi_MeasConfig->csi_IM_ResourceToAddModList = NULL;
   csi_MeasConfig->csi_IM_ResourceSetToAddModList = NULL;
 }

 csi_MeasConfig->csi_IM_ResourceToReleaseList = NULL;
 csi_MeasConfig->csi_IM_ResourceSetToReleaseList = NULL;
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

void nr_rrc_config_dl_tda(NR_ServingCellConfigCommon_t *scc,
                          NR_PDSCH_TimeDomainResourceAllocationList_t	*pdsch_TimeDomainAllocationList,
                          int curr_bwp) {

  frame_type_t frame_type = get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);

  // coreset duration setting to be improved in the framework of RRC harmonization, potentially using a common function
  int len_coreset = 1;
  if (curr_bwp < 48)
    len_coreset = 2;
  // setting default TDA for DL with TDA index 0
  struct NR_PDSCH_TimeDomainResourceAllocation *timedomainresourceallocation = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
  // k0: Slot offset between DCI and its scheduled PDSCH (see TS 38.214 clause 5.1.2.1) When the field is absent the UE applies the value 0.
  //timedomainresourceallocation->k0 = calloc(1,sizeof(*timedomainresourceallocation->k0));
  //*timedomainresourceallocation->k0 = 0;
  timedomainresourceallocation->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  timedomainresourceallocation->startSymbolAndLength = get_SLIV(len_coreset,14-len_coreset); // basic slot configuration starting in symbol 1 til the end of the slot
  ASN_SEQUENCE_ADD(&pdsch_TimeDomainAllocationList->list, timedomainresourceallocation);
  // setting TDA for CSI-RS symbol with index 1
  struct NR_PDSCH_TimeDomainResourceAllocation *timedomainresourceallocation1 = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
  timedomainresourceallocation1->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  timedomainresourceallocation1->startSymbolAndLength = get_SLIV(len_coreset,14-len_coreset-1); // 1 symbol CSI-RS
  ASN_SEQUENCE_ADD(&pdsch_TimeDomainAllocationList->list, timedomainresourceallocation1);
  if(frame_type==TDD) {
    if(scc->tdd_UL_DL_ConfigurationCommon) {
      int dl_symb = scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols;
      if(dl_symb > 1) {
        // mixed slot TDA with TDA index 2
        struct NR_PDSCH_TimeDomainResourceAllocation *timedomainresourceallocation2 = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
        timedomainresourceallocation2->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
        timedomainresourceallocation2->startSymbolAndLength = get_SLIV(len_coreset,dl_symb-len_coreset); // mixed slot configuration starting in symbol 1 til the end of the dl allocation
        ASN_SEQUENCE_ADD(&pdsch_TimeDomainAllocationList->list, timedomainresourceallocation2);
      }
    }
  }
}


void nr_rrc_config_ul_tda(NR_ServingCellConfigCommon_t *scc, int min_fb_delay){

  //TODO change to accomodate for SRS

  frame_type_t frame_type = get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);
  const int k2 = min_fb_delay;

  uint8_t DELTA[4]= {2,3,4,6}; // Delta parameter for Msg3
  int mu = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;

  // UL TDA index 0 is basic slot configuration starting in symbol 0 til the last but one symbol
  struct NR_PUSCH_TimeDomainResourceAllocation *pusch_timedomainresourceallocation = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
  pusch_timedomainresourceallocation->k2  = CALLOC(1,sizeof(long));
  *pusch_timedomainresourceallocation->k2 = k2;
  pusch_timedomainresourceallocation->mappingType = NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  pusch_timedomainresourceallocation->startSymbolAndLength = get_SLIV(0,13); 
  ASN_SEQUENCE_ADD(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,pusch_timedomainresourceallocation); 

  if(frame_type==TDD) {
    if(scc->tdd_UL_DL_ConfigurationCommon) {
      int ul_symb = scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols;
      if (ul_symb>1) {
        // UL TDA index 1 for mixed slot (TDD)
        pusch_timedomainresourceallocation = CALLOC(1,sizeof(struct NR_PUSCH_TimeDomainResourceAllocation));
        pusch_timedomainresourceallocation->k2  = CALLOC(1,sizeof(long));
        *pusch_timedomainresourceallocation->k2 = k2;
        pusch_timedomainresourceallocation->mappingType = NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
        pusch_timedomainresourceallocation->startSymbolAndLength = get_SLIV(14-ul_symb,ul_symb-1); // starting in fist ul symbol til the last but one
        ASN_SEQUENCE_ADD(&scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list,pusch_timedomainresourceallocation);

        // UL TDA index 2 for msg3 in the mixed slot (TDD)
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
}


void set_dl_mcs_table(int scs, NR_UE_NR_Capability_t *cap,
                      NR_SpCellConfig_t *SpCellConfig,
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
// set table 2 in correct entry in SpCellConfig->spCellConfigDedicated->csi_MeasConfig->csi_ReportConfigToAddModList->list 
    AssertFatal(SpCellConfig!=NULL,"SpCellConfig shouldn't be null\n");
    AssertFatal(SpCellConfig->spCellConfigDedicated!=NULL,"SpCellConfigDedicated shouldn't be null\n");
    if (SpCellConfig->spCellConfigDedicated->csi_MeasConfig &&
        SpCellConfig->spCellConfigDedicated->csi_MeasConfig->present== NR_SetupRelease_CSI_MeasConfig_PR_setup &&
        SpCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup &&
        SpCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup->csi_ReportConfigToAddModList)
       for (int i=0;i<SpCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup->csi_ReportConfigToAddModList->list.count;i++) {
          if (SpCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup->csi_ReportConfigToAddModList->list.array[i]->cqi_Table)
             *SpCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup->csi_ReportConfigToAddModList->list.array[i]->cqi_Table=NR_CSI_ReportConfig__cqi_Table_table2;
       } 
  }
  else
    bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table = NULL;
}



