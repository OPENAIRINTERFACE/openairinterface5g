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
                        int bwp_id,
                        int curr_bwp,
                        uint64_t ssb_bitmap) {

  // frequency domain resources depending on BWP size
  coreset->frequencyDomainResources.buf = calloc(1,6);
  coreset->frequencyDomainResources.buf[0] = (curr_bwp < 48) ? 0xf0 : 0xff;
  coreset->frequencyDomainResources.buf[1] = (curr_bwp < 96) ? 0x00 : 0xff;
  coreset->frequencyDomainResources.buf[2] = (curr_bwp < 144) ? 0x00 : 0xff;
  coreset->frequencyDomainResources.buf[3] = (curr_bwp < 192) ? 0x00 : 0xff;
  coreset->frequencyDomainResources.buf[4] = (curr_bwp < 240) ? 0x00 : 0xff;
  coreset->frequencyDomainResources.buf[5] = 0x00;
  coreset->frequencyDomainResources.size = 6;
  coreset->frequencyDomainResources.bits_unused = 3;
  coreset->duration = (curr_bwp < 48) ? 2 : 1;
  coreset->cce_REG_MappingType.present = NR_ControlResourceSet__cce_REG_MappingType_PR_nonInterleaved;
  coreset->precoderGranularity = NR_ControlResourceSet__precoderGranularity_sameAsREG_bundle;

  // The ID space is used across the BWPs of a Serving Cell as per 38.331
  coreset->controlResourceSetId = bwp_id + 1;

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

uint64_t get_ssb_bitmap(const NR_ServingCellConfigCommon_t *scc) {
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

void config_csirs(const NR_ServingCellConfigCommon_t *servingcellconfigcommon,
                  NR_CSI_MeasConfig_t *csi_MeasConfig,
                  int uid,
                  int num_dl_antenna_ports,
                  int curr_bwp,
                  int do_csirs,
                  int id) {

  if (do_csirs) {

    if(!csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList)
      csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList  = calloc(1,sizeof(*csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList));
    NR_NZP_CSI_RS_ResourceSet_t *nzpcsirs0 = calloc(1,sizeof(*nzpcsirs0));
    nzpcsirs0->nzp_CSI_ResourceSetId = id;
    NR_NZP_CSI_RS_ResourceId_t *nzpid0 = calloc(1,sizeof(*nzpid0));
    *nzpid0 = id;
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

    if(!csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList)
      csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList = calloc(1,sizeof(*csi_MeasConfig->nzp_CSI_RS_ResourceToAddModList));
    NR_NZP_CSI_RS_Resource_t *nzpcsi0 = calloc(1,sizeof(*nzpcsi0));
    nzpcsi0->nzp_CSI_RS_ResourceId = id;
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
    nzpcsi0->qcl_InfoPeriodicCSI_RS = calloc(1,sizeof(*nzpcsi0->qcl_InfoPeriodicCSI_RS));
    *nzpcsi0->qcl_InfoPeriodicCSI_RS = 0;
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
                  NR_CSI_MeasConfig_t *csi_MeasConfig, int id) {

 if (do_csirs && dl_antenna_ports > 1) {
   csi_MeasConfig->csi_IM_ResourceToAddModList = calloc(1,sizeof(*csi_MeasConfig->csi_IM_ResourceToAddModList));
   NR_CSI_IM_Resource_t *imres = calloc(1,sizeof(*imres));
   imres->csi_IM_ResourceId = id;
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
   imset->csi_IM_ResourceSetId = id;
   NR_CSI_IM_ResourceId_t *res = calloc(1,sizeof(*res));
   *res = id;
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

// TODO: Implement to b_SRS = 1 and b_SRS = 2
long rrc_get_max_nr_csrs(const uint8_t max_rbs, const long b_SRS) {

  if(b_SRS>0) {
    LOG_E(NR_RRC,"rrc_get_max_nr_csrs(): Not implemented yet for b_SRS>0\n");
    return 0; // This c_srs is always valid
  }

  const uint16_t m_SRS[64] = { 4, 8, 12, 16, 16, 20, 24, 24, 28, 32, 36, 40, 48, 48, 52, 56, 60, 64, 72, 72, 76, 80, 88,
                               96, 96, 104, 112, 120, 120, 120, 128, 128, 128, 132, 136, 144, 144, 144, 144, 152, 160,
                               160, 160, 168, 176, 184, 192, 192, 192, 192, 208, 216, 224, 240, 240, 240, 240, 256, 256,
                               256, 264, 272, 272, 272 };

  long c_srs = 0;
  uint16_t m = 4;
  for(int c = 1; c<64; c++) {
    if(m_SRS[c]>m && m_SRS[c]<max_rbs) {
      c_srs = c;
      m = m_SRS[c];
    }
  }

  return c_srs;
}

void config_srs(NR_SetupRelease_SRS_Config_t *setup_release_srs_Config,
                const NR_UE_NR_Capability_t *uecap,
                const int curr_bwp,
                const int uid,
                const int res_id,
                const int do_srs) {

  setup_release_srs_Config->present = NR_SetupRelease_SRS_Config_PR_setup;

  NR_SRS_Config_t *srs_Config;
  if (setup_release_srs_Config->choice.setup) {
    srs_Config = setup_release_srs_Config->choice.setup;
    if (srs_Config->srs_ResourceSetToReleaseList) {
      free(srs_Config->srs_ResourceSetToReleaseList);
    }
    if (srs_Config->srs_ResourceSetToAddModList) {
      free(srs_Config->srs_ResourceSetToAddModList);
    }
    if (srs_Config->srs_ResourceToReleaseList) {
      free(srs_Config->srs_ResourceToReleaseList);
    }
    if (srs_Config->srs_ResourceToAddModList) {
      free(srs_Config->srs_ResourceToAddModList);
    }
    free(srs_Config);
  }

  setup_release_srs_Config->choice.setup = calloc(1,sizeof(*setup_release_srs_Config->choice.setup));
  srs_Config = setup_release_srs_Config->choice.setup;

  srs_Config->srs_ResourceSetToReleaseList = NULL;

  srs_Config->srs_ResourceSetToAddModList = calloc(1,sizeof(*srs_Config->srs_ResourceSetToAddModList));
  NR_SRS_ResourceSet_t *srs_resset0 = calloc(1,sizeof(*srs_resset0));
  srs_resset0->srs_ResourceSetId = res_id;
  srs_resset0->srs_ResourceIdList = calloc(1,sizeof(*srs_resset0->srs_ResourceIdList));
  NR_SRS_ResourceId_t *srs_resset0_id = calloc(1,sizeof(*srs_resset0_id));
  *srs_resset0_id = res_id;
  ASN_SEQUENCE_ADD(&srs_resset0->srs_ResourceIdList->list, srs_resset0_id);
  srs_Config->srs_ResourceToReleaseList=NULL;
  if (do_srs) {
    srs_resset0->resourceType.present =  NR_SRS_ResourceSet__resourceType_PR_periodic;
    srs_resset0->resourceType.choice.periodic = calloc(1,sizeof(*srs_resset0->resourceType.choice.periodic));
    srs_resset0->resourceType.choice.periodic->associatedCSI_RS = NULL;
  } else {
    srs_resset0->resourceType.present =  NR_SRS_ResourceSet__resourceType_PR_aperiodic;
    srs_resset0->resourceType.choice.aperiodic = calloc(1,sizeof(*srs_resset0->resourceType.choice.aperiodic));
    srs_resset0->resourceType.choice.aperiodic->aperiodicSRS_ResourceTrigger=1;
    srs_resset0->resourceType.choice.aperiodic->csi_RS=NULL;
    srs_resset0->resourceType.choice.aperiodic->slotOffset = calloc(1,sizeof(*srs_resset0->resourceType.choice.aperiodic->slotOffset));
    *srs_resset0->resourceType.choice.aperiodic->slotOffset = 2;
    srs_resset0->resourceType.choice.aperiodic->ext1 = NULL;
  }
  srs_resset0->usage=NR_SRS_ResourceSet__usage_codebook;
  srs_resset0->alpha = calloc(1,sizeof(*srs_resset0->alpha));
  *srs_resset0->alpha = NR_Alpha_alpha1;
  srs_resset0->p0 = calloc(1,sizeof(*srs_resset0->p0));
  *srs_resset0->p0 =-80;
  srs_resset0->pathlossReferenceRS = NULL;
  srs_resset0->srs_PowerControlAdjustmentStates = NULL;
  ASN_SEQUENCE_ADD(&srs_Config->srs_ResourceSetToAddModList->list,srs_resset0);

  srs_Config->srs_ResourceToReleaseList = NULL;

  srs_Config->srs_ResourceToAddModList = calloc(1,sizeof(*srs_Config->srs_ResourceToAddModList));
  NR_SRS_Resource_t *srs_res0=calloc(1,sizeof(*srs_res0));
  srs_res0->srs_ResourceId = res_id;
  srs_res0->nrofSRS_Ports = NR_SRS_Resource__nrofSRS_Ports_port1;
  //  if (uecap &&
  //      uecap->featureSets &&
  //      uecap->featureSets->featureSetsUplink &&
  //      uecap->featureSets->featureSetsUplink->list.count > 0) {
  //    NR_FeatureSetUplink_t *ul_feature_setup = uecap->featureSets->featureSetsUplink->list.array[0];
  //    switch (ul_feature_setup->supportedSRS_Resources->maxNumberSRS_Ports_PerResource) {
  //      case NR_SRS_Resources__maxNumberSRS_Ports_PerResource_n1:
  //        srs_res0->nrofSRS_Ports = NR_SRS_Resource__nrofSRS_Ports_port1;
  //        break;
  //      case NR_SRS_Resources__maxNumberSRS_Ports_PerResource_n2:
  //        srs_res0->nrofSRS_Ports = NR_SRS_Resource__nrofSRS_Ports_ports2;
  //        break;
  //      case NR_SRS_Resources__maxNumberSRS_Ports_PerResource_n4:
  //        srs_res0->nrofSRS_Ports = NR_SRS_Resource__nrofSRS_Ports_ports4;
  //        break;
  //      default:
  //        LOG_E(NR_RRC, "Max Number of SRS Ports Per Resource %ld is invalid!\n",
  //              ul_feature_setup->supportedSRS_Resources->maxNumberSRS_Ports_PerResource);
  //    }
  //  }
  srs_res0->ptrs_PortIndex = NULL;
  srs_res0->transmissionComb.present = NR_SRS_Resource__transmissionComb_PR_n2;
  srs_res0->transmissionComb.choice.n2 = calloc(1,sizeof(*srs_res0->transmissionComb.choice.n2));
  srs_res0->transmissionComb.choice.n2->combOffset_n2 = 0;
  srs_res0->transmissionComb.choice.n2->cyclicShift_n2 = 0;
  srs_res0->resourceMapping.startPosition = 2 + uid%2;
  srs_res0->resourceMapping.nrofSymbols = NR_SRS_Resource__resourceMapping__nrofSymbols_n1;
  srs_res0->resourceMapping.repetitionFactor = NR_SRS_Resource__resourceMapping__repetitionFactor_n1;
  srs_res0->freqDomainPosition = 0;
  srs_res0->freqDomainShift = 0;
  srs_res0->freqHopping.b_SRS = 0;
  srs_res0->freqHopping.b_hop = 0;
  srs_res0->freqHopping.c_SRS = rrc_get_max_nr_csrs(curr_bwp, srs_res0->freqHopping.b_SRS);
  srs_res0->groupOrSequenceHopping = NR_SRS_Resource__groupOrSequenceHopping_neither;
  if (do_srs) {
    srs_res0->resourceType.present = NR_SRS_Resource__resourceType_PR_periodic;
    srs_res0->resourceType.choice.periodic = calloc(1,sizeof(*srs_res0->resourceType.choice.periodic));
    srs_res0->resourceType.choice.periodic->periodicityAndOffset_p.present = NR_SRS_PeriodicityAndOffset_PR_sl160;
    srs_res0->resourceType.choice.periodic->periodicityAndOffset_p.choice.sl160 = 17 + (uid>1)*10; // 17/17/.../147/157 are mixed slots
  } else {
    srs_res0->resourceType.present = NR_SRS_Resource__resourceType_PR_aperiodic;
    srs_res0->resourceType.choice.aperiodic = calloc(1,sizeof(*srs_res0->resourceType.choice.aperiodic));
  }
  srs_res0->sequenceId = 40;
  srs_res0->spatialRelationInfo = calloc(1,sizeof(*srs_res0->spatialRelationInfo));
  srs_res0->spatialRelationInfo->servingCellId = NULL;
  srs_res0->spatialRelationInfo->referenceSignal.present = NR_SRS_SpatialRelationInfo__referenceSignal_PR_csi_RS_Index;
  srs_res0->spatialRelationInfo->referenceSignal.choice.csi_RS_Index = 0;
  ASN_SEQUENCE_ADD(&srs_Config->srs_ResourceToAddModList->list,srs_res0);
}

void prepare_sim_uecap(NR_UE_NR_Capability_t *cap,
                       NR_ServingCellConfigCommon_t *scc,
                       int numerology,
                       int rbsize,
                       int mcs_table) {

  NR_Phy_Parameters_t *phy_Parameters = &cap->phy_Parameters;
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
      phy_Parameters->phy_ParametersFR1 = CALLOC(1,sizeof(*phy_Parameters->phy_ParametersFR1));
      NR_Phy_ParametersFR1_t *phy_fr1 = phy_Parameters->phy_ParametersFR1;
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
    ASN_SEQUENCE_ADD(&fs->featureSetsDownlinkPerCC->list, fs_cc);
  }

  phy_Parameters->phy_ParametersFRX_Diff = CALLOC(1,sizeof(*phy_Parameters->phy_ParametersFRX_Diff));
  phy_Parameters->phy_ParametersFRX_Diff->pucch_F0_2WithoutFH = NULL;
}

void nr_rrc_config_dl_tda(struct NR_PDSCH_TimeDomainResourceAllocationList *pdsch_TimeDomainAllocationList,
                          frame_type_t frame_type,
                          NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon,
                          int curr_bwp) {

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
    // TDD
    if(tdd_UL_DL_ConfigurationCommon) {
      int dl_symb = tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols;
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

static void set_SR_periodandoffset(NR_SchedulingRequestResourceConfig_t *schedulingRequestResourceConfig,
                            const NR_ServingCellConfigCommon_t *scc)
{
  const NR_TDD_UL_DL_Pattern_t *tdd = scc->tdd_UL_DL_ConfigurationCommon ? &scc->tdd_UL_DL_ConfigurationCommon->pattern1 : NULL;
  int sr_slot = 1; // in FDD SR in slot 1
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

void scheduling_request_config(const NR_ServingCellConfigCommon_t *scc,
                               NR_PUCCH_Config_t *pucch_Config) {

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

void set_dl_mcs_table(int scs,
                      NR_UE_NR_Capability_t *cap,
                      NR_BWP_DownlinkDedicated_t *bwp_Dedicated,
                      const NR_ServingCellConfigCommon_t *scc) {

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
  AssertFatal(bw>=0,"Supported band corresponding to %d RBs not found\n", bw_rb);

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
                        const NR_ServingCellConfigCommon_t *scc,
                        const NR_ServingCellConfig_t *servingcellconfigdedicated,
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
  rrc_coreset_config(coreset, bwp->bwp_Id, curr_bwp, ssb_bitmap);
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
  ss->monitoringSymbolsWithinSlot->buf[0] = 0x80;
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
  ss2->monitoringSymbolsWithinSlot->buf[0]=0x80;
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

  struct NR_SetupRelease_PDSCH_Config *pdsch_Config = NULL;
  pdsch_Config = calloc(1,sizeof(*pdsch_Config));
  pdsch_Config->present = NR_SetupRelease_PDSCH_Config_PR_setup;
  pdsch_Config->choice.setup = calloc(1,sizeof(*pdsch_Config->choice.setup));
  pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA = calloc(1,sizeof(*pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA));
  pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->present= NR_SetupRelease_DMRS_DownlinkConfig_PR_setup;
  pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup = calloc(1,sizeof(*pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup));
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

  bwp->bwp_Dedicated->pdsch_Config = pdsch_Config;
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

void config_uplinkBWP(NR_BWP_Uplink_t *ubwp,
                      long bwp_loop, bool is_SA, int uid,
                      const gNB_RrcConfigurationReq *configuration,
                      const NR_ServingCellConfig_t *servingcellconfigdedicated,
                      const NR_ServingCellConfigCommon_t *scc,
                      NR_UE_NR_Capability_t *uecap) {

  ubwp->bwp_Common = calloc(1,sizeof(*ubwp->bwp_Common));
  if(servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList &&
     bwp_loop < servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count) {
    ubwp->bwp_Id = servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Id;
    ubwp->bwp_Common->genericParameters.locationAndBandwidth = servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Common->genericParameters.locationAndBandwidth;
    ubwp->bwp_Common->genericParameters.subcarrierSpacing = servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Common->genericParameters.subcarrierSpacing;
    ubwp->bwp_Common->genericParameters.cyclicPrefix = servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Common->genericParameters.cyclicPrefix;
  } else {
    ubwp->bwp_Id=bwp_loop+1;
    ubwp->bwp_Common->genericParameters.locationAndBandwidth = PRBalloc_to_locationandbandwidth(scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth,0);
    ubwp->bwp_Common->genericParameters.subcarrierSpacing = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
    ubwp->bwp_Common->genericParameters.cyclicPrefix = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.cyclicPrefix;
  }

  int curr_bwp = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth,MAX_BWP_SIZE);

  ubwp->bwp_Common->rach_ConfigCommon  = is_SA ? NULL : scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon;
  ubwp->bwp_Common->pusch_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon;
  ubwp->bwp_Common->pucch_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon;

  if (!ubwp->bwp_Dedicated) {
    ubwp->bwp_Dedicated = calloc(1,sizeof(*ubwp->bwp_Dedicated));
  }

  ubwp->bwp_Dedicated->pucch_Config = calloc(1,sizeof(*ubwp->bwp_Dedicated->pucch_Config));
  ubwp->bwp_Dedicated->pucch_Config->present = NR_SetupRelease_PUCCH_Config_PR_setup;
  NR_PUCCH_Config_t *pucch_Config = calloc(1,sizeof(*pucch_Config));
  ubwp->bwp_Dedicated->pucch_Config->choice.setup = pucch_Config;
  pucch_Config->resourceSetToAddModList = calloc(1,sizeof(*pucch_Config->resourceSetToAddModList));
  pucch_Config->resourceSetToReleaseList = NULL;
  pucch_Config->resourceToAddModList = calloc(1,sizeof(*pucch_Config->resourceToAddModList));
  pucch_Config->resourceToReleaseList = NULL;
  config_pucch_resset0(pucch_Config, uid, curr_bwp, uecap);
  config_pucch_resset1(pucch_Config, uecap);
  set_pucch_power_config(pucch_Config, configuration->do_CSIRS);
  scheduling_request_config(scc, pucch_Config);
  set_dl_DataToUL_ACK(pucch_Config, configuration->minRXTXTIME);

  ubwp->bwp_Dedicated->pusch_Config = calloc(1,sizeof(*ubwp->bwp_Dedicated->pusch_Config));
  ubwp->bwp_Dedicated->pusch_Config->present = NR_SetupRelease_PUSCH_Config_PR_setup;
  NR_PUSCH_Config_t *pusch_Config = NULL;
  if(servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList &&
     bwp_loop < servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count) {
    pusch_Config = servingcellconfigdedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_loop]->bwp_Dedicated->pusch_Config->choice.setup;
  } else {
    pusch_Config = calloc(1,sizeof(*pusch_Config));
  }
  ubwp->bwp_Dedicated->pusch_Config->choice.setup = pusch_Config;
  pusch_Config->txConfig=calloc(1,sizeof(*pusch_Config->txConfig));
  *pusch_Config->txConfig= NR_PUSCH_Config__txConfig_codebook;
  pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA = NULL;
  if (!pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB) {
    pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB = calloc(1,sizeof(*pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB));
    pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->present = NR_SetupRelease_DMRS_UplinkConfig_PR_setup;
    pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup = calloc(1,sizeof(*pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup));
  }
  NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup;
  NR_DMRS_UplinkConfig->dmrs_Type = NULL;
  NR_DMRS_UplinkConfig->dmrs_AdditionalPosition = calloc(1,sizeof(*NR_DMRS_UplinkConfig->dmrs_AdditionalPosition));
  *NR_DMRS_UplinkConfig->dmrs_AdditionalPosition = NR_DMRS_UplinkConfig__dmrs_AdditionalPosition_pos0;
  NR_DMRS_UplinkConfig->phaseTrackingRS=NULL;
  NR_DMRS_UplinkConfig->maxLength=NULL;
  NR_DMRS_UplinkConfig->transformPrecodingDisabled = calloc(1,sizeof(*NR_DMRS_UplinkConfig->transformPrecodingDisabled));
  NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID0 = NULL;
  NR_DMRS_UplinkConfig->transformPrecodingDisabled->scramblingID1 = NULL;
  NR_DMRS_UplinkConfig->transformPrecodingEnabled = NULL;
  pusch_Config->pusch_PowerControl = calloc(1,sizeof(*pusch_Config->pusch_PowerControl));
  pusch_Config->pusch_PowerControl->tpc_Accumulation = NULL;
  pusch_Config->pusch_PowerControl->msg3_Alpha = calloc(1,sizeof(*pusch_Config->pusch_PowerControl->msg3_Alpha));
  *pusch_Config->pusch_PowerControl->msg3_Alpha = NR_Alpha_alpha1;
  pusch_Config->pusch_PowerControl->p0_NominalWithoutGrant = NULL;
  pusch_Config->pusch_PowerControl->p0_AlphaSets = calloc(1,sizeof(*pusch_Config->pusch_PowerControl->p0_AlphaSets));
  NR_P0_PUSCH_AlphaSet_t *aset = calloc(1,sizeof(*aset));
  aset->p0_PUSCH_AlphaSetId=0;
  aset->p0=calloc(1,sizeof(*aset->p0));
  *aset->p0 = 0;
  aset->alpha=calloc(1,sizeof(*aset->alpha));
  *aset->alpha=NR_Alpha_alpha1;
  ASN_SEQUENCE_ADD(&pusch_Config->pusch_PowerControl->p0_AlphaSets->list,aset);
  pusch_Config->pusch_PowerControl->pathlossReferenceRSToAddModList = NULL;
  pusch_Config->pusch_PowerControl->pathlossReferenceRSToReleaseList = NULL;
  pusch_Config->pusch_PowerControl->twoPUSCH_PC_AdjustmentStates = NULL;
  pusch_Config->pusch_PowerControl->deltaMCS = NULL;
  pusch_Config->pusch_PowerControl->sri_PUSCH_MappingToAddModList = NULL;
  pusch_Config->pusch_PowerControl->sri_PUSCH_MappingToReleaseList = NULL;
  pusch_Config->frequencyHopping=NULL;
  pusch_Config->frequencyHoppingOffsetLists=NULL;
  pusch_Config->resourceAllocation = NR_PUSCH_Config__resourceAllocation_resourceAllocationType1;
  pusch_Config->pusch_TimeDomainAllocationList = NULL;
  pusch_Config->pusch_AggregationFactor=NULL;
  pusch_Config->mcs_Table=NULL;
  pusch_Config->mcs_TableTransformPrecoder=NULL;
  pusch_Config->transformPrecoder= NULL;
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL) {
    pusch_Config->transformPrecoder=calloc(1,sizeof(*pusch_Config->transformPrecoder));
    *pusch_Config->transformPrecoder = NR_PUSCH_Config__transformPrecoder_disabled;
  }
  pusch_Config->codebookSubset=calloc(1,sizeof(*pusch_Config->codebookSubset));
  *pusch_Config->codebookSubset = NR_PUSCH_Config__codebookSubset_nonCoherent;
  pusch_Config->maxRank=calloc(1,sizeof(*pusch_Config->maxRank));
  *pusch_Config->maxRank= 1;
  pusch_Config->rbg_Size=NULL;
  pusch_Config->uci_OnPUSCH=NULL;
  pusch_Config->tp_pi2BPSK=NULL;

  ubwp->bwp_Dedicated->srs_Config = calloc(1,sizeof(*ubwp->bwp_Dedicated->srs_Config));
  config_srs(ubwp->bwp_Dedicated->srs_Config,
             NULL,
             curr_bwp,
             uid,
             bwp_loop+1,
             configuration->do_SRS);

  ubwp->bwp_Dedicated->configuredGrantConfig = NULL;
  ubwp->bwp_Dedicated->beamFailureRecoveryConfig = NULL;
}

void set_phr_config(NR_MAC_CellGroupConfig_t *mac_CellGroupConfig)
{
  mac_CellGroupConfig->phr_Config                                         = calloc(1, sizeof(*mac_CellGroupConfig->phr_Config));
  mac_CellGroupConfig->phr_Config->present                                = NR_SetupRelease_PHR_Config_PR_setup;
  mac_CellGroupConfig->phr_Config->choice.setup                           = calloc(1, sizeof(*mac_CellGroupConfig->phr_Config->choice.setup));
  mac_CellGroupConfig->phr_Config->choice.setup->phr_PeriodicTimer        = NR_PHR_Config__phr_PeriodicTimer_sf10;
  mac_CellGroupConfig->phr_Config->choice.setup->phr_ProhibitTimer        = NR_PHR_Config__phr_ProhibitTimer_sf10;
  mac_CellGroupConfig->phr_Config->choice.setup->phr_Tx_PowerFactorChange = NR_PHR_Config__phr_Tx_PowerFactorChange_dB1;
}

