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

/*! \file     gNB_scheduler_RA.c
 * \brief     primitives used for random access
 * \author    Guido Casati
 * \date      2019
 * \email:    guido.casati@iis.fraunhofer.de
 * \version
 */

#include "platform_types.h"

/* MAC */
#include "nr_mac_gNB.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"

/* Utils */
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/nr/nr_common.h"
#include "UTIL/OPT/opt.h"
#include "SIMULATION/TOOLS/sim.h" // for taus

extern RAN_CONTEXT_t RC;

void nr_add_subframe(uint16_t *frameP, uint16_t *slotP, int offset){
    *frameP = (*frameP + ((*slotP + offset) / 10)) % 1024;
    *slotP = ((*slotP + offset) % 10);
}

// WIP
// handles the event of msg1 reception
// todo:
// - offset computation
// - fix nr_add_subframe
// - msg2 time location
void nr_initiate_ra_proc(module_id_t module_idP,
                         int CC_id,
                         frame_t frameP,
                         sub_frame_t slotP,
                         uint16_t preamble_index,
                         int16_t timing_offset){

  uint16_t ra_rnti=157;
  uint16_t msg2_frame = frameP, msg2_slot = slotP;
  int offset;
  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];
  NR_RA_t *ra = &cc->ra[0];
  static uint8_t failure_cnt = 0;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 1);

  LOG_D(MAC, "[gNB %d][RAPROC] CC_id %d Frame %d, Subframe %d  Initiating RA procedure for preamble index %d\n", module_idP, CC_id, frameP, slotP, preamble_index);

  if (ra->state == RA_IDLE) {
    int loop = 0;
    LOG_D(MAC, "Frame %d, Subframe %d: Activating RA process \n", frameP, slotP);
    ra->state = Msg2;
    ra->timing_offset = timing_offset;
    ra->preamble_subframe = slotP;

    // if(cc->tdd_Config!=NULL){
    //   switch(cc->tdd_Config->subframeAssignment){
    //     default: printf("%s:%d: TODO\n", __FILE__, __LINE__); abort();
    //     case 1 :
    //       offset = 6;
    //       break;
    //   }
    // }else{//FDD
    //     // DJP - this is because VNF is 2 subframes ahead of PNF and TX needs 4 subframes
    //     if (nfapi_mode)
    //       offset = 7;
    //     else
    //       offset = 5;
    // }

    nr_add_subframe(&msg2_frame, &msg2_slot, offset);

    ra->Msg2_frame = msg2_frame;
    ra->Msg2_slot = msg2_slot;
    // ra->Msg2_slot = (slotP + offset) % 10;

    LOG_D(MAC, "%s() Msg2[%04d%d] SFN/SF:%04d%d offset:%d\n", __FUNCTION__, ra->Msg2_frame, ra->Msg2_slot, frameP, slotP, offset);

    do {
      ra->rnti = (taus() % 65518) + 1;
      loop++;
    }
    while (loop != 100 && !(find_nr_UE_id(module_idP, ra->rnti) == -1 && ra->rnti >= 1 && ra->rnti <= 65519));
    if (loop == 100) {
      LOG_E(MAC,"%s:%d:%s: [RAPROC] initialisation random access aborted\n", __FILE__, __LINE__, __FUNCTION__);
      abort();
    }

    ra->RA_rnti = ra_rnti;
    ra->preamble_index = preamble_index;
    failure_cnt = 0;

    LOG_D(MAC, "[gNB %d][RAPROC] CC_id %d Frame %d Activating Msg2 generation in frame %d, slot %d for rnti %x\n",
      module_idP,
      CC_id,
      frameP,
      ra->Msg2_frame,
      ra->Msg2_slot,
      ra->state);

    return;
  }
  LOG_E(MAC, "[gNB %d][RAPROC] FAILURE: CC_id %d Frame %d initiating RA procedure for preamble index %d\n", module_idP, CC_id, frameP, preamble_index);

  failure_cnt++;
  
  if(failure_cnt > 20) {
    LOG_E(MAC,"[gNB %d][RAPROC] CC_id %d Frame %d Clear Random access information\n", module_idP, CC_id, frameP);
    nr_clear_ra_proc(module_idP, CC_id, frameP);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 0);
}

