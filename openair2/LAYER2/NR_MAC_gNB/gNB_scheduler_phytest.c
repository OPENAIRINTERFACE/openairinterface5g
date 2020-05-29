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
 * \author  Guy De Souza, G. Casati
 * \date 07/2018
 * \email: desouza@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \version 1.0
 * @ingroup _mac
 */

#include "nr_mac_gNB.h"
#include "SCHED_NR/sched_nr.h"
#include "mac_proto.h"
#include "nr_mac_common.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"
#include "executables/nr-softmodem.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "executables/softmodem-common.h"
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

//uint8_t mac_pdu[MAX_NR_DLSCH_PAYLOAD_BYTES];

/*Scheduling of DLSCH with associated DCI in common search space
 * current version has only a DCI for type 1 PDCCH for C_RNTI*/
void nr_schedule_css_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP) {
  uint8_t  CC_id;
  gNB_MAC_INST                      *nr_mac      = RC.nrmac[module_idP];
  NR_COMMON_channels_t              *cc = &nr_mac->common_channels[0];
  nfapi_nr_dl_tti_request_body_t    *dl_req;
  nfapi_nr_dl_tti_request_pdu_t     *dl_tti_pdcch_pdu;
  nfapi_nr_dl_tti_request_pdu_t     *dl_tti_pdsch_pdu;
  nfapi_nr_pdu_t        *TX_req;

  uint16_t rnti = 0x1234;
  
  //  int time_domain_assignment,k0;

  NR_ServingCellConfigCommon_t *scc=cc->ServingCellConfigCommon;

  int dlBWP_carrier_bandwidth = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275);

  
  /*
  int scs               = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
  
  int slots_per_frame   = 10*(1<<scs);

  int FR                = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0] >= 257 ? nr_FR2 : nr_FR1;
  */

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "Scheduling common search space DCI type 1 dlBWP BW.firstRB %d.%d\n",
	  dlBWP_carrier_bandwidth,
	  NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth,275));
    
    
    dl_req = &nr_mac->DL_req[CC_id].dl_tti_request_body;
    dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
    dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
    
    dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs+1];
    memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
    dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

    
    //    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;
    
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
    
    pdsch_pdu_rel15->dmrsConfigType = 0; // Type 1 by default for InitialBWP
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
    int StartSymbolIndex=-1,NrOfSymbols=14;
    int StartSymbolIndex_tmp,NrOfSymbols_tmp;

    for (int i=0;
	 i<scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;
	 i++) {
      startSymbolAndLength = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
      SLIV2SL(startSymbolAndLength,&StartSymbolIndex_tmp,&NrOfSymbols_tmp);
      if (NrOfSymbols_tmp < NrOfSymbols) {
	NrOfSymbols = NrOfSymbols_tmp;
        StartSymbolIndex = StartSymbolIndex_tmp;
	//	k0 = *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0;
	//	time_domain_assignment = i;
      }
    }
    AssertFatal(StartSymbolIndex>=0,"StartSymbolIndex is negative\n");
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
    
    
    pdu_rel15->frequency_domain_assignment = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize, 
                                                                               pdsch_pdu_rel15->rbStart, 
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
    nr_get_tbs_dl(&dl_tti_pdsch_pdu->pdsch_pdu, dl_tti_dci_pdu->dci_dl_pdu,0);
    LOG_D(MAC, "DLSCH PDU: start PRB %d n_PRB %d start symbol %d nb_symbols %d nb_layers %d nb_codewords %d mcs %d\n",
	  pdsch_pdu_rel15->rbStart,
	  pdsch_pdu_rel15->rbSize,
	  pdsch_pdu_rel15->StartSymbolIndex,
	  pdsch_pdu_rel15->NrOfSymbols,
	  pdsch_pdu_rel15->nrOfLayers,
	  pdsch_pdu_rel15->NrOfCodewords,
	  pdsch_pdu_rel15->mcsIndex[0]);
    */
    
    dl_req->nPDUs+=2;
    
    TX_req = &nr_mac->TX_req[CC_id].pdu_list[nr_mac->TX_req[CC_id].Number_of_PDUs];
    TX_req->PDU_length = 6;
    TX_req->PDU_index = nr_mac->pdu_index[CC_id]++;
    TX_req->num_TLV = 1;
    TX_req->TLVs[0].length = 8;
    memcpy((void*)&TX_req->TLVs[0].value.direct[0],(void*)&cc[CC_id].RAR_pdu.payload[0],TX_req->TLVs[0].length);
    nr_mac->TX_req[CC_id].Number_of_PDUs++;
    nr_mac->TX_req[CC_id].SFN=frameP;
    nr_mac->TX_req[CC_id].Slot=slotP;
  }
}





