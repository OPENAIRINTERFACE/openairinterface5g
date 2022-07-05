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

/*! \file eNB_scheduler_RA.c
 * \brief primitives used for random access
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

/* indented with: indent -kr eNB_scheduler_RA.c */


#include "assertions.h"
#include "platform_types.h"

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_extern.h"

#include "LAYER2/MAC/mac_proto.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

#include "SCHED/sched_common.h"
//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#include "intertask_interface.h"

#include "SIMULATION/TOOLS/sim.h" // for taus

#include "T.h"
#include "executables/softmodem-common.h"
#include "common/ran_context.h"
#include "LAYER2/MAC/eNB_scheduler_fairRR.h"

extern RAN_CONTEXT_t RC;

extern const uint8_t pusch_repetition_Table8_2_36213[3][4];

extern int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req);

void add_subframe(uint16_t *frameP, uint16_t *subframeP, int offset) {
  *frameP    = (*frameP + ((*subframeP + offset) / 10)) % 1024;
  *subframeP = ((*subframeP + offset) % 10);
}

uint16_t sfnsf_add_subframe(uint16_t frameP, uint16_t subframeP, int offset) {
  add_subframe(&frameP, &subframeP, offset);
  return frameP<<4|subframeP;
}

void subtract_subframe(uint16_t *frameP, uint16_t *subframeP, int offset) {
  if (*subframeP < offset) {
    *frameP = (*frameP+1024-1)%1024;
  }

  *subframeP = (*subframeP+10-offset)%10;
}

uint16_t sfnsf_subtract_subframe(uint16_t frameP, uint16_t subframeP, int offset) {
  subtract_subframe(&frameP, &subframeP, offset);
  return frameP<<4|subframeP;
}

void
add_msg3(module_id_t module_idP, int CC_id, RA_t *ra, frame_t frameP,
         sub_frame_t subframeP) {
  eNB_MAC_INST *mac = RC.mac[module_idP];
  eNB_RRC_INST *rrc = RC.rrc[module_idP];
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
  uint8_t pusch_maxNumRepetitionCEmodeA_r13;
  ul_req = &mac->UL_req_tmp[CC_id][ra->Msg3_subframe];
  ul_req_body = &ul_req->ul_config_request_body;
  AssertFatal(ra->state != IDLE, "RA is not active for RA %X\n",
              ra->rnti);

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

    if (ra->rach_resource_type > 0) {
      pusch_maxNumRepetitionCEmodeA_r13= *(rrc->configuration.pusch_maxNumRepetitionCEmodeA_r13[CC_id]);
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.total_number_of_repetitions= pusch_repetition_Table8_2_36213[pusch_maxNumRepetitionCEmodeA_r13][ra->pusch_repetition_levels];
    } else {
      ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.total_number_of_repetitions=1;
    }

    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.repetition_number = 1;
    ul_config_pdu->ulsch_pdu.ulsch_pdu_rel13.initial_transmission_sf_io = (ra->Msg3_frame * 10) + ra->Msg3_subframe;
    ul_req_body->number_of_pdus++;
  }                             //  if (ra->rach_resource_type>0) {
  else {
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

      // save UL scheduling information for preprocessor
      for (j = 0; j < ra->msg3_nb_rb; j++)
        cc->vrb_map_UL[ra->msg3_first_rb + j] = 1;

      LOG_D(MAC,
            "[eNB %d][PUSCH-RA %x] CC_id %d Frame %d subframeP %d Scheduled (PHICH) RA (mcs %d, first rb %d, nb_rb %d,round %d)\n",
            module_idP, ra->rnti, CC_id, frameP, subframeP, 10, 1, 1,
            ra->msg3_round - 1);
    }     //       if (ra->msg3_round != 0) { // program HI too
  }       // non-BL/CE UE case
}

//------------------------------------------------------------------------------
/*
 * Generate the RAR (message2)
 */
void generate_Msg2(module_id_t module_idP,
                   int CC_idP,
                   frame_t frameP,
                   sub_frame_t subframeP,
                   RA_t *ra)
