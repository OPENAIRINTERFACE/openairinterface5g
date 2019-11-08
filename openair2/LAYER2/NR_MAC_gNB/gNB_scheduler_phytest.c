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
extern RAN_CONTEXT_t RC;
//#define ENABLE_MAC_PAYLOAD_DEBUG 1

/*Scheduling of DLSCH with associated DCI in common search space
 * current version has only a DCI for type 1 PDCCH for C_RNTI*/
void nr_schedule_css_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP) {
  uint8_t  CC_id;
  gNB_MAC_INST                        *nr_mac      = RC.nrmac[module_idP];
  NR_COMMON_channels_t                *cc          = nr_mac->common_channels;
  nfapi_nr_dl_config_request_body_t   *dl_req;
  nfapi_nr_dl_config_request_pdu_t  *dl_config_dci_pdu;
  nfapi_nr_dl_config_request_pdu_t  *dl_config_dlsch_pdu;
  nfapi_tx_request_pdu_t            *TX_req;
  nfapi_nr_config_request_t *cfg = &nr_mac->config[0];
  uint16_t rnti = 0x1234;
  uint16_t sfn_sf = frameP << 7 | slotP;
  int dl_carrier_bandwidth = cfg->rf_config.dl_carrier_bandwidth.value;
  // everything here is hard-coded to 30 kHz
  int scs = get_dlscs(cfg);
  int slots_per_frame = get_spf(cfg);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "Scheduling common search space DCI type 1 for CC_id %d\n",CC_id);
    dl_req = &nr_mac->DL_req[CC_id].dl_config_request_body;
    dl_config_dci_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
    memset((void *)dl_config_dci_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
    dl_config_dci_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE;
    dl_config_dci_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_dci_dl_pdu));
    dl_config_dlsch_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu+1];
    memset((void *)dl_config_dlsch_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
    dl_config_dlsch_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DLSCH_PDU_TYPE;
    dl_config_dlsch_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_dlsch_pdu));
    nfapi_nr_dl_config_dci_dl_pdu_rel15_t *dci_dl_pdu_rel15 = &dl_config_dci_pdu->dci_dl_pdu.dci_dl_pdu_rel15;
    nfapi_nr_dl_config_pdcch_parameters_rel15_t *params_rel15 = &dl_config_dci_pdu->dci_dl_pdu.pdcch_params_rel15;
    nfapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_pdu_rel15 = &dl_config_dlsch_pdu->dlsch_pdu.dlsch_pdu_rel15;
    dlsch_pdu_rel15->start_prb = 0;
    dlsch_pdu_rel15->n_prb = 50;
    dlsch_pdu_rel15->start_symbol = 2;
    dlsch_pdu_rel15->nb_symbols = 8;
    dlsch_pdu_rel15->rnti = rnti;
    dlsch_pdu_rel15->nb_layers =1;
    dlsch_pdu_rel15->nb_codewords = 1;
    dlsch_pdu_rel15->mcs_idx = 9;
    dlsch_pdu_rel15->ndi = 1;
    dlsch_pdu_rel15->redundancy_version = 0;
    nr_configure_css_dci_initial(params_rel15,
                                 scs, scs, nr_FR1, 0, 0, 0,
                                 sfn_sf, slotP,
                                 slots_per_frame,
                                 dl_carrier_bandwidth);
    params_rel15->first_slot = 0;
    dci_dl_pdu_rel15->frequency_domain_assignment = get_RIV(dlsch_pdu_rel15->start_prb, dlsch_pdu_rel15->n_prb, cfg->rf_config.dl_carrier_bandwidth.value);
    dci_dl_pdu_rel15->time_domain_assignment = 3; // row index used here instead of SLIV
    dci_dl_pdu_rel15->vrb_to_prb_mapping = 1;
    dci_dl_pdu_rel15->mcs = 9;
    dci_dl_pdu_rel15->tb_scaling = 1;
    dci_dl_pdu_rel15->ra_preamble_index = 25;
    dci_dl_pdu_rel15->format_indicator = 1;
    dci_dl_pdu_rel15->ndi = 1;
    dci_dl_pdu_rel15->rv = 0;
    dci_dl_pdu_rel15->harq_pid = 0;
    dci_dl_pdu_rel15->dai = 2;
    dci_dl_pdu_rel15->tpc = 2;
    dci_dl_pdu_rel15->pucch_resource_indicator = 7;
    dci_dl_pdu_rel15->pdsch_to_harq_feedback_timing_indicator = 7;
    LOG_D(MAC, "[gNB scheduler phytest] DCI type 1 payload: freq_alloc %d, time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d ndi %d rv %d\n",
          dci_dl_pdu_rel15->frequency_domain_assignment,
          dci_dl_pdu_rel15->time_domain_assignment,
          dci_dl_pdu_rel15->vrb_to_prb_mapping,
          dci_dl_pdu_rel15->mcs,
          dci_dl_pdu_rel15->tb_scaling,
          dci_dl_pdu_rel15->ndi,
          dci_dl_pdu_rel15->rv);
    params_rel15->rnti = rnti;
    params_rel15->rnti_type = NFAPI_NR_RNTI_C;
    params_rel15->dci_format = NFAPI_NR_DL_DCI_FORMAT_1_0;
    //params_rel15->aggregation_level = 1;
    LOG_D(MAC, "DCI type 1 params: rmsi_pdcch_config %d, rnti %d, rnti_type %d, dci_format %d\n \
                coreset params: mux_pattern %d, n_rb %d, n_symb %d, rb_offset %d  \n \
                ss params : nb_ss_sets_per_slot %d, first symb %d, nb_slots %d, sfn_mod2 %d, first slot %d\n",
          0,
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
    nr_get_tbs_dl(&dl_config_dlsch_pdu->dlsch_pdu, dl_config_dci_pdu->dci_dl_pdu, *cfg);
    LOG_D(MAC, "DLSCH PDU: start PRB %d n_PRB %d start symbol %d nb_symbols %d nb_layers %d nb_codewords %d mcs %d\n",
          dlsch_pdu_rel15->start_prb,
          dlsch_pdu_rel15->n_prb,
          dlsch_pdu_rel15->start_symbol,
          dlsch_pdu_rel15->nb_symbols,
          dlsch_pdu_rel15->nb_layers,
          dlsch_pdu_rel15->nb_codewords,
          dlsch_pdu_rel15->mcs_idx);
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

int configure_fapi_dl_Tx(nfapi_nr_dl_config_request_body_t *dl_req,
                         nfapi_tx_request_pdu_t *TX_req,
                         nfapi_nr_config_request_t *cfg,
                         nfapi_nr_coreset_t *coreset,
                         nfapi_nr_search_space_t *search_space,
                         int16_t pdu_index,
                         nfapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config) {
  nfapi_nr_dl_config_request_pdu_t  *dl_config_dci_pdu;
  nfapi_nr_dl_config_request_pdu_t  *dl_config_dlsch_pdu;
  int TBS;
  uint16_t rnti = 0x1234;
  int dl_carrier_bandwidth = cfg->rf_config.dl_carrier_bandwidth.value;
  dl_config_dci_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
  memset((void *)dl_config_dci_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
  dl_config_dci_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE;
  dl_config_dci_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_dci_dl_pdu));
  dl_config_dlsch_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu+1];
  memset((void *)dl_config_dlsch_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
  dl_config_dlsch_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DLSCH_PDU_TYPE;
  dl_config_dlsch_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_dlsch_pdu));
  nfapi_nr_dl_config_dci_dl_pdu_rel15_t *pdu_rel15 = &dl_config_dci_pdu->dci_dl_pdu.dci_dl_pdu_rel15;
  nfapi_nr_dl_config_pdcch_parameters_rel15_t *params_rel15 = &dl_config_dci_pdu->dci_dl_pdu.pdcch_params_rel15;
  nfapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_pdu_rel15 = &dl_config_dlsch_pdu->dlsch_pdu.dlsch_pdu_rel15;
  dlsch_pdu_rel15->start_prb = 0;
  dlsch_pdu_rel15->n_prb = 50;
  dlsch_pdu_rel15->start_symbol = 2;
  dlsch_pdu_rel15->nb_symbols = 9;
  dlsch_pdu_rel15->rnti = rnti;
  dlsch_pdu_rel15->nb_layers =1;
  dlsch_pdu_rel15->nb_codewords = 1;
  dlsch_pdu_rel15->mcs_idx = 9;
  dlsch_pdu_rel15->ndi = 1;
  dlsch_pdu_rel15->redundancy_version = 0;

  if (dlsch_config != NULL) {
    dlsch_pdu_rel15->start_prb = dlsch_config->start_prb;
    dlsch_pdu_rel15->n_prb = dlsch_config->n_prb;
    dlsch_pdu_rel15->start_symbol = dlsch_config->start_symbol;
    dlsch_pdu_rel15->nb_symbols = dlsch_config->nb_symbols;
    dlsch_pdu_rel15->mcs_idx = dlsch_config->mcs_idx;
  }

  nr_configure_dci_from_pdcch_config(params_rel15,
                                     coreset,
                                     search_space,
                                     *cfg,
                                     dl_carrier_bandwidth);
  pdu_rel15->frequency_domain_assignment = get_RIV(dlsch_pdu_rel15->start_prb, dlsch_pdu_rel15->n_prb, cfg->rf_config.dl_carrier_bandwidth.value);
  pdu_rel15->time_domain_assignment = 3; // row index used here instead of SLIV;
  pdu_rel15->vrb_to_prb_mapping = 1;
  pdu_rel15->mcs = dlsch_pdu_rel15->mcs_idx;
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
  LOG_D(MAC, "DCI params: rnti %d, rnti_type %d, dci_format %d, config type %d\n \
	                      coreset params: mux_pattern %d, n_rb %d, n_symb %d, rb_offset %d  \n \
	                      ss params : first symb %d, ss type %d\n",
        params_rel15->rnti,
        params_rel15->rnti_type,
        params_rel15->config_type,
        params_rel15->dci_format,
        params_rel15->mux_pattern,
        params_rel15->n_rb,
        params_rel15->n_symb,
        params_rel15->rb_offset,
        params_rel15->first_symbol,
        params_rel15->search_space_type);
  nr_get_tbs_dl(&dl_config_dlsch_pdu->dlsch_pdu, dl_config_dci_pdu->dci_dl_pdu, *cfg);
  TBS = dl_config_dlsch_pdu->dlsch_pdu.dlsch_pdu_rel15.transport_block_size;
  LOG_D(MAC, "DLSCH PDU: start PRB %d n_PRB %d start symbol %d nb_symbols %d nb_layers %d nb_codewords %d mcs %d TBS: %d\n",
        dlsch_pdu_rel15->start_prb,
        dlsch_pdu_rel15->n_prb,
        dlsch_pdu_rel15->start_symbol,
        dlsch_pdu_rel15->nb_symbols,
        dlsch_pdu_rel15->nb_layers,
        dlsch_pdu_rel15->nb_codewords,
        dlsch_pdu_rel15->mcs_idx,
        TBS);
  dl_req->number_dci++;
  dl_req->number_pdsch_rnti++;
  dl_req->number_pdu+=2;
  TX_req->pdu_length = dlsch_pdu_rel15->transport_block_size/8;
  TX_req->pdu_index = pdu_index++;
  TX_req->num_segments = 1;
  return TBS/8; //Return TBS in bytes
}

