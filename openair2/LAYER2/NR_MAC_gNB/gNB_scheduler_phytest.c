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
                                   sub_frame_t   slotP)
{
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
    memset((void*)dl_config_dci_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
    dl_config_dci_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE;
    dl_config_dci_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_dci_dl_pdu));

    dl_config_dlsch_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu+1];
    memset((void*)dl_config_dlsch_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
    dl_config_dlsch_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DLSCH_PDU_TYPE;
    dl_config_dlsch_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_dlsch_pdu));

    nfapi_nr_dl_config_dci_dl_pdu_rel15_t *pdu_rel15 = &dl_config_dci_pdu->dci_dl_pdu.dci_dl_pdu_rel15;
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

    pdu_rel15->frequency_domain_assignment = get_RIV(dlsch_pdu_rel15->start_prb, dlsch_pdu_rel15->n_prb, cfg->rf_config.dl_carrier_bandwidth.value);
    pdu_rel15->time_domain_assignment = 3; // row index used here instead of SLIV
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
  nr_get_tbs(&dl_config_dlsch_pdu->dlsch_pdu, dl_config_dci_pdu->dci_dl_pdu, *cfg);
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
						  nfapi_nr_coreset_t* coreset,
						  nfapi_nr_search_space_t* search_space,
						  int16_t pdu_index){

	nfapi_nr_dl_config_request_pdu_t  *dl_config_dci_pdu;
	nfapi_nr_dl_config_request_pdu_t  *dl_config_dlsch_pdu;
	int TBS;
	uint16_t rnti = 0x1234;
	int dl_carrier_bandwidth = cfg->rf_config.dl_carrier_bandwidth.value;


	          dl_config_dci_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
	          memset((void*)dl_config_dci_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
	          dl_config_dci_pdu->pdu_type = NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE;
	          dl_config_dci_pdu->pdu_size = (uint8_t)(2+sizeof(nfapi_nr_dl_config_dci_dl_pdu));

	          dl_config_dlsch_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu+1];
	          memset((void*)dl_config_dlsch_pdu,0,sizeof(nfapi_nr_dl_config_request_pdu_t));
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

	          nr_configure_dci_from_pdcch_config(params_rel15,
	                                             coreset,
	                                             search_space,
	                                             *cfg,
	                                             dl_carrier_bandwidth);

	          pdu_rel15->frequency_domain_assignment = get_RIV(dlsch_pdu_rel15->start_prb, dlsch_pdu_rel15->n_prb, cfg->rf_config.dl_carrier_bandwidth.value);
	          pdu_rel15->time_domain_assignment = 3; // row index used here instead of SLIV;
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
	        nr_get_tbs(&dl_config_dlsch_pdu->dlsch_pdu, dl_config_dci_pdu->dci_dl_pdu, *cfg);
		    // Hardcode it for now
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
                                   sub_frame_t   slotP)
{
	LOG_D(MAC, "In nr_schedule_uss_dlsch_phytest \n");
  uint8_t  CC_id;

  gNB_MAC_INST                        *nr_mac      = RC.nrmac[module_idP];
  //NR_COMMON_channels_t                *cc           = nr_mac->common_channels;
  nfapi_nr_dl_config_request_body_t   *dl_req;
  nfapi_tx_request_pdu_t            *TX_req;
  uint16_t rnti = 0x1234;

  nfapi_nr_config_request_t *cfg = &nr_mac->config[0];

  uint16_t sfn_sf = frameP << 7 | slotP;

  // everything here is hard-coded to 30 kHz
  //int scs = get_dlscs(cfg);
  //int slots_per_frame = get_spf(cfg);

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

    DLSCH_PDU DLSCH_pdu;
    //DLSCH_PDU *DLSCH_pdu = (DLSCH_PDU*) malloc(sizeof(DLSCH_PDU));


  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

	  LOG_D(MAC, "Scheduling UE specific search space DCI type 1 for CC_id %d\n",CC_id);

	    dl_req = &nr_mac->DL_req[CC_id].dl_config_request_body;
	    TX_req = &nr_mac->TX_req[CC_id].tx_request_body.tx_pdu_list[nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus];

	    //The --NOS1 use case currently schedules DLSCH transmissions only when there is IP traffic arriving
	    //through the LTE stack
	    if (IS_SOFTMODEM_NOS1){

	    	memset(&DLSCH_pdu, 0, sizeof(DLSCH_pdu));
      int ta_update = 31;
      ta_len = 0;

      // Hardcode it for now
      TBS = 6784/8; //TBS in bytes
      //nr_get_tbs(&dl_config_dlsch_pdu->dlsch_pdu, dl_config_dci_pdu->dci_dl_pdu, *cfg);
      //TBS = dl_config_dlsch_pdu->dlsch_pdu.dlsch_pdu_rel15.transport_block_size;

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

    	offset = generate_dlsch_header((unsigned char *)nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0], //DLSCH_pdu.payload[0],
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
        memcpy(&nr_mac->UE_list.DLSCH_pdu[CC_id][0][UE_id].payload[0][offset], dlsch_buffer, sdu_length_total);
          

    	// fill remainder of DLSCH with 0
    	for (int j = 0; j < (TBS - sdu_length_total - offset); j++) {
    		//UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset + sdu_length_total + j] = 0;
                nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0][offset + sdu_length_total + j] = 0;
    	}

    	TBS_bytes = configure_fapi_dl_Tx(dl_req, TX_req, cfg, &nr_mac->coreset[CC_id][1], &nr_mac->search_space[CC_id][1], nr_mac->pdu_index[CC_id]);        

        #if defined(ENABLE_MAC_PAYLOAD_DEBUG)
    		LOG_I(MAC, "Printing first 10 payload bytes at the gNB side, Frame: %d, slot: %d, , TBS size: %d \n \n", frameP, slotP, TBS_bytes);
    	  	for(int i = 0; i < 10; i++) { // TBS_bytes dlsch_pdu_rel15->transport_block_size/8 6784/8
    	  	  	LOG_I(MAC, "%x. ", ((uint8_t *)nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0])[i]);
    	  	}
        #endif

    	  //TX_req->segments[0].segment_length = 8;
    	  TX_req->segments[0].segment_length = TBS_bytes +2;
    	  TX_req->segments[0].segment_data = nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0];


    	  nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
    	  nr_mac->TX_req[CC_id].sfn_sf = sfn_sf;
    	  nr_mac->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
    	  nr_mac->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;
    } //if (ta_len + sdu_length_total + header_length_total > 0)
	  } //if (IS_SOFTMODEM_NOS1)

	    //When the --NOS1 option is not enabled, DLSCH transmissions with random data
	    //occur every time that the current function is called (dlsch phytest mode)
	  else{
		  TBS_bytes = configure_fapi_dl_Tx(dl_req, TX_req, cfg, &nr_mac->coreset[CC_id][1], &nr_mac->search_space[CC_id][1], nr_mac->pdu_index[CC_id]);
		  // HOT FIX for all zero pdu problem
		  // ------------------------------------------------------------------------------------------------
		  
		  for(int i = 0; i < TBS_bytes; i++) { //
		  	  ((uint8_t *)nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0])[i] = (unsigned char) rand();
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
		  TX_req->segments[0].segment_data = nr_mac->UE_list.DLSCH_pdu[CC_id][0][0].payload[0];

		  nr_mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
		  nr_mac->TX_req[CC_id].sfn_sf = sfn_sf;
		  nr_mac->TX_req[CC_id].tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
		  nr_mac->TX_req[CC_id].header.message_id = NFAPI_TX_REQUEST;
		  // ------------------------------------------------------------------------------------------------
	  }
  } //for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++)
}


