/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file gNB_scheduler_phytest.c
 * \brief gNB scheduling procedures in phy_test mode
 * \author  Guy De Souza
 * \date 07/2018
 * \email: desouza@eurecom.fr
 * \version 1.0
 * @ingroup _mac
 */

#include "nr_mac_gNB.h"
#include "SCHED_NR/sched_nr.h"
#include "mac_proto.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"
#include "executables/nr-softmodem.h"
#include "common/utils/nr/nr_common.h"

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

extern RAN_CONTEXT_t RC;
//#define ENABLE_MAC_PAYLOAD_DEBUG 1

/*Scheduling of DLSCH with associated DCI in common search space on initialBWP
 * current version has only a DCI for type 1 PDCCH for C_RNTI*/
void nr_schedule_css_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP) {
  uint8_t  CC_id;
  gNB_MAC_INST                      *nr_mac      = RC.nrmac[module_idP];
  NR_COMMON_channels_t              *cc = &nr_mac->common_channels[0];
  nfapi_nr_dl_config_request_body_t *dl_req;
  nfapi_nr_dl_config_request_pdu_t  *dl_config_pdcch_pdu;
  nfapi_nr_dl_config_request_pdu_t  *dl_config_pdsch_pdu;
  nfapi_tx_request_pdu_t            *TX_req;

  uint16_t rnti = 0x1234;
  

  NR_ServingCellConfigCommon_t *scc=cc->ServingCellConfigCommon;

  uint16_t sfn_sf = frameP << 7 | slotP;
  int dlBWP_carrier_bandwidth = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275);

  // everything here is hard-coded to 30 kHz
  int scs               = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
  int slots_per_frame   = 10*(1<<scs);
  int FR                = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0] >= 257 ? nr_FR2 : nr_FR1;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "Scheduling common search space DCI type 1 dlBWP BW.firstRB %d.%d\n",
	  dlBWP_carrier_bandwidth,
	  NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275));
    
    
    dl_req = &nr_mac->DL_req[CC_id].dl_config_request_body;
    dl_config_pdcch_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
    memset((void*)dl_config_pdcch_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
    dl_config_pdcch_pdu->pdu_type = NFAPI_NR_DL_CONFIG_PDCCH_PDU_TYPE;
    dl_config_pdcch_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_pdcch_pdu));
    
    dl_config_pdsch_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu+1];
    memset((void *)dl_config_pdsch_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
    dl_config_pdsch_pdu->pdu_type = NFAPI_NR_DL_CONFIG_PDSCH_PDU_TYPE;
    dl_config_pdsch_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_pdsch_pdu));

    
    nfapi_nr_dl_config_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_config_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
    nfapi_nr_dl_config_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_config_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;
    
    pdsch_pdu_rel15->pduBitmap = 0;
    pdsch_pdu_rel15->rnti = rnti;
    pdsch_pdu_rel15->pduIndex = 0;

    // BWP
    pdsch_pdu_rel15->BWPSize  = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275);
    pdsch_pdu_rel15->BWPStart = NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275);
    pdsch_pdu_rel15->SubcarrierSpacing = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
    pdsch_pdu_rel15->CyclicPrefix = 0;
    pdsch_pdu_rel15->NrOfCodewords = 1;
    int mcsIndex = 9;
    pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(mcsIndex,0);
    pdsch_pdu_rel15->qamModOrder[0] = 2;
    pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
    pdsch_pdu_rel15->mcsTable[0] = 0;
    pdsch_pdu_rel15->rvIndex[0] = 0;
    pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->nrOfLayers = 1;    
    pdsch_pdu_rel15->transmissionScheme = 0;
    pdsch_pdu_rel15->refPoint = 0; // Point A
    
    pdsch_pdu_rel15->dmrsConfigType = 1; // 1 by default for InitialBWP
    pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->SCID = 0;
    pdsch_pdu_rel15->numDmrsCdmGrpsNoData = 1;
    pdsch_pdu_rel15->dmrsPorts = 1;
    pdsch_pdu_rel15->resourceAlloc = 1;
    pdsch_pdu_rel15->rbStart = 0;
    pdsch_pdu_rel15->rbSize = 6;
    pdsch_pdu_rel15->VRBtoPRBMapping = 1; // non-interleaved, check if this is ok for initialBWP
    // choose shortest PDSCH
    int startSymbolAndLength=0;
    int StartSymbolIndex,NrOfSymbols=14,k0=0;
    int StartSymbolIndex_tmp,NrOfSymbols_tmp;
    int time_domain_assignment=0;

    for (int i=0;
	 i<scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;
	 i++) {
      startSymbolAndLength = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
      SLIV2SL(startSymbolAndLength,&StartSymbolIndex_tmp,&NrOfSymbols_tmp);
      if (NrOfSymbols_tmp < NrOfSymbols) {
	NrOfSymbols = NrOfSymbols_tmp;
        StartSymbolIndex = StartSymbolIndex_tmp;
	k0 = *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0;
	time_domain_assignment = i;
      }
    }
    pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
    pdsch_pdu_rel15->NrOfSymbols      = NrOfSymbols;
    pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(NULL,
						    scc->dmrs_TypeA_Position,
						    NrOfSymbols);

    /*
    AssertFatal(k0==0,"k0 is not zero for Initial DL BWP TimeDomain Alloc\n");
    nr_configure_css_dci_initial(pdcch_pdu_rel15,
				 scs, 
				 scs, 
				 FR, 
				 0, 
				 0, 
				 0,
				 sfn_sf, slotP,
				 slots_per_frame,
				 dlBWP_carrier_bandwidth);
    
    
    pdu_rel15->frequency_domain_assignment = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbStart, 
									       pdsch_pdu_rel15->rbSize, 
									       dlBWP_carrier_bandwidth);
    pdu_rel15->time_domain_assignment = time_domain_assignment;
    
    pdu_rel15->vrb_to_prb_mapping = 1;
    pdu_rel15->mcs = 9;
    pdu_rel15->tb_scaling = 1;
    
    pdu_rel15->ra_preamble_index = 25;
    pdu_rel15->format_indicator = 1;
    pdu_rel15->ndi = 1;
    pdu_rel15->rv = 0;
    pdu_rel15->harq_pid = 0;
    pdu_rel15->dai = 2;
    pdu_rel15->tpc = 2;
    pdu_rel15->pucch_resource_indicator = 7;
    pdu_rel15->pdsch_to_harq_feedback_timing_indicator = 7;
    
    LOG_D(MAC, "[gNB scheduler phytest] DCI type 1 payload: freq_alloc %d, time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d ndi %d rv %d\n",
	  pdu_rel15->frequency_domain_assignment,
	  pdu_rel15->time_domain_assignment,
	  pdu_rel15->vrb_to_prb_mapping,
	  pdu_rel15->mcs,
	  pdu_rel15->tb_scaling,
	  pdu_rel15->ndi,
	  pdu_rel15->rv);
    
    params_rel15->rnti = rnti;
    params_rel15->rnti_type = NFAPI_NR_RNTI_C;
    params_rel15->dci_format = NFAPI_NR_DL_DCI_FORMAT_1_0;
    //params_rel15->aggregation_level = 1;
    LOG_D(MAC, "DCI type 1 params: rnti %x, rnti_type %d, dci_format %d\n \
                coreset params: mux_pattern %d, n_rb %d, n_symb %d, rb_offset %d  \n \
                ss params : nb_ss_sets_per_slot %d, first symb %d, nb_slots %d, sfn_mod2 %d, first slot %d\n",
	  params_rel15->rnti,
	  params_rel15->rnti_type,
	  params_rel15->dci_format,
	  params_rel15->mux_pattern,
	  params_rel15->n_rb,
	  params_rel15->n_symb,
	  params_rel15->rb_offset,
	  params_rel15->nb_ss_sets_per_slot,
	  params_rel15->first_symbol,
	  params_rel15->nb_slots,
	  params_rel15->sfn_mod2,
	  params_rel15->first_slot);
    nr_get_tbs_dl(&dl_config_pdsch_pdu->pdsch_pdu, dl_config_dci_pdu->dci_dl_pdu,0);
    LOG_D(MAC, "DLSCH PDU: start PRB %d n_PRB %d start symbol %d nb_symbols %d nb_layers %d nb_codewords %d mcs %d\n",
	  pdsch_pdu_rel15->rbStart,
	  pdsch_pdu_rel15->rbSize,
	  pdsch_pdu_rel15->StartSymbolIndex,
	  pdsch_pdu_rel15->NrOfSymbols,
	  pdsch_pdu_rel15->nrOfLayers,
	  pdsch_pdu_rel15->NrOfCodewords,
	  pdsch_pdu_rel15->mcsIndex[0]);
    */
    
    dl_req->number_dci++;
    dl_req->number_pdsch_rnti++;
    dl_req->number_pdu+=2;
    
    TX_req = &nr_mac->TX_req[CC_id].tx_request_body.tx_pdu_list[nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus];
    TX_req->pdu_length = 6;
    TX_req->pdu_index = nr_mac->pdu_index[CC_id]++;
    TX_req->num_segments = 1;
    TX_req->segments[0].segment_length = 8;
    TX_req->segments[0].segment_data   = &cc[CC_id].RAR_pdu.payload[0];
    nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
    nr_mac->TX_req[CC_id].sfn_sf = sfn_sf;
    nr_mac->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
    nr_mac->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;
    
  }
}