int configure_fapi_dl_pdu(int Mod_idP,
                          int *CCEIndex,
                          nfapi_nr_dl_tti_request_body_t *dl_req,
			  NR_sched_pucch *pucch_sched,
                          uint8_t *mcsIndex,
                          uint16_t *rbSize,
                          uint16_t *rbStart) {


  gNB_MAC_INST                        *nr_mac  = RC.nrmac[Mod_idP];
  NR_COMMON_channels_t                *cc      = nr_mac->common_channels;
  NR_ServingCellConfigCommon_t        *scc     = cc->ServingCellConfigCommon;

  nfapi_nr_dl_tti_request_pdu_t  *dl_tti_pdcch_pdu;
  nfapi_nr_dl_tti_request_pdu_t  *dl_tti_pdsch_pdu;
  int TBS, bwp_id = 1, UE_id = 0;
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;

  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  AssertFatal(secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
	      "downlinkBWP_ToAddModList has %d BWP!\n",
	      secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
  NR_BWP_Downlink_t *bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];


  dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
  memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
  dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
  
  dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs+1];
  memset((void*)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
  dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;


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
  int mcs = (mcsIndex!=NULL) ? *mcsIndex : 9;
  pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(mcs,0);
  pdsch_pdu_rel15->qamModOrder[0] = 2;
  pdsch_pdu_rel15->mcsIndex[0] = mcs;
  pdsch_pdu_rel15->mcsTable[0] = 0;
  pdsch_pdu_rel15->rvIndex[0] = 0;
  pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->nrOfLayers = 1;    
  pdsch_pdu_rel15->transmissionScheme = 0;
  pdsch_pdu_rel15->refPoint = 0; // Point A
    
  pdsch_pdu_rel15->dmrsConfigType = bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? 0 : 1;  
  pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->SCID = 0;
  pdsch_pdu_rel15->numDmrsCdmGrpsNoData = 1;
  pdsch_pdu_rel15->dmrsPorts = 1;
  pdsch_pdu_rel15->resourceAlloc = 1;
  pdsch_pdu_rel15->rbStart = (rbStart!=NULL) ? *rbStart : 0;
  pdsch_pdu_rel15->rbSize = (rbSize!=NULL) ? *rbSize : pdsch_pdu_rel15->BWPSize;
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
  pdsch_pdu_rel15->dlDmrsSymbPos    = fill_dmrs_mask(bwp->bwp_Dedicated->pdsch_Config->choice.setup,
						     scc->dmrs_TypeA_Position,
						     pdsch_pdu_rel15->NrOfSymbols);

  dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
  
  dci_pdu_rel15[0].frequency_domain_assignment = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize, 
										   pdsch_pdu_rel15->rbStart, 
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
  dci_pdu_rel15[0].dai = (pucch_sched->dai_c-1)&3;
  dci_pdu_rel15[0].tpc = 2;
  dci_pdu_rel15[0].pucch_resource_indicator = pucch_sched->resource_indicator;
  dci_pdu_rel15[0].pdsch_to_harq_feedback_timing_indicator = pucch_sched->timing_indicator;
  
  LOG_D(MAC, "[gNB scheduler phytest] DCI type 1 payload: freq_alloc %d (%d,%d,%d), time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d ndi %d rv %d\n",
	dci_pdu_rel15[0].frequency_domain_assignment,
	pdsch_pdu_rel15->rbStart, 
	pdsch_pdu_rel15->rbSize,	
	NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275),
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
  pdcch_pdu_rel15->CceIndex[0] = CCEIndex[0];
  pdcch_pdu_rel15->beta_PDCCH_1_0[0]=0;
  pdcch_pdu_rel15->powerControlOffsetSS[0]=1;
  
  int dci_formats[2];
  int rnti_types[2];
  
  dci_formats[0]  = NR_DL_DCI_FORMAT_1_0;
  rnti_types[0]   = NR_RNTI_C;

  pdcch_pdu_rel15->PayloadSizeBits[0]=nr_dci_size(dci_formats[0],rnti_types[0],pdcch_pdu_rel15->BWPSize);
  fill_dci_pdu_rel15(pdcch_pdu_rel15,&dci_pdu_rel15[0],dci_formats,rnti_types);

  LOG_D(MAC, "DCI params: rnti %d, rnti_type %d, dci_format %d\n \
	                      coreset params: FreqDomainResource %llx, start_symbol %d  n_symb %d\n",
	pdcch_pdu_rel15->RNTI[0],
	rnti_types[0],
	dci_formats[0],
	(unsigned long long)pdcch_pdu_rel15->FreqDomainResource,
	pdcch_pdu_rel15->StartSymbolIndex,
	pdcch_pdu_rel15->DurationSymbols);

  int x_Overhead = 0; // should be 0 for initialBWP
  nr_get_tbs_dl(&dl_tti_pdsch_pdu->pdsch_pdu, x_Overhead);

  // Hardcode it for now
  TBS = dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15.TBSize[0];
  LOG_D(MAC, "DLSCH PDU: start PRB %d n_PRB %d startSymbolAndLength %d start symbol %d nb_symbols %d nb_layers %d nb_codewords %d mcs %d TBS: %d\n",
	pdsch_pdu_rel15->rbStart,
	pdsch_pdu_rel15->rbSize,
	startSymbolAndLength,
	pdsch_pdu_rel15->StartSymbolIndex,
	pdsch_pdu_rel15->NrOfSymbols,
	pdsch_pdu_rel15->nrOfLayers,
	pdsch_pdu_rel15->NrOfCodewords,
	pdsch_pdu_rel15->mcsIndex[0],
	TBS);
  return TBS; //Return TBS in bytes
}

