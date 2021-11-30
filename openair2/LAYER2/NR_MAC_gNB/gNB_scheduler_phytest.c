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
#include "NR_MAC_gNB/mac_proto.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
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

const uint8_t nr_rv_round_map[4] = {0, 2, 3, 1};
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

  int dlBWP_carrier_bandwidth = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  
  /*
  int scs               = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
  
  int slots_per_frame   = 10*(1<<scs);

  int FR                = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0] >= 257 ? nr_FR2 : nr_FR1;
  */

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "Scheduling common search space DCI type 1 dlBWP BW.firstRB %d.%d\n",
	  dlBWP_carrier_bandwidth,
	  NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE));
    
    
    dl_req = &nr_mac->DL_req[CC_id].dl_tti_request_body;
    dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
    dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
    
    dl_tti_pdsch_pdu = &nr_mac->DL_req[CC_id].dl_tti_request_body.dl_tti_pdu_list[nr_mac->DL_req[CC_id].dl_tti_request_body.nPDUs+1];
    memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
    dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

    
    //    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;
    
    pdsch_pdu_rel15->pduBitmap = 0;
    pdsch_pdu_rel15->rnti = rnti;
    pdsch_pdu_rel15->pduIndex = 0;

    // BWP
    pdsch_pdu_rel15->BWPSize  = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    pdsch_pdu_rel15->BWPStart = NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
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
    int mappingtype_tmp, mappingtype=0;

    for (int i=0;
	 i<scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;
	 i++) {
      startSymbolAndLength = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
      SLIV2SL(startSymbolAndLength,&StartSymbolIndex_tmp,&NrOfSymbols_tmp);
      mappingtype_tmp = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->mappingType;
      if (NrOfSymbols_tmp < NrOfSymbols) {
	NrOfSymbols = NrOfSymbols_tmp;
        StartSymbolIndex = StartSymbolIndex_tmp;
        mappingtype = mappingtype_tmp;
	//	k0 = *scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->k0;
	//	time_domain_assignment = i;
      }
    }
    AssertFatal(StartSymbolIndex>=0,"StartSymbolIndex is negative\n");
    pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
    pdsch_pdu_rel15->NrOfSymbols      = NrOfSymbols;
    pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(NULL,
                                                    scc->dmrs_TypeA_Position,
                                                    NrOfSymbols,
                                                    StartSymbolIndex,
                                                    mappingtype);

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
    
    nr_mac->DL_req[CC_id].dl_tti_request_body.nPDUs+=2;
    
    TX_req = &nr_mac->TX_req[CC_id].pdu_list[nr_mac->TX_req[CC_id].Number_of_PDUs];
    TX_req->PDU_length = 6;
    TX_req->PDU_index = nr_mac->pdu_index[CC_id]++;
    TX_req->num_TLV = 1;
    TX_req->TLVs[0].length = 8;
    // why do we copy from RAR_pdu here? Shouldn't we fill some more or less
    // meaningful data, e.g., padding + random data?
    //memcpy((void *)&TX_req->TLVs[0].value.direct[0], (void *)&cc[CC_id].RAR_pdu[0].payload[0], TX_req->TLVs[0].length);
    nr_mac->TX_req[CC_id].Number_of_PDUs++;
    nr_mac->TX_req[CC_id].SFN=frameP;
    nr_mac->TX_req[CC_id].Slot=slotP;
  }
}

