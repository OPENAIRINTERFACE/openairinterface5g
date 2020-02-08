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
#include "UTIL/OPT/opt.h"

extern RAN_CONTEXT_t RC;

void nr_add_subframe(uint16_t *frameP, uint16_t *slotP, int offset){
    *frameP = (*frameP + ((*slotP + offset) / 10)) % 1024;
    *slotP = ((*slotP + offset) % 10);
}

// TBR
// handles the event of msg1 reception
// todo:
// - offset computation
// - fix nr_add_subframe
void nr_initiate_ra_proc(module_id_t module_idP,
                         int CC_id,
                         frame_t frameP,
                         sub_frame_t slotP,
                         uint16_t preamble_index,
                         int16_t timing_offset,
                         uint16_t ra_rnti){
  uint8_t i;
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
    //   switch(cc->tdd_Config->subframeAssignment){ // TBR missing tdd_Config
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
    ra->Msg2_subframe = msg2_slot;

    LOG_D(MAC, "%s() Msg2[%04d%d] SFN/SF:%04d%d offset:%d\n", __FUNCTION__, ra->Msg2_frame, ra->Msg2_subframe, frameP, slotP, offset);

    ra->Msg2_subframe = (slotP + offset) % 10; // TBR this is done twice ?

    do {
      ra->rnti = taus(); // todo 5.1.3 TS 38.321
      loop++;
    }
    // Range coming from 5.1.3 TS 38.321
    while (loop != 100 && !(find_nr_UE_id(module_idP, ra->rnti) == -1 && ra->rnti >= 1 && ra->rnti <= 17920));
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
      ra->Msg2_subframe,
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

  gNB_MAC_INST *mac = RC.mac[module_idP];
  NR_COMMON_channels_t *cc = mac->common_channels;
  NR_RA_t *ra;
  uint8_t i, CC_id = 0;

  start_meas(&mac->schedule_ra);

  for (i = 0; i < NR_NB_RA_PROC_MAX; i++) {
    ra = (NR_RA_t *) & cc[CC_id].ra[i];
    LOG_D(MAC,"RA[state:%d]\n",ra->state);
    switch (ra->state){
      case Msg2:
      nr_generate_Msg2(module_idP, CC_id, frameP, slotP, ra);
      break;
      case Msg4:
      //generate_Msg4(module_idP, CC_id, frameP, slotP, ra); // TBR
      break;
      case WAIT_Msg4_ACK:
      check_Msg4_retransmission(module_idP, CC_id, frameP, slotP, ra);
      break;
    }
  }
  stop_meas(&mac->schedule_ra);
}

