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

/*!\file openair1/SIMULATION/NR_PHY/nr_unitary_defs.h
 * \brief
 * \author Turker Yilmaz
 * \date 2019
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr
 * \note
 * \warning
*/

#ifndef __NR_UNITARY_DEFS__H__
#define __NR_UNITARY_DEFS__H__

volatile int oai_exit=0;

void exit_function(const char* file, const char* function, const int line, const char *s) {
  const char * msg= s==NULL ? "no comment": s;
  printf("Exiting at: %s:%d %s(), %s\n", file, line, function, msg);
  exit(-1);
}

signed char quantize(double D, double x, unsigned char B) {
  double qxd;
  short maxlev;
  qxd = floor(x / D);
  maxlev = 1 << (B - 1); //(char)(pow(2,B-1));

  if (qxd <= -maxlev)
    qxd = -maxlev;
  else if (qxd >= maxlev)
    qxd = maxlev - 1;

  return ((char) qxd);
}

int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind) {return(0);}
//NR_IF_Module_t *NR_IF_Module_init(int Mod_id){return(NULL);}
int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) { return(0); }


void fill_scc(NR_ServingCellConfigCommon_t *scc,uint64_t *ssb_bitmap,int N_RB_DL,int N_RB_UL,int mu_dl,int mu_ul) {

  *scc->physCellId=0;							\
  //  *scc->n_TimingAdvanceOffset=NR_ServingCellConfigCommon__n_TimingAdvanceOffset_n0;
  *scc->ssb_periodicityServingCell=NR_ServingCellConfigCommon__ssb_periodicityServingCell_ms20;
  scc->dmrs_TypeA_Position=NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2;
  *scc->ssbSubcarrierSpacing=NR_SubcarrierSpacing_kHz30;
  *scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB=641032;
  *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0]=78;
  scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA=640000;
  scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier=0;
  scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing=NR_SubcarrierSpacing_kHz30;

  scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth=N_RB_DL;
  scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth=13036;
  scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing=mu_dl;//NR_SubcarrierSpacing_kHz30;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->controlResourceSetZero=12;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->searchSpaceZero=0;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[0]->k0=0;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[0]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[0]->startSymbolAndLength=40;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[1]->k0=0;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[1]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[1]->startSymbolAndLength=53;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[2]->k0=0;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[2]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[2]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[3]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[3]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[3]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[4]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[4]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[4]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[5]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[5]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[5]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[6]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[6]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[6]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[7]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[7]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[7]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[8]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[8]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[8]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[9]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[9]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[9]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[10]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[10]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[10]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[11]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[11]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[11]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[12]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[12]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[12]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[13]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[13]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[13]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[14]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[14]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[14]->startSymbolAndLength=54;
  *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[15]->k0=-1;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[15]->mappingType=NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
  scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[15]->startSymbolAndLength=54;
  *scc->uplinkConfigCommon->frequencyInfoUL->frequencyBandList->list.array[0]=78;
  *scc->uplinkConfigCommon->frequencyInfoUL->absoluteFrequencyPointA=-1;
  scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier=0;
  scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing=NR_SubcarrierSpacing_kHz30;
  scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth=N_RB_UL;
  *scc->uplinkConfigCommon->frequencyInfoUL->p_Max=20;
  scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth=13036;
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
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[0]->k2=6;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[0]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[0]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[1]->k2=6;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[1]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[1]->startSymbolAndLength=69;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[2]->k2=6;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[2]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[2]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[3]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[3]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[3]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[4]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[4]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[4]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[5]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[5]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[5]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[6]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[6]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[6]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[7]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[7]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[7]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[8]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[8]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[8]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[9]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[9]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[9]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[10]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[10]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[10]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[11]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[11]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[11]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[12]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[12]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[12]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[13]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[13]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[13]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[14]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[14]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[14]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[15]->k2=33;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[15]->mappingType=NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB;
  scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[15]->startSymbolAndLength=55;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble=1;
  *scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->p0_NominalWithGrant=-90;
 scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_GroupHopping=NR_PUCCH_ConfigCommon__pucch_GroupHopping_neither; 
 *scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->hoppingId=40;
 *scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->p0_nominal=-90;
 scc->ssb_PositionsInBurst->present=NR_ServingCellConfigCommon__ssb_PositionsInBurst_PR_mediumBitmap;
 *ssb_bitmap=0xff;
 scc->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing=NR_SubcarrierSpacing_kHz30;
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

void fix_scc(NR_ServingCellConfigCommon_t *scc,uint64_t ssbmap);
void prepare_scc(NR_ServingCellConfigCommon_t *scc);
void prepare_scd(NR_ServingCellConfig_t *scd);
ngap_gNB_config_t ngap_config;
uint32_t ngap_generate_gNB_id(void) {return 0;}
void configure_nfapi_pnf(char *vnf_ip_addr, int vnf_p5_port, char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port) { return;}
void configure_nfapi_vnf(char *vnf_addr, int vnf_p5_port) { return;}
void configure_nr_nfapi_pnf(char *vnf_ip_addr, int vnf_p5_port, char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port) { return;}
void configure_nr_nfapi_vnf(char *vnf_addr, int vnf_p5_port) { return;}

#endif