void nr_schedule_uss_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP,
                                   nfapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config) {

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_idP];
  UE_list_t *UE_list = &gNB_mac->UE_list;
  nfapi_nr_dl_config_request_body_t *dl_req;
  nfapi_tx_request_pdu_t *TX_req;
  mac_rlc_status_resp_t rlc_status;
  nfapi_nr_config_request_t *cfg = &gNB_mac->config[0];

  // TODO not sure why NR_MAX_NB_RB was used here
  unsigned char sdu_lcids[NB_RB_MAX] = {0};
  uint16_t sdu_lengths[NB_RB_MAX] = {0};

  int padding = 0, post_padding = 0, ta_len = 0, header_length_total = 0, sdu_length_total = 0, num_sdus = 0;
  int CC_id, lcid, offset, i, j=0, k=0, header_length_last, TBS_bytes;

  // hardcoded parameters to 30 kHz
  // for DMRS configuration type 1
  unsigned char dlsch_buffer[MAX_NR_DLSCH_PAYLOAD_BYTES];
  uint8_t Qm = 2;
  uint16_t R = 697;
  uint16_t nb_rb = 50;
  uint32_t TBS = nr_compute_tbs(Qm, R, nb_rb, 12, 6, 0, 1)/8; // this is in bits TODO use nr_get_tbs
  int UE_id = 0; // UE_list->head is -1 !
  uint16_t rnti = 0x1234;
  uint16_t sfn_sf = frameP << 7 | slotP;

  UE_sched_ctrl_t *ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];
  //ta_update = ue_sched_ctl->ta_update;

  for (CC_id = 0; CC_id < RC.nb_nr_mac_CC[module_idP]; CC_id++) {

    LOG_D(MAC, "Scheduling UE specific search space DCI type 1 for UE_id %d CC_id %d frame %d slot %d\n", UE_id, CC_id, frameP, slotP);

    dl_req = &gNB_mac->DL_req[CC_id].dl_config_request_body;
    TX_req = &gNB_mac->TX_req[CC_id].tx_request_body.tx_pdu_list[gNB_mac->TX_req[CC_id].tx_request_body.number_of_pdus];
    ta_len = gNB_mac->ta_len;

    //The --NOS1 use case currently schedules DLSCH transmissions only when there is IP traffic arriving
    //through the LTE stack
    if (IS_SOFTMODEM_NOS1){


      // Hardcode it for now
      //TBS = 6784/8; //TBS in bytes
      //nr_get_tbs_dl(&dl_config_dlsch_pdu->dlsch_pdu, dl_config_dci_pdu->dci_dl_pdu, *cfg);
      //TBS = dl_config_dlsch_pdu->dlsch_pdu.dlsch_pdu_rel15.transport_block_size;

      /* TODO
      // RLC data on DCCH
      if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
      }*/

      /* TODO
      // RLC data on DCCH1
      if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
      }*/

      for (lcid = NB_RB_MAX - 1; lcid >= DTCH; lcid--) {
        LOG_D(MAC, "[gNB %d], Frame %d, DTCH%d->DLSCH, Checking RLC status (tbs %d, len %d)\n",
          module_idP, frameP, lcid, TBS, TBS - ta_len - header_length_total - sdu_length_total - 3);

        if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
          rlc_status = mac_rlc_status_ind(module_idP,
                                          rnti,
                                          module_idP,
                                          frameP,
                                          slotP,
                                          ENB_FLAG_YES,
                                          MBMS_FLAG_NO,
                                          lcid,
                                          TBS - ta_len - header_length_total - sdu_length_total - 3,
                                          0,
                                          0);

          if (rlc_status.bytes_in_buffer > 0) {
            LOG_D(MAC, "[gNB %d][USER-PLANE DEFAULT DRB] Frame %d : DTCH->DLSCH, Requesting %d bytes from RLC (lcid %d total hdr len %d), TBS: %d \n \n",
              module_idP, frameP, TBS - ta_len - header_length_total - sdu_length_total - 3,
              lcid, header_length_total, TBS);

            sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
                                                     rnti,
                                                     module_idP,
                                                     frameP,
                                                     ENB_FLAG_YES,
                                                     MBMS_FLAG_NO,
                                                     lcid,
                                                     TBS,
                                                     (char *)&dlsch_buffer[sdu_length_total],
                                                     0,
                                                     0);

            LOG_D(MAC, "[gNB %d][USER-PLANE DEFAULT DRB] Got %d bytes for DTCH %d \n", module_idP, sdu_lengths[num_sdus], lcid);

            sdu_lcids[num_sdus] = lcid;
            sdu_length_total += sdu_lengths[num_sdus];

            UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[lcid]++;
            UE_list->eNB_UE_stats[CC_id][UE_id].lcid_sdu[num_sdus] = lcid;
            UE_list->eNB_UE_stats[CC_id][UE_id].sdu_length_tx[lcid] = sdu_lengths[num_sdus];
            UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[lcid] += sdu_lengths[num_sdus];

            header_length_last = 1 + 1 + (sdu_lengths[num_sdus] >= 128);
            header_length_total += header_length_last;

            num_sdus++;

            ue_sched_ctl->uplane_inactivity_timer = 0;
          }
        } else { // no TBS left
        break;
        }
      }

      // // last header does not have length field
      // if (header_length_total) {
      //   header_length_total -= header_length_last;
      //   header_length_total++;
      // }

    } //if (IS_SOFTMODEM_NOS1)
    else {
      //When the --NOS1 option is not enabled, DLSCH transmissions with random data
      //occur every time that the current function is called (dlsch phytest mode)

      // fill dlsch_buffer with random data
      for (i = 0; i < MAX_NR_DLSCH_PAYLOAD_BYTES; i++){
        dlsch_buffer[i] = (unsigned char) rand();
      }

      //Sending SDUs with size 1
      //Initialize elements of sdu_lcids and sdu_lengths
      //TODO this will be eventually be removed
      while (TBS >= 3 + header_length_total + sdu_length_total + ta_len){
        if (k < NR_MAX_NB_RB && j < NR_MAX_NB_RB){
          sdu_lcids[j] = 0x05; // DRB
          sdu_lengths[k] = 1;
          header_length_total += 2;
          sdu_length_total += 1;
          num_sdus +=1;
          k++, j++;
        }
        else {
          break;
        }
      }
    } // else IS_SOFTMODEM_NOS1

    // there is at least one SDU or TA command
    // if (num_sdus > 0 ){
    if (ta_len + sdu_length_total + header_length_total > 0) {

      // Check if there is data from RLC or CE
      if (TBS >= 2 + header_length_total + sdu_length_total + ta_len) {
        // we have to consider padding
        // padding param currently not in use
        padding = TBS - header_length_total - sdu_length_total - ta_len - 1;
        post_padding = 1;
      } else {
        padding = 0;
        post_padding = 0;
      }


      offset = nr_generate_dlsch_pdu(module_idP,
                                     (unsigned char *) dlsch_buffer,
                                     (unsigned char *) UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0],
                                     num_sdus, //num_sdus
                                     sdu_lengths,
                                     sdu_lcids,
                                     255, // no drx
                                     NULL, // contention res id
                                     post_padding);

      // Padding: fill remainder of DLSCH with 0
      if (post_padding > 0){
        for (int j = 0; j < (TBS - offset); j++)
          UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset + j] = 0;
      }

      TBS_bytes = configure_fapi_dl_Tx(dl_req, TX_req, cfg, &gNB_mac->coreset[CC_id][1], &gNB_mac->search_space[CC_id][1], gNB_mac->pdu_index[CC_id], dlsch_config);

      //TX_req->segments[0].segment_length = 8;
      TX_req->segments[0].segment_length = TBS_bytes + 2;
      TX_req->segments[0].segment_data = gNB_mac->UE_list.DLSCH_pdu[CC_id][0][UE_id].payload[0];

      gNB_mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
      gNB_mac->TX_req[CC_id].sfn_sf = sfn_sf;
      gNB_mac->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
      gNB_mac->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;

      if(IS_SOFTMODEM_NOS1){
        #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
          LOG_I(MAC, "Printing first 10 payload bytes at the gNB side, Frame: %d, slot: %d, TBS size: %d \n \n", frameP, slotP, TBS_bytes);
          for(int i = 0; i < 10; i++) { // TBS_bytes dlsch_pdu_rel15->transport_block_size/8 6784/8
            LOG_I(MAC, "%x. ", ((uint8_t *)gNB_mac->UE_list.DLSCH_pdu[CC_id][0][UE_id].payload[0])[i]);
          }
        #endif
      } else {
        #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
        if (frameP%100 == 0){
          LOG_I(MAC, "Printing first 10 payload bytes at the gNB side, Frame: %d, slot: %d, TBS size: %d \n", frameP, slotP, TBS_bytes);
          for(int i = 0; i < 10; i++) {
            LOG_I(MAC, "%x. ", ((uint8_t *)gNB_mac->UE_list.DLSCH_pdu[CC_id][0][UE_id].payload[0])[i]);
          }
        }
        #endif
      }
      // Printing bit by bit for debugging purpose
      /*for (int k = 0; k < TBS; k++){
        printf("MAC PDU %u\n",((( UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][k/8]) & (1 << (k & 7))) >> (k & 7)));
        if ((k+1)%8 == 0)
          printf("\n");
      }*/
    }
    else {  // There is no data from RLC or MAC header, so don't schedule
    }
  } // CC_id loop
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