void nr_generate_Msg2(module_id_t module_idP, int CC_id, frame_t frameP, 
  sub_frame_t slotP, NR_RA_t * ra){

  int first_rb, N_RB_DL;
  gNB_MAC_INST *mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &mac->common_channels[CC_id];
  uint8_t *vrb_map = cc[CC_id].vrb_map;

//  /* TBR - MIGRATE TO THE NEW NFAPI */
//  nfapi_nr_dl_tti_request_body_t *dl_req = &mac->DL_req[CC_id].dl_config_request_body;
//  nfapi_nr_dl_tti_request_pdu_t *dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
//  nfapi_tx_request_pdu_t *TX_req;
//
//  /* TBR: BW should be retrieved from
//  // NR_ServingCellConfigCommonSIB_t -> NR_BWP_DownlinkCommon_t -> NR_BWP_t
//  N_RB_DL = to_prb(cc[CC_id].mib->message.dl_Bandwidth);
//  // Temporary hardcoded
//  */
//  N_RB_DL = 106;
//
//  if ((ra->Msg2_frame == frameP) && (ra->Msg2_subframe == slotP)) {
//    
//    LOG_D(MAC,"[gNB %d] CC_id %d Frame %d, slotP %d: 
//      Generating RAR DCI, state %d\n",
//      module_idP, CC_id, frameP, slotP, ra->state);
//
//    // Allocate 4 PRBS starting in RB 0
//    // commented out because preprocessor is missing
//    /*first_rb = 0;
//    vrb_map[first_rb] = 1;
//    vrb_map[first_rb + 1] = 1;
//    vrb_map[first_rb + 2] = 1;
//    vrb_map[first_rb + 3] = 1;*/
//
//    memset((void *) dl_config_pdu, 0, sizeof(nfapi_nr_dl_config_request_pdu_t));
//
//    dl_config_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE;
//    dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format = NFAPI_DL_DCI_FORMAT_1A;
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level = 4;
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti = ra->RA_rnti;
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type = 2;  // RA-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power = 6000;  // equal to RS power
//
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process = 0;
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc = 1;  // no TPC
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1 = 1;
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1 = 0;
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1 = 0;
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = 0;
//
//    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = getRIV(N_RB_DL, first_rb, 4);
//
//    // This checks if the above DCI allocation is feasible in current subframe
//    if (!nr_CCE_allocation_infeasible(module_idP, CC_id, 0, slotP,
//      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level, ra->RA_rnti)) {
//      
//      LOG_D(MAC, "Frame %d: Subframe %d : Adding common DCI for RA_RNTI %x\n", frameP, slotP, ra->RA_rnti);
//      dl_req->number_dci++;
//      dl_req->number_pdu++;
//
//      dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
//      memset((void *) dl_config_pdu, 0, sizeof(nfapi_nr_dl_config_request_pdu_t));
//      dl_config_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE;
//      dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_nr_dl_config_dlsch_pdu));
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.tl.tag = NFAPI_NR_DL_CONFIG_REQUEST_DLSCH_PDU_REL15_TAG;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index = mac->pdu_index[CC_id];
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti = ra->RA_rnti;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type = 2; // format 1A/1B/1D
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0; // localized
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding = getRIV(N_RB_DL, first_rb, 4);
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation = 2; //QPSK
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version = 0;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks = 1; // first block
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag  = 0;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme = (cc->p_eNB == 1) ? 0 : 1;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers = 1;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands = 1;
//      //    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index = ;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity = 1;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa = 4; // 0 dB
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index = 0;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap = 0;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb = get_subbandsize(cc->mib->message.dl_Bandwidth);  // ignored
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode = (cc->p_eNB == 1) ? 1 : 2;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband = 1;
//      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector = 1;
//      //    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ; 
//      dl_req->number_pdu++;
//      mac->DL_req[CC_id].sfn_sf = frameP<<4 | slotP;
//
//      // Program UL processing for Msg3
//      nr_get_Msg3alloc(&cc[CC_id], slotP, frameP,&ra->Msg3_frame, &ra->Msg3_subframe);
//
//      LOG_D(MAC, "Frame %d, Subframe %d: Setting Msg3 reception for Frame %d Subframe %d\n",
//            frameP, slotP, ra->Msg3_frame,
//            ra->Msg3_subframe);
//
//      nr_fill_rar(module_idP, CC_id, ra, frameP, cc[CC_id].RAR_pdu.payload, N_RB_DL, 7);
//      nr_add_msg3(module_idP, CC_id, ra, frameP, slotP);
//      ra->state = WAITMSG3;
//      LOG_D(MAC,"[gNB %d][RAPROC] Frame %d, Subframe %d: state:WAITMSG3\n", module_idP, frameP, slotP);
//
//      // DL request
//      mac->TX_req[CC_id].sfn_sf = (frameP << 4) + slotP;
//      TX_req = &mac->TX_req[CC_id].tx_request_body.tx_pdu_list[mac->TX_req[CC_id].tx_request_body.number_of_pdus];
//      TX_req->pdu_length = 7; // This should be changed if we have more than 1 preamble 
//      TX_req->pdu_index = mac->pdu_index[CC_id]++;
//      TX_req->num_segments = 1;
//      TX_req->segments[0].segment_length = 7;
//      TX_req->segments[0].segment_data = cc[CC_id].RAR_pdu.payload;
//      mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
//    
//      /*if(RC.mac[module_idP]->scheduler_mode == SCHED_MODE_FAIR_RR){
//        set_dl_ue_select_msg2(CC_id, 4, -1, ra->rnti);
//      }*/  
//    } // PDCCH CCE allocation is feasible
//  } // Msg2 frame/subframe condition
}