int configure_fapi_dl_Tx(int Mod_idP,
			 nfapi_nr_dl_config_request_body_t *dl_req,
			 nfapi_tx_request_pdu_t *TX_req,
			 int16_t pdu_index) {

  gNB_MAC_INST                        *nr_mac  = RC.nrmac[Mod_idP];
  NR_COMMON_channels_t                *cc      = nr_mac->common_channels;
  NR_ServingCellConfigCommon_t        *scc     = cc->ServingCellConfigCommon;
  
  nfapi_nr_dl_config_request_pdu_t  *dl_config_pdcch_pdu;
  nfapi_nr_dl_config_request_pdu_t  *dl_config_pdsch_pdu;
  int TBS;

  
  int bwp_id=1;

  
  int UE_id = 0;
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;

  AssertFatal(UE_list->active[UE_id] >=0,"Cannot find UE_id %d is not active\n",UE_id);




  LOG_I(PHY,"UE_id %d\n",UE_id);

  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  AssertFatal(secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
	      "downlinkBWP_ToAddModList has %d BWP!\n",
	      secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
  NR_BWP_Downlink_t *bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];


  dl_config_pdcch_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
  memset((void*)dl_config_pdcch_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
  dl_config_pdcch_pdu->pdu_type = NFAPI_NR_DL_CONFIG_PDCCH_PDU_TYPE;
  dl_config_pdcch_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_pdcch_pdu));
  
  dl_config_pdsch_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu+1];
  memset((void*)dl_config_pdsch_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
  dl_config_pdsch_pdu->pdu_type = NFAPI_NR_DL_CONFIG_PDSCH_PDU_TYPE;
  dl_config_pdsch_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_pdsch_pdu));

  nfapi_nr_dl_config_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_config_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
  nfapi_nr_dl_config_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_config_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;


  pdsch_pdu_rel15->pduBitmap = 0;
  pdsch_pdu_rel15->rnti = UE_list->rnti[UE_id];
  pdsch_pdu_rel15->pduIndex = 0;

  // BWP
  pdsch_pdu_rel15->BWPSize  = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pdsch_pdu_rel15->BWPStart = NRRIV2PRBOFFSET(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pdsch_pdu_rel15->SubcarrierSpacing = bwp->bwp_Common->genericParameters.subcarrierSpacing;
  if (bwp->bwp_Common->genericParameters.cyclicPrefix) pdsch_pdu_rel15->CyclicPrefix = *bwp->bwp_Common->genericParameters.cyclicPrefix;
  else pdsch_pdu_rel15->CyclicPrefix=0;

  pdsch_pdu_rel15->NrOfCodewords = 1;
  int mcsIndex = 9;
  pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(mcsIndex,0);
  pdsch_pdu_rel15->qamModOrder[0] = 2;
  pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
  pdsch_pdu_rel15->mcsTable[0] = 0;
  pdsch_pdu_rel15->rvIndex[0] = 0;
  pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->nrOfLayers = 1;    
  pdsch_pdu_rel15->transmissionScheme = 0;
  pdsch_pdu_rel15->refPoint = 0; // Point A
    
  pdsch_pdu_rel15->dmrsConfigType = 1; // 1 by default for InitialBWP
  pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->SCID = 0;
  pdsch_pdu_rel15->numDmrsCdmGrpsNoData = 1;
  pdsch_pdu_rel15->dmrsPorts = 1;
  pdsch_pdu_rel15->resourceAlloc = 1;
  pdsch_pdu_rel15->rbStart = 0;
  pdsch_pdu_rel15->rbSize = 50;
  pdsch_pdu_rel15->VRBtoPRBMapping = 1; // non-interleaved, check if this is ok for initialBWP
    // choose shortest PDSCH
  int startSymbolAndLength=0;
  int time_domain_assignment=2;
  int StartSymbolIndex,NrOfSymbols;

  AssertFatal(time_domain_assignment<bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count,"time_domain_assignment %d>=%d\n",time_domain_assignment,bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count);
  startSymbolAndLength = bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[time_domain_assignment]->startSymbolAndLength;
  SLIV2SL(startSymbolAndLength,&StartSymbolIndex,&NrOfSymbols);
  pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
  pdsch_pdu_rel15->NrOfSymbols      = NrOfSymbols;
 
  //  k0 = *bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0;
  pdsch_pdu_rel15->dlDmrsSymbPos    = fill_dmrs_mask(NULL,
						     scc->dmrs_TypeA_Position,
						     pdsch_pdu_rel15->NrOfSymbols);
    dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
    
    dci_pdu_rel15[0].frequency_domain_assignment = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbStart, 
										  pdsch_pdu_rel15->rbSize, 
										  NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275));
    dci_pdu_rel15[0].time_domain_assignment = time_domain_assignment; // row index used here instead of SLIV;
    dci_pdu_rel15[0].vrb_to_prb_mapping = 1;
    dci_pdu_rel15[0].mcs = pdsch_pdu_rel15->mcsIndex[0];
    dci_pdu_rel15[0].tb_scaling = 1;
    
    dci_pdu_rel15[0].ra_preamble_index = 25;
    dci_pdu_rel15[0].format_indicator = 1;
    dci_pdu_rel15[0].ndi = 1;
    dci_pdu_rel15[0].rv = 0;
    dci_pdu_rel15[0].harq_pid = 0;
    dci_pdu_rel15[0].dai = 2;
    dci_pdu_rel15[0].tpc = 2;
    dci_pdu_rel15[0].pucch_resource_indicator = 7;
    dci_pdu_rel15[0].pdsch_to_harq_feedback_timing_indicator = 7;
    
    LOG_I(MAC, "[gNB scheduler phytest] DCI type 1 payload: freq_alloc %d, time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d ndi %d rv %d\n",
	  dci_pdu_rel15[0].frequency_domain_assignment,
	  dci_pdu_rel15[0].time_domain_assignment,
	  dci_pdu_rel15[0].vrb_to_prb_mapping,
	  dci_pdu_rel15[0].mcs,
	  dci_pdu_rel15[0].tb_scaling,
	  dci_pdu_rel15[0].ndi, 
	  dci_pdu_rel15[0].rv);
    
    nr_configure_pdcch(pdcch_pdu_rel15,
		       1, // ue-specific
		       scc,
		       bwp);
    
    pdcch_pdu_rel15->numDlDci = 1;
    pdcch_pdu_rel15->AggregationLevel[0] = 4;  
    pdcch_pdu_rel15->RNTI[0]=UE_list->rnti[0];
    pdcch_pdu_rel15->CceIndex[0] = 0;
    pdcch_pdu_rel15->beta_PDCCH_1_0[0]=0;
    pdcch_pdu_rel15->powerControlOffsetSS[0]=1;

    int dci_formats[pdcch_pdu_rel15->numDlDci];
    int rnti_types[pdcch_pdu_rel15->numDlDci];

    dci_formats[0]  = NR_DL_DCI_FORMAT_1_0;
    rnti_types[0]   = NR_RNTI_C;
    config_uldci(pdcch_pdu_rel15, &dci_pdu_rel15[pdcch_pdu_rel15->numDlDci], dci_formats, rnti_types);
    
    for (int i=0;i<pdcch_pdu_rel15->numDlDci;i++) 
      pdcch_pdu_rel15->PayloadSizeBits[i]=nr_dci_size(dci_formats[i],rnti_types[i],pdsch_pdu_rel15->BWPSize);

    fill_dci_pdu_rel15(pdcch_pdu_rel15,&dci_pdu_rel15[i],dci_formats,rnti_types);
    
    LOG_I(MAC, "DCI params: rnti %d, rnti_type %d, dci_format %d\n \
	                      coreset params: FreqDomainResource %llx, start_symbol %d  n_symb %d\n",
	pdcch_pdu_rel15->RNTI[0],
	rnti_types[0],
	dci_formats[0],
	(unsigned long long)pdcch_pdu_rel15->FreqDomainResource,
	pdcch_pdu_rel15->StartSymbolIndex,
	pdcch_pdu_rel15->DurationSymbols);

  int x_Overhead = 0; // should be 0 for initialBWP
  nr_get_tbs_dl(&dl_config_pdsch_pdu->pdsch_pdu, 
		x_Overhead);
  // Hardcode it for now
  TBS = dl_config_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15.TBSize[0];
  LOG_I(MAC, "DLSCH PDU: start PRB %d n_PRB %d startSymbolAndLength %d start symbol %d nb_symbols %d nb_layers %d nb_codewords %d mcs %d TBS: %d\n",
	pdsch_pdu_rel15->rbStart,
	pdsch_pdu_rel15->rbSize,
	startSymbolAndLength,
	pdsch_pdu_rel15->StartSymbolIndex,
	pdsch_pdu_rel15->NrOfSymbols,
	pdsch_pdu_rel15->nrOfLayers,
	pdsch_pdu_rel15->NrOfCodewords,
	pdsch_pdu_rel15->mcsIndex[0],
	TBS);
  
  dl_req->number_dci++;
  dl_req->number_pdsch_rnti++;
  dl_req->number_pdu+=2;
  
  TX_req->pdu_length = pdsch_pdu_rel15->TBSize[0];
  TX_req->pdu_index = pdu_index++;
  TX_req->num_segments = 1;
  
  return TBS/8; //Return TBS in bytes
}