void config_uldci(NR_BWP_Uplink_t *ubwp,nfapi_nr_pusch_pdu_t *pusch_pdu,nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15, dci_pdu_rel15_t *dci_pdu_rel15, int *dci_formats, int *rnti_types) {

  dci_pdu_rel15->frequency_domain_assignment = PRBalloc_to_locationandbandwidth0(pusch_pdu->rb_size, 
										 pusch_pdu->rb_start, 
										 NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275));

  dci_pdu_rel15->time_domain_assignment = 2; // row index used here instead of SLIV;
  dci_pdu_rel15->frequency_hopping_flag = 0;
  dci_pdu_rel15->mcs = 9;
  
  dci_pdu_rel15->format_indicator = 0;
  dci_pdu_rel15->ndi = 1;
  dci_pdu_rel15->rv = 0;
  dci_pdu_rel15->harq_pid = 0;
  dci_pdu_rel15->tpc = 2;
  
  LOG_D(MAC, "[gNB scheduler phytest] ULDCI type 0 payload: PDCCH CCEIndex %d, freq_alloc %d, time_alloc %d, freq_hop_flag %d, mcs %d tpc %d ndi %d rv %d\n",
	pdcch_pdu_rel15->CceIndex[pdcch_pdu_rel15->numDlDci],
	dci_pdu_rel15->frequency_domain_assignment,
	dci_pdu_rel15->time_domain_assignment,
	dci_pdu_rel15->frequency_hopping_flag,
	dci_pdu_rel15->mcs,
	dci_pdu_rel15->tpc,
	dci_pdu_rel15->ndi, 
	dci_pdu_rel15->rv);
  
  dci_formats[pdcch_pdu_rel15->numDlDci] = NR_UL_DCI_FORMAT_0_0;
  rnti_types[pdcch_pdu_rel15->numDlDci]  = NR_RNTI_C;
  pdcch_pdu_rel15->numDlDci++;

}
    