void nr_schedule_RA(module_id_t module_idP, frame_t frameP, sub_frame_t slotP){

  //uint8_t i = 0;
  int CC_id = 0;
  gNB_MAC_INST *mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &mac->common_channels[CC_id];
  NR_RA_t *ra = &cc->ra[0];

  start_meas(&mac->schedule_ra);

  //for (i = 0; i < NR_NB_RA_PROC_MAX; i++) {
  LOG_D(MAC,"RA[state:%d]\n",ra->state);
  switch (ra->state){
    case Msg2:
    //nr_generate_Msg2(module_idP, CC_id, frameP, slotP);
    break;
    case Msg4:
    //generate_Msg4(module_idP, CC_id, frameP, slotP, ra);
    break;
    case WAIT_Msg4_ACK:
    //check_Msg4_retransmission(module_idP, CC_id, frameP, slotP, ra);
    break;
    default:
    break;
  }
  //}
  stop_meas(&mac->schedule_ra);
}

// WIP
// todo: fix
void nr_add_msg3(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP){

  gNB_MAC_INST                                   *mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t                            *cc = &mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t                   *scc = cc->ServingCellConfigCommon;
  NR_RA_t                                         *ra = &cc->ra[0];
  NR_UE_list_t                               *UE_list = &mac->UE_list;

  nfapi_nr_ul_tti_request_t                   *ul_req = &mac->UL_tti_req[0];
  nfapi_nr_ul_dci_request_t               *UL_dci_req = &mac->UL_dci_req[0];
  nfapi_nr_pusch_pdu_t                     *pusch_pdu = &ul_req->pdus_list[0].pusch_pdu;
  nfapi_nr_ul_dci_request_pdus_t  *ul_dci_request_pdu = &UL_dci_req->ul_dci_pdu_list[UL_dci_req->numPdus];

  int UE_id = 0, CCEIndex, dci_formats[2], rnti_types[2], bwp_id = 1, coreset_id = 0, aggregation = 4, search_space = 1;

  AssertFatal(ra->state != RA_IDLE, "RA is not active for RA %X\n", ra->rnti);

  // if(N_RB_UL == 25) {
  //   ra->msg3_first_rb = 1;
  // } else {
  //   if (cc->tdd_Config && N_RB_UL == 100) {
  //     ra->msg3_first_rb = 3;
  //   } else {
  //     ra->msg3_first_rb = 2;
  //   }
  // }
  // ra->msg3_nb_rb = 1;
  // /* UL Grant */
  // ra->msg3_mcs = 10;
  // ra->msg3_TPC = 3;
  // ra->msg3_ULdelay = 0;
  // ra->msg3_cqireq = 0;
  // ra->msg3_round = 0;

  LOG_D(MAC, "[gNB %d][RAPROC] Frame %d, Subframe %d : CC_id %d RA is active, Msg3 in (%d,%d)\n", module_idP, frameP, slotP, CC_id, ra->Msg3_frame, ra->Msg3_slot);

  ul_req->SFN = ra->Msg3_frame << 4 | ra->Msg3_slot;
  ul_req->Slot = slotP;
  ul_req->n_pdus = 1;
  ul_req->pdus_list[0].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
  ul_req->pdus_list[0].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
  memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));

  memset((void*)ul_dci_request_pdu,0,sizeof(nfapi_nr_ul_dci_request_pdus_t));
  ul_dci_request_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
  ul_dci_request_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));

  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &ul_dci_request_pdu->pdcch_pdu.pdcch_pdu_rel15;

  AssertFatal(UE_list->active[UE_id] >=0,"Cannot find UE_id %d is not active\n", UE_id);

  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  AssertFatal(secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
    "downlinkBWP_ToAddModList has %d BWP!\n", secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_id-1];
  NR_BWP_Downlink_t *bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];
  LOG_D(MAC, "Frame %d, Subframe %d Adding Msg3 UL Config Request for (%d,%d) : (%d,%d,%d) for rnti: %d\n",
    frameP,
    slotP,
    ra->Msg3_frame,
    ra->Msg3_slot,
    ra->msg3_nb_rb,
    ra->msg3_first_rb,
    ra->msg3_round,
    ra->rnti);

  pusch_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
  pusch_pdu->rnti = ra->rnti;
  pusch_pdu->handle = 0;
  pusch_pdu->bwp_size  = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pusch_pdu->bwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pusch_pdu->subcarrier_spacing = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  pusch_pdu->cyclic_prefix = 0;
  pusch_pdu->mcs_index = 9;
  pusch_pdu->mcs_table = 0;
  pusch_pdu->target_code_rate = nr_get_code_rate_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table);
  pusch_pdu->qam_mod_order = nr_get_Qm_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table);
  pusch_pdu->transform_precoding = 0;
  pusch_pdu->data_scrambling_id = 0;
  pusch_pdu->nrOfLayers = 1;
  pusch_pdu->ul_dmrs_symb_pos = 1;
  pusch_pdu->dmrs_config_type = 0;
  pusch_pdu->ul_dmrs_scrambling_id = 0; //If provided and the PUSCH is not a msg3 PUSCH, otherwise, L2 should set this to physical cell id.
  pusch_pdu->scid = 0; //DMRS sequence initialization [TS38.211, sec 6.4.1.1.1]. Should match what is sent in DCI 0_1, otherwise set to 0.
  pusch_pdu->resource_alloc = 1; //type 1
  //pusch_pdu->rb_bitmap;// for ressource alloc type 0
  pusch_pdu->rb_start = ra->msg3_first_rb;
  pusch_pdu->rb_size = ra->msg3_nb_rb;
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
  pusch_pdu->pusch_data.num_cb = 0;

  CCEIndex = allocate_nr_CCEs(mac, bwp_id, coreset_id, aggregation, search_space, UE_id, 0);
  AssertFatal(CCEIndex >= 0, "CCEIndex is negative\n");
  pdcch_pdu_rel15->dci_pdu.CceIndex[pdcch_pdu_rel15->numDlDci] = CCEIndex;
  LOG_D(PHY, "CCEIndex %d\n", pdcch_pdu_rel15->dci_pdu.CceIndex[pdcch_pdu_rel15->numDlDci]);

  LOG_D(MAC,"Configuring ULDCI/PDCCH in %d.%d\n", frameP,slotP);
  nr_configure_pdcch(pdcch_pdu_rel15, 1, scc, bwp);

  dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
  config_uldci(ubwp, pusch_pdu, pdcch_pdu_rel15, &dci_pdu_rel15[0], dci_formats, rnti_types);
  pdcch_pdu_rel15->dci_pdu.PayloadSizeBits[0] = nr_dci_size(dci_formats[0], rnti_types[0], pdcch_pdu_rel15->BWPSize);
  fill_dci_pdu_rel15(pdcch_pdu_rel15, &dci_pdu_rel15[0], dci_formats, rnti_types);
}