void config_uldci(nfapi_nr_dl_config_pdcch_pdu_rel15_t *pdcch_pdu_rel15, dci_pdu_rel15_t *dci_pdu_rel15, int *dci_formats, int *rnti_types) {

  dci_pdu_rel15.frequency_domain_assignment = PRBalloc_to_locationandbandwidth0(0, 
                    50, 
                    NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275)); // to be changed with UL bwp
  dci_pdu_rel15.time_domain_assignment = 2; // row index used here instead of SLIV;
  dci_pdu_rel15.frequency_hopping_flag = 1;
  dci_pdu_rel15.mcs = 9;
  
  dci_pdu_rel15.format_indicator = 0;
  dci_pdu_rel15.ndi = 1;
  dci_pdu_rel15.rv = 0;
  dci_pdu_rel15.harq_pid = 0;
  dci_pdu_rel15.tpc = 2;
  
  LOG_I(MAC, "[gNB scheduler phytest] DCI type 0 payload: freq_alloc %d, time_alloc %d, freq_hop_flag %d, mcs %d tpc %d ndi %d rv %d\n",
  dci_pdu_rel15.frequency_domain_assignment,
  dci_pdu_rel15.time_domain_assignment,
  dci_pdu_rel15.frequency_hopping_flag,
  dci_pdu_rel15.mcs,
  dci_pdu_rel15.tpc,
  dci_pdu_rel15.ndi, 
  dci_pdu_rel15.rv);
  
  dci_formats[pdcch_pdu_rel15->numDlDci] = NR_UL_DCI_FORMAT_0_0;
  rnti_types[pdcch_pdu_rel15->numDlDci]  = NR_RNTI_C;
  pdcch_pdu_rel15->numDlDci++;

}
    