void configure_fapi_dl_Tx(module_id_t Mod_idP,
                          frame_t       frameP,
                          sub_frame_t   slotP,
                          nfapi_nr_dl_tti_request_body_t *dl_req,
                          nfapi_nr_pdu_t *tx_req,
                          int tbs_bytes,
                          int16_t pdu_index){

  int CC_id = 0;

  nfapi_nr_dl_tti_request_pdu_t  *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs+1];
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;
  gNB_MAC_INST *nr_mac  = RC.nrmac[Mod_idP];

  LOG_D(MAC, "DLSCH PDU: start PRB %d n_PRB %d start symbol %d nb_symbols %d nb_layers %d nb_codewords %d mcs %d TBS (bytes): %d\n",
        pdsch_pdu_rel15->rbStart,
        pdsch_pdu_rel15->rbSize,
        pdsch_pdu_rel15->StartSymbolIndex,
        pdsch_pdu_rel15->NrOfSymbols,
        pdsch_pdu_rel15->nrOfLayers,
        pdsch_pdu_rel15->NrOfCodewords,
        pdsch_pdu_rel15->mcsIndex[0],
        tbs_bytes);

  dl_req->nPDUs+=2;

  tx_req->PDU_length = pdsch_pdu_rel15->TBSize[0];
  tx_req->PDU_index  = nr_mac->pdu_index[0]++;
  tx_req->num_TLV = 1;
  tx_req->TLVs[0].length = tbs_bytes +2;

  memcpy((void*)&tx_req->TLVs[0].value.direct[0], (void*)&nr_mac->UE_list.DLSCH_pdu[0][0].payload[0], tbs_bytes);;

  nr_mac->TX_req[CC_id].Number_of_PDUs++;
  nr_mac->TX_req[CC_id].SFN = frameP;
  nr_mac->TX_req[CC_id].Slot = slotP;
}