//------------------------------------------------------------------------------
{
  eNB_MAC_INST *mac = RC.mac[module_idP];
  eNB_RRC_INST *rrc = RC.rrc[module_idP];
  COMMON_channels_t *cc = mac->common_channels;
  uint8_t *vrb_map = NULL;
  int first_rb = 0;
  int N_RB_DL = 0;
  nfapi_dl_config_request_pdu_t *dl_config_pdu = NULL;
  nfapi_tx_request_pdu_t *TX_req = NULL;
  nfapi_dl_config_request_body_t *dl_req_body = NULL;
  vrb_map = cc[CC_idP].vrb_map;
  dl_req_body = &mac->DL_req[CC_idP].dl_config_request_body;
  dl_config_pdu = &dl_req_body->dl_config_pdu_list[dl_req_body->number_pdu];
  N_RB_DL = to_prb(cc[CC_idP].mib->message.dl_Bandwidth);
  uint8_t PUSCH_Rep_Level;
  int             rmax = 0;
  int             rep = 0;
  int             reps = 0;
  int             num_nb = 0;
  first_rb = 0;
  struct LTE_PRACH_ConfigSIB_v1310 *ext4_prach = NULL;
  LTE_PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13 = NULL;
  LTE_PRACH_ParametersCE_r13_t *p[4] = { NULL, NULL, NULL, NULL };
  uint16_t        absSF = (10 * frameP) + subframeP;
  uint16_t        absSF_Msg2 = (10 * ra->Msg2_frame) + ra->Msg2_subframe;

  if (ra->rach_resource_type > 0) {
    PUSCH_Rep_Level= *(rrc->configuration.pusch_repetitionLevelCEmodeA_r13[CC_idP]);
  } else {
    PUSCH_Rep_Level= 0;
  }

  if (absSF > absSF_Msg2) {
    return; // we're not ready yet
  }

  if (cc[CC_idP].mib->message.schedulingInfoSIB1_BR_r13 > 0 && cc[CC_idP].radioResourceConfigCommon_BR) {
    ext4_prach = cc[CC_idP].radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
    prach_ParametersListCE_r13 = &ext4_prach->prach_ParametersListCE_r13;

    switch (prach_ParametersListCE_r13->list.count) {
      case 4:
        p[3] = prach_ParametersListCE_r13->list.array[3];

      case 3:
        p[2] = prach_ParametersListCE_r13->list.array[2];

      case 2:
        p[1] = prach_ParametersListCE_r13->list.array[1];

      case 1:
        p[0] = prach_ParametersListCE_r13->list.array[0];
        break;

      default:
        AssertFatal (1 == 0, "Illegal count for prach_ParametersListCE_r13 %d\n", (int) prach_ParametersListCE_r13->list.count);
        break;
    }
  }

  if (ra->rach_resource_type > 0) {
    /* This uses an MPDCCH Type 2 common allocation according to Section 9.1.5 36-213
     * Parameters:
     *    p = 2 + 4 PRB set (number of PRB pairs 3)
     *    rmax = mpdcch-NumRepetition-RA-r13 => Table 9.1.5-3
     *    if CELevel = 0,1 => Table 9.1.5-1b for MPDCCH candidates
     *    if CELevel = 2,3 => Table 9.1.5-2b for MPDCCH candidates
     *    distributed transmission
     */
    /* rmax from SIB2 information */
    AssertFatal (rmax < 9, "rmax > 8!\n"); // not sure of this assertion
    rmax = 1 << p[ra->rach_resource_type - 1]->mpdcch_NumRepetition_RA_r13;
    /* Choose r1 by default for RAR (Table 9.1.5-5) */
    rep = 0;
    /* Get actual repetition count from Table 9.1.5-3 */
    reps = (rmax <= 8) ? (1 << rep) : (rmax >> (3 - rep));
    /* Get narrowband according to higher-layer config */
    num_nb = p[ra->rach_resource_type - 1]->mpdcch_NarrowbandsToMonitor_r13.list.count;
    ra->msg2_narrowband = *p[ra->rach_resource_type - 1]->mpdcch_NarrowbandsToMonitor_r13.list.array[ra->preamble_index % num_nb] - 1;
    first_rb = narrowband_to_first_rb(&cc[CC_idP], ra->msg2_narrowband);

    if ((ra->msg2_mpdcch_repetition_cnt == 0) && (mpdcch_sf_condition(mac, CC_idP, frameP, subframeP, rmax, TYPE2, -1) > 0)) {
      ra->msg2_mpdcch_done = 0;
      /* MPDCCH configuration for RAR */
      LOG_D(MAC, "[eNB %d][RAPROC] Frame %d, Subframe %d : In generate_Msg2 for CE Level %d, Programming MPDCCH %d repetitions\n",
            module_idP,
            frameP,
            subframeP,
            ra->rach_resource_type - 1,
            reps);
      memset ((void *) dl_config_pdu, 0, sizeof (nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE;
      dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_dl_config_mpdcch_pdu));
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_format = (ra->rach_resource_type > 1) ? 11 : 10;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_narrow_band = ra->msg2_narrowband;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_prb_pairs = 6;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_assignment = 0; // Note: this can be dynamic
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_tansmission_type = 1;   // imposed (9.1.5 in 213) for Type 2 Common search space
      AssertFatal (cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13 != NULL, "cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13 is null\n");
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.start_symbol = cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
      LOG_E(MAC, "start_symbol = %d \n", dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.start_symbol);
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ecce_index = 0;        // Note: this should be dynamic
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.aggregation_level = 24;        // OK for CEModeA r1-3 (9.1.5-1b) or CEModeB r1-4
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti_type = 2; // RA-RNTI
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti = ra->RA_rnti;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ce_mode = (ra->rach_resource_type < 3) ? 1 : 2;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.drms_scrambling_init = cc[CC_idP].physCellId;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.initial_transmission_sf_io = (frameP * 10) + subframeP;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.transmission_power = 6000;     // 0dB
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_coding = getRIV(6, 0, 6) | (ra->msg2_narrowband<<5);
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mcs = 0;       // adjust according to size of RAR, 208 bits with N1A_PRB=3
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pdsch_reptition_levels = 0;    // fix to 4 for now
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.redundancy_version = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.new_data_indicator = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_process = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi_length = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_resource_offset = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_subframe_repetition_number = rep;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpc = 1;       // N1A_PRB=3 (36.212) => 56 bits
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index_length = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.allocate_prach_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.preamble_index = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.prach_mask_index = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.starting_ce_level = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.srs_request = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.frequency_hopping_enabled_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.paging_direct_indication_differentiation_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.direct_indication = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.total_dci_length_including_padding = 0;        // this is not needed by OAI L1, but should be filled in
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_tx_antenna_ports = 1;
      ra->msg2_mpdcch_repetition_cnt++;
      dl_req_body->number_pdu++;
      ra->Msg2_subframe = (ra->Msg2_subframe + 9) % 10;
    } // repetition_count == 0 && SF condition met

    if (ra->msg2_mpdcch_repetition_cnt > 0) {  // we're in a stream of repetitions
      if ((ra->msg2_mpdcch_repetition_cnt == reps) && (ra->msg2_mpdcch_done == 0)) {  // this is the last mpdcch repetition
        ra->msg2_mpdcch_done = 1;

        if (cc[CC_idP].tdd_Config == NULL) {    // FDD case
          // wait 2 subframes for PDSCH transmission
          if (subframeP > 7)
            ra->Msg2_frame = (frameP + 1) & 1023;
          else
            ra->Msg2_frame = frameP;

          ra->Msg2_subframe = (subframeP + 2) % 10;    // +2 is the "n+x" from Section 7.1.11  in 36.213
          LOG_D(MAC, "[eNB %d][RAPROC] Frame %d, Subframe %d : In generate_Msg2, programmed Msg2 for %d.%d\n",
                module_idP,
                frameP,
                subframeP,
                ra->Msg2_frame,
                ra->Msg2_subframe);
        } else {
          AssertFatal(1 == 0, "TDD case not done yet\n");
        }
      } else if (ra->msg2_mpdcch_done == 0) {  // mpdcch_repetition_count != reps
        LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d : In generate_Msg2, MPDCCH repetition %d\n",
              module_idP,
              frameP,
              subframeP,
              ra->msg2_mpdcch_repetition_cnt);
        ra->msg2_mpdcch_repetition_cnt++;
      }

      ra->pusch_repetition_levels = PUSCH_Rep_Level;

      if((ra->Msg2_frame == frameP) && (ra->Msg2_subframe == subframeP)) {
        /* Program PDSCH */
        LOG_D(MAC, "[eNB %d][RAPROC] Frame %d, Subframe %d : In generate_Msg2, Programming PDSCH\n",
              module_idP,
              frameP,
              subframeP);
        dl_config_pdu = &dl_req_body->dl_config_pdu_list[dl_req_body->number_pdu];
        memset ((void *) dl_config_pdu, 0, sizeof (nfapi_dl_config_request_pdu_t));
        dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
        dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_dl_config_dlsch_pdu));
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index = mac->pdu_index[CC_idP];
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti = ra->RA_rnti;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type = 2;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;     // localized
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding = getRIV(N_RB_DL, first_rb, 6);
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation = 2; // QPSK
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version = 0;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks = 1;   // first block
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag = 0;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme = (cc[CC_idP].p_eNB == 1) ? 0 : 1;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers = 1;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands = 1;
        //      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index                         = ;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity = 1;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa = 4; // 0 dB
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index = 0;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap = 0;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb = get_subbandsize(cc[CC_idP].mib->message.dl_Bandwidth); // ignored
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode = (cc[CC_idP].p_eNB == 1) ? 1 : 2;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband = 1;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector = 1;
        //      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ;
        /* Rel10 fields */
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start = cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
        /* Rel13 fields */
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type = (ra->rach_resource_type < 3) ? 1 : 2;;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type = 2;        // not SI message
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io = (10 * frameP) + subframeP;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.drms_table_flag = 0;
        dl_req_body->number_pdu++;
        fill_rar_br(mac, CC_idP, ra, frameP, subframeP, cc[CC_idP].RAR_pdu.payload, ra->rach_resource_type - 1);
        /* Program UL processing for Msg3, same as regular LTE */
        get_Msg3alloc (&cc[CC_idP], subframeP, frameP, &ra->Msg3_frame, &ra->Msg3_subframe);
        add_msg3 (module_idP, CC_idP, ra, frameP, subframeP);
        ra->state = WAITMSG3;
        /* DL request */
        LOG_D(MAC, "[eNB %d][RAPROC] Frame %d, Subframe %d : In generate_Msg2, Programming TX Req\n",
              module_idP,
              frameP,
              subframeP);
        mac->TX_req[CC_idP].sfn_sf = (frameP << 4) + subframeP;
        TX_req = &mac->TX_req[CC_idP].tx_request_body.tx_pdu_list[mac->TX_req[CC_idP].tx_request_body.number_of_pdus];
        TX_req->pdu_length = 7; // This should be changed if we have more than 1 preamble
        TX_req->pdu_index = mac->pdu_index[CC_idP]++;
        TX_req->num_segments = 1;
        TX_req->segments[0].segment_length = 7;
        TX_req->segments[0].segment_data = cc[CC_idP].RAR_pdu.payload;
        mac->TX_req[CC_idP].tx_request_body.number_of_pdus++;
      }
    }
  } else {
    if ((ra->Msg2_frame == frameP) && (ra->Msg2_subframe == subframeP)) {
      LOG_D(MAC,
            "[eNB %d] CC_id %d Frame %d, subframeP %d: Generating RAR DCI, state %d\n",
            module_idP, CC_idP, frameP, subframeP, ra->state);
      // Allocate 4 PRBS starting in RB 0
      first_rb = 0;
      vrb_map[first_rb] = 1;
      vrb_map[first_rb + 1] = 1;
      //vrb_map[first_rb + 2] = 1;
      //vrb_map[first_rb + 3] = 1;
      memset((void *) dl_config_pdu, 0, sizeof(nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
      dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format = NFAPI_DL_DCI_FORMAT_1A;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level = 4;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti = ra->RA_rnti;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type = 2;  // RA-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power = 6000;  // equal to RS power
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process = 0;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc = 0;  // no TPC
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1 = 1;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1 = 1;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1 = 0;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = 0;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = getRIV(N_RB_DL, first_rb, 2);

      // This checks if the above DCI allocation is feasible in current subframe
      if (!CCE_allocation_infeasible(module_idP, CC_idP, 0, subframeP,
                                     dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level, ra->RA_rnti)) {
        LOG_D(MAC,
              "Frame %d: Subframe %d : Adding common DCI for RA_RNTI %x\n",
              frameP, subframeP, ra->RA_rnti);
        dl_req_body->number_dci++;
        dl_req_body->number_pdu++;
        dl_config_pdu = &dl_req_body->dl_config_pdu_list[dl_req_body->number_pdu];
        memset((void *) dl_config_pdu, 0, sizeof(nfapi_dl_config_request_pdu_t));
        dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
        dl_config_pdu->pdu_size                                                        = (uint8_t) (2 + sizeof(nfapi_dl_config_dlsch_pdu));
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.tl.tag                                 = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = mac->pdu_index[CC_idP];
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = ra->RA_rnti;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 2; // format 1A/1B/1D
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0; // localized
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(N_RB_DL, first_rb, 2);
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation                             = 2; //QPSK
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version                     = 0;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks                       = 1; // first block
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag  = 0;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme                    = (cc->p_eNB == 1) ? 0 : 1;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers                       = 1;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands                     = 1;
        //    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index                         = ;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity                   = 1;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa                                     = 4; // 0 dB
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index               = 0;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap                                   = 0;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb                                   = get_subbandsize(cc->mib->message.dl_Bandwidth);  // ignored
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode                      = (cc->p_eNB == 1) ? 1 : 2;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband                 = 1;
        dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector                          = 1;
        //    dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ;
        dl_req_body->number_pdu++;
        mac->DL_req[CC_idP].sfn_sf = frameP<<4 | subframeP;
        // Program UL processing for Msg3
        get_Msg3alloc(&cc[CC_idP], subframeP, frameP,&ra->Msg3_frame, &ra->Msg3_subframe);
        LOG_D(MAC,
              "Frame %d, Subframe %d: Setting Msg3 reception for Frame %d Subframe %d\n",
              frameP, subframeP, ra->Msg3_frame,
              ra->Msg3_subframe);
        fill_rar(module_idP, CC_idP, ra, frameP, cc[CC_idP].RAR_pdu.payload, N_RB_DL, 7);
        add_msg3(module_idP, CC_idP, ra, frameP, subframeP);
        ra->state = WAITMSG3;
        LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d: state:WAITMSG3\n", module_idP, frameP, subframeP);
        T(T_ENB_MAC_UE_DL_RAR_PDU_WITH_DATA, T_INT(module_idP),
          T_INT(CC_idP), T_INT(ra->RA_rnti), T_INT(frameP),
          T_INT(subframeP), T_INT(0 /*harq_pid always 0? */ ),
          T_BUFFER(cc[CC_idP].RAR_pdu.payload, 7));
        // DL request
        mac->TX_req[CC_idP].sfn_sf = (frameP << 4) + subframeP;
        TX_req =
          &mac->TX_req[CC_idP].tx_request_body.tx_pdu_list[mac->TX_req[CC_idP].tx_request_body.number_of_pdus];
        TX_req->pdu_length = 7; // This should be changed if we have more than 1 preamble
        TX_req->pdu_index = mac->pdu_index[CC_idP]++;
        TX_req->num_segments = 1;
        TX_req->segments[0].segment_length = 7;
        TX_req->segments[0].segment_data =
          cc[CC_idP].RAR_pdu.payload;
        mac->TX_req[CC_idP].tx_request_body.number_of_pdus++;

        if(RC.mac[module_idP]->scheduler_mode == SCHED_MODE_FAIR_RR) {
          set_dl_ue_select_msg2(CC_idP, 4, -1, ra->rnti);
        }
      }     // PDCCH CCE allocation is feasible
    }     // Msg2 frame/subframe condition
  }       // else BL/CE
}

//------------------------------------------------------------------------------
/*
 * Generate message 4 of RA procedure (RRC connection setup)
 */
void
generate_Msg4(module_id_t module_idP,
              int CC_idP,
              frame_t frameP,
              sub_frame_t subframeP,
              RA_t *ra)
//------------------------------------------------------------------------------
{
  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc = mac->common_channels;
  UE_info_t *UE_info = &mac->UE_info;
  int16_t rrc_sdu_length = 0;
  uint16_t msg4_padding = 0;
  uint16_t msg4_post_padding = 0;
  uint16_t msg4_header = 0;
  int UE_id = -1;
  int first_rb = 0;
  int N_RB_DL = 0;
  uint8_t lcid = 0;
  uint8_t offset = 0;
  uint8_t *vrb_map = NULL;
  nfapi_dl_config_request_pdu_t   *dl_config_pdu = NULL;
  nfapi_ul_config_request_pdu_t   *ul_config_pdu = NULL;
  nfapi_tx_request_pdu_t          *TX_req = NULL;
  nfapi_dl_config_request_t      *dl_req = NULL;
  nfapi_dl_config_request_body_t *dl_req_body = NULL;
  nfapi_ul_config_request_body_t *ul_req_body = NULL;
  int             rmax = 0;
  int             rep = 0;
  int             reps = 0;
  first_rb = 0;
  struct LTE_PRACH_ConfigSIB_v1310 *ext4_prach = NULL;
  struct LTE_PUCCH_ConfigCommon_v1310 *ext4_pucch = NULL;
  LTE_PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13 = NULL;
  struct LTE_N1PUCCH_AN_InfoList_r13 *pucch_N1PUCCH_AN_InfoList_r13 = NULL;
  LTE_PRACH_ParametersCE_r13_t *p[4] = { NULL, NULL, NULL, NULL };
  int             pucchreps[4] = { 1, 1, 1, 1 };
  int             n1pucchan[4] = { 0, 0, 0, 0 };

  if (cc[CC_idP].mib->message.schedulingInfoSIB1_BR_r13 > 0 && cc[CC_idP].radioResourceConfigCommon_BR) {
    ext4_prach = cc[CC_idP].radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
    prach_ParametersListCE_r13 = &ext4_prach->prach_ParametersListCE_r13;
    ext4_pucch = cc[CC_idP].radioResourceConfigCommon_BR->ext4->pucch_ConfigCommon_v1310;
    pucch_N1PUCCH_AN_InfoList_r13 = ext4_pucch->n1PUCCH_AN_InfoList_r13;
    AssertFatal (prach_ParametersListCE_r13 != NULL, "prach_ParametersListCE_r13 is null\n");
    AssertFatal (pucch_N1PUCCH_AN_InfoList_r13 != NULL, "pucch_N1PUCCH_AN_InfoList_r13 is null\n");
    /* Check to verify CE-Level compatibility in SIB2_BR */
    AssertFatal (prach_ParametersListCE_r13->list.count == pucch_N1PUCCH_AN_InfoList_r13->list.count, "prach_ParametersListCE_r13->list.count!= pucch_N1PUCCH_AN_InfoList_r13->list.count\n");

    switch (prach_ParametersListCE_r13->list.count) {
      case 4:
        p[3] = prach_ParametersListCE_r13->list.array[3];
        n1pucchan[3] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[3];
        AssertFatal(ext4_pucch->pucch_NumRepetitionCE_Msg4_Level3_r13 != NULL, "pucch_NumRepetitionCE_Msg4_Level3 shouldn't be NULL\n");
        pucchreps[3] = (int) (4 << *ext4_pucch->pucch_NumRepetitionCE_Msg4_Level3_r13);

      case 3:
        p[2] = prach_ParametersListCE_r13->list.array[2];
        n1pucchan[2] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[2];
        AssertFatal(ext4_pucch->pucch_NumRepetitionCE_Msg4_Level2_r13 != NULL, "pucch_NumRepetitionCE_Msg4_Level2 shouldn't be NULL\n");
        pucchreps[2] = (int) (4 << *ext4_pucch->pucch_NumRepetitionCE_Msg4_Level2_r13);

      case 2:
        p[1] = prach_ParametersListCE_r13->list.array[1];
        n1pucchan[1] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[1];
        AssertFatal(ext4_pucch->pucch_NumRepetitionCE_Msg4_Level1_r13 != NULL, "pucch_NumRepetitionCE_Msg4_Level1 shouldn't be NULL\n");
        pucchreps[1] = (int) (1 << *ext4_pucch->pucch_NumRepetitionCE_Msg4_Level1_r13);

      case 1:
        p[0] = prach_ParametersListCE_r13->list.array[0];
        n1pucchan[0] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[0];
        AssertFatal(ext4_pucch->pucch_NumRepetitionCE_Msg4_Level0_r13 != NULL, "pucch_NumRepetitionCE_Msg4_Level0 shouldn't be NULL\n");
        pucchreps[0] = (int) (1 << *ext4_pucch->pucch_NumRepetitionCE_Msg4_Level0_r13);
        break;

      default:
        AssertFatal(1 == 0, "Illegal count for prach_ParametersListCE_r13 %d\n", prach_ParametersListCE_r13->list.count);
    }
  }

  vrb_map = cc[CC_idP].vrb_map;
  dl_req        = &mac->DL_req[CC_idP];
  dl_req_body   = &dl_req->dl_config_request_body;
  dl_config_pdu = &dl_req_body->dl_config_pdu_list[dl_req_body->number_pdu];
  N_RB_DL = to_prb(cc[CC_idP].mib->message.dl_Bandwidth);
  UE_id = find_UE_id(module_idP, ra->rnti);

  if (UE_id < 0) {
    LOG_E(MAC, "Can't find UE for t-crnti %x, kill RA procedure for this UE\n",
          ra->rnti);
    cancel_ra_proc(module_idP, CC_idP, frameP, ra->rnti);
    return;
  }

  // set HARQ process round to 0 for this UE
  ra->harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config,frameP,subframeP);
  /* // Get RRCConnectionSetup for Piggyback
   rrc_sdu_length = mac_rrc_data_req(module_idP, CC_idP, frameP, CCCH, 1,  // 1 transport block
           &cc[CC_idP].CCCH_pdu.payload[0], 0);  // not used in this case
   if(rrc_sdu_length <= 0) {
     LOG_D(MAC,"[MAC][eNB Scheduler] CCCH not allocated (%d)\n",rrc_sdu_length);
     return;
   }
   //AssertFatal(rrc_sdu_length > 0,
  //"[MAC][eNB Scheduler] CCCH not allocated\n");


   LOG_D(MAC,
   "[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: UE_id %d, rrc_sdu_length %d\n",
   module_idP, CC_idP, frameP, subframeP, UE_id, rrc_sdu_length);*/

  if (ra->rach_resource_type > 0) {
    ra->harq_pid = 0;
    // Generate DCI + repetitions first
    // This uses an MPDCCH Type 2 allocation according to Section 9.1.5 36-213, Type2 common allocation according to Table 7.1-8 (36-213)
    // Parameters:
    //    p=2+4 PRB set (number of PRB pairs 6)
    //    rmax = mpdcch-NumRepetition-RA-r13 => Table 9.1.5-3
    //    if CELevel = 0,1 => Table 9.1.5-1b for MPDCCH candidates
    //    if CELevel = 2,3 => Table 9.1.5-2b for MPDCCH candidates
    //    distributed transmission
    // rmax from SIB2 information
    rmax = 1<<p[ra->rach_resource_type - 1]->mpdcch_NumRepetition_RA_r13;
    // choose r3 by default for Msg4 (this is ok from table 9.1.5-3 for rmax = >=4, if we choose rmax <4 it has to be less
    rep = 0;
    // get actual repetition count from Table 9.1.5-3
    reps = (rmax <= 8) ? (1 << rep) : (rmax >> (3 - rep));
    // get first narrowband
    first_rb = narrowband_to_first_rb (&cc[CC_idP], ra->msg34_narrowband);

    if ((ra->msg4_mpdcch_repetition_cnt == 0) && (mpdcch_sf_condition (mac, CC_idP, frameP, subframeP, rmax, TYPE2, -1) > 0)) {
      // Get RRCConnectionSetup for Piggyback
      ra->msg4_rrc_sdu_length = mac_rrc_data_req (module_idP, CC_idP, frameP, CCCH,
                                UE_RNTI(module_idP, UE_id), 1,        // 1 transport block
                                &cc[CC_idP].CCCH_pdu.payload[0], 0);  // not used in this case
      AssertFatal (ra->msg4_rrc_sdu_length > 0, "[MAC][eNB Scheduler] CCCH not allocated\n");
      LOG_D (MAC, "[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: UE_id %d, rrc_sdu_length %d, dl_req->num_pdu %d\n", module_idP, CC_idP, frameP, subframeP, UE_id, ra->msg4_rrc_sdu_length,
             dl_req_body->number_pdu);
      // MPDCCH configuration for Msg4
      ra->msg4_mpdcch_done=0;
      memset ((void *) dl_config_pdu, 0, sizeof (nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE;
      dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_dl_config_mpdcch_pdu));
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_format = (ra->rach_resource_type > 1) ? 11 : 10;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_narrow_band = ra->msg2_narrowband;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_prb_pairs = 6;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_assignment = 0; // Note: this can be dynamic
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_tansmission_type = 1;   // imposed (9.1.5 in 213) for Type 2 Common search space
      AssertFatal (cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13 != NULL, "cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13 is null\n");
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.start_symbol = cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ecce_index = 0;        // Note: this should be dynamic
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.aggregation_level = 24;        // OK for CEModeA r1-3 (9.1.5-1b) or CEModeB r1-4
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti_type = 0; // t-CRNTI
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti = ra->rnti;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ce_mode = (ra->rach_resource_type < 3) ? 1 : 2;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.drms_scrambling_init = cc[CC_idP].physCellId;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.initial_transmission_sf_io = (frameP * 10) + subframeP;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.transmission_power = 6000;     // 0dB
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_coding = getRIV (6, 0, 6) | (ra->msg2_narrowband<<5);
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mcs = 4;       // adjust according to size of RAR, 208 bits with N1A_PRB=3
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pdsch_reptition_levels = 0;    // fix to 4 for now
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.redundancy_version = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.new_data_indicator = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_process = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi_length = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_resource_offset = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_subframe_repetition_number = rep;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpc = 3;       // N1A_PRB=3 (36.212) => 56 bits
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index_length = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.allocate_prach_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.preamble_index = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.prach_mask_index = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.starting_ce_level = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.srs_request = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.frequency_hopping_enabled_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.paging_direct_indication_differentiation_flag = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.direct_indication = 0;
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.total_dci_length_including_padding = 0;        // this is not needed by OAI L1, but should be filled in
      dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_tx_antenna_ports = 1;
      ra->msg4_mpdcch_repetition_cnt++;
      dl_req_body->number_pdu++;
      ra->msg4_TBsize = get_TBS_DL(dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mcs,
                                   6);
    }                           //repetition_count==0 && SF condition met

    if ((ra->msg4_mpdcch_repetition_cnt > 0)&&
        (ra->msg4_mpdcch_done==0)) {     // we're in a stream of repetitions
      LOG_D(MAC,"SFN.SF %d.%d : msg4 mpdcch repetition number %d/%d\n",
            frameP,subframeP,ra->msg4_mpdcch_repetition_cnt,reps);

      if (ra->msg4_mpdcch_repetition_cnt == reps) {    // this is the last mpdcch repetition
        ra->msg4_mpdcch_done = 1;

        if (cc[CC_idP].tdd_Config == NULL) {    // FDD case
          // wait 2 subframes for PDSCH transmission
          if (subframeP > 7)
            ra->Msg4_frame = (frameP + 1) & 1023;
          else
            ra->Msg4_frame = frameP;

          ra->Msg4_subframe = (subframeP + 2) % 10;
          LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Set Msg4 PDSCH in %d.%d\n",
                module_idP, CC_idP, frameP, subframeP, ra->Msg4_frame,ra->Msg4_subframe);
        } else {
          AssertFatal (1 == 0, "TDD case not done yet\n");
        }
      } else if (ra->msg4_mpdcch_done==0)
        ra->msg4_mpdcch_repetition_cnt++;
    }
    // mpdcch_repetition_count == reps
    else if ((ra->Msg4_frame == frameP) && (ra->Msg4_subframe == subframeP)) {
      // Program PDSCH
      LOG_D (MAC, "[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Generating Msg4 BR with RRC Piggyback (ce_level %d RNTI %x)\n",
             module_idP, CC_idP, frameP, subframeP, ra->rach_resource_type - 1, ra->rnti);
      dl_config_pdu = &dl_req_body->dl_config_pdu_list[dl_req_body->number_pdu];
      memset ((void *) dl_config_pdu, 0, sizeof (nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
      dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_dl_config_dlsch_pdu));
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index = mac->pdu_index[CC_idP];
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti = ra->rnti;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type = 2;   // format 1A/1B/1D
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;     // localized
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding = getRIV (N_RB_DL, first_rb, 6);  // check that this isn't getRIV(6,0,6)
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation = 2; //QPSK
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version = 0;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks = 1;   // first block
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag = 0;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme = (cc[CC_idP].p_eNB == 1) ? 0 : 1;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers = 1;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands = 1;
      //      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index                         = ;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity = 1;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa = 4; // 0 dB
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index = 0;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap = 0;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb = get_subbandsize (cc[CC_idP].mib->message.dl_Bandwidth); // ignored
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode = (cc[CC_idP].p_eNB == 1) ? 1 : 2;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband = 1;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector = 1;
      //      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start = cc[CC_idP].sib1_v13ext->bandwidthReducedAccessRelatedInfo_r13->startSymbolBR_r13;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type = (ra->rach_resource_type < 3) ? 1 : 2;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type = 2;        // not SI message
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io = (10 * frameP) + subframeP;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.drms_table_flag = 0;
      dl_req_body->number_pdu++;
      ra->state = WAITMSG4ACK;
      lcid = 0;
      UE_info->UE_sched_ctrl[UE_id].round[CC_idP][ra->harq_pid] = 0;
      msg4_header = 1 + 6 + 1;        // CR header, CR CE, SDU header
      AssertFatal((ra->msg4_TBsize - ra->msg4_rrc_sdu_length - msg4_header)>=0,
                  "msg4_TBS %d is too small, change mcs to increase by %d bytes\n",ra->msg4_TBsize,ra->msg4_rrc_sdu_length+msg4_header-ra->msg4_TBsize);

      if ((ra->msg4_TBsize - ra->msg4_rrc_sdu_length - msg4_header) <= 2) {
        msg4_padding = ra->msg4_TBsize - ra->msg4_rrc_sdu_length - msg4_header;
        msg4_post_padding = 0;
      } else {
        msg4_padding = 0;
        msg4_post_padding = ra->msg4_TBsize - ra->msg4_rrc_sdu_length - msg4_header - 1;
      }

      LOG_D (MAC, "[eNB %d][RAPROC] CC_id %d Frame %d subframeP %d Msg4 : TBS %d, sdu_len %d, msg4_header %d, msg4_padding %d, msg4_post_padding %d\n",
             module_idP, CC_idP, frameP, subframeP, ra->msg4_TBsize, ra->msg4_rrc_sdu_length, msg4_header, msg4_padding, msg4_post_padding);
      DevAssert (UE_id != UE_INDEX_INVALID);  // FIXME not sure how to gracefully return
      // CHECK THIS: &cc[CC_idP].CCCH_pdu.payload[0]
      offset = generate_dlsch_header ((unsigned char *) mac->UE_info.DLSCH_pdu[CC_idP][0][(unsigned char) UE_id].payload[0], 1,       //num_sdus
                                      (unsigned short *) &ra->msg4_rrc_sdu_length,     //
                                      &lcid,  // sdu_lcid
                                      255,    // no drx
                                      31,     // no timing advance
                                      ra->cont_res_id,       // contention res id
                                      msg4_padding,   // no padding
                                      msg4_post_padding);
      memcpy ((void *) &mac->UE_info.DLSCH_pdu[CC_idP][0][(unsigned char) UE_id].payload[0][(unsigned char) offset], &cc[CC_idP].CCCH_pdu.payload[0], ra->msg4_rrc_sdu_length);
      // DL request
      mac->TX_req[CC_idP].sfn_sf = (frameP << 4) + subframeP;
      TX_req = &mac->TX_req[CC_idP].tx_request_body.tx_pdu_list[mac->TX_req[CC_idP].tx_request_body.number_of_pdus];
      TX_req->pdu_length = ra->msg4_TBsize;
      TX_req->pdu_index = mac->pdu_index[CC_idP]++;
      TX_req->num_segments = 1;
      TX_req->segments[0].segment_length = ra->msg4_TBsize;
      TX_req->segments[0].segment_data = mac->UE_info.DLSCH_pdu[CC_idP][0][(unsigned char) UE_id].payload[0];
      mac->TX_req[CC_idP].tx_request_body.number_of_pdus++;
      // Program ACK/NAK for Msg4 PDSCH
      int             absSF = (frameP * 10) + subframeP;
      // see Section 10.2 from 36.213
      int             ackNAK_absSF = absSF + reps + 3;
      AssertFatal (reps == 1, "Have to handle programming of ACK when PDSCH repetitions is > 1\n");
      ul_req_body = &mac->UL_req_tmp[CC_idP][ackNAK_absSF % 10].ul_config_request_body;
      ul_config_pdu = &ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus];
      ul_config_pdu->pdu_type = NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE;
      ul_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_ul_config_uci_harq_pdu));
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.handle = 0;      // don't know how to use this
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti = ra->rnti;
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL13_TAG;
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.ue_type = (ra->rach_resource_type < 3) ? 1 : 2;
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.empty_symbols = 0;
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.total_number_of_repetitions = pucchreps[ra->rach_resource_type - 1];
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.repetition_number = 0;

      // Note need to keep sending this across reptitions!!!! Not really for PUCCH, to ask small-cell forum, we'll see for the other messages, maybe parameters change across repetitions and FAPI has to provide for that
      if (cc[CC_idP].tdd_Config == NULL) {    // FDD case
        ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel9_fdd.n_pucch_1_0 = n1pucchan[ra->rach_resource_type - 1];
        // NOTE: How to fill in the rest of the n_pucch_1_0 information 213 Section 10.1.2.1 in the general case
        // = N_ECCE_q + Delta_ARO + n1pucchan[ce_level]
        // higher in the MPDCCH configuration, N_ECCE_q is hard-coded to 0, and harq resource offset to 0 =>
        // Delta_ARO = 0 from Table 10.1.2.1-1
        ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel9_fdd.harq_size = 1; // 1-bit ACK/NAK
        ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel9_fdd.number_of_pucch_resources = 1;
      } else {
        AssertFatal (1 == 0, "PUCCH configuration for ACK/NAK not handled yet for TDD BL/CE case\n");
      }

      ul_req_body->number_of_pdus++;
      T (T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT (module_idP), T_INT (CC_idP), T_INT (ra->rnti), T_INT (frameP), T_INT (subframeP),
         T_INT (0 /*harq_pid always 0? */ ), T_BUFFER (&mac->UE_info.DLSCH_pdu[CC_idP][0][UE_id].payload[0], ra->msg4_TBsize));
      trace_pdu (DIRECTION_DOWNLINK, (uint8_t *) mac->UE_info.DLSCH_pdu[CC_idP][0][(unsigned char) UE_id].payload[0],
                 ra->msg4_rrc_sdu_length,
                 UE_id, 3, UE_RNTI (module_idP, UE_id),
                 mac->frame, mac->subframe, 0, 0);
    }                           // Msg4 frame/subframe
  }                             // rach_resource_type > 0
  else {
    // This is normal LTE case
    LOG_D(MAC, "generate_Msg4 ra->Msg4_frame SFN/SF: %d.%d,  frameP SFN/SF: %d.%d FOR eNB_Mod: %d \n", ra->Msg4_frame, ra->Msg4_subframe, frameP, subframeP, module_idP);

    if ((ra->Msg4_frame == frameP) && (ra->Msg4_subframe == subframeP)) {
      // Get RRCConnectionSetup for Piggyback
      /*rrc_sdu_length = mac_rrc_data_req(module_idP, CC_idP, frameP, CCCH, 1,  // 1 transport block
                &cc[CC_idP].CCCH_pdu.payload[0], ENB_FLAG_YES, module_idP, 0);  // not used in this case*/
      // check if there's data on the CCCH to send with Msg4
      rrc_sdu_length = mac_rrc_data_req(module_idP, CC_idP, frameP, CCCH,
                                        UE_RNTI(module_idP,UE_id),1,  // 1 transport block
                                        &cc[CC_idP].CCCH_pdu.payload[0], 0);  // not used in this case

      if (rrc_sdu_length > 0) {
        LOG_D(MAC,
              "[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: UE_id %d, rrc_sdu_length %d\n",
              module_idP, CC_idP, frameP, subframeP, UE_id, rrc_sdu_length);
        //          AssertFatal(rrc_sdu_length > 0,
        //          "[MAC][eNB Scheduler] CCCH not allocated, rrc_sdu_length: %d\n", rrc_sdu_length);
        LOG_A(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Generating Msg4 with RRC Piggyback (RNTI %x)\n",
              module_idP, CC_idP, frameP, subframeP, ra->rnti);
        /// Choose first 4 RBs for Msg4, should really check that these are free!
        first_rb = 0;
        vrb_map[first_rb] = 1;
        vrb_map[first_rb + 1] = 1;
        vrb_map[first_rb + 2] = 1;
        vrb_map[first_rb + 3] = 1;
        // Compute MCS/TBS for 3 PRB (coded on 4 vrb)
        msg4_header = 1 + 6 + 1;  // CR header, CR CE, SDU header

        if ((rrc_sdu_length + msg4_header) <= 22) {
          ra->msg4_mcs = 4;
          ra->msg4_TBsize = 22;
        } else if ((rrc_sdu_length + msg4_header) <= 28) {
          ra->msg4_mcs = 5;
          ra->msg4_TBsize = 28;
        } else if ((rrc_sdu_length + msg4_header) <= 32) {
          ra->msg4_mcs = 6;
          ra->msg4_TBsize = 32;
        } else if ((rrc_sdu_length + msg4_header) <= 41) {
          ra->msg4_mcs = 7;
          ra->msg4_TBsize = 41;
        } else if ((rrc_sdu_length + msg4_header) <= 49) {
          ra->msg4_mcs = 8;
          ra->msg4_TBsize = 49;
        } else if ((rrc_sdu_length + msg4_header) <= 57) {
          ra->msg4_mcs = 9;
          ra->msg4_TBsize = 57;
        }

        fill_nfapi_dl_dci_1A(dl_config_pdu, 4,  // aggregation_level
                             ra->rnti,  // rnti
                             1, // rnti_type, CRNTI
                             ra->harq_pid,  // harq_process
                             1, // tpc, none
                             getRIV(N_RB_DL, first_rb, 4),  // resource_block_coding
                             ra->msg4_mcs,  // mcs
                             1 - UE_info->UE_template[CC_idP][UE_id].oldNDI[ra->harq_pid],
                             0, // rv
                             0);  // vrb_flag
        UE_info->UE_template[CC_idP][UE_id].oldNDI[ra->harq_pid] = 1 - UE_info->UE_template[CC_idP][UE_id].oldNDI[ra->harq_pid];
        LOG_D(MAC,
              "Frame %d, subframe %d: Msg4 DCI pdu_num %d (rnti %x,rnti_type %d,harq_pid %d, resource_block_coding (%p) %d\n",
              frameP, subframeP, dl_req_body->number_pdu,
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti,
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type,
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process,
              &dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding,
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding);
        AssertFatal(dl_config_pdu->dci_dl_pdu.
                    dci_dl_pdu_rel8.resource_block_coding < 8192,
                    "resource_block_coding %u < 8192\n",
                    dl_config_pdu->dci_dl_pdu.
                    dci_dl_pdu_rel8.resource_block_coding);

        if (!CCE_allocation_infeasible(module_idP, CC_idP, 1, subframeP,
                                       dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level, ra->rnti)) {
          dl_req_body->number_dci++;
          dl_req_body->number_pdu++;
          ra->state = WAITMSG4ACK;
          LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d: state:WAITMSG4ACK\n", module_idP, frameP, subframeP);
          // increment Absolute subframe by 8 for Msg4 retransmission
          LOG_D(MAC,
                "Frame %d, Subframe %d: Preparing for Msg4 retransmission currently %d.%d\n",
                frameP, subframeP, ra->Msg4_frame,
                ra->Msg4_subframe);
          get_retransmission_timing(mac->common_channels[CC_idP].tdd_Config,&ra->Msg4_frame,&ra->Msg4_subframe);
          LOG_D(MAC,
                "Frame %d, Subframe %d: Msg4 retransmission in %d.%d\n",
                frameP, subframeP, ra->Msg4_frame,
                ra->Msg4_subframe);
          lcid = 0;
          // put HARQ process round to 0
          ra->harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config,frameP,subframeP);
          UE_info->UE_sched_ctrl[UE_id].round[CC_idP][ra->harq_pid] = 0;

          if ((ra->msg4_TBsize - rrc_sdu_length - msg4_header) <= 2) {
            msg4_padding = ra->msg4_TBsize - rrc_sdu_length - msg4_header;
            msg4_post_padding = 0;
          } else {
            msg4_padding = 0;
            msg4_post_padding = ra->msg4_TBsize - rrc_sdu_length - msg4_header - 1;
          }

          LOG_D(MAC,
                "[eNB %d][RAPROC] CC_idP %d Frame %d subframeP %d Msg4 : TBS %d, sdu_len %d, msg4_header %d, msg4_padding %d, msg4_post_padding %d\n",
                module_idP, CC_idP, frameP, subframeP,
                ra->msg4_TBsize, rrc_sdu_length, msg4_header,
                msg4_padding, msg4_post_padding);
          DevAssert(UE_id != UE_INDEX_INVALID); // FIXME not sure how to gracefully return
          // CHECK THIS: &cc[CC_idP].CCCH_pdu.payload[0]
          int num_sdus = rrc_sdu_length > 0 ? 1 : 0;
          offset = generate_dlsch_header((unsigned char *) mac->UE_info.DLSCH_pdu[CC_idP][0][(unsigned char) UE_id].payload[0],
                                         num_sdus,  //num_sdus
                                         (unsigned short *) &rrc_sdu_length,  //
                                         &lcid, // sdu_lcid
                                         255, // no drx
                                         31,  // no timing advance
                                         ra->cont_res_id, // contention res id
                                         msg4_padding,  // no padding
                                         msg4_post_padding);
          memcpy((void *) &mac->UE_info.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0][(unsigned char)offset],
                 &cc[CC_idP].CCCH_pdu.payload[0], rrc_sdu_length);
          // DLSCH Config
          fill_nfapi_dlsch_config(&dl_req_body->dl_config_pdu_list[dl_req_body->number_pdu],
                                  ra->msg4_TBsize,
                                  mac->pdu_index[CC_idP],
                                  ra->rnti,
                                  2, // resource_allocation_type : format 1A/1B/1D
                                  0,  // virtual_resource_block_assignment_flag : localized
                                  getRIV(N_RB_DL, first_rb, 4), // resource_block_coding : RIV, 4 PRB
                                  2,  // modulation: QPSK
                                  0,  // redundancy version
                                  1,  // transport_blocks
                                  0,  // transport_block_to_codeword_swap_flag (0)
                                  (cc->p_eNB == 1) ? 0 : 1, // transmission_scheme
                                  1,  // number of layers
                                  1,  // number of subbands
                                  //0,                         // codebook index
                                  1,  // ue_category_capacity
                                  4,  // pa: 0 dB
                                  0,  // delta_power_offset_index
                                  0,  // ngap
                                  1,  // NPRB = 3 like in DCI
                                  (cc->p_eNB == 1) ? 1 : 2, // transmission mode
                                  1,  // num_bf_prb_per_subband
                                  1); // num_bf_vector
          dl_req_body->number_pdu++;
          LOG_D(MAC,
                "Filled DLSCH config, pdu number %d, non-dci pdu_index %d\n",
                dl_req_body->number_pdu, mac->pdu_index[CC_idP]);
          // Tx request
          mac->TX_req[CC_idP].sfn_sf =
            fill_nfapi_tx_req(&mac->TX_req[CC_idP].tx_request_body,
                              (frameP * 10) + subframeP,
                              rrc_sdu_length+offset,
                              mac->pdu_index[CC_idP],
                              mac->UE_info.
                              DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0]);
          mac->pdu_index[CC_idP]++;
          dl_req->sfn_sf = mac->TX_req[CC_idP].sfn_sf;
          LOG_D(MAC, "Filling UCI ACK/NAK information, cce_idx %d\n",
                dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
          // Program PUCCH1a for ACK/NAK
          // Program ACK/NAK for Msg4 PDSCH
          fill_nfapi_uci_acknak(module_idP,
                                CC_idP,
                                ra->rnti,
                                (frameP * 10) + subframeP,
                                dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
          T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP),
            T_INT(CC_idP), T_INT(ra->rnti), T_INT(frameP),
            T_INT(subframeP), T_INT(0 /*harq_pid always 0? */ ),
            T_BUFFER(&mac->UE_info.DLSCH_pdu[CC_idP][0][UE_id].
                     payload[0], ra->msg4_TBsize));
          trace_pdu(DIRECTION_DOWNLINK,
                    (uint8_t *) mac->
                    UE_info.DLSCH_pdu[CC_idP][0][(unsigned char)UE_id].payload[0],
                    rrc_sdu_length, UE_id,  WS_C_RNTI,
                    UE_RNTI(module_idP, UE_id), mac->frame,
                    mac->subframe, 0, 0);

          if(RC.mac[module_idP]->scheduler_mode == SCHED_MODE_FAIR_RR) {
            set_dl_ue_select_msg4(CC_idP, 4, UE_id, ra->rnti);
          }
        }     // CCE Allocation feasible
      } else {
        LOG_D(MAC,
              "eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Delaying Msg4 for RRC Piggyback (RNTI %x)\n",
              module_idP, CC_idP, frameP, subframeP, ra->rnti);
        ra->Msg4_subframe ++;
        ra->Msg4_delay_cnt++;

        if (ra->Msg4_delay_cnt==10) cancel_ra_proc(module_idP, CC_idP, frameP, ra->rnti);

        if (ra->Msg4_subframe == 10) {
          ra->Msg4_frame++;
          ra->Msg4_frame&=1023;
          ra->Msg4_subframe = 0;
        }
      }
    }     // msg4 frame/subframe
  }       // else rach_resource_type
}

void
check_Msg4_retransmission(module_id_t module_idP, int CC_idP,
                          frame_t frameP, sub_frame_t subframeP, RA_t *ra) {
  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc = mac->common_channels;
  int UE_id = -1;
  uint8_t *vrb_map;
  int first_rb;
  int N_RB_DL;
  nfapi_dl_config_request_pdu_t *dl_config_pdu;
  UE_info_t *UE_info = &mac->UE_info;
  nfapi_dl_config_request_t *dl_req;
  nfapi_dl_config_request_body_t *dl_req_body;
  int round;
  // check HARQ status and retransmit if necessary
  UE_id = find_UE_id(module_idP, ra->rnti);
  AssertFatal(UE_id >= 0, "Can't find UE for t-crnti\n");
  round = UE_info->UE_sched_ctrl[UE_id].round[CC_idP][ra->harq_pid];
  vrb_map = cc[CC_idP].vrb_map;
  dl_req = &mac->DL_req[CC_idP];
  dl_req_body = &dl_req->dl_config_request_body;
  dl_config_pdu = &dl_req_body->dl_config_pdu_list[dl_req_body->number_pdu];
  N_RB_DL = to_prb(cc[CC_idP].mib->message.dl_Bandwidth);
  LOG_D(MAC,
        "[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Checking if Msg4 for harq_pid %d was acknowledged (round %d), UE_id: %d \n",
        module_idP, CC_idP, frameP, subframeP, ra->harq_pid, round, UE_id);

  if (round != 8) {
    if (ra->rach_resource_type > 0 && round > 0) {
      AssertFatal(1 == 0,
                  "Msg4 Retransmissions not handled yet for BL/CE UEs, Frame %d, subframeP %d harq_pid %d round %d, UE_id: %d \n",
                  frameP, subframeP, ra->harq_pid, round, UE_id);
    } else {
      if ((ra->Msg4_frame == frameP)
          && (ra->Msg4_subframe == subframeP)) {
        //ra->wait_ack_Msg4++;
        // we have to schedule a retransmission
        dl_req->sfn_sf = frameP<<4 | subframeP;
        first_rb = 0;
        vrb_map[first_rb] = 1;
        vrb_map[first_rb + 1] = 1;
        vrb_map[first_rb + 2] = 1;
        vrb_map[first_rb + 3] = 1;
        fill_nfapi_dl_dci_1A(dl_config_pdu, 4,  // aggregation_level
                             ra->rnti,  // rnti
                             1, // rnti_type, CRNTI
                             ra->harq_pid,  // harq_process
                             1, // tpc, none
                             getRIV(N_RB_DL, first_rb, 4),  // resource_block_coding
                             ra->msg4_mcs,  // mcs
                             UE_info->UE_template[CC_idP][UE_id].oldNDI[ra->harq_pid],
                             round & 3, // rv
                             0);  // vrb_flag

        if (!CCE_allocation_infeasible
            (module_idP, CC_idP, 1, subframeP,
             dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.
             aggregation_level, ra->rnti)) {
          dl_req_body->number_dci++;
          dl_req_body->number_pdu++;
          dl_req_body->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
          LOG_D(MAC,
                "msg4 retransmission for rnti %x (round %d) fsf %d/%d\n",
                ra->rnti, round, frameP, subframeP);
          // DLSCH Config
          //DJP - fix this pdu_index = -1
          LOG_D(MAC, "check_Msg4_retransmission() before fill_nfapi_dlsch_config() with pdu_index = -1 \n");
          fill_nfapi_dlsch_config(&dl_req_body->dl_config_pdu_list[dl_req_body->number_pdu],
                                  ra->msg4_TBsize,
                                  -1
                                  /* retransmission, no pdu_index */
                                  , ra->rnti, 2,  // resource_allocation_type : format 1A/1B/1D
                                  0,  // virtual_resource_block_assignment_flag : localized
                                  getRIV(N_RB_DL, first_rb, 4), // resource_block_coding : RIV, 4 PRB
                                  2,  // modulation: QPSK
                                  round & 3,  // redundancy version
                                  1,  // transport_blocks
                                  0,  // transport_block_to_codeword_swap_flag (0)
                                  (cc->p_eNB == 1) ? 0 : 1, // transmission_scheme
                                  1,  // number of layers
                                  1,  // number of subbands
                                  //0,                         // codebook index
                                  1,  // ue_category_capacity
                                  4,  // pa: 0 dB
                                  0,  // delta_power_offset_index
                                  0,  // ngap
                                  1,  // NPRB = 3 like in DCI
                                  (cc->p_eNB == 1) ? 1 : 2, // transmission mode
                                  1,  // num_bf_prb_per_subband
                                  1); // num_bf_vector
          dl_req_body->number_pdu++;

          if(RC.mac[module_idP]->scheduler_mode == SCHED_MODE_FAIR_RR) {
            set_dl_ue_select_msg4(CC_idP, 4, UE_id, ra->rnti);
          }
        } else
          LOG_D(MAC,
                "msg4 retransmission for rnti %x (round %d) fsf %d/%d CCE allocation failed!\n",
                ra->rnti, round, frameP, subframeP);

        // Program PUCCH1a for ACK/NAK
        fill_nfapi_uci_acknak(module_idP, CC_idP,
                              ra->rnti,
                              (frameP * 10) + subframeP,
                              dl_config_pdu->dci_dl_pdu.
                              dci_dl_pdu_rel8.cce_idx);
        // prepare frame for retransmission
        get_retransmission_timing(mac->common_channels[CC_idP].tdd_Config,&ra->Msg4_frame,&ra->Msg4_subframe);
        LOG_W(MAC,
              "[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Msg4 not acknowledged, adding ue specific dci (rnti %x) for RA (Msg4 Retransmission round %d in %d.%d)\n",
              module_idP, CC_idP, frameP, subframeP, ra->rnti,
              round, ra->Msg4_frame, ra->Msg4_subframe);
      }     // Msg4 frame/subframe
    }     // regular LTE case
  } else {
    LOG_D(MAC,
          "[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d : Msg4 acknowledged\n",
          module_idP, CC_idP, frameP, subframeP);
    ra->state = IDLE;
    LOG_D(MAC,"[eNB %d][RAPROC] Frame %d, Subframe %d: state:IDLE\n", module_idP, frameP, subframeP);
    UE_id = find_UE_id(module_idP, ra->rnti);
    DevAssert(UE_id != -1);
    mac->UE_info.UE_template[UE_PCCID(module_idP, UE_id)][UE_id].configured = true;
    mac->UE_info.UE_template[UE_PCCID(module_idP, UE_id)][UE_id].pusch_repetition_levels=ra->pusch_repetition_levels;
    cancel_ra_proc(module_idP, CC_idP, frameP, ra->rnti);
  }
}

void
schedule_RA(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP) {
  int CC_id;
  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc = mac->common_channels;
  RA_t *ra;
  uint8_t i;
  start_meas(&mac->schedule_ra);

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    // skip UL component carriers if TDD
    if (is_UL_sf(&cc[CC_id], subframeP) == 1)
      continue;

    for (i = 0; i < NB_RA_PROC_MAX; i++) {
      ra = (RA_t *) & cc[CC_id].ra[i];

      if (ra->state == MSG2)
        generate_Msg2(module_idP, CC_id, frameP, subframeP, ra);
      else if (ra->state == MSG4 && ra->Msg4_frame == frameP && ra->Msg4_subframe == subframeP )
        generate_Msg4(module_idP, CC_id, frameP, subframeP, ra);
      else if (ra->state == WAITMSG4ACK)
        check_Msg4_retransmission(module_idP, CC_id, frameP,
                                  subframeP, ra);
    }     // for i=0 .. N_RA_PROC-1
  }       // CC_id

  stop_meas(&mac->schedule_ra);
}


// handles the event of MSG1 reception
void
initiate_ra_proc(module_id_t module_idP,
                 int CC_id,
                 frame_t frameP,
                 sub_frame_t subframeP,
                 uint16_t preamble_index,
                 int16_t timing_offset,
                 uint16_t ra_rnti,
                 uint8_t rach_resource_type
                ) {
  uint8_t i;
  COMMON_channels_t *cc = &RC.mac[module_idP]->common_channels[CC_id];
  RA_t *ra = &cc->ra[0];
  struct LTE_PRACH_ConfigSIB_v1310 *ext4_prach = NULL;
  LTE_PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13 = NULL;

  if (cc->mib->message.schedulingInfoSIB1_BR_r13>0) {
    AssertFatal(cc->radioResourceConfigCommon_BR != NULL,"radioResourceConfigCommon_BR is null\n");
    AssertFatal(cc->radioResourceConfigCommon_BR->ext4 != NULL, "radioResourceConfigCommon_BR->ext4 is null\n");
    ext4_prach = cc->radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
    AssertFatal(ext4_prach!=NULL,"ext4_prach is null\n");
    prach_ParametersListCE_r13 = &ext4_prach->prach_ParametersListCE_r13;
  }

  LOG_D(MAC,
        "[eNB %d][RAPROC] CC_id %d Frame %d, Subframe %d  Initiating RA procedure for preamble index %d, timing offset %d\n",
        module_idP, CC_id, frameP, subframeP, preamble_index, timing_offset);
  LOG_D(MAC,
        "[eNB %d][RAPROC] CC_id %d Frame %d, Subframe %d  PRACH resource type %d\n",
        module_idP, CC_id, frameP, subframeP, rach_resource_type);
  uint16_t msg2_frame = frameP;
  uint16_t msg2_subframe = subframeP;
  int offset;
  static uint8_t failure_cnt = 0 ;

  if (prach_ParametersListCE_r13 &&
      prach_ParametersListCE_r13->list.count < rach_resource_type) {
    LOG_E(MAC,
          "[eNB %d][RAPROC] CC_id %d Received impossible PRACH resource type %d, only %d CE levels configured\n",
          module_idP, CC_id, rach_resource_type,
          (int) prach_ParametersListCE_r13->list.count);
    return;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 1);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 0);

  for (i = 0; i < NB_RA_PROC_MAX; i++) {
    if (ra[i].state == IDLE) {
      int loop = 0;
      LOG_D(MAC, "Frame %d, Subframe %d: Activating RA process %d\n",
            frameP, subframeP, i);
      ra[i].state = MSG2;
      ra[i].Msg4_delay_cnt=0;
      ra[i].timing_offset = timing_offset;
      ra[i].preamble_subframe = subframeP;
      ra[i].rach_resource_type = rach_resource_type;
      ra[i].msg2_mpdcch_repetition_cnt = 0;
      ra[i].msg4_mpdcch_repetition_cnt = 0;

      //TODO Fill in other TDD config. What about nfapi_mode?
      if(cc->tdd_Config!=NULL) {
        switch(cc->tdd_Config->subframeAssignment) {
          default:
            printf("%s:%d: TODO\n", __FILE__, __LINE__);
            abort();

          case 1 :
	  case 3 :
	  case 4 :
          case 5 :
            offset = 6;
            break;
        }
      } else { //FDD
        // DJP - this is because VNF is 2 subframes ahead of PNF and TX needs 4 subframes
        if (NFAPI_MODE != NFAPI_MONOLITHIC)
          offset = 7;
        else
          offset = 5;
      }

      add_subframe(&msg2_frame, &msg2_subframe, offset);
      ra[i].Msg2_frame         = msg2_frame;
      ra[i].Msg2_subframe      = msg2_subframe;
      LOG_D(MAC,"%s() Msg2[%04d%d] SFN/SF:%04d%d offset:%d\n", __FUNCTION__,ra[i].Msg2_frame,ra[i].Msg2_subframe,frameP,subframeP,offset);
      ra[i].Msg2_subframe = (subframeP + offset) % 10;

      /* TODO: find better procedure to allocate RNTI */
      do {
        if (IS_SOFTMODEM_IQPLAYER) {  /* iq player mode, use deterministic rnti */
          static int drnti[MAX_MOBILES_PER_ENB];
          static int drnti_def[]={ 0xbda7, 0x71da, 0x9c40, 0xc350, 0x2710, 0x4e20, 0x7530, 0x1388, 0x3a98, 0x61a8, 0x88b8, 0xafc8, 0xd6d8, 0x1b58, 0x4268, 0x6978 };
          for (int j=0; j<MAX_MOBILES_PER_ENB && j< (sizeof(drnti_def)/sizeof(int));j++)
          	  drnti[i]=drnti_def[i];
          
          int nb_ue = 0;

          for (int j = 0; j < MAX_MOBILES_PER_ENB; j++) {
            if (UE_RNTI(module_idP, j) > 0) {
              nb_ue++;
            } else {
              break;
            }
          }

          if (nb_ue >= MAX_MOBILES_PER_ENB || nb_ue >= (sizeof(drnti_def)/sizeof(int))) {
            printf("No more free RNTI available, increase MAX_MOBILES_PER_ENB\n");
            abort();
          }

          ra[i].rnti = drnti[nb_ue];
        } else {
          ra[i].rnti = taus();
        }
        loop++;
      } while (loop != 100 &&
               /* TODO: this is not correct, the rnti may be in use without
                * being in the MAC yet. To be refined.
                */
               !(find_UE_id(module_idP, ra[i].rnti) == -1 &&
                 /* 1024 and 60000 arbirarily chosen, not coming from standard */
                 ra[i].rnti >= 1024 && ra[i].rnti < 60000));

      if (loop == 100) {
        printf("%s:%d:%s: FATAL ERROR! contact the authors\n",
               __FILE__, __LINE__, __FUNCTION__);
        abort();
      }

      ra[i].RA_rnti = ra_rnti;
      ra[i].preamble_index = preamble_index;
      failure_cnt = 0;
      LOG_D(MAC,
            "[eNB %d][RAPROC] CC_id %d Frame %d Activating RAR generation in Frame %d, subframe %d for process %d, rnti %x, state %d\n",
            module_idP, CC_id, frameP, ra[i].Msg2_frame,
            ra[i].Msg2_subframe, i, ra[i].rnti, ra[i].state);
      return;
    }
  }

  LOG_E(MAC,
        "[eNB %d][RAPROC] FAILURE: CC_id %d Frame %d Initiating RA procedure for preamble index %d\n",
        module_idP, CC_id, frameP, preamble_index);
  failure_cnt++;

  if(failure_cnt > 20) {
    LOG_E(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Clear Random access information\n", module_idP, CC_id, frameP);
    clear_ra_proc(module_idP, CC_id, frameP);
  }
}