void nr_schedule_uss_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP,
                                   nfapi_nr_dl_config_pdsch_pdu_rel15_t *dlsch_config)
{
  LOG_D(MAC, "In nr_schedule_uss_dlsch_phytest \n");
  
  gNB_MAC_INST                        *nr_mac      = RC.nrmac[module_idP];
  //NR_COMMON_channels_t                *cc           = nr_mac->common_channels;
  //NR_ServingCellConfigCommon_t *scc=cc->ServingCellConfigCommon;
  nfapi_nr_dl_config_request_body_t   *dl_req;
  nfapi_tx_request_pdu_t            *TX_req;
  uint16_t rnti = 0x1234;
  uint16_t sfn_sf = frameP << 7 | slotP;

  int TBS;
  int TBS_bytes;
  int lcid;
  int ta_len = 0;
  int header_length_total=0;
  int header_length_last;
  int sdu_length_total = 0;
  mac_rlc_status_resp_t rlc_status;
  uint16_t sdu_lengths[NB_RB_MAX];
  int num_sdus = 0;
  unsigned char dlsch_buffer[MAX_DLSCH_PAYLOAD_BYTES];
  int offset;
  int UE_id = 0;
  unsigned char sdu_lcids[NB_RB_MAX];
  int padding = 0, post_padding = 0;
  UE_list_t *UE_list = &nr_mac->UE_list;
  
  DLSCH_PDU dlsch_pdu;
  //PDSCH_PDU *pdsch_pdu = (PDSCH_PDU*) malloc(sizeof(PDSCH_PDU));
  

  
  LOG_D(MAC, "Scheduling UE specific search space DCI type 1\n");
  int CC_id=0;
  dl_req = &nr_mac->DL_req[CC_id].dl_config_request_body;
  TX_req = &nr_mac->TX_req[CC_id].tx_request_body.tx_pdu_list[nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus];
  
  //The --NOS1 use case currently schedules DLSCH transmissions only when there is IP traffic arriving
  //through the LTE stack
  if (IS_SOFTMODEM_NOS1){
    
    memset(&dlsch_pdu, 0, sizeof(DLSCH_PDU));
    int ta_update = 31;
    ta_len = 0;
    
    // Hardcode it for now
    TBS = 6784/8; //TBS in bytes
    //TBS = dl_config_pdsch_pdu->pdsch_pdu.dlsch_pdu_rel15.transport_block_size;
    
    for (lcid = NB_RB_MAX - 1; lcid >= DTCH; lcid--) {
      // TODO: check if the lcid is active
      
      LOG_D(MAC, "[eNB %d], Frame %d, DTCH%d->DLSCH, Checking RLC status (tbs %d, len %d)\n",
	    module_idP, frameP, lcid, TBS,
	    TBS - ta_len - header_length_total - sdu_length_total - 3);
      
      if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
	rlc_status = mac_rlc_status_ind(module_idP,
					rnti,
					module_idP,
					frameP,
					slotP,
					ENB_FLAG_YES,
					MBMS_FLAG_NO,
					lcid,
					TBS - ta_len - header_length_total - sdu_length_total - 3
					//#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
					,0, 0
					//#endif
					);
	
	if (rlc_status.bytes_in_buffer > 0) {
	  LOG_D(MAC,
		"[eNB %d][USER-PLANE DEFAULT DRB] Frame %d : DTCH->DLSCH, Requesting %d bytes from RLC (lcid %d total hdr len %d), TBS: %d \n \n",
		module_idP, frameP,
		TBS - ta_len - header_length_total - sdu_length_total - 3,
		lcid,
		header_length_total,
		TBS);
	  
	  sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP, rnti, module_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, lcid,
						   TBS,
						   (char *)&dlsch_buffer[sdu_length_total]
						   //#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
						   ,0, 0
						   //#endif
						   );
	  
	  
	  
	  LOG_D(MAC,
		"[eNB %d][USER-PLANE DEFAULT DRB] Got %d bytes for DTCH %d \n",
		module_idP, sdu_lengths[num_sdus], lcid);
	  
	  sdu_lcids[num_sdus] = lcid;
	  sdu_length_total += sdu_lengths[num_sdus];
	  UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[lcid]++;
	  UE_list->eNB_UE_stats[CC_id][UE_id].lcid_sdu[num_sdus] = lcid;
	  UE_list->eNB_UE_stats[CC_id][UE_id].sdu_length_tx[lcid] = sdu_lengths[num_sdus];
	  UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[lcid] += sdu_lengths[num_sdus];
	  
	  header_length_last = 1 + 1 + (sdu_lengths[num_sdus] >= 128);
	  header_length_total += header_length_last;
	  
	  num_sdus++;
	  
	  UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer = 0;
	}
      } else {
	// no TBS left
	break;
      }
    }
    
    // last header does not have length field
    if (header_length_total) {
      header_length_total -= header_length_last;
      header_length_total++;
    }
    
    
    if (ta_len + sdu_length_total + header_length_total > 0) {
      
      
      if (TBS - header_length_total - sdu_length_total - ta_len <= 2) {
	padding = TBS - header_length_total - sdu_length_total - ta_len;
	post_padding = 0;
      } else {
	padding = 0;
	post_padding = 1;
      }
      
      offset = generate_dlsch_header((unsigned char *)nr_mac->UE_list.DLSCH_pdu[0][0].payload[0], //DLSCH_pdu.payload[0],
				     num_sdus,    //num_sdus
				     sdu_lengths,    //
				     sdu_lcids, 255,    // no drx
				     ta_update,    // timing advance
				     NULL,    // contention res id
				     padding, post_padding);

      LOG_D(MAC, "Offset bits: %d \n", offset);
      
      // Probably there should be other actions done before that
      // cycle through SDUs and place in dlsch_buffer
      
      //memcpy(&UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset], dlsch_buffer, sdu_length_total);
      memcpy(&nr_mac->UE_list.DLSCH_pdu[0][UE_id].payload[0][offset], dlsch_buffer, sdu_length_total);
      
      
      // fill remainder of DLSCH with 0
      for (int j = 0; j < (TBS - sdu_length_total - offset); j++) {
	//UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset + sdu_length_total + j] = 0;
	nr_mac->UE_list.DLSCH_pdu[0][0].payload[0][offset + sdu_length_total + j] = 0;
      }
      
      TBS_bytes = configure_fapi_dl_Tx(module_idP,dl_req, TX_req, 
				       nr_mac->pdu_index[CC_id]);        

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
      LOG_I(MAC, "Printing first 10 payload bytes at the gNB side, Frame: %d, slot: %d, , TBS size: %d \n \n", frameP, slotP, TBS_bytes);
      for(int i = 0; i < 10; i++) { // TBS_bytes dlsch_pdu_rel15->transport_block_size/8 6784/8
      LOG_I(MAC, "%x. ", ((uint8_t *)nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0])[i]);
    }