void nr_schedule_uss_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP,
                                   NR_sched_pucch *pucch_sched,
                                   nfapi_nr_dl_tti_pdsch_pdu_rel15_t *dlsch_config){

  LOG_D(MAC, "In nr_schedule_uss_dlsch_phytest \n");

  int post_padding = 0, ta_len = 0, header_length_total = 0, sdu_length_total = 0, num_sdus = 0;
  int lcid, offset, i, header_length_last, TBS_bytes;
  int UE_id = 0, CCEIndex = -1, CC_id = 0;

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_idP];
  //NR_COMMON_channels_t                *cc           = nr_mac->common_channels;
  //NR_ServingCellConfigCommon_t *scc=cc->ServingCellConfigCommon;
  nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;
  nfapi_nr_pdu_t *tx_req = &gNB_mac->TX_req[CC_id].pdu_list[gNB_mac->TX_req[CC_id].Number_of_PDUs];

  mac_rlc_status_resp_t rlc_status;

  NR_UE_list_t *UE_list = &gNB_mac->UE_list;
 
  if (UE_list->num_UEs ==0) return;
 
  unsigned char sdu_lcids[NB_RB_MAX] = {0};
  uint16_t sdu_lengths[NB_RB_MAX] = {0};
  uint16_t rnti = UE_list->rnti[UE_id];

  uint8_t mac_sdus[MAX_NR_DLSCH_PAYLOAD_BYTES];
  
  LOG_D(MAC, "Scheduling UE specific search space DCI type 1\n");

  ta_len = gNB_mac->ta_len;

  CCEIndex = allocate_nr_CCEs(gNB_mac,
                              1, // bwp_id
                              0, // coreset_id
                              4, // aggregation,
                              1, // search_space, 0 common, 1 ue-specific
                              UE_id,
                              0); // m

  if (CCEIndex == -1) return;

  AssertFatal(CCEIndex>0,"CCEIndex is negative\n");
  int CCEIndices[2];
  CCEIndices[0] = CCEIndex;

  TBS_bytes = configure_fapi_dl_pdu(module_idP,
                                    CCEIndices,
                                    dl_req,
				    pucch_sched, 
                                    dlsch_config!=NULL ? dlsch_config->mcsIndex : NULL,
                                    dlsch_config!=NULL ? &dlsch_config->rbSize : NULL,
                                    dlsch_config!=NULL ? &dlsch_config->rbStart : NULL);
 
  //The --NOS1 use case currently schedules DLSCH transmissions only when there is IP traffic arriving
  //through the LTE stack
  if (IS_SOFTMODEM_NOS1){

    for (lcid = NB_RB_MAX - 1; lcid >= DTCH; lcid--) {

      // TODO: check if the lcid is active

      LOG_D(MAC, "[gNB %d], Frame %d, DTCH%d->DLSCH, Checking RLC status (TBS %d bytes, len %d)\n",
        module_idP, frameP, lcid, TBS_bytes, TBS_bytes - ta_len - header_length_total - sdu_length_total - 3);

      if (TBS_bytes - ta_len - header_length_total - sdu_length_total - 3 > 0) {
        rlc_status = mac_rlc_status_ind(module_idP,
                                        rnti,
                                        module_idP,
                                        frameP,
                                        slotP,
                                        ENB_FLAG_YES,
                                        MBMS_FLAG_NO,
                                        lcid,
                                        0,
                                        0);

        if (rlc_status.bytes_in_buffer > 0) {

          LOG_D(MAC, "[gNB %d][USER-PLANE DEFAULT DRB] Frame %d : DTCH->DLSCH, Requesting %d bytes from RLC (lcid %d total hdr len %d), TBS_bytes: %d \n \n",
            module_idP, frameP, TBS_bytes - ta_len - header_length_total - sdu_length_total - 3,
            lcid, header_length_total, TBS_bytes);

          sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
                                                   rnti,
                                                   module_idP,
                                                   frameP,
                                                   ENB_FLAG_YES,
                                                   MBMS_FLAG_NO,
                                                   lcid,
                                                   TBS_bytes - ta_len - header_length_total - sdu_length_total - 3,
                                                   (char *)&mac_sdus[sdu_length_total],
                                                   0,
                                                   0);

          LOG_D(MAC, "[gNB %d][USER-PLANE DEFAULT DRB] Got %d bytes for DTCH %d \n", module_idP, sdu_lengths[num_sdus], lcid);

          sdu_lcids[num_sdus] = lcid;
          sdu_length_total += sdu_lengths[num_sdus];
          header_length_last = 1 + 1 + (sdu_lengths[num_sdus] >= 128);
          header_length_total += header_length_last;

          num_sdus++;

          //ue_sched_ctl->uplane_inactivity_timer = 0;
        }
      } else { // no TBS_bytes left
      break;
      }
    }
  } //if (IS_SOFTMODEM_NOS1)
  else {

    //When the --NOS1 option is not enabled, DLSCH transmissions with random data
    //occur every time that the current function is called (dlsch phytest mode)
    LOG_D(MAC,"Configuring DL_TX in %d.%d\n", frameP, slotP);

    // fill dlsch_buffer with random data
    for (i = 0; i < TBS_bytes; i++){
      //mac_sdus[i] = (unsigned char) rand(); 
      ((uint8_t *)gNB_mac->UE_list.DLSCH_pdu[0][0].payload[0])[i] = (unsigned char) (lrand48()&0xff);
    }

    //Sending SDUs with size 1
    //Initialize elements of sdu_lcids and sdu_lengths
    sdu_lcids[0] = 0x05; // DRB
    sdu_lengths[0] = TBS_bytes - ta_len - 3;
    header_length_total += 2 + (sdu_lengths[0] >= 128);
    sdu_length_total += sdu_lengths[0];
    num_sdus +=1;

    #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
    if (frameP%100 == 0){
      LOG_I(MAC, "Printing first 10 payload bytes at the gNB side, Frame: %d, slot: %d, TBS size: %d \n", frameP, slotP, TBS_bytes);
      for(int i = 0; i < 10; i++) {
        LOG_I(MAC, "%x. ", ((uint8_t *)gNB_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0])[i]);
      }
    }
    #endif

  } // else IS_SOFTMODEM_NOS1

  // there is at least one SDU or TA command
  // if (num_sdus > 0 ){
  if (ta_len + sdu_length_total + header_length_total > 0) {

    // Check if there is data from RLC or CE
    if (TBS_bytes >= 2 + header_length_total + sdu_length_total + ta_len) {
      // we have to consider padding
      // padding param currently not in use
      //padding = TBS_bytes - header_length_total - sdu_length_total - ta_len - 1;
      post_padding = 1;
    } else {
      //padding = 0;
      post_padding = 0;
    }

    offset = nr_generate_dlsch_pdu(module_idP,
                                   (unsigned char *) mac_sdus,
                                   (unsigned char *) gNB_mac->UE_list.DLSCH_pdu[0][0].payload[0], //(unsigned char *) mac_pdu,
                                   num_sdus, //num_sdus
                                   sdu_lengths,
                                   sdu_lcids,
                                   255, // no drx
                                   NULL, // contention res id
                                   post_padding);

    // Padding: fill remainder of DLSCH with 0
    if (post_padding > 0){
      for (int j = 0; j < (TBS_bytes - offset); j++)
        gNB_mac->UE_list.DLSCH_pdu[0][0].payload[0][offset + j] = 0; // mac_pdu[offset + j] = 0;
    }

    configure_fapi_dl_Tx(module_idP, frameP, slotP, dl_req, tx_req, TBS_bytes, gNB_mac->pdu_index[CC_id]);

    if(IS_SOFTMODEM_NOS1){
      #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
        LOG_I(MAC, "Printing first 10 payload bytes at the gNB side, Frame: %d, slot: %d, TBS size: %d \n \n", frameP, slotP, TBS_bytes);
        for(int i = 0; i < 10; i++) { // TBS_bytes dlsch_pdu_rel15->transport_block_size/8 6784/8
          LOG_I(MAC, "%x. ", mac_payload[i]);
        }
      #endif
    } else {
      #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
      if (frameP%100 == 0){
        LOG_I(MAC, "Printing first 10 payload bytes at the gNB side, Frame: %d, slot: %d, TBS size: %d \n", frameP, slotP, TBS_bytes);
        for(int i = 0; i < 10; i++) {
          LOG_I(MAC, "%x. ", ((uint8_t *)gNB_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0])[i]); //LOG_I(MAC, "%x. ", mac_payload[i]);
        }
      }
      #endif
    }
  }
  else {  // There is no data from RLC or MAC header, so don't schedule
  }

}