void nr_clear_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP){
  unsigned char i;
  NR_RA_t *ra = (NR_RA_t *) &RC.mac[module_idP]->common_channels[CC_id].ra[0];
  LOG_D(MAC,"[gNB %d][RAPROC] CC_id %d Frame %d Clear Random access information rnti %x\n", module_idP, CC_id, frameP, ra->rnti);
  ra->state = IDLE;
  ra->timing_offset = 0;
  ra->RRC_timer = 20;
  ra->rnti = 0;
  ra->msg3_round = 0;
}

unsigned short nr_fill_rar(const module_id_t mod_id,
                           const int CC_id,
                           NR_RA_t * ra,
                           const frame_t frameP,
                           uint8_t * const dlsch_buffer,
                           const uint16_t N_RB_UL,
                           const uint8_t input_buffer_length){

    RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) dlsch_buffer;
    uint8_t *rar = (uint8_t *) (dlsch_buffer + 1);

    /* E/T/RAPID subheader 
    - E = 0, one only RAR, first and last
    - T = 1, E/T/RAPID subheader  */
    rarh->E = 0;
    rarh->T = 1;
    rarh->RAPID = ra->preamble_index; // TBR Respond to Preamble 0 only for the moment
    /* TBR handle MAC RAR BI subheader*/

    /* TC-RNTI */
    // TBR double check
    rar[5] = (uint8_t) (ra->rnti >> 8);
    rar[6] = (uint8_t) (ra->rnti & 0xff);
    // TBR double check

    /* Timing Advance Command */
    // TBR double check
    //ra->timing_offset = 0;
    //printf("ra->timing_offset %d\n", ra->timing_offset); // TBR remove

    ra->timing_offset /= 16;    //T_A = N_TA/16, where N_TA should be on a 30.72Msps
    rar[0] = (uint8_t) (ra->timing_offset >> (2 + 4));  // 7 MSBs of timing advance + divide by 4
    rar[1] = (uint8_t) (ra->timing_offset << (4 - 2)) & 0xf0;   // 4 LSBs of timing advance + divide by 4
    // TBR double check

    // TBR double check
    COMMON_channels_t *cc = &RC.mac[mod_id]->common_channels[CC_id];
    if(N_RB_UL == 25){
      ra->msg3_first_rb = 1;
    }else{
      // if (cc->tdd_Config && N_RB_UL == 100) {  // TBR missing tdd_Config
      //   ra->msg3_first_rb = 3;
      // } else {
      //   ra->msg3_first_rb = 2;
      // }
    }
    ra->msg3_nb_rb = 1;
    // TBR double check

    /* UL Grant */
    // TBR fix this
    uint16_t rballoc = nr_mac_compute_RIV(N_RB_UL, ra->msg3_first_rb, ra->msg3_nb_rb);  // first PRB only for UL Grant
    rar[1] |= (rballoc >> 7) & 7;   // Hopping = 0 (bit 3), 3 MSBs of rballoc
    rar[2] = ((uint8_t) (rballoc & 0xff)) << 1; // 7 LSBs of rballoc
    ra->msg3_mcs = 10;
    ra->msg3_TPC = 3;
    ra->msg3_ULdelay = 0;
    ra->msg3_cqireq = 0;
    ra->msg3_round = 0;
    rar[2] |= ((ra->msg3_mcs & 0x8) >> 3);  // mcs 10
    rar[3] = (((ra->msg3_mcs & 0x7) << 5)) | ((ra->msg3_TPC & 7) << 2) | ((ra->msg3_ULdelay & 1) << 1) | (ra->msg3_cqireq & 1);
    // TBR double check

    return ra->rnti;
}