#endif
      
      //TX_req->segments[0].segment_length = 8;
      TX_req->segments[0].segment_length = TBS_bytes +2;
      TX_req->segments[0].segment_data = nr_mac->UE_list.DLSCH_pdu[0][0].payload[0];
      
      
      nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
      nr_mac->TX_req[CC_id].sfn_sf = sfn_sf;
      nr_mac->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
      nr_mac->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;
    } //if (ta_len + sdu_length_total + header_length_total > 0)
  } //if (IS_SOFTMODEM_NOS1)
  
    //When the --NOS1 option is not enabled, DLSCH transmissions with random data
    //occur every time that the current function is called (dlsch phytest mode)
  else{
    TBS_bytes = configure_fapi_dl_Tx(module_idP,dl_req, TX_req, 
				     nr_mac->pdu_index[CC_id]);
    // HOT FIX for all zero pdu problem
    // ------------------------------------------------------------------------------------------------
    
    for(int i = 0; i < TBS_bytes; i++) { //
      ((uint8_t *)nr_mac->UE_list.DLSCH_pdu[0][0].payload[0])[i] = (unsigned char) rand();
      //LOG_I(MAC, "%x. ", ((uint8_t *)nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0])[i]);
    }
#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
    if (frameP%100 == 0){
    LOG_I(MAC, "Printing first 10 payload bytes at the gNB side, Frame: %d, slot: %d, TBS size: %d \n", frameP, slotP, TBS_bytes);
    for(int i = 0; i < 10; i++) {
    LOG_I(MAC, "%x. ", ((uint8_t *)nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0])[i]);
  }
    }