// WIP
// todo:
// - fix me
// - get msg3 alloc (see nr_process_rar)
void nr_generate_Msg2(module_id_t module_idP,
                      int CC_id,
                      frame_t frameP,
                      sub_frame_t slotP){

  int UE_id = 0, dci_formats[2], rnti_types[2], CCEIndex, dlBWP_carrier_bandwidth, mcsIndex = 9;
  int startSymbolAndLength = 0, StartSymbolIndex = -1, NrOfSymbols = 14, StartSymbolIndex_tmp, NrOfSymbols_tmp, x_Overhead, time_domain_assignment = 2;
  int bwp_id = 1, coreset_id = 0, aggregation = 4, search_space = 0, N_RB_UL = 106;
  gNB_MAC_INST                      *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t                  *cc = &nr_mac->common_channels[0];
  NR_RA_t                               *ra = &cc->ra[0];
  NR_UE_list_t                     *UE_list = &nr_mac->UE_list;

  uint16_t rnti = ra->rnti, RA_rnti = ra->RA_rnti, numDlDci;
  long locationAndBandwidth;
  // uint8_t *vrb_map = cc[CC_id].vrb_map, CC_id;

  if ((ra->Msg2_frame == frameP) && (ra->Msg2_slot == slotP)) {

    nfapi_nr_dl_tti_request_body_t *dl_req = &nr_mac->DL_req[CC_id].dl_tti_request_body;
    nfapi_nr_pdu_t *tx_req = &nr_mac->TX_req[CC_id].pdu_list[nr_mac->TX_req[CC_id].Number_of_PDUs];

    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
    dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));

    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs+1];
    memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
    dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;
    nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu_rel15->dci_pdu;
    numDlDci = pdcch_pdu_rel15->numDlDci;

    // Checking if the DCI allocation is feasible in current subframe
    if (dl_req->nPDUs == NFAPI_NR_MAX_DL_TTI_PDUS) {
      LOG_I(MAC, "[RAPROC] Subframe %d: FAPI DL structure is full, skip scheduling UE %d\n", slotP, RA_rnti);
      return;
    } else {
      LOG_D(MAC, "[RAPROC] Subframe %d: Checking CCE feasibility format : (%x,%d) \n", slotP, RA_rnti, aggregation);
      CCEIndex = allocate_nr_CCEs(nr_mac, bwp_id, coreset_id, aggregation, search_space, UE_id, 0);
      AssertFatal(CCEIndex > 0,"CCEIndex is negative\n");
    }

    LOG_D(MAC,"[gNB %d] [RAPROC] CC_id %d Frame %d, slotP %d: Generating RAR DCI, state %d\n", module_idP, CC_id, frameP, slotP, ra->state);

    NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
    locationAndBandwidth = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth;
    dlBWP_carrier_bandwidth = NRRIV2BW(locationAndBandwidth,275);
    NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
    AssertFatal(secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
      "downlinkBWP_ToAddModList has %d BWP!\n", secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
    NR_BWP_Downlink_t *bwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id - 1];

    LOG_D(MAC, "[RAPROC] Scheduling common search space DCI type 1 dlBWP BW.firstRB %d.%d\n", dlBWP_carrier_bandwidth,
      NRRIV2PRBOFFSET(locationAndBandwidth, 275));

    pdsch_pdu_rel15->pduBitmap = 0;
    pdsch_pdu_rel15->rnti = rnti;
    pdsch_pdu_rel15->pduIndex = 0;
    pdsch_pdu_rel15->BWPSize  = NRRIV2BW(locationAndBandwidth,275);
    pdsch_pdu_rel15->BWPStart = NRRIV2PRBOFFSET(locationAndBandwidth,275);
    pdsch_pdu_rel15->SubcarrierSpacing = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
    pdsch_pdu_rel15->CyclicPrefix = 0;
    pdsch_pdu_rel15->NrOfCodewords = 1;
    pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(mcsIndex,0);
    pdsch_pdu_rel15->qamModOrder[0] = 2;
    pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
    pdsch_pdu_rel15->mcsTable[0] = 0;
    pdsch_pdu_rel15->rvIndex[0] = 0;
    pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->nrOfLayers = 1;
    pdsch_pdu_rel15->transmissionScheme = 0;
    pdsch_pdu_rel15->refPoint = 0;
    pdsch_pdu_rel15->dmrsConfigType = 0;
    pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->SCID = 0;
    pdsch_pdu_rel15->numDmrsCdmGrpsNoData = 1;
    pdsch_pdu_rel15->dmrsPorts = 1;
    pdsch_pdu_rel15->resourceAlloc = 1;
    pdsch_pdu_rel15->rbStart = 0;
    pdsch_pdu_rel15->rbSize = 6;
    pdsch_pdu_rel15->VRBtoPRBMapping = 1;

    for (int i=0; i<scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count; i++) {
      startSymbolAndLength = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
      SLIV2SL(startSymbolAndLength, &StartSymbolIndex_tmp, &NrOfSymbols_tmp);
      if (NrOfSymbols_tmp < NrOfSymbols) {
        NrOfSymbols = NrOfSymbols_tmp;
        StartSymbolIndex = StartSymbolIndex_tmp;
      }
    }

    AssertFatal(StartSymbolIndex >= 0, "StartSymbolIndex is negative\n");
    pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
    pdsch_pdu_rel15->NrOfSymbols      = NrOfSymbols;
    pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(NULL, scc->dmrs_TypeA_Position, NrOfSymbols);

    dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
    dci_pdu_rel15[0].frequency_domain_assignment = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize,
      pdsch_pdu_rel15->rbStart,
      NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth, 275));
    dci_pdu_rel15[0].time_domain_assignment = time_domain_assignment;
    dci_pdu_rel15[0].vrb_to_prb_mapping = 1;
    dci_pdu_rel15[0].mcs = pdsch_pdu_rel15->mcsIndex[0];
    dci_pdu_rel15[0].tb_scaling = 1;
    dci_pdu_rel15[0].frequency_hopping_flag = 0;
    dci_pdu_rel15[0].ra_preamble_index = ra->preamble_index;
    dci_pdu_rel15[0].format_indicator = 1;
    dci_pdu_rel15[0].ndi = 1;
    dci_pdu_rel15[0].rv = 0;
    dci_pdu_rel15[0].harq_pid = 0;
    dci_pdu_rel15[0].dai = 2;
    dci_pdu_rel15[0].tpc = 2;
    dci_pdu_rel15[0].pucch_resource_indicator = 7;
    dci_pdu_rel15[0].pdsch_to_harq_feedback_timing_indicator = 7;

    LOG_D(MAC, "[RAPROC] DCI type 1 payload: freq_alloc %d (%d,%d,%d), time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d ndi %d rv %d\n",
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

    nr_configure_pdcch(pdcch_pdu_rel15, 0, scc, bwp);

    LOG_D(MAC, "Frame %d: Subframe %d : Adding common DL DCI for RA_RNTI %x CCEIndex %d\n", frameP, slotP, RA_rnti, CCEIndex);
      dci_pdu->RNTI[numDlDci] = RA_rnti;
      dci_pdu->AggregationLevel[numDlDci] = aggregation;
      dci_pdu->powerControlOffsetSS[numDlDci] = 0;
      dci_pdu->CceIndex[numDlDci] = CCEIndex;
      dci_pdu->beta_PDCCH_1_0[numDlDci] = 0;
      dci_pdu->powerControlOffsetSS[numDlDci] = 1;

    dci_formats[0] = NR_DL_DCI_FORMAT_1_0;
    rnti_types[0] = NR_RNTI_RA;

    LOG_D(MAC, "[RAPROC] DCI params: rnti %d, rnti_type %d, dci_format %d coreset params: FreqDomainResource %llx, start_symbol %d  n_symb %d\n",
      pdcch_pdu_rel15->dci_pdu.RNTI[0],
      rnti_types[0],
      dci_formats[0],
      (unsigned long long)pdcch_pdu_rel15->FreqDomainResource,
      pdcch_pdu_rel15->StartSymbolIndex,
      pdcch_pdu_rel15->DurationSymbols);

    dci_pdu->PayloadSizeBits[0] = nr_dci_size(dci_formats[0], rnti_types[0], pdcch_pdu_rel15->BWPSize);
    fill_dci_pdu_rel15(pdcch_pdu_rel15, &dci_pdu_rel15[0], dci_formats, rnti_types);
    pdcch_pdu_rel15->numDlDci++;
    dl_req->nPDUs++;

    // Program UL processing for Msg3
    // nr_get_Msg3alloc(&cc[CC_id], slotP, frameP,&ra->Msg3_frame, &ra->Msg3_slot); // todo
    LOG_D(MAC, "Frame %d, Subframe %d: Setting Msg3 reception for Frame %d Subframe %d\n", frameP, slotP, ra->Msg3_frame, ra->Msg3_slot);
    nr_fill_rar(ra, cc[CC_id].RAR_pdu.payload, N_RB_UL);
    nr_add_msg3(module_idP, CC_id, frameP, slotP);
    ra->state = WAIT_Msg3;
    LOG_D(MAC,"[gNB %d][RAPROC] Frame %d, Subframe %d: RA state %d\n", module_idP, frameP, slotP, ra->state);

    x_Overhead = 0;
    nr_get_tbs_dl(&dl_tti_pdsch_pdu->pdsch_pdu, x_Overhead);

    // DL TX request
    tx_req->PDU_length = pdsch_pdu_rel15->TBSize[0];
    tx_req->PDU_index = nr_mac->pdu_index[CC_id]++;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length = 8;
    nr_mac->TX_req[CC_id].SFN = frameP;
    nr_mac->TX_req[CC_id].Number_of_PDUs++;
    nr_mac->TX_req[CC_id].Slot = slotP;
    memcpy((void*)&tx_req->TLVs[0].value.direct[0], (void*)&cc[CC_id].RAR_pdu.payload[0], tx_req->TLVs[0].length);
  }
}