void nr_add_msg3(module_id_t module_idP, int CC_id, NR_RA_t *ra, frame_t frameP, sub_frame_t subframeP){

  #if 0
  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc = &mac->common_channels[CC_id];
  uint8_t j;
  nfapi_ul_config_request_t *ul_req;
  nfapi_ul_config_request_body_t *ul_req_body;
  nfapi_ul_config_request_pdu_t *ul_config_pdu;
  nfapi_hi_dci0_request_t        *hi_dci0_req;
  nfapi_hi_dci0_request_body_t   *hi_dci0_req_body;
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu;
  uint8_t sf_ahead_dl;
  uint8_t rvseq[4] = {0, 2, 3, 1};
  ul_req = &mac->UL_req_tmp[CC_id][ra->Msg3_subframe];
  ul_req_body = &ul_req->ul_config_request_body;
  AssertFatal(ra->state != IDLE, "RA is not active for RA %X\n",
              ra->rnti);
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

  if (ra->rach_resource_type > 0) {
    LOG_D (MAC, "[eNB %d][RAPROC] Frame %d, Subframe %d : CC_id %d CE level %d is active, Msg3 in (%d,%d)\n",
           module_idP, frameP, subframeP, CC_id, ra->rach_resource_type - 1, ra->Msg3_frame, ra->Msg3_subframe);
    LOG_D (MAC, "Frame %d, Subframe %d Adding Msg3 UL Config Request for (%d,%d) : (%d,%d)\n",
           frameP, subframeP, ra->Msg3_frame, ra->Msg3_subframe, ra->msg3_nb_rb, ra->msg3_round);
    ul_config_pdu = &ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus];
    memset ((void *) ul_config_pdu, 0, sizeof (nfapi_ul_config_request_pdu_t));
    ul_config_pdu->pdu_type = NFAPI_UL_CONFIG_ULSCH_PDU_TYPE;
    ul_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_ul_config_ulsch_pdu));
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.handle = mac->ul_handle++;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti = ra->rnti;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.resource_block_start = narrowband_to_first_rb (cc, ra->msg34_narrowband) + ra->msg3_first_rb;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.number_of_resource_blocks = ra->msg3_nb_rb;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.modulation_type = 2;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.cyclic_shift_2_for_drms = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_enabled_flag = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_bits = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.new_data_indication = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.redundancy_version = rvseq[ra->msg3_round];
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.harq_process_number = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.ul_tx_mode = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.current_tx_nb = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.n_srs = 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.size = get_TBS_UL (ra->msg3_mcs, ra->msg3_nb_rb);
    // Re13 fields
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.ue_type = ra->rach_resource_type > 2 ? 2 : 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.total_number_of_repetitions = 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.repetition_number = 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.initial_transmission_sf_io = (ra->Msg3_frame * 10) + ra->Msg3_subframe;
    ul_req_body->number_of_pdus++;
  }                             //  if (ra->rach_resource_type>0) {
  else