#endif
    
    //TX_req->segments[0].segment_length = 8;
    TX_req->segments[0].segment_length = TBS_bytes +2;
    TX_req->segments[0].segment_data = nr_mac->UE_list.DLSCH_pdu[0][0].payload[0];
    
    nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
    nr_mac->TX_req[CC_id].sfn_sf = sfn_sf;
    nr_mac->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
    nr_mac->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;
    // ------------------------------------------------------------------------------------------------
  }
}


void nr_schedule_uss_ulsch_phytest(nfapi_nr_ul_tti_request_t *UL_tti_req,
                                   frame_t       frameP,
                                   sub_frame_t   slotP) {
  //gNB_MAC_INST                      *nr_mac      = RC.nrmac[module_idP];
  //nfapi_nr_ul_tti_request_t         *UL_tti_req;
  uint16_t rnti = 0x1234;

  for (uint8_t CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "Scheduling UE specific PUSCH for CC_id %d\n",CC_id);
    //UL_tti_req = &nr_mac->UL_tti_req[CC_id];
    UL_tti_req->sfn = frameP;
    UL_tti_req->slot = slotP;
    UL_tti_req->n_pdus = 1;
    UL_tti_req->pdus_list[0].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
    UL_tti_req->pdus_list[0].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
    nfapi_nr_pusch_pdu_t  *pusch_pdu = &UL_tti_req->pdus_list[0].pusch_pdu;
    memset(pusch_pdu,0,sizeof(nfapi_nr_pusch_pdu_t));
    /*
    // original configuration
        rel15_ul->rnti                           = 0x1234;
        rel15_ul->ulsch_pdu_rel15.start_rb       = 30;
        rel15_ul->ulsch_pdu_rel15.number_rbs     = 50;
        rel15_ul->ulsch_pdu_rel15.start_symbol   = 2;
        rel15_ul->ulsch_pdu_rel15.number_symbols = 12;
        rel15_ul->ulsch_pdu_rel15.nb_re_dmrs     = 6;
        rel15_ul->ulsch_pdu_rel15.length_dmrs    = 1;
        rel15_ul->ulsch_pdu_rel15.Qm             = 2;
        rel15_ul->ulsch_pdu_rel15.mcs            = 9;
        rel15_ul->ulsch_pdu_rel15.rv             = 0;
        rel15_ul->ulsch_pdu_rel15.n_layers       = 1;
    */
    pusch_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
    pusch_pdu->rnti = rnti;
    pusch_pdu->handle = 0; //not yet used
    //BWP related paramters - we don't yet use them as the PHY only uses one default BWP
    //pusch_pdu->bwp_size;
    //pusch_pdu->bwp_start;
    //pusch_pdu->subcarrier_spacing;
    //pusch_pdu->cyclic_prefix;
    //pusch information always include
    //this informantion seems to be redundant. with hthe mcs_index and the modulation table, the mod_order and target_code_rate can be determined.
    pusch_pdu->mcs_index = 9;
    pusch_pdu->mcs_table = 0; //0: notqam256 [TS38.214, table 5.1.3.1-1] - corresponds to nr_target_code_rate_table1 in PHY
    pusch_pdu->target_code_rate = nr_get_code_rate_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table) ;
    pusch_pdu->qam_mod_order = nr_get_Qm_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table) ;
    pusch_pdu->transform_precoding = 0;
    pusch_pdu->data_scrambling_id = 0; //It equals the higher-layer parameter Data-scrambling-Identity if configured and the RNTI equals the C-RNTI, otherwise L2 needs to set it to physical cell id.;
    pusch_pdu->nrOfLayers = 1;
    //DMRS
    pusch_pdu->ul_dmrs_symb_pos = 1;
    pusch_pdu->dmrs_config_type = 0;  //dmrs-type 1 (the one with a single DMRS symbol in the beginning)
    pusch_pdu->ul_dmrs_scrambling_id =  0; //If provided and the PUSCH is not a msg3 PUSCH, otherwise, L2 should set this to physical cell id.
    pusch_pdu->scid = 0; //DMRS sequence initialization [TS38.211, sec 6.4.1.1.1]. Should match what is sent in DCI 0_1, otherwise set to 0.
    //pusch_pdu->num_dmrs_cdm_grps_no_data;
    //pusch_pdu->dmrs_ports; //DMRS ports. [TS38.212 7.3.1.1.2] provides description between DCI 0-1 content and DMRS ports. Bitmap occupying the 11 LSBs with: bit 0: antenna port 1000 bit 11: antenna port 1011 and for each bit 0: DMRS port not used 1: DMRS port used
    //Pusch Allocation in frequency domain [TS38.214, sec 6.1.2.2]
    pusch_pdu->resource_alloc = 1; //type 1
    //pusch_pdu->rb_bitmap;// for ressource alloc type 0
    pusch_pdu->rb_start = 0;
    pusch_pdu->rb_size = 50;
    pusch_pdu->vrb_to_prb_mapping = 0;
    pusch_pdu->frequency_hopping = 0;
    //pusch_pdu->tx_direct_current_location;//The uplink Tx Direct Current location for the carrier. Only values in the value range of this field between 0 and 3299, which indicate the subcarrier index within the carrier corresponding 1o the numerology of the corresponding uplink BWP and value 3300, which indicates "Outside the carrier" and value 3301, which indicates "Undetermined position within the carrier" are used. [TS38.331, UplinkTxDirectCurrentBWP IE]
    pusch_pdu->uplink_frequency_shift_7p5khz = 0;
    //Resource Allocation in time domain
    pusch_pdu->start_symbol_index = 2;
    pusch_pdu->nr_of_symbols = 12;
    //Optional Data only included if indicated in pduBitmap
    pusch_pdu->pusch_data.rv_index = 0;
    pusch_pdu->pusch_data.harq_process_id = 0;
    pusch_pdu->pusch_data.new_data_indicator = 0;
    pusch_pdu->pusch_data.tb_size = nr_compute_tbs(pusch_pdu->mcs_index,
						   pusch_pdu->target_code_rate,
						   pusch_pdu->rb_size,
						   pusch_pdu->nr_of_symbols,
						   6, //nb_re_dmrs - not sure where this is coming from - its not in the FAPI
						   0, //nb_rb_oh
						   pusch_pdu->nrOfLayers = 1);
    pusch_pdu->pusch_data.num_cb = 0; //CBG not supported
    //pusch_pdu->pusch_data.cb_present_and_position;
    //pusch_pdu->pusch_uci;
    //pusch_pdu->pusch_ptrs;
    //pusch_pdu->dfts_ofdm;
    //beamforming
    //pusch_pdu->beamforming; //not used for now
  }
}