void nr_clear_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP){
  NR_RA_t *ra = &RC.nrmac[module_idP]->common_channels[CC_id].ra[0];
  LOG_D(MAC,"[gNB %d][RAPROC] CC_id %d Frame %d Clear Random access information rnti %x\n", module_idP, CC_id, frameP, ra->rnti);
  ra->state = IDLE;
  ra->timing_offset = 0;
  ra->RRC_timer = 20;
  ra->rnti = 0;
  ra->msg3_round = 0;
}

// WIP
// todo:
// - handle MAC RAR BI subheader
// - sending only 1 RAR subPDU
// - UL Grant
// - padding
void nr_fill_rar(NR_RA_t * ra,
                 uint8_t * dlsch_buffer,
                 uint16_t N_RB_UL){

    LOG_D(MAC, "[gNB] Generate RAR MAC PDU frame %d slot %d ", ra->Msg2_frame, ra-> Msg2_slot);
    NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) dlsch_buffer;
    NR_MAC_RAR *rar = (NR_MAC_RAR *) (dlsch_buffer + 1);

    // E/T/RAPID subheader
    // E = 0, one only RAR, first and last
    // T = 1, RAPID
    rarh->E = 0;
    rarh->T = 1;
    rarh->RAPID = ra->preamble_index;

    // RAR MAC payload
    rar->R = 0;
    rar->TA1 = (uint8_t) (ra->timing_offset >> 5);    // 7 MSBs of timing advance
    rar->TA2 = (uint8_t) (ra->timing_offset & 0x1f);  // 5 LSBs of timing advance
    rar->TCRNTI_1 = (uint8_t) (ra->rnti >> 8);        // 8 MSBs of rnti
    rar->TCRNTI_2 = (uint8_t) (ra->rnti & 0xff);      // 8 LSBs of rnti

    // uint16_t rballoc = nr_mac_compute_RIV(N_RB_UL, ra->msg3_first_rb, ra->msg3_nb_rb);  // first PRB only for UL Grant
    // uint32_t buffer = 0;
    // rar->UL_GRANT_1 = (uint8_t) (buffer >> 24) & 0x7;
    // rar->UL_GRANT_2 = (uint8_t) (buffer >> 16) & 0xFF;
    // rar->UL_GRANT_3 = (uint8_t) (buffer >> 8) & 0xFF;
    // rar->UL_GRANT_4 = (uint8_t) buffer & 0xff;

}

