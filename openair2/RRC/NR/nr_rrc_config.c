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

void nr_rrc_config_dl_tda(NR_ServingCellConfigCommon_t *scc){

  lte_frame_type_t frame_type = get_frame_type(*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);

  // setting default TDA for DL with
  struct NR_PDSCH_TimeDomainResourceAllocation *timedomainresourceallocation = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
  timedomainresourceallocation->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  timedomainresourceallocation->startSymbolAndLength = get_SLIV(1,13); // basic slot configuration starting in symbol 1 til the end of the slot
  ASN_SEQUENCE_ADD(&scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list,
                   timedomainresourceallocation);

  if(frame_type==TDD) {
    // TDD
    if(scc->tdd_UL_DL_ConfigurationCommon) {
      int dl_symb = scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols;
      if(dl_symb > 1) {
        timedomainresourceallocation = CALLOC(1,sizeof(NR_PDSCH_TimeDomainResourceAllocation_t));
        timedomainresourceallocation->mappingType = NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
        timedomainresourceallocation->startSymbolAndLength = get_SLIV(1,dl_symb-1); // mixed slot configuration starting in symbol 1 til the end of the dl allocation
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