void nr_schedule_uss_ulsch_phytest(int Mod_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP) {

  gNB_MAC_INST                      *nr_mac    = RC.nrmac[Mod_idP];
  NR_COMMON_channels_t                  *cc    = nr_mac->common_channels;
  NR_ServingCellConfigCommon_t         *scc    = cc->ServingCellConfigCommon;

  int bwp_id=1;

  int UE_id = 0;
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;
  AssertFatal(UE_list->active[UE_id] >=0,"Cannot find UE_id %d is not active\n",UE_id);

  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  AssertFatal(secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
	      "downlinkBWP_ToAddModList has %d BWP!\n",
	      secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_id-1];
  NR_BWP_Downlink_t *bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];
  nfapi_nr_ul_tti_request_t *UL_tti_req = &RC.nrmac[Mod_idP]->UL_tti_req[0];
  nfapi_nr_ul_dci_request_t *UL_dci_req = &RC.nrmac[Mod_idP]->UL_dci_req[0];


  uint16_t rnti = UE_list->rnti[UE_id];
  nfapi_nr_ul_dci_request_pdus_t  *ul_dci_request_pdu;

  UL_tti_req->SFN = frameP;
  UL_tti_req->Slot = slotP;
  UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
  UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
  nfapi_nr_pusch_pdu_t  *pusch_pdu = &UL_tti_req->pdus_list[UL_tti_req->n_pdus].pusch_pdu;
  memset(pusch_pdu,0,sizeof(nfapi_nr_pusch_pdu_t));
  UL_tti_req->n_pdus+=1;  

  LOG_D(MAC, "Scheduling UE specific PUSCH\n");
  //UL_tti_req = &nr_mac->UL_tti_req[CC_id];
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
  
  pusch_pdu->bwp_size  = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pusch_pdu->bwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pusch_pdu->subcarrier_spacing = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  pusch_pdu->cyclic_prefix = 0;
  //pusch information always include
  //this informantion seems to be redundant. with hthe mcs_index and the modulation table, the mod_order and target_code_rate can be determined.
  pusch_pdu->mcs_index = 9;
  pusch_pdu->mcs_table = 0; //0: notqam256 [TS38.214, table 5.1.3.1-1] - corresponds to nr_target_code_rate_table1 in PHY
  pusch_pdu->target_code_rate = nr_get_code_rate_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table) ;
  pusch_pdu->qam_mod_order = nr_get_Qm_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table) ;
  pusch_pdu->transform_precoding = 0;
  pusch_pdu->data_scrambling_id = 0; //It equals the higher-layer parameter Data-scrambling-Identity if configured and the RNTI equals the C-RNTI, otherwise L2 needs to set it to physical cell id.;
  pusch_pdu->nrOfLayers = 1;
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

  // --------------------
  // ------- DMRS -------
  // --------------------
  uint16_t l_prime_mask            = get_l_prime(pusch_pdu->nr_of_symbols, typeB, pusch_dmrs_pos0, pusch_len1);
  pusch_pdu->ul_dmrs_symb_pos      = l_prime_mask << pusch_pdu->start_symbol_index;
  pusch_pdu->dmrs_config_type      = 0;      // dmrs-type 1 (the one with a single DMRS symbol in the beginning)
  pusch_pdu->ul_dmrs_scrambling_id = 0;      // If provided and the PUSCH is not a msg3 PUSCH, otherwise, L2 should set this to physical cell id
  pusch_pdu->scid                  = 0;      // DMRS sequence initialization [TS38.211, sec 6.4.1.1.1]
                                             // Should match what is sent in DCI 0_1, otherwise set to 0
  //pusch_pdu->num_dmrs_cdm_grps_no_data;
  //pusch_pdu->dmrs_ports; // DMRS ports. [TS38.212 7.3.1.1.2] provides description between DCI 0-1 content and DMRS ports
                           // Bitmap occupying the 11 LSBs with: bit 0: antenna port 1000 bit 11: antenna port 1011,
                           // and for each bit 0: DMRS port not used 1: DMRS port used
  // --------------------------------------------------------------------------------------------------------------------------------------------

  // --------------------
  // ------- PTRS -------
  // --------------------
  uint8_t ptrs_mcs1 = 2;  // higher layer parameter in PTRS-UplinkConfig
  uint8_t ptrs_mcs2 = 4;  // higher layer parameter in PTRS-UplinkConfig
  uint8_t ptrs_mcs3 = 10; // higher layer parameter in PTRS-UplinkConfig
  uint16_t n_rb0 = 25;    // higher layer parameter in PTRS-UplinkConfig
  uint16_t n_rb1 = 75;    // higher layer parameter in PTRS-UplinkConfig
  pusch_pdu->pusch_ptrs.ptrs_time_density = get_L_ptrs(ptrs_mcs1, ptrs_mcs2, ptrs_mcs3, pusch_pdu->mcs_index, pusch_pdu->mcs_table);
  pusch_pdu->pusch_ptrs.ptrs_freq_density = get_K_ptrs(n_rb0, n_rb1, pusch_pdu->rb_size);
  pusch_pdu->pusch_ptrs.ptrs_ports_list   = (nfapi_nr_ptrs_ports_t *) malloc(2*sizeof(nfapi_nr_ptrs_ports_t));
  pusch_pdu->pusch_ptrs.ptrs_ports_list[0].ptrs_re_offset = 0;

  if(1<<pusch_pdu->pusch_ptrs.ptrs_time_density >= pusch_pdu->nr_of_symbols)
    pusch_pdu->pdu_bit_map &= ~PUSCH_PDU_BITMAP_PUSCH_PTRS; // disable PUSCH PTRS
  // --------------------------------------------------------------------------------------------------------------------------------------------

  //Pusch Allocation in frequency domain [TS38.214, sec 6.1.2.2]
  //Optional Data only included if indicated in pduBitmap
  pusch_pdu->pusch_data.rv_index = 0;
  pusch_pdu->pusch_data.harq_process_id = 0;
  pusch_pdu->pusch_data.new_data_indicator = 0;
  pusch_pdu->pusch_data.tb_size = nr_compute_tbs(pusch_pdu->qam_mod_order,
						 pusch_pdu->target_code_rate,
						 pusch_pdu->rb_size,
						 pusch_pdu->nr_of_symbols,
						 pusch_pdu->dmrs_config_type?4:6, //nb_re_dmrs - not sure where this is coming from - its not in the FAPI
						 0, //nb_rb_oh
						 pusch_pdu->nrOfLayers)>>3;
  pusch_pdu->pusch_data.num_cb = 0; //CBG not supported
  //pusch_pdu->pusch_data.cb_present_and_position;
  //pusch_pdu->pusch_uci;
  //pusch_pdu->pusch_ptrs;
  //pusch_pdu->dfts_ofdm;
  //beamforming
  //pusch_pdu->beamforming; //not used for now


  ul_dci_request_pdu = &UL_dci_req->ul_dci_pdu_list[UL_dci_req->numPdus];
  memset((void*)ul_dci_request_pdu,0,sizeof(nfapi_nr_ul_dci_request_pdus_t));
  ul_dci_request_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
  ul_dci_request_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &ul_dci_request_pdu->pdcch_pdu.pdcch_pdu_rel15;

  int dci_formats[2];
  int rnti_types[2];

  int CCEIndex = allocate_nr_CCEs(nr_mac,
				  1, // bwp_id
				  0, // coreset_id
				  4, // aggregation,
				  1, // search_space, 0 common, 1 ue-specific
				  UE_id,
				  0); // m

  dci_formats[0]  = NR_UL_DCI_FORMAT_0_0;
  rnti_types[0]   = NR_RNTI_C;
  LOG_D(MAC,"Configuring ULDCI/PDCCH in %d.%d\n", frameP,slotP);
  nr_configure_pdcch(pdcch_pdu_rel15,
		     1, // ue-specific,
		     scc,
		     bwp);
  
  dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];

  AssertFatal(CCEIndex>=0,"CCEIndex is negative \n");
  pdcch_pdu_rel15->CceIndex[pdcch_pdu_rel15->numDlDci] = CCEIndex;

  LOG_D(PHY,"CCEIndex %d\n",pdcch_pdu_rel15->CceIndex[pdcch_pdu_rel15->numDlDci]);

  config_uldci(ubwp,pusch_pdu,pdcch_pdu_rel15, &dci_pdu_rel15[0], dci_formats, rnti_types);
  
  pdcch_pdu_rel15->PayloadSizeBits[0]=nr_dci_size(dci_formats[0],rnti_types[0],pdcch_pdu_rel15->BWPSize);
  fill_dci_pdu_rel15(pdcch_pdu_rel15,&dci_pdu_rel15[0],dci_formats,rnti_types);
  
}