#endif
  {
    LOG_D(MAC,
          "[eNB %d][RAPROC] Frame %d, Subframe %d : CC_id %d RA is active, Msg3 in (%d,%d)\n",
          module_idP, frameP, subframeP, CC_id, ra->Msg3_frame,
          ra->Msg3_subframe);
    LOG_D(MAC,
          "Frame %d, Subframe %d Adding Msg3 UL Config Request for (%d,%d) : (%d,%d,%d) for rnti: %d\n",
          frameP, subframeP, ra->Msg3_frame, ra->Msg3_subframe,
          ra->msg3_nb_rb, ra->msg3_first_rb, ra->msg3_round, ra->rnti);
    ul_config_pdu = &ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus];
    memset((void *) ul_config_pdu, 0, sizeof(nfapi_ul_config_request_pdu_t));
    ul_config_pdu->pdu_type                                                = NFAPI_UL_CONFIG_ULSCH_PDU_TYPE;
    ul_config_pdu->pdu_size                                                = (uint8_t) (2 + sizeof(nfapi_ul_config_ulsch_pdu));
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.tl.tag                         = NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL8_TAG;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.handle                         = mac->ul_handle++;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.rnti                           = ra->rnti;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.resource_block_start           = ra->msg3_first_rb;
    AssertFatal(ra->msg3_nb_rb > 0, "nb_rb = 0\n");
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.number_of_resource_blocks      = ra->msg3_nb_rb;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.modulation_type                = 2;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.cyclic_shift_2_for_drms        = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_enabled_flag = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_bits         = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.new_data_indication            = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.redundancy_version             = rvseq[ra->msg3_round];
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.harq_process_number            = subframe2harqpid(cc, ra->Msg3_frame, ra->Msg3_subframe);
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.ul_tx_mode                     = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.current_tx_nb                  = 0;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.n_srs                          = 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel8.size                           = get_TBS_UL(10, ra->msg3_nb_rb);
    ul_req_body->number_of_pdus++;
    ul_req_body->tl.tag                                                    = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
    ul_req->sfn_sf                                                         = ra->Msg3_frame<<4|ra->Msg3_subframe;
    ul_req->header.message_id                                              = NFAPI_UL_CONFIG_REQUEST;

    // save UL scheduling information for preprocessor
    for (j = 0; j < ra->msg3_nb_rb; j++)
      cc->vrb_map_UL[ra->msg3_first_rb + j] = 1;

    LOG_D(MAC, "MSG3: UL_CONFIG SFN/SF:%d number_of_pdus:%d ra->msg3_round:%d\n", NFAPI_SFNSF2DEC(ul_req->sfn_sf), ul_req_body->number_of_pdus, ra->msg3_round);

    if (ra->msg3_round != 0) {  // program HI too
      sf_ahead_dl = ul_subframe2_k_phich(cc, subframeP);
      hi_dci0_req = &mac->HI_DCI0_req[CC_id][(subframeP+sf_ahead_dl)%10];
      hi_dci0_req_body = &hi_dci0_req->hi_dci0_request_body;
      hi_dci0_pdu = &hi_dci0_req_body->hi_dci0_pdu_list[hi_dci0_req_body->number_of_dci + hi_dci0_req_body->number_of_hi];
      memset((void *) hi_dci0_pdu, 0,
             sizeof(nfapi_hi_dci0_request_pdu_t));
      hi_dci0_pdu->pdu_type                                   = NFAPI_HI_DCI0_HI_PDU_TYPE;
      hi_dci0_pdu->pdu_size                                   = 2 + sizeof(nfapi_hi_dci0_hi_pdu);
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.tl.tag                  = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.resource_block_start    = ra->msg3_first_rb;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms = 0;
      hi_dci0_pdu->hi_pdu.hi_pdu_rel8.hi_value                = 0;
      hi_dci0_req_body->number_of_hi++;
      hi_dci0_req_body->sfnsf                                 = sfnsf_add_subframe(ra->Msg3_frame, ra->Msg3_subframe, 0);
      hi_dci0_req_body->tl.tag                                = NFAPI_HI_DCI0_REQUEST_BODY_TAG;
      hi_dci0_req->sfn_sf                                     = sfnsf_add_subframe(frameP, subframeP, sf_ahead_dl);
      hi_dci0_req->header.message_id                          = NFAPI_HI_DCI0_REQUEST;

      if (NFAPI_MODE != NFAPI_MONOLITHIC) {
        oai_nfapi_hi_dci0_req(hi_dci0_req);
        hi_dci0_req_body->number_of_hi=0;
      }

      LOG_D(MAC, "MSG3: HI_DCI0 SFN/SF:%d number_of_dci:%d number_of_hi:%d\n", NFAPI_SFNSF2DEC(hi_dci0_req->sfn_sf), hi_dci0_req_body->number_of_dci, hi_dci0_req_body->number_of_hi);

      // save UL scheduling information for preprocessor
      for (j = 0; j < ra->msg3_nb_rb; j++)
        cc->vrb_map_UL[ra->msg3_first_rb + j] = 1;

      LOG_D(MAC,
            "[eNB %d][PUSCH-RA %x] CC_id %d Frame %d subframeP %d Scheduled (PHICH) RA (mcs %d, first rb %d, nb_rb %d,round %d)\n",
            module_idP, ra->rnti, CC_id, frameP, subframeP, 10, 1, 1,
            ra->msg3_round - 1);
    }     //       if (ra->msg3_round != 0) { // program HI too
  }       // non-BL/CE UE case
  #endif
}