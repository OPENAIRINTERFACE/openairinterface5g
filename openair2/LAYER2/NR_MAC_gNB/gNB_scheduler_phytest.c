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
#include "NR_MAC_gNB/mac_proto.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
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

//#define UL_HARQ_PRINT
extern RAN_CONTEXT_t RC;

const uint8_t nr_rv_round_map[4] = {0, 2, 1, 3}; 
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

/* schedules whole bandwidth for first user, all the time */
void nr_preprocessor_phytest(module_id_t module_id,
                             frame_t frame,
                             sub_frame_t slot,
                             int num_slots_per_tdd)
{
  if (slot != 1)
    return; /* only schedule in slot 1 for now */
  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  const int UE_id = 0;
  const int CC_id = 0;
  AssertFatal(UE_info->active[UE_id],
              "%s(): expected UE %d to be active\n",
              __func__,
              UE_id);

  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  /* find largest unallocated chunk */
  const int bwpSize = NRRIV2BW(sched_ctrl->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
  int rbStart = 0;
  int tStart = 0;
  int rbSize = 0;
  uint16_t *vrb_map = RC.nrmac[module_id]->common_channels[CC_id].vrb_map;
  /* find largest unallocated RB region */
  do {
    /* advance to first free RB */
    while (tStart < bwpSize && vrb_map[tStart])
      tStart++;
    /* find maximum rbSize at current rbStart */
    int tSize = 1;
    while (tStart + tSize < bwpSize && !vrb_map[tStart + tSize])
      tSize++;
    if (tSize > rbSize) {
      rbStart = tStart;
      rbSize = tSize;
    }
    tStart += tSize;
  } while (tStart < bwpSize);

  sched_ctrl->num_total_bytes = 0;
  const int lcid = DL_SCH_LCID_DTCH;
  const uint16_t rnti = UE_info->rnti[UE_id];
  /* update sched_ctrl->num_total_bytes so that postprocessor schedules data,
   * if available */
  sched_ctrl->rlc_status[lcid] = mac_rlc_status_ind(module_id,
                                                    rnti,
                                                    module_id,
                                                    frame,
                                                    slot,
                                                    ENB_FLAG_YES,
                                                    MBMS_FLAG_NO,
                                                    lcid,
                                                    0,
                                                    0);
  sched_ctrl->num_total_bytes += sched_ctrl->rlc_status[lcid].bytes_in_buffer;

  const int target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  sched_ctrl->search_space = get_searchspace(sched_ctrl->active_bwp, target_ss);
  uint8_t nr_of_candidates;
  find_aggregation_candidates(&sched_ctrl->aggregation_level,
                              &nr_of_candidates,
                              sched_ctrl->search_space);
  sched_ctrl->coreset = get_coreset(
      sched_ctrl->active_bwp, sched_ctrl->search_space, 1 /* dedicated */);
  const int cid = sched_ctrl->coreset->controlResourceSetId;
  const uint16_t Y = UE_info->Y[UE_id][cid][RC.nrmac[module_id]->current_slot];
  const int m = UE_info->num_pdcch_cand[UE_id][cid];
  sched_ctrl->cce_index = allocate_nr_CCEs(RC.nrmac[module_id],
                                  sched_ctrl->active_bwp,
                                  sched_ctrl->coreset,
                                  sched_ctrl->aggregation_level,
                                  Y,
                                  m,
                                  nr_of_candidates);
  AssertFatal(sched_ctrl->cce_index >= 0,
              "%s(): could not find CCE for UE %d\n",
              __func__,
              UE_id);

  nr_acknack_scheduling(module_id,
                        UE_id,
                        frame,
                        slot,
                        num_slots_per_tdd,
                        &sched_ctrl->pucch_sched_idx,
                        &sched_ctrl->pucch_occ_idx);
  AssertFatal(sched_ctrl->pucch_sched_idx >= 0, "no uplink slot for PUCCH found!\n");

  sched_ctrl->rbStart = rbStart;
  sched_ctrl->rbSize = rbSize;
  sched_ctrl->time_domain_allocation = 2;
  sched_ctrl->mcsTableIdx = 0;
  sched_ctrl->mcs = 9;
  sched_ctrl->numDmrsCdmGrpsNoData = 1;

  /* mark the corresponding RBs as used */
  for (int rb = 0; rb < sched_ctrl->rbSize; rb++)
    vrb_map[rb + sched_ctrl->rbStart] = 1;
}

void config_uldci(NR_BWP_Uplink_t *ubwp,
                  nfapi_nr_pusch_pdu_t *pusch_pdu,
                  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15,
                  dci_pdu_rel15_t *dci_pdu_rel15,
                  int *dci_formats, int *rnti_types,
                  int time_domain_assignment, uint8_t tpc,
                  int n_ubwp, int bwp_id) {

  switch(dci_formats[(pdcch_pdu_rel15->numDlDci)-1]) {
    case NR_UL_DCI_FORMAT_0_0:
      dci_pdu_rel15->frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(pusch_pdu->rb_size,
                                                                                         pusch_pdu->rb_start,
	                                                                                 NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275));

      dci_pdu_rel15->time_domain_assignment.val = time_domain_assignment;
      dci_pdu_rel15->frequency_hopping_flag.val = pusch_pdu->frequency_hopping;
      dci_pdu_rel15->mcs = 9;

      dci_pdu_rel15->format_indicator = 0;
      dci_pdu_rel15->ndi = 1;
      dci_pdu_rel15->rv = 0;
      dci_pdu_rel15->harq_pid = 0;
      dci_pdu_rel15->tpc = 1;
      break;
    case NR_UL_DCI_FORMAT_0_1:
      dci_pdu_rel15->ndi = pusch_pdu->pusch_data.new_data_indicator;
      dci_pdu_rel15->rv = pusch_pdu->pusch_data.rv_index;
      dci_pdu_rel15->harq_pid = pusch_pdu->pusch_data.harq_process_id;
      dci_pdu_rel15->frequency_hopping_flag.val = pusch_pdu->frequency_hopping;
      dci_pdu_rel15->dai[0].val = 0; //TODO
      // bwp indicator
      if (n_ubwp < 4)
        dci_pdu_rel15->bwp_indicator.val = bwp_id;
      else
        dci_pdu_rel15->bwp_indicator.val = bwp_id - 1; // as per table 7.3.1.1.2-1 in 38.212
      // frequency domain assignment
      if (ubwp->bwp_Dedicated->pusch_Config->choice.setup->resourceAllocation==NR_PUSCH_Config__resourceAllocation_resourceAllocationType1)
        dci_pdu_rel15->frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(pusch_pdu->rb_size,
                                                                                             pusch_pdu->rb_start,
                                                                                             NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275));
      else
        AssertFatal(1==0,"Only frequency resource allocation type 1 is currently supported\n");
      // time domain assignment
      dci_pdu_rel15->time_domain_assignment.val = time_domain_assignment;
      // mcs
      dci_pdu_rel15->mcs = pusch_pdu->mcs_index;
      // tpc command for pusch
      dci_pdu_rel15->tpc = tpc;
      // SRS resource indicator
      if (ubwp->bwp_Dedicated->pusch_Config->choice.setup->txConfig != NULL) {
        if (*ubwp->bwp_Dedicated->pusch_Config->choice.setup->txConfig == NR_PUSCH_Config__txConfig_codebook)
          dci_pdu_rel15->srs_resource_indicator.val = 0; // taking resource 0 for SRS
        else
          AssertFatal(1==0,"Non Codebook configuration non supported\n");
      }
      // Antenna Ports
      dci_pdu_rel15->antenna_ports.val = 0; // TODO for now it is hardcoded, it should depends on cdm group no data and rank
      // DMRS sequence initialization
      dci_pdu_rel15->dmrs_sequence_initialization.val = pusch_pdu->scid;
      break;
    default :
      AssertFatal(1==0,"Valid UL formats are 0_0 and 0_1 \n");
  }

  LOG_D(MAC, "[gNB scheduler phytest] ULDCI type 0 payload: PDCCH CCEIndex %d, freq_alloc %d, time_alloc %d, freq_hop_flag %d, mcs %d tpc %d ndi %d rv %d\n",
	pdcch_pdu_rel15->dci_pdu.CceIndex[pdcch_pdu_rel15->numDlDci],
	dci_pdu_rel15->frequency_domain_assignment.val,
	dci_pdu_rel15->time_domain_assignment.val,
	dci_pdu_rel15->frequency_hopping_flag.val,
	dci_pdu_rel15->mcs,
	dci_pdu_rel15->tpc,
	dci_pdu_rel15->ndi, 
	dci_pdu_rel15->rv);

}