extern int getNrOfSymbols(NR_BWP_Downlink_t *bwp, int tda);
extern uint8_t getN_PRB_DMRS(NR_BWP_Downlink_t *bwp, int numDmrsCdmGrpsNoData);
uint32_t target_dl_mcs = 9;
uint32_t target_dl_Nl = 1;
uint32_t target_dl_bw = 50;
uint64_t dlsch_slot_bitmap = (1<<1);
/* schedules whole bandwidth for first user, all the time */
void nr_preprocessor_phytest(module_id_t module_id,
                             frame_t frame,
                             sub_frame_t slot)
{
  if (!is_xlsch_in_slot(dlsch_slot_bitmap, slot))
    return;
  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon;
  const int UE_id = 0;
  const int CC_id = 0;
  AssertFatal(UE_info->active[UE_id],
              "%s(): expected UE %d to be active\n",
              __func__,
              UE_id);
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  /* find largest unallocated chunk */
  const int bwpSize = NRRIV2BW(sched_ctrl->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  const int BWPStart = NRRIV2PRBOFFSET(sched_ctrl->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  int rbStart = 0;
  int rbSize = 0;
  if (target_dl_bw>bwpSize)
    target_dl_bw = bwpSize;
  uint16_t *vrb_map = RC.nrmac[module_id]->common_channels[CC_id].vrb_map;
  /* loop ensures that we allocate exactly target_dl_bw, or return */
  while (true) {
    /* advance to first free RB */
    while (rbStart < bwpSize && vrb_map[rbStart + BWPStart])
      rbStart++;
    rbSize = 1;
    /* iterate until we are at target_dl_bw or no available RBs */
    while (rbStart + rbSize < bwpSize && !vrb_map[rbStart + rbSize + BWPStart] && rbSize < target_dl_bw)
      rbSize++;
    /* found target_dl_bw? */
    if (rbSize == target_dl_bw)
      break;
    /* at end and below target_dl_bw? */
    if (rbStart + rbSize >= bwpSize)
      return;
    rbStart += rbSize;
  }

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
  sched_ctrl->lcid_to_schedule = lcid;

  uint8_t nr_of_candidates;
  for (int i=0; i<5; i++) {
    // for now taking the lowest value among the available aggregation levels
    find_aggregation_candidates(&sched_ctrl->aggregation_level,
                                &nr_of_candidates,
                                sched_ctrl->search_space,
                                1<<i);
    if(nr_of_candidates>0) break;
  }
  AssertFatal(nr_of_candidates>0,"nr_of_candidates is 0\n");

  const int cid = sched_ctrl->coreset->controlResourceSetId;
  const uint16_t Y = UE_info->Y[UE_id][cid%3][slot];
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

  const int alloc = nr_acknack_scheduling(module_id, UE_id, frame, slot, -1,0);
  if (alloc < 0) {
    LOG_D(MAC,
          "%s(): could not find PUCCH for UE %d/%04x@%d.%d\n",
          __func__,
          UE_id,
          rnti,
          frame,
          slot);
    UE_info->num_pdcch_cand[UE_id][cid]--;
    int *cce_list = RC.nrmac[module_id]->cce_list[cid];
    for (int i = 0; i < sched_ctrl->aggregation_level; i++)
      cce_list[sched_ctrl->cce_index + i] = 0;
    return;
  }

  //AssertFatal(alloc,
  //            "could not find uplink slot for PUCCH (RNTI %04x@%d.%d)!\n",
  //            rnti, frame, slot);

  NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
  NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;
  sched_pdsch->pucch_allocation = alloc;
  sched_pdsch->rbStart = rbStart;
  sched_pdsch->rbSize = rbSize;
  const int tda = sched_ctrl->active_bwp ? RC.nrmac[module_id]->preferred_dl_tda[sched_ctrl->active_bwp->bwp_Id][slot] : 1;
  const long f = sched_ctrl->active_bwp ? sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats : 0;
  ps->nrOfLayers = target_dl_Nl;
  if (ps->time_domain_allocation != tda)
    nr_set_pdsch_semi_static(
        scc, UE_info->CellGroup[UE_id], sched_ctrl->active_bwp, NULL, tda, f, ps);

  sched_pdsch->mcs = target_dl_mcs;
  sched_pdsch->Qm = nr_get_Qm_dl(sched_pdsch->mcs, ps->mcsTableIdx);
  sched_pdsch->R = nr_get_code_rate_dl(sched_pdsch->mcs, ps->mcsTableIdx);
  sched_pdsch->tb_size = nr_compute_tbs(sched_pdsch->Qm,
                                        sched_pdsch->R,
                                        sched_pdsch->rbSize,
                                        ps->nrOfSymbols,
                                        ps->N_PRB_DMRS * ps->N_DMRS_SLOT,
                                        0 /* N_PRB_oh, 0 for initialBWP */,
                                        0 /* tb_scaling */,
                                        ps->nrOfLayers)
                         >> 3;

  /* get the PID of a HARQ process awaiting retransmission, or -1 otherwise */
  sched_pdsch->dl_harq_pid = sched_ctrl->retrans_dl_harq.head;

  /* mark the corresponding RBs as used */
  for (int rb = 0; rb < sched_pdsch->rbSize; rb++)
    vrb_map[rb + sched_pdsch->rbStart + BWPStart] = 1;

  if ((frame&127) == 0) LOG_D(MAC,"phytest: %d.%d DL mcs %d, DL rbStart %d, DL rbSize %d\n", frame, slot, sched_pdsch->mcs, rbStart,rbSize);
}

uint32_t target_ul_mcs = 9;
uint32_t target_ul_bw = 50;
uint64_t ulsch_slot_bitmap = (1 << 8);
bool nr_ul_preprocessor_phytest(module_id_t module_id, frame_t frame, sub_frame_t slot)
{
  gNB_MAC_INST *nr_mac = RC.nrmac[module_id];
  NR_COMMON_channels_t *cc = nr_mac->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  const int mu = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
  NR_UE_info_t *UE_info = &nr_mac->UE_info;

  AssertFatal(UE_info->num_UEs <= 1,
              "%s() cannot handle more than one UE, but found %d\n",
              __func__,
              UE_info->num_UEs);
  if (UE_info->num_UEs == 0)
    return false;

  const int UE_id = 0;
  const int CC_id = 0;

  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  const int tda = sched_ctrl->active_ubwp ? RC.nrmac[module_id]->preferred_ul_tda[sched_ctrl->active_ubwp->bwp_Id][slot] : 1;
  if (tda < 0)
    return false;
  const struct NR_PUSCH_TimeDomainResourceAllocationList *tdaList =
    sched_ctrl->active_ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
  AssertFatal(tda < tdaList->list.count,
              "time domain assignment %d >= %d\n",
              tda,
              tdaList->list.count);
  int K2 = get_K2(scc,sched_ctrl->active_ubwp, tda, mu);
  const int sched_frame = frame + (slot + K2 >= nr_slots_per_frame[mu]);
  const int sched_slot = (slot + K2) % nr_slots_per_frame[mu];
  /* check if slot is UL, and that slot is 8 (assuming K2=6 because of UE
   * limitations).  Note that if K2 or the TDD configuration is changed, below
   * conditions might exclude each other and never be true */
  if (!is_xlsch_in_slot(ulsch_slot_bitmap, sched_slot))
    return false;

  const long f = sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats;
  const int dci_format = f ? NR_UL_DCI_FORMAT_0_1 : NR_UL_DCI_FORMAT_0_0;
  const uint8_t num_dmrs_cdm_grps_no_data = 1;
  /* we want to avoid a lengthy deduction of DMRS and other parameters in
   * every TTI if we can save it, so check whether dci_format, TDA, or
   * num_dmrs_cdm_grps_no_data has changed and only then recompute */
  NR_pusch_semi_static_t *ps = &sched_ctrl->pusch_semi_static;
  if (ps->time_domain_allocation != tda
      || ps->dci_format != dci_format
      || ps->num_dmrs_cdm_grps_no_data != num_dmrs_cdm_grps_no_data)
    nr_set_pusch_semi_static(scc, sched_ctrl->active_ubwp, NULL,dci_format, tda, num_dmrs_cdm_grps_no_data, ps);

  uint16_t rbStart = 0;
  uint16_t rbSize;

  const int bw = NRRIV2BW(sched_ctrl->active_ubwp ?
                          sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth :
                          scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  const int BWPStart = NRRIV2PRBOFFSET(sched_ctrl->active_ubwp ?
                                       sched_ctrl->active_ubwp->bwp_Common->genericParameters.locationAndBandwidth :
                                       scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  if (target_ul_bw>bw)
    rbSize = bw;
  else
    rbSize = target_ul_bw;

  uint16_t *vrb_map_UL =
      &RC.nrmac[module_id]->common_channels[CC_id].vrb_map_UL[sched_slot * MAX_BWP_SIZE];
  const uint16_t symb = ((1 << ps->nrOfSymbols) - 1) << ps->startSymbolIndex;
  for (int i = rbStart; i < rbStart + rbSize; ++i) {
    if ((vrb_map_UL[i+BWPStart] & symb) != 0) {
      LOG_E(MAC,
            "%s(): %4d.%2d RB %d is already reserved, cannot schedule UE\n",
            __func__,
            frame,
            slot,
            i);
      return false;
    }
  }

  sched_ctrl->sched_pusch.slot = sched_slot;
  sched_ctrl->sched_pusch.frame = sched_frame;

  uint8_t nr_of_candidates;
  for (int i=0; i<5; i++) {
    // for now taking the lowest value among the available aggregation levels
    find_aggregation_candidates(&sched_ctrl->aggregation_level,
                                &nr_of_candidates,
                                sched_ctrl->search_space,
                                1<<i);
    if(nr_of_candidates>0) break;
  }
  AssertFatal(nr_of_candidates>0,"nr_of_candidates is 0\n");

  const int cid = sched_ctrl->coreset->controlResourceSetId;
  const uint16_t Y = UE_info->Y[UE_id][cid%3][slot];
  const int m = UE_info->num_pdcch_cand[UE_id][cid];
  sched_ctrl->cce_index = allocate_nr_CCEs(RC.nrmac[module_id],
                                           sched_ctrl->active_bwp,
                                           sched_ctrl->coreset,
                                           sched_ctrl->aggregation_level,
                                           Y,
                                           m,
                                           nr_of_candidates);
  if (sched_ctrl->cce_index < 0) {
    LOG_E(MAC, "%s(): CCE list not empty, couldn't schedule PUSCH\n", __func__);
    return false;
  }
  UE_info->num_pdcch_cand[UE_id][cid]++;

  const int mcs = target_ul_mcs;
  NR_sched_pusch_t *sched_pusch = &sched_ctrl->sched_pusch;
  sched_pusch->mcs = mcs;
  sched_pusch->rbStart = rbStart;
  sched_pusch->rbSize = rbSize;
  /* get the PID of a HARQ process awaiting retransmission, or -1 for "any new" */
  sched_pusch->ul_harq_pid = sched_ctrl->retrans_ul_harq.head;

  /* Calculate TBS from MCS */
  sched_pusch->R = nr_get_code_rate_ul(mcs, ps->mcs_table);
  sched_pusch->Qm = nr_get_Qm_ul(mcs, ps->mcs_table);
  if (ps->pusch_Config->tp_pi2BPSK
      && ((ps->mcs_table == 3 && mcs < 2) || (ps->mcs_table == 4 && mcs < 6))) {
    sched_pusch->R >>= 1;
    sched_pusch->Qm <<= 1;
  }
  sched_pusch->tb_size = nr_compute_tbs(sched_pusch->Qm,
                                        sched_pusch->R,
                                        sched_pusch->rbSize,
                                        ps->nrOfSymbols,
                                        ps->N_PRB_DMRS * ps->num_dmrs_symb,
                                        0, // nb_rb_oh
                                        0,
                                        1 /* NrOfLayers */)
                         >> 3;

  /* mark the corresponding RBs as used */
  for (int rb = rbStart; rb < rbStart + rbSize; rb++)
    vrb_map_UL[rb+BWPStart] = 1;
  return true;
}