void
cancel_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP,
               rnti_t rnti) {
  unsigned char i;
  RA_t *ra = (RA_t *) & RC.mac[module_idP]->common_channels[CC_id].ra[0];
  LOG_D(MAC,
        "[eNB %d][RAPROC] CC_id %d Frame %d Cancelling RA procedure for UE rnti %x\n",
        module_idP, CC_id, frameP, rnti);

  for (i = 0; i < NB_RA_PROC_MAX; i++) {
    if (rnti == ra[i].rnti) {
      ra[i].state = IDLE;
      ra[i].timing_offset = 0;
      ra[i].RRC_timer = 20;
      ra[i].rnti = 0;
      ra[i].msg3_round = 0;
      LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Canceled RA procedure for UE rnti %x\n", module_idP, CC_id, frameP, rnti);
    }
  }
}

void clear_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP) {
  unsigned char i;
  RA_t *ra = (RA_t *) & RC.mac[module_idP]->common_channels[CC_id].ra[0];

  for (i = 0; i < NB_RA_PROC_MAX; i++) {
    LOG_D(MAC,"[eNB %d][RAPROC] CC_id %d Frame %d Clear Random access information rnti %x\n", module_idP, CC_id, frameP, ra[i].rnti);
    ra[i].state = IDLE;
    ra[i].timing_offset = 0;
    ra[i].RRC_timer = 20;
    ra[i].rnti = 0;
    ra[i].msg3_round = 0;
  }
}
