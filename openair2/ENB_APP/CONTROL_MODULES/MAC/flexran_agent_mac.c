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

/*! \file flexran_agent_mac.c
 * \brief FlexRAN agent message handler for MAC layer
 * \author Xenofon Foukas, Mohamed Kassem and Navid Nikaein
 * \date 2016
 * \version 0.1
 */

#include "flexran_agent_mac.h"
#include "flexran_agent_extern.h"
#include "flexran_agent_common.h"
#include "flexran_agent_mac_internal.h"
#include "flexran_agent_net_comm.h"

#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/flexran_agent_mac_proto.h"
#include "LAYER2/MAC/flexran_agent_scheduler_dlsch_ue_remote.h"

#include "liblfds700.h"

#include "log.h"


/*Flags showing if a mac agent has already been registered*/
unsigned int mac_agent_registered[NUM_MAX_ENB];

/*Array containing the Agent-MAC interfaces*/
AGENT_MAC_xface *agent_mac_xface[NUM_MAX_ENB];

/* Ringbuffer related structs used for maintaining the dl mac config messages */
//message_queue_t *dl_mac_config_queue;
struct lfds700_misc_prng_state ps[NUM_MAX_ENB];
struct lfds700_ringbuffer_element *dl_mac_config_array[NUM_MAX_ENB];
struct lfds700_ringbuffer_state ringbuffer_state[NUM_MAX_ENB];


int flexran_agent_mac_handle_stats(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg){

  // TODO: Must deal with sanitization of input
  // TODO: Must check if RNTIs and cell ids of the request actually exist
  // TODO: Must resolve conflicts among stats requests

  int i;
  err_code_t err_code;
  xid_t xid;
  uint32_t usec_interval, sec_interval;

  //TODO: We do not deal with multiple CCs at the moment and eNB id is 0
  int enb_id = mod_id;

  //eNB_MAC_INST *eNB = &eNB_mac_inst[enb_id];
  //UE_list_t *eNB_UE_list=  &eNB->UE_list;

  report_config_t report_config;

  uint32_t ue_flags = 0;
  uint32_t c_flags = 0;

  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;

  Protocol__FlexStatsRequest *stats_req = input->stats_request_msg;
  xid = (stats_req->header)->xid;

  // Check the type of request that is made
  switch(stats_req->body_case) {
  case PROTOCOL__FLEX_STATS_REQUEST__BODY_COMPLETE_STATS_REQUEST: ;
    Protocol__FlexCompleteStatsRequest *comp_req = stats_req->complete_stats_request;
    if (comp_req->report_frequency == PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_OFF) {
      /*Disable both periodic and continuous updates*/
      flexran_agent_disable_cont_mac_stats_update(mod_id);
      flexran_agent_destroy_timer_by_task_id(xid);
      *msg = NULL;
      return 0;
    } else { //One-off, periodical or continuous reporting
      //Set the proper flags
      ue_flags = comp_req->ue_report_flags;
      c_flags = comp_req->cell_report_flags;
      //Create a list of all eNB RNTIs and cells

      //Set the number of UEs and create list with their RNTIs stats configs
      report_config.nr_ue = flexran_get_num_ues(mod_id); //eNB_UE_list->num_UEs
      report_config.ue_report_type = (ue_report_type_t *) malloc(sizeof(ue_report_type_t) * report_config.nr_ue);
      if (report_config.ue_report_type == NULL) {
	// TODO: Add appropriate error code
	err_code = -100;
	goto error;
      }
      for (i = 0; i < report_config.nr_ue; i++) {
	report_config.ue_report_type[i].ue_rnti = flexran_get_ue_crnti(enb_id, i); //eNB_UE_list->eNB_UE_stats[UE_PCCID(enb_id,i)][i].crnti;
	report_config.ue_report_type[i].ue_report_flags = ue_flags;
      }
      //Set the number of CCs and create a list with the cell stats configs
      report_config.nr_cc = MAX_NUM_CCs;
      report_config.cc_report_type = (cc_report_type_t *) malloc(sizeof(cc_report_type_t) * report_config.nr_cc);
      if (report_config.cc_report_type == NULL) {
	// TODO: Add appropriate error code
	err_code = -100;
	goto error;
      }
      for (i = 0; i < report_config.nr_cc; i++) {
	//TODO: Must fill in the proper cell ids
	report_config.cc_report_type[i].cc_id = i;
	report_config.cc_report_type[i].cc_report_flags = c_flags;
      }
      /* Check if request was periodical */
      if (comp_req->report_frequency == PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_PERIODICAL) {
	/* Create a one off flexran message as an argument for the periodical task */
	Protocol__FlexranMessage *timer_msg;
	stats_request_config_t request_config;
	request_config.report_type = PROTOCOL__FLEX_STATS_TYPE__FLST_COMPLETE_STATS;
	request_config.report_frequency = PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_ONCE;
	request_config.period = 0;
	/* Need to make sure that the ue flags are saved (Bug) */
	if (report_config.nr_ue == 0) {
	  report_config.nr_ue = 1;
	  report_config.ue_report_type = (ue_report_type_t *) malloc(sizeof(ue_report_type_t));
	   if (report_config.ue_report_type == NULL) {
	     // TODO: Add appropriate error code
	     err_code = -100;
	     goto error;
	   }
	   report_config.ue_report_type[0].ue_rnti = 0; // Dummy value
	   report_config.ue_report_type[0].ue_report_flags = ue_flags;
	}
	request_config.config = &report_config;
	flexran_agent_mac_stats_request(enb_id, xid, &request_config, &timer_msg);
	/* Create a timer */
	long timer_id = 0;
	flexran_agent_timer_args_t *timer_args;
	timer_args = malloc(sizeof(flexran_agent_timer_args_t));
	memset (timer_args, 0, sizeof(flexran_agent_timer_args_t));
	timer_args->mod_id = enb_id;
	timer_args->msg = timer_msg;
	/*Convert subframes to usec time*/
	usec_interval = 1000*comp_req->sf;
	sec_interval = 0;
	/*add seconds if required*/
	if (usec_interval >= 1000*1000) {
	  sec_interval = usec_interval/(1000*1000);
	  usec_interval = usec_interval%(1000*1000);
	}
	flexran_agent_create_timer(sec_interval, usec_interval, FLEXRAN_AGENT_DEFAULT, enb_id, FLEXRAN_AGENT_TIMER_TYPE_PERIODIC, xid, flexran_agent_handle_timed_task,(void*) timer_args, &timer_id);
      } else if (comp_req->report_frequency == PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_CONTINUOUS) {
	/*If request was for continuous updates, disable the previous configuration and
	  set up a new one*/
	flexran_agent_disable_cont_mac_stats_update(mod_id);
	stats_request_config_t request_config;
	request_config.report_type = PROTOCOL__FLEX_STATS_TYPE__FLST_COMPLETE_STATS;
	request_config.report_frequency = PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_ONCE;
	request_config.period = 0;
	/* Need to make sure that the ue flags are saved (Bug) */
	if (report_config.nr_ue == 0) {
	  report_config.nr_ue = 1;
	  report_config.ue_report_type = (ue_report_type_t *) malloc(sizeof(ue_report_type_t));
	  if (report_config.ue_report_type == NULL) {
	    // TODO: Add appropriate error code
	    err_code = -100;
	    goto error;
	  }
	  report_config.ue_report_type[0].ue_rnti = 0; // Dummy value
	  report_config.ue_report_type[0].ue_report_flags = ue_flags;
	}
	request_config.config = &report_config;
	flexran_agent_enable_cont_mac_stats_update(enb_id, xid, &request_config);
      }
    }
    break;
  case PROTOCOL__FLEX_STATS_REQUEST__BODY_CELL_STATS_REQUEST:;
    Protocol__FlexCellStatsRequest *cell_req = stats_req->cell_stats_request;
    // UE report config will be blank
    report_config.nr_ue = 0;
    report_config.ue_report_type = NULL;
    report_config.nr_cc = cell_req->n_cell;
    report_config.cc_report_type = (cc_report_type_t *) malloc(sizeof(cc_report_type_t) * report_config.nr_cc);
    if (report_config.cc_report_type == NULL) {
      // TODO: Add appropriate error code
      err_code = -100;
      goto error;
    }
    for (i = 0; i < report_config.nr_cc; i++) {
	//TODO: Must fill in the proper cell ids
      report_config.cc_report_type[i].cc_id = cell_req->cell[i];
      report_config.cc_report_type[i].cc_report_flags = cell_req->flags;
    }
    break;
  case PROTOCOL__FLEX_STATS_REQUEST__BODY_UE_STATS_REQUEST:;
    Protocol__FlexUeStatsRequest *ue_req = stats_req->ue_stats_request;
    // Cell report config will be blank
    report_config.nr_cc = 0;
    report_config.cc_report_type = NULL;
    report_config.nr_ue = ue_req->n_rnti;
    report_config.ue_report_type = (ue_report_type_t *) malloc(sizeof(ue_report_type_t) * report_config.nr_ue);
    if (report_config.ue_report_type == NULL) {
      // TODO: Add appropriate error code
      err_code = -100;
      goto error;
    }
    for (i = 0; i < report_config.nr_ue; i++) {
      report_config.ue_report_type[i].ue_rnti = ue_req->rnti[i];
      report_config.ue_report_type[i].ue_report_flags = ue_req->flags;
    }
    break;
  default:
    //TODO: Add appropriate error code
    err_code = -100;
    goto error;
  }

  if (flexran_agent_mac_stats_reply(enb_id, xid, &report_config, msg) < 0 ){
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_BUILD;
    goto error;
  }

  free(report_config.ue_report_type);
  free(report_config.cc_report_type);

  return 0;

 error :
  LOG_E(FLEXRAN_AGENT, "errno %d occured\n", err_code);
  return err_code;
}

int flexran_agent_mac_stats_request(mid_t mod_id,
				    xid_t xid,
				    const stats_request_config_t *report_config,
				    Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header;
  int i;

  Protocol__FlexStatsRequest *stats_request_msg;
  stats_request_msg = malloc(sizeof(Protocol__FlexStatsRequest));
  if(stats_request_msg == NULL)
    goto error;
  protocol__flex_stats_request__init(stats_request_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_STATS_REQUEST, &header) != 0)
    goto error;

  stats_request_msg->header = header;

  stats_request_msg->type = report_config->report_type;
  stats_request_msg->has_type = 1;

  switch (report_config->report_type) {
  case PROTOCOL__FLEX_STATS_TYPE__FLST_COMPLETE_STATS:
    stats_request_msg->body_case =  PROTOCOL__FLEX_STATS_REQUEST__BODY_COMPLETE_STATS_REQUEST;
    Protocol__FlexCompleteStatsRequest *complete_stats;
    complete_stats = malloc(sizeof(Protocol__FlexCompleteStatsRequest));
    if(complete_stats == NULL)
      goto error;
    protocol__flex_complete_stats_request__init(complete_stats);
    complete_stats->report_frequency = report_config->report_frequency;
    complete_stats->has_report_frequency = 1;
    complete_stats->sf = report_config->period;
    complete_stats->has_sf = 1;
    complete_stats->has_cell_report_flags = 1;
    complete_stats->has_ue_report_flags = 1;
    if (report_config->config->nr_cc > 0) {
      complete_stats->cell_report_flags = report_config->config->cc_report_type[0].cc_report_flags;
    }
    if (report_config->config->nr_ue > 0) {
      complete_stats->ue_report_flags = report_config->config->ue_report_type[0].ue_report_flags;
    }
    stats_request_msg->complete_stats_request = complete_stats;
    break;
  case  PROTOCOL__FLEX_STATS_TYPE__FLST_CELL_STATS:
    stats_request_msg->body_case = PROTOCOL__FLEX_STATS_REQUEST__BODY_CELL_STATS_REQUEST;
     Protocol__FlexCellStatsRequest *cell_stats;
     cell_stats = malloc(sizeof(Protocol__FlexCellStatsRequest));
    if(cell_stats == NULL)
      goto error;
    protocol__flex_cell_stats_request__init(cell_stats);
    cell_stats->n_cell = report_config->config->nr_cc;
    cell_stats->has_flags = 1;
    if (cell_stats->n_cell > 0) {
      uint32_t *cells;
      cells = (uint32_t *) malloc(sizeof(uint32_t)*cell_stats->n_cell);
      for (i = 0; i < cell_stats->n_cell; i++) {
	cells[i] = report_config->config->cc_report_type[i].cc_id;
      }
      cell_stats->cell = cells;
      cell_stats->flags = report_config->config->cc_report_type[i].cc_report_flags;
    }
    stats_request_msg->cell_stats_request = cell_stats;
    break;
  case PROTOCOL__FLEX_STATS_TYPE__FLST_UE_STATS:
    stats_request_msg->body_case = PROTOCOL__FLEX_STATS_REQUEST__BODY_UE_STATS_REQUEST;
     Protocol__FlexUeStatsRequest *ue_stats;
     ue_stats = malloc(sizeof(Protocol__FlexUeStatsRequest));
    if(ue_stats == NULL)
      goto error;
    protocol__flex_ue_stats_request__init(ue_stats);
    ue_stats->n_rnti = report_config->config->nr_ue;
    ue_stats->has_flags = 1;
    if (ue_stats->n_rnti > 0) {
      uint32_t *ues;
      ues = (uint32_t *) malloc(sizeof(uint32_t)*ue_stats->n_rnti);
      for (i = 0; i < ue_stats->n_rnti; i++) {
	ues[i] = report_config->config->ue_report_type[i].ue_rnti;
      }
      ue_stats->rnti = ues;
      ue_stats->flags = report_config->config->ue_report_type[i].ue_report_flags;
    }
    stats_request_msg->ue_stats_request = ue_stats;
    break;
  default:
    goto error;
  }
  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->stats_request_msg = stats_request_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (stats_request_msg != NULL)
    free(stats_request_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_destroy_stats_request(Protocol__FlexranMessage *msg) {
   if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REQUEST_MSG)
    goto error;
  free(msg->stats_request_msg->header);
  if (msg->stats_request_msg->body_case == PROTOCOL__FLEX_STATS_REQUEST__BODY_CELL_STATS_REQUEST) {
    free(msg->stats_request_msg->cell_stats_request->cell);
  }
  if (msg->stats_request_msg->body_case == PROTOCOL__FLEX_STATS_REQUEST__BODY_UE_STATS_REQUEST) {
    free(msg->stats_request_msg->ue_stats_request->rnti);
  }
  free(msg->stats_request_msg);
  free(msg);
  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_stats_reply(mid_t mod_id,
				  xid_t xid,
				  const report_config_t *report_config,
				  Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header;
  int i, j, k;
  int enb_id = mod_id;
  //eNB_MAC_INST *eNB = &eNB_mac_inst[enb_id];
  //UE_list_t *eNB_UE_list=  &eNB->UE_list;

  Protocol__FlexStatsReply *stats_reply_msg;
  stats_reply_msg = malloc(sizeof(Protocol__FlexStatsReply));
  if (stats_reply_msg == NULL)
    goto error;
  protocol__flex_stats_reply__init(stats_reply_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_STATS_REPLY, &header) != 0)
    goto error;

  stats_reply_msg->header = header;

  stats_reply_msg->n_ue_report = report_config->nr_ue;
  stats_reply_msg->n_cell_report = report_config->nr_cc;

  Protocol__FlexUeStatsReport **ue_report;
  Protocol__FlexCellStatsReport **cell_report;


  /* Allocate memory for list of UE reports */
  if (report_config->nr_ue > 0) {
    ue_report = malloc(sizeof(Protocol__FlexUeStatsReport *) * report_config->nr_ue);
    if (ue_report == NULL)
      goto error;
    for (i = 0; i < report_config->nr_ue; i++) {
      ue_report[i] = malloc(sizeof(Protocol__FlexUeStatsReport));
      protocol__flex_ue_stats_report__init(ue_report[i]);
      ue_report[i]->rnti = report_config->ue_report_type[i].ue_rnti;
      ue_report[i]->has_rnti = 1;
      ue_report[i]->flags = report_config->ue_report_type[i].ue_report_flags;
      ue_report[i]->has_flags = 1;
      /* Check the types of reports that need to be constructed based on flag values */

      /* Check flag for creation of buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_BSR) {
	ue_report[i]->n_bsr = 4;
	uint32_t *elem;
	elem = (uint32_t *) malloc(sizeof(uint32_t)*ue_report[i]->n_bsr);
	if (elem == NULL)
	  goto error;
	for (j = 0; j < ue_report[i]->n_bsr; j++) {
	  // NN: we need to know the cc_id here, consider the first one
	  elem[j] = flexran_get_ue_bsr (enb_id, i, j); 
	}
	ue_report[i]->bsr = elem;
      }

      /* Check flag for creation of PRH report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PRH) {
	ue_report[i]->phr = flexran_get_ue_phr (enb_id, i); // eNB_UE_list->UE_template[UE_PCCID(enb_id,i)][i].phr_info;
	ue_report[i]->has_phr = 1;
      }

      /* Check flag for creation of RLC buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_RLC_BS) {
	ue_report[i]->n_rlc_report = 3; // Set this to the number of LCs for this UE. This needs to be generalized for for LCs
	Protocol__FlexRlcBsr ** rlc_reports;
	rlc_reports = malloc(sizeof(Protocol__FlexRlcBsr *) * ue_report[i]->n_rlc_report);
	if (rlc_reports == NULL)
	  goto error;

	// NN: see LAYER2/openair2_proc.c for rlc status
	for (j = 0; j < ue_report[i]->n_rlc_report; j++) {
	  rlc_reports[j] = malloc(sizeof(Protocol__FlexRlcBsr));
	  if (rlc_reports[j] == NULL)
	    goto error;
	  protocol__flex_rlc_bsr__init(rlc_reports[j]);
	  rlc_reports[j]->lc_id = j + 1;
	  rlc_reports[j]->has_lc_id = 1;
	  rlc_reports[j]->tx_queue_size = flexran_get_tx_queue_size(enb_id,i,j + 1);
	  rlc_reports[j]->has_tx_queue_size = 1;
	  
	  //TODO:Set tx queue head of line delay in ms
	  rlc_reports[j]->tx_queue_hol_delay = flexran_get_hol_delay(enb_id, i, j+1);
	  rlc_reports[j]->has_tx_queue_hol_delay = 1;
	  //TODO:Set retransmission queue size in bytes
	  rlc_reports[j]->retransmission_queue_size = 10;
	  rlc_reports[j]->has_retransmission_queue_size = 0;
	  //TODO:Set retransmission queue head of line delay in ms
	  rlc_reports[j]->retransmission_queue_hol_delay = 100;
	  rlc_reports[j]->has_retransmission_queue_hol_delay = 0;
	  //TODO:Set current size of the pending message in bytes
	  rlc_reports[j]->status_pdu_size = 100;
	  rlc_reports[j]->has_status_pdu_size = 0;
	}
	// Add RLC buffer status reports to the full report
	if (ue_report[i]->n_rlc_report > 0)
	  ue_report[i]->rlc_report = rlc_reports;
      }

      /* Check flag for creation of MAC CE buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_CE_BS) {
	// TODO: Fill in the actual MAC CE buffer status report
	ue_report[i]->pending_mac_ces = (flexran_get_MAC_CE_bitmap_TA(enb_id,i,0) | (0 << 1) | (0 << 2) | (0 << 3)) & 15;  /* Use as bitmap. Set one or more of the
					       PROTOCOL__FLEX_CE_TYPE__FLPCET_ values
					       found in stats_common.pb-c.h. See
					       flex_ce_type in FlexRAN specification */
	ue_report[i]->has_pending_mac_ces = 1;
      }

      /* Check flag for creation of DL CQI report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_DL_CQI) {
	// TODO: Fill in the actual DL CQI report for the UE based on its configuration
	Protocol__FlexDlCqiReport * dl_report;
	dl_report = malloc(sizeof(Protocol__FlexDlCqiReport));
	if (dl_report == NULL)
	  goto error;
	protocol__flex_dl_cqi_report__init(dl_report);

	dl_report->sfn_sn = flexran_get_sfn_sf(enb_id);
	dl_report->has_sfn_sn = 1;
	//Set the number of DL CQI reports for this UE. One for each CC
	dl_report->n_csi_report = flexran_get_active_CC(enb_id,i);

	//Create the actual CSI reports.
	Protocol__FlexDlCsi **csi_reports;
	csi_reports = malloc(sizeof(Protocol__FlexDlCsi *)*dl_report->n_csi_report);
	if (csi_reports == NULL)
	  goto error;
	for (j = 0; j < dl_report->n_csi_report; j++) {
	  csi_reports[j] = malloc(sizeof(Protocol__FlexDlCsi));
	  if (csi_reports[j] == NULL)
	    goto error;
	  protocol__flex_dl_csi__init(csi_reports[j]);
	  //The servCellIndex for this report
	  csi_reports[j]->serv_cell_index = j;
	  csi_reports[j]->has_serv_cell_index = 1;
	  //The rank indicator value for this cc
	  csi_reports[j]->ri = flexran_get_current_RI(enb_id,i,j);
	  csi_reports[j]->has_ri = 1;
	  //TODO: the type of CSI report based on the configuration of the UE
	  //For now we only support type P10, which only needs a wideband value
	  //The full set of types can be found in stats_common.pb-c.h and
	  //in the FlexRAN specifications
    csi_reports[j]->type =  PROTOCOL__FLEX_CSI_TYPE__FLCSIT_P10;
		  csi_reports[j]->has_type = 1;
		  csi_reports[j]->report_case = PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI;
		  if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI){
			  Protocol__FlexCsiP10 *csi10;
			  csi10 = malloc(sizeof(Protocol__FlexCsiP10));
			  if (csi10 == NULL)
				goto error;
			  protocol__flex_csi_p10__init(csi10);
			  //TODO: set the wideband value
			  // NN: this is also depends on cc_id
			  csi10->wb_cqi = flexran_get_ue_wcqi (enb_id, i); //eNB_UE_list->eNB_UE_stats[UE_PCCID(enb_id,i)][i].dl_cqi;
			  csi10->has_wb_cqi = 1;
			  //Add the type of measurements to the csi report in the proper union type
			  csi_reports[j]->p10csi = csi10;
		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P11CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P20CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P21CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A12CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A22CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A20CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A30CSI){

		  }
		  else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A31CSI){

		  }
		}
	//Add the csi reports to the full DL CQI report
	dl_report->csi_report = csi_reports;
	//Add the DL CQI report to the stats report
	ue_report[i]->dl_cqi_report = dl_report;
      }

      /* Check flag for creation of paging buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PBS) {
	//TODO: Fill in the actual paging buffer status report. For this field to be valid, the RNTI
	//set in the report must be a P-RNTI
	Protocol__FlexPagingBufferReport *paging_report;
	paging_report = malloc(sizeof(Protocol__FlexPagingBufferReport));
	if (paging_report == NULL)
	  goto error;
	protocol__flex_paging_buffer_report__init(paging_report);
	//Set the number of pending paging messages
	paging_report->n_paging_info = 1;
	//Provide a report for each pending paging message
	Protocol__FlexPagingInfo **p_info;
	p_info = malloc(sizeof(Protocol__FlexPagingInfo *) * paging_report->n_paging_info);
	if (p_info == NULL)
	  goto error;
	for (j = 0; j < paging_report->n_paging_info; j++) {
	  p_info[j] = malloc(sizeof(Protocol__FlexPagingInfo));
	  if(p_info[j] == NULL)
	    goto error;
	  protocol__flex_paging_info__init(p_info[j]);
	  //TODO: Set paging index. This index is the same that will be used for the scheduling of the
	  //paging message by the controller
	  p_info[j]->paging_index = 10;
	  p_info[j]->has_paging_index = 0;
	  //TODO:Set the paging message size
	  p_info[j]->paging_message_size = 100;
	  p_info[j]->has_paging_message_size = 0;
	  //TODO: Set the paging subframe
	  p_info[j]->paging_subframe = 10;
	  p_info[j]->has_paging_subframe = 0;
	  //TODO: Set the carrier index for the pending paging message
	  p_info[j]->carrier_index = 0;
	  p_info[j]->has_carrier_index = 0;
	}
	//Add all paging info to the paging buffer rerport
	paging_report->paging_info = p_info;
	//Add the paging report to the UE report
	ue_report[i]->pbr = paging_report;
      }

      /* Check flag for creation of UL CQI report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_UL_CQI) {
	//Fill in the full UL CQI report of the UE
	Protocol__FlexUlCqiReport *full_ul_report;
	full_ul_report = malloc(sizeof(Protocol__FlexUlCqiReport));
	if(full_ul_report == NULL)
	  goto error;
	protocol__flex_ul_cqi_report__init(full_ul_report);
	//TODO:Set the SFN and SF of the generated report
	full_ul_report->sfn_sn = flexran_get_sfn_sf(enb_id);
	full_ul_report->has_sfn_sn = 1;
	//TODO:Set the number of UL measurement reports based on the types of measurements
	//configured for this UE and on the servCellIndex
	full_ul_report->n_cqi_meas = 1;
	Protocol__FlexUlCqi **ul_report;
	ul_report = malloc(sizeof(Protocol__FlexUlCqi *) * full_ul_report->n_cqi_meas);
	if(ul_report == NULL)
	  goto error;
	//Fill each UL report of the UE for each of the configured report types
	for(j = 0; j < full_ul_report->n_cqi_meas; j++) {
	  ul_report[j] = malloc(sizeof(Protocol__FlexUlCqi));
	  if(ul_report[j] == NULL)
	  goto error;
	  protocol__flex_ul_cqi__init(ul_report[j]);
	  //TODO: Set the type of the UL report. As an example set it to SRS UL report
	  // See enum flex_ul_cqi_type in FlexRAN specification for more details
	  ul_report[j]->type = PROTOCOL__FLEX_UL_CQI_TYPE__FLUCT_SRS;
	  ul_report[j]->has_type = 1;
	  //TODO:Set the number of SINR measurements based on the report type
	  //See struct flex_ul_cqi in FlexRAN specification for more details
	  ul_report[j]->n_sinr = 0;
	  uint32_t *sinr_meas;
	  sinr_meas = (uint32_t *) malloc(sizeof(uint32_t) * ul_report[j]->n_sinr);
	  if (sinr_meas == NULL)
	    goto error;
	  //TODO:Set the SINR measurements for the specified type
	  for (k = 0; k < ul_report[j]->n_sinr; k++) {
	    sinr_meas[k] = 10;
	  }
	  ul_report[j]->sinr = sinr_meas;
	  //TODO: Set the servCellIndex for this report
	  ul_report[j]->serv_cell_index = 0;
	  ul_report[j]->has_serv_cell_index = 1;
	  
	  //Set the list of UL reports of this UE to the full UL report
	  full_ul_report->cqi_meas = ul_report;

	  full_ul_report->n_pucch_dbm = MAX_NUM_CCs;
	  full_ul_report->pucch_dbm = malloc(sizeof(Protocol__FlexPucchDbm *) * full_ul_report->n_pucch_dbm);

	  for (j = 0; j < MAX_NUM_CCs; j++) {
	    full_ul_report->pucch_dbm[j] = malloc(sizeof(Protocol__FlexPucchDbm));
	    protocol__flex_pucch_dbm__init(full_ul_report->pucch_dbm[j]);
	    full_ul_report->pucch_dbm[j]->has_serv_cell_index = 1;
	    full_ul_report->pucch_dbm[j]->serv_cell_index = j;
	    if(flexran_get_p0_pucch_dbm(enb_id,i, j) != -1){
	      full_ul_report->pucch_dbm[j]->p0_pucch_dbm = flexran_get_p0_pucch_dbm(enb_id,i,j);
	      full_ul_report->pucch_dbm[j]->has_p0_pucch_dbm = 1;
	    }
	    full_ul_report->pucch_dbm[j]->has_p0_pucch_updated = 1;
	    full_ul_report->pucch_dbm[j]->p0_pucch_updated = flexran_get_p0_pucch_status(enb_id, i, j);
	  }

	  //Add full UL CQI report to the UE report
	  ue_report[i]->ul_cqi_report = full_ul_report;
	}
      }
    }
    /* Add list of all UE reports to the message */
    stats_reply_msg->ue_report = ue_report;
  }

  /* Allocate memory for list of cell reports */
  if (report_config->nr_cc > 0) {
    cell_report = malloc(sizeof(Protocol__FlexCellStatsReport *) * report_config->nr_cc);
    if (cell_report == NULL)
      goto error;
    // Fill in the Cell reports
    for (i = 0; i < report_config->nr_cc; i++) {
      cell_report[i] = malloc(sizeof(Protocol__FlexCellStatsReport));
      if(cell_report[i] == NULL)
	goto error;
      protocol__flex_cell_stats_report__init(cell_report[i]);
      cell_report[i]->carrier_index = report_config->cc_report_type[i].cc_id;
      cell_report[i]->has_carrier_index = 1;
      cell_report[i]->flags = report_config->cc_report_type[i].cc_report_flags;
      cell_report[i]->has_flags = 1;

      /* Check flag for creation of noise and interference report */
      if(report_config->cc_report_type[i].cc_report_flags & PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE) {
	// TODO: Fill in the actual noise and interference report for this cell
	Protocol__FlexNoiseInterferenceReport *ni_report;
	ni_report = malloc(sizeof(Protocol__FlexNoiseInterferenceReport));
	if(ni_report == NULL)
	  goto error;
	protocol__flex_noise_interference_report__init(ni_report);
	// Current frame and subframe number
	ni_report->sfn_sf = flexran_get_sfn_sf(enb_id);
	ni_report->has_sfn_sf = 1;
	//TODO:Received interference power in dbm
	ni_report->rip = 0;
	ni_report->has_rip = 0;
	//TODO:Thermal noise power in dbm
	ni_report->tnp = 0;
	ni_report->has_tnp = 0;

	ni_report->p0_nominal_pucch = flexran_get_p0_nominal_pucch(enb_id, 0);
	ni_report->has_p0_nominal_pucch = 1;
	cell_report[i]->noise_inter_report = ni_report;
      }
    }
    /* Add list of all cell reports to the message */
    stats_reply_msg->cell_report = cell_report;
  }

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->stats_reply_msg = stats_reply_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (stats_reply_msg != NULL)
    free(stats_reply_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_destroy_stats_reply(Protocol__FlexranMessage *msg) {
  //TODO: Need to deallocate memory for the stats reply message
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG)
    goto error;
  free(msg->stats_reply_msg->header);
  int i, j, k;

  Protocol__FlexStatsReply *reply = msg->stats_reply_msg;
  Protocol__FlexDlCqiReport *dl_report;
  Protocol__FlexUlCqiReport *ul_report;
  Protocol__FlexPagingBufferReport *paging_report;

  // Free the memory for the UE reports
  for (i = 0; i < reply->n_ue_report; i++) {
    free(reply->ue_report[i]->bsr);
    for (j = 0; j < reply->ue_report[i]->n_rlc_report; j++) {
      free(reply->ue_report[i]->rlc_report[j]);
    }
    free(reply->ue_report[i]->rlc_report);
    // If DL CQI report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_DL_CQI) {
      dl_report = reply->ue_report[i]->dl_cqi_report;
      // Delete all CSI reports
      for (j = 0; j < dl_report->n_csi_report; j++) {
	//Must free memory based on the type of report
	switch(dl_report->csi_report[j]->report_case) {
	case PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI:
	  free(dl_report->csi_report[j]->p10csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_P11CSI:
	  free(dl_report->csi_report[j]->p11csi->wb_cqi);
	  free(dl_report->csi_report[j]->p11csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_P20CSI:
	  free(dl_report->csi_report[j]->p20csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_P21CSI:
	  free(dl_report->csi_report[j]->p21csi->wb_cqi);
	  free(dl_report->csi_report[j]->p21csi->sb_cqi);
	  free(dl_report->csi_report[j]->p21csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A12CSI:
	  free(dl_report->csi_report[j]->a12csi->wb_cqi);
	  free(dl_report->csi_report[j]->a12csi->sb_pmi);
	  free(dl_report->csi_report[j]->a12csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A22CSI:
	  free(dl_report->csi_report[j]->a22csi->wb_cqi);
	  free(dl_report->csi_report[j]->a22csi->sb_cqi);
	  free(dl_report->csi_report[j]->a22csi->sb_list);
	  free(dl_report->csi_report[j]->a22csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A20CSI:
	  free(dl_report->csi_report[j]->a20csi->sb_list);
	  free(dl_report->csi_report[j]->a20csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A30CSI:
	  free(dl_report->csi_report[j]->a30csi->sb_cqi);
	  free(dl_report->csi_report[j]->a30csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A31CSI:
	  free(dl_report->csi_report[j]->a31csi->wb_cqi);
	  for (k = 0; k < dl_report->csi_report[j]->a31csi->n_sb_cqi; k++) {
	    free(dl_report->csi_report[j]->a31csi->sb_cqi[k]);
	  }
	  free(dl_report->csi_report[j]->a31csi->sb_cqi);
	  break;
	default:
	  break;
	}

	free(dl_report->csi_report[j]);
      }
      free(dl_report->csi_report);
      free(dl_report);
    }
    // If Paging buffer report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PBS) {
      paging_report = reply->ue_report[i]->pbr;
      // Delete all paging buffer reports
      for (j = 0; j < paging_report->n_paging_info; j++) {
	free(paging_report->paging_info[j]);
      }
      free(paging_report->paging_info);
      free(paging_report);
    }
    // If UL CQI report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_UL_CQI) {
      ul_report = reply->ue_report[i]->ul_cqi_report;
      for (j = 0; j < ul_report->n_cqi_meas; j++) {
	free(ul_report->cqi_meas[j]->sinr);
	free(ul_report->cqi_meas[j]);
      }
      free(ul_report->cqi_meas);
      for (j = 0; j < ul_report->n_pucch_dbm; j++) {
	free(ul_report->pucch_dbm[j]);
      }
      free(ul_report->pucch_dbm);
    }
    free(reply->ue_report[i]);
  }
  free(reply->ue_report);

  // Free memory for all Cell reports
  for (i = 0; i < reply->n_cell_report; i++) {
    free(reply->cell_report[i]->noise_inter_report);
    free(reply->cell_report[i]);
  }
  free(reply->cell_report);

  free(reply);
  free(msg);
  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_sr_info(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header;
  int i;
  const int xid = *((int *)params);

  Protocol__FlexUlSrInfo *ul_sr_info_msg;
  ul_sr_info_msg = malloc(sizeof(Protocol__FlexUlSrInfo));
  if (ul_sr_info_msg == NULL) {
    goto error;
  }
  protocol__flex_ul_sr_info__init(ul_sr_info_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_UL_SR_INFO, &header) != 0)
    goto error;

  ul_sr_info_msg->header = header;
  ul_sr_info_msg->has_sfn_sf = 1;
  ul_sr_info_msg->sfn_sf = flexran_get_sfn_sf(mod_id);
  /*TODO: Set the number of UEs that sent an SR */
  ul_sr_info_msg->n_rnti = 1;
  ul_sr_info_msg->rnti = (uint32_t *) malloc(ul_sr_info_msg->n_rnti * sizeof(uint32_t));

  if(ul_sr_info_msg->rnti == NULL) {
    goto error;
  }
  /*TODO:Set the rnti of the UEs that sent an SR */
  for (i = 0; i < ul_sr_info_msg->n_rnti; i++) {
    ul_sr_info_msg->rnti[i] = 1;
  }

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_SR_INFO_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->ul_sr_info_msg = ul_sr_info_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (ul_sr_info_msg != NULL) {
    free(ul_sr_info_msg->rnti);
    free(ul_sr_info_msg);
  }
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_destroy_sr_info(Protocol__FlexranMessage *msg) {
   if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_SR_INFO_MSG)
     goto error;

   free(msg->ul_sr_info_msg->header);
   free(msg->ul_sr_info_msg->rnti);
   free(msg->ul_sr_info_msg);
   free(msg);
   return 0;

 error:
   //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
   return -1;
}

int flexran_agent_mac_sf_trigger(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header;
  int i, j, UE_id;
  
  int available_harq[NUMBER_OF_UE_MAX];
  
  const int xid = *((int *)params);


  Protocol__FlexSfTrigger *sf_trigger_msg;
  sf_trigger_msg = malloc(sizeof(Protocol__FlexSfTrigger));
  if (sf_trigger_msg == NULL) {
    goto error;
  }
  protocol__flex_sf_trigger__init(sf_trigger_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_SF_TRIGGER, &header) != 0)
    goto error;

  frame_t frame;
  sub_frame_t subframe;

  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    available_harq[i] = -1;
  }

  int ahead_of_time = 0;
  
  frame = (frame_t) flexran_get_current_system_frame_num(mod_id);
  subframe = (sub_frame_t) flexran_get_current_subframe(mod_id);

  subframe = ((subframe + ahead_of_time) % 10);
  
  if (subframe < flexran_get_current_subframe(mod_id)) {
    frame = (frame + 1) % 1024;
  }

  int additional_frames = ahead_of_time / 10;
  frame = (frame + additional_frames) % 1024;

  sf_trigger_msg->header = header;
  sf_trigger_msg->has_sfn_sf = 1;
  sf_trigger_msg->sfn_sf = flexran_get_future_sfn_sf(mod_id, 3);

  sf_trigger_msg->n_dl_info = 0;

  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    for (j = 0; j < 8; j++) {
      if (harq_pid_updated[i][j] == 1) {
	available_harq[i] = j;
	sf_trigger_msg->n_dl_info++;
	break;
      }
    }
  }
  

  //  LOG_I(FLEXRAN_AGENT, "Sending subframe trigger for frame %d and subframe %d\n", flexran_get_current_frame(mod_id), (flexran_get_current_subframe(mod_id) + 1) % 10);

  /*TODO: Fill in the number of dl HARQ related info, based on the number of currently
   *transmitting UEs
   */
  //  sf_trigger_msg->n_dl_info = flexran_get_num_ues(mod_id);

  Protocol__FlexDlInfo **dl_info = NULL;

  if (sf_trigger_msg->n_dl_info > 0) {
    dl_info = malloc(sizeof(Protocol__FlexDlInfo *) * sf_trigger_msg->n_dl_info);
    if(dl_info == NULL)
      goto error;
    i = -1;
    //Fill the status of the current HARQ process for each UE
    for(UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
      if (available_harq[UE_id] < 0) {
	continue;
      } else {
	i++;
      }
      dl_info[i] = malloc(sizeof(Protocol__FlexDlInfo));
      if(dl_info[i] == NULL)
	goto error;
      protocol__flex_dl_info__init(dl_info[i]);
      dl_info[i]->rnti = flexran_get_ue_crnti(mod_id, UE_id);
      dl_info[i]->has_rnti = 1;
      /*Fill in the right id of this round's HARQ process for this UE*/
      //      uint8_t harq_id;
      //uint8_t harq_status;
      //      flexran_get_harq(mod_id, UE_PCCID(mod_id,i), i, frame, subframe, &harq_id, &harq_status);
      
      
      dl_info[i]->harq_process_id = available_harq[UE_id];
      harq_pid_updated[UE_id][available_harq[UE_id]] = 0;
      dl_info[i]->has_harq_process_id = 1;
      /* Fill in the status of the HARQ process (2 TBs)*/
      dl_info[i]->n_harq_status = 2;
      dl_info[i]->harq_status = malloc(sizeof(uint32_t) * dl_info[i]->n_harq_status);
      for (j = 0; j < dl_info[i]->n_harq_status; j++) {
	dl_info[i]->harq_status[j] = harq_pid_round[UE_id][available_harq[UE_id]];
	// TODO: This should be different per TB
      }
      //      LOG_I(FLEXRAN_AGENT, "Sending subframe trigger for frame %d and subframe %d and harq %d (round %d)\n", flexran_get_current_frame(mod_id), (flexran_get_current_subframe(mod_id) + 1) % 10, dl_info[i]->harq_process_id, dl_info[i]->harq_status[0]);
      if(dl_info[i]->harq_status[0] > 0) {
	//	LOG_I(FLEXRAN_AGENT, "[Frame %d][Subframe %d]Need to make a retransmission for harq %d (round %d)\n", flexran_get_current_frame(mod_id), flexran_get_current_subframe(mod_id), dl_info[i]->harq_process_id, dl_info[i]->harq_status[0]);
      }
      /*Fill in the serving cell index for this UE */
      dl_info[i]->serv_cell_index = UE_PCCID(mod_id,i);
      dl_info[i]->has_serv_cell_index = 1;
    }
  }

  sf_trigger_msg->dl_info = dl_info;

  /* Fill in the number of UL reception status related info, based on the number of currently
   * transmitting UEs
   */
  sf_trigger_msg->n_ul_info = flexran_get_num_ues(mod_id);

  Protocol__FlexUlInfo **ul_info = NULL;

  if (sf_trigger_msg->n_ul_info > 0) {
    ul_info = malloc(sizeof(Protocol__FlexUlInfo *) * sf_trigger_msg->n_ul_info);
    if(ul_info == NULL)
      goto error;
    //Fill the reception info for each transmitting UE
    for(i = 0; i < sf_trigger_msg->n_ul_info; i++) {
      ul_info[i] = malloc(sizeof(Protocol__FlexUlInfo));
      if(ul_info[i] == NULL)
	goto error;
      protocol__flex_ul_info__init(ul_info[i]);
      ul_info[i]->rnti = flexran_get_ue_crnti(mod_id, i);
      ul_info[i]->has_rnti = 1;
      /*Fill in the Tx power control command for this UE (if available)*/
      if(flexran_get_tpc(mod_id,i) != 1){
    	  ul_info[i]->tpc = flexran_get_tpc(mod_id,i);
          ul_info[i]->has_tpc = 1;
      }
      else{
    	  ul_info[i]->tpc = flexran_get_tpc(mod_id,i);
    	  ul_info[i]->has_tpc = 0;
      }
      /*TODO: fill in the amount of data in bytes in the MAC SDU received in this subframe for the
	given logical channel*/
      ul_info[i]->n_ul_reception = 0;
      ul_info[i]->ul_reception = malloc(sizeof(uint32_t) * ul_info[i]->n_ul_reception);
      for (j = 0; j < ul_info[i]->n_ul_reception; j++) {
	ul_info[i]->ul_reception[j] = 100;
      }
      /*TODO: Fill in the reception status for each UEs data*/
      ul_info[i]->reception_status = PROTOCOL__FLEX_RECEPTION_STATUS__FLRS_OK;
      ul_info[i]->has_reception_status = 1;
      /*Fill in the serving cell index for this UE */
      ul_info[i]->serv_cell_index = UE_PCCID(mod_id,i);
      ul_info[i]->has_serv_cell_index = 1;
    }
  }

  sf_trigger_msg->ul_info = ul_info;

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_SF_TRIGGER_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->sf_trigger_msg = sf_trigger_msg;
  return 0;

 error:
  if (header != NULL)
    free(header);
  if (sf_trigger_msg != NULL) {
    for (i = 0; i < sf_trigger_msg->n_dl_info; i++) {
      free(sf_trigger_msg->dl_info[i]->harq_status);
    }
    free(sf_trigger_msg->dl_info);    
    free(sf_trigger_msg->ul_info);
    free(sf_trigger_msg);
  }
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_destroy_sf_trigger(Protocol__FlexranMessage *msg) {
  int i;
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_SF_TRIGGER_MSG)
    goto error;

  free(msg->sf_trigger_msg->header);
  for (i = 0; i < msg->sf_trigger_msg->n_dl_info; i++) {
    free(msg->sf_trigger_msg->dl_info[i]->harq_status);
    free(msg->sf_trigger_msg->dl_info[i]);
  }
  free(msg->sf_trigger_msg->dl_info);
  for (i = 0; i < msg->sf_trigger_msg->n_ul_info; i++) {
    free(msg->sf_trigger_msg->ul_info[i]->ul_reception);
    free(msg->sf_trigger_msg->ul_info[i]);
  }
  free(msg->sf_trigger_msg->ul_info);
  free(msg->sf_trigger_msg);
  free(msg);

  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_create_empty_dl_config(mid_t mod_id, Protocol__FlexranMessage **msg) {

  int xid = 0;
  Protocol__FlexHeader *header;
  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_DL_MAC_CONFIG, &header) != 0)
    goto error;

  Protocol__FlexDlMacConfig *dl_mac_config_msg;
  dl_mac_config_msg = malloc(sizeof(Protocol__FlexDlMacConfig));
  if (dl_mac_config_msg == NULL) {
    goto error;
  }
  protocol__flex_dl_mac_config__init(dl_mac_config_msg);

  dl_mac_config_msg->header = header;
  dl_mac_config_msg->has_sfn_sf = 1;
  dl_mac_config_msg->sfn_sf = flexran_get_sfn_sf(mod_id);

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_DL_MAC_CONFIG_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->dl_mac_config_msg = dl_mac_config_msg;

  return 0;

 error:
  return -1;
}

int flexran_agent_mac_destroy_dl_config(Protocol__FlexranMessage *msg) {
  int i,j, k;
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_DL_MAC_CONFIG_MSG)
    goto error;

  Protocol__FlexDlDci *dl_dci;

  free(msg->dl_mac_config_msg->header);
  for (i = 0; i < msg->dl_mac_config_msg->n_dl_ue_data; i++) {
    free(msg->dl_mac_config_msg->dl_ue_data[i]->ce_bitmap);
    for (j = 0; j < msg->dl_mac_config_msg->dl_ue_data[i]->n_rlc_pdu; j++) {
      for (k = 0; k <  msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->n_rlc_pdu_tb; k++) {
	free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->rlc_pdu_tb[k]);
      }
      free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->rlc_pdu_tb);
      free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]);
    }
    free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu);
    dl_dci = msg->dl_mac_config_msg->dl_ue_data[i]->dl_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_ue_data[i]);
  }
  free(msg->dl_mac_config_msg->dl_ue_data);

  for (i = 0; i <  msg->dl_mac_config_msg->n_dl_rar; i++) {
    dl_dci = msg->dl_mac_config_msg->dl_rar[i]->rar_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_rar[i]);
  }
  free(msg->dl_mac_config_msg->dl_rar);

  for (i = 0; i < msg->dl_mac_config_msg->n_dl_broadcast; i++) {
    dl_dci = msg->dl_mac_config_msg->dl_broadcast[i]->broad_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_broadcast[i]);
  }
  free(msg->dl_mac_config_msg->dl_broadcast);

    for ( i = 0; i < msg->dl_mac_config_msg->n_ofdm_sym; i++) {
    free(msg->dl_mac_config_msg->ofdm_sym[i]);
  }
  free(msg->dl_mac_config_msg->ofdm_sym);
  free(msg->dl_mac_config_msg);
  free(msg);

  return 0;

 error:
  return -1;
}

void flexran_agent_get_pending_dl_mac_config(mid_t mod_id, Protocol__FlexranMessage **msg) {

  struct lfds700_misc_prng_state ls;
  
  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);
  
  if (lfds700_ringbuffer_read(&ringbuffer_state[mod_id], NULL, (void **) msg, &ls) == 0) {
    *msg = NULL;
  }
}

int flexran_agent_mac_handle_dl_mac_config(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {

  struct lfds700_misc_prng_state ls;
  enum lfds700_misc_flag overwrite_occurred_flag;
  Protocol__FlexranMessage *overwritten_dl_config;
   
  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);
  
  lfds700_ringbuffer_write( &ringbuffer_state[mod_id],
			    NULL,
			    (void *) params,
			    &overwrite_occurred_flag,
			    NULL,
			    (void **)&overwritten_dl_config,
			    &ls);

  if (overwrite_occurred_flag == LFDS700_MISC_FLAG_RAISED) {
    // Delete unmanaged dl_config
    flexran_agent_mac_destroy_dl_config(overwritten_dl_config);
  }
  *msg = NULL;
  return 2;

  // error:
  //*msg = NULL;
  //return -1;
}

void flexran_agent_init_mac_agent(mid_t mod_id) {
  int i, j;
  lfds700_misc_library_init_valid_on_current_logical_core();
  lfds700_misc_prng_init(&ps[mod_id]);
  int num_elements = RINGBUFFER_SIZE + 1;
  //Allow RINGBUFFER_SIZE messages to be stored in the ringbuffer at any time
  dl_mac_config_array[mod_id] = malloc( sizeof(struct lfds700_ringbuffer_element) *  num_elements);
  lfds700_ringbuffer_init_valid_on_current_logical_core( &ringbuffer_state[mod_id], dl_mac_config_array[mod_id], num_elements, &ps[mod_id], NULL );
  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    for (j = 0; j < 8; j++) {
      harq_pid_updated[i][j] = 0;
    }
  }
}

/***********************************************
 * FlexRAN agent - technology mac API implementation
 ***********************************************/

void flexran_agent_send_sr_info(mid_t mod_id) {
  int size;
  Protocol__FlexranMessage *msg;
  void *data;
  int priority = 0;
  err_code_t err_code;

  int xid = 0;

  /*TODO: Must use a proper xid*/
  err_code = flexran_agent_mac_sr_info(mod_id, (void *) &xid, &msg);
  if (err_code < 0) {
    goto error;
  }

  if (msg != NULL){
    data=flexran_agent_pack_message(msg, &size);
    /*Send sr info using the MAC channel of the eNB*/
    if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_MAC, data, size, priority)) {
      err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
    return;
  }
 error:
  LOG_D(FLEXRAN_AGENT, "Could not send sr message\n");
}

void flexran_agent_send_sf_trigger(mid_t mod_id) {
  int size;
  Protocol__FlexranMessage *msg;
  void *data;
  int priority = 0;
  err_code_t err_code;

  int xid = 0;

  /*TODO: Must use a proper xid*/
  err_code = flexran_agent_mac_sf_trigger(mod_id, (void *) &xid, &msg);
  if (err_code < 0) {
    goto error;
  }

  if (msg != NULL){
    data=flexran_agent_pack_message(msg, &size);
    /*Send sr info using the MAC channel of the eNB*/
    if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_MAC, data, size, priority)) {
      err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
    return;
  }
 error:
  LOG_D(FLEXRAN_AGENT, "Could not send sf trigger message\n");
}

void flexran_agent_send_update_mac_stats(mid_t mod_id) {

  Protocol__FlexranMessage *current_report = NULL;
  void *data;
  int size;
  err_code_t err_code;
  int priority = 0;
  
  if (pthread_mutex_lock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }

  if (mac_stats_context[mod_id].cont_update == 1) {
  
    /*Create a fresh report with the required flags*/
    err_code = flexran_agent_mac_handle_stats(mod_id, (void *) mac_stats_context[mod_id].stats_req, &current_report);
    if (err_code < 0) {
      goto error;
    }
  }
  /* /\*TODO:Check if a previous reports exists and if yes, generate a report */
  /*  *that is the diff between the old and the new report, */
  /*  *respecting the thresholds. Otherwise send the new report*\/ */
  /* if (mac_stats_context[mod_id].prev_stats_reply != NULL) { */

  /*   msg = flexran_agent_generate_diff_mac_stats_report(current_report, mac_stats_context[mod_id].prev_stats_reply); */

  /*   /\*Destroy the old stats*\/ */
  /*    flexran_agent_destroy_flexran_message(mac_stats_context[mod_id].prev_stats_reply); */
  /* } */
  /* /\*Use the current report for future comparissons*\/ */
  /* mac_stats_context[mod_id].prev_stats_reply = current_report; */


  if (pthread_mutex_unlock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }

  if (current_report != NULL){
    data=flexran_agent_pack_message(current_report, &size);
    /*Send any stats updates using the MAC channel of the eNB*/
    if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_MAC, data, size, priority)) {
      err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
    return;
  }
 error:
  LOG_D(FLEXRAN_AGENT, "Could not send sf trigger message\n");
}

int flexran_agent_register_mac_xface(mid_t mod_id, AGENT_MAC_xface *xface) {
  if (mac_agent_registered[mod_id]) {
    LOG_E(MAC, "MAC agent for eNB %d is already registered\n", mod_id);
    return -1;
  }

  //xface->agent_ctxt = &shared_ctxt[mod_id];
  xface->flexran_agent_send_sr_info = flexran_agent_send_sr_info;
  xface->flexran_agent_send_sf_trigger = flexran_agent_send_sf_trigger;
  xface->flexran_agent_send_update_mac_stats = flexran_agent_send_update_mac_stats;
  xface->flexran_agent_schedule_ue_spec = flexran_schedule_ue_spec_default;
  //xface->flexran_agent_schedule_ue_spec = flexran_schedule_ue_spec_remote;
  xface->flexran_agent_get_pending_dl_mac_config = flexran_agent_get_pending_dl_mac_config;
  xface->flexran_agent_notify_ue_state_change = flexran_agent_ue_state_change;
  
  xface->dl_scheduler_loaded_lib = NULL;

  mac_agent_registered[mod_id] = 1;
  agent_mac_xface[mod_id] = xface;

  return 0;
}

int flexran_agent_unregister_mac_xface(mid_t mod_id, AGENT_MAC_xface *xface) {

  //xface->agent_ctxt = NULL;
  xface->flexran_agent_send_sr_info = NULL;
  xface->flexran_agent_send_sf_trigger = NULL;
  xface->flexran_agent_send_update_mac_stats = NULL;
  xface->flexran_agent_schedule_ue_spec = NULL;
  xface->flexran_agent_get_pending_dl_mac_config = NULL;
  xface->flexran_agent_notify_ue_state_change = NULL;

  xface->dl_scheduler_loaded_lib = NULL;

  mac_agent_registered[mod_id] = 0;
  agent_mac_xface[mod_id] = NULL;

  return 0;
}


/******************************************************
 *Implementations of flexran_agent_mac_internal.h functions
 ******************************************************/

err_code_t flexran_agent_init_cont_mac_stats_update(mid_t mod_id) {

  /*Initialize the Mac stats update structure*/
  /*Initially the continuous update is set to false*/
  mac_stats_context[mod_id].cont_update = 0;
  mac_stats_context[mod_id].is_initialized = 1;
  mac_stats_context[mod_id].stats_req = NULL;
  mac_stats_context[mod_id].prev_stats_reply = NULL;
  mac_stats_context[mod_id].mutex = calloc(1, sizeof(pthread_mutex_t));
  if (mac_stats_context[mod_id].mutex == NULL)
    goto error;
  if (pthread_mutex_init(mac_stats_context[mod_id].mutex, NULL))
    goto error;;

  return 0;

 error:
  return -1;
}

err_code_t flexran_agent_destroy_cont_mac_stats_update(mid_t mod_id) {
  /*Disable the continuous updates for the MAC*/
  mac_stats_context[mod_id].cont_update = 0;
  mac_stats_context[mod_id].is_initialized = 0;
  flexran_agent_destroy_flexran_message(mac_stats_context[mod_id].stats_req);
  flexran_agent_destroy_flexran_message(mac_stats_context[mod_id].prev_stats_reply);
  free(mac_stats_context[mod_id].mutex);

  mac_agent_registered[mod_id] = 0;
  return 1;
}


err_code_t flexran_agent_enable_cont_mac_stats_update(mid_t mod_id,
						      xid_t xid, stats_request_config_t *stats_req) {
  /*Enable the continuous updates for the MAC*/
  if (pthread_mutex_lock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }

  Protocol__FlexranMessage *req_msg;

  flexran_agent_mac_stats_request(mod_id, xid, stats_req, &req_msg);
  mac_stats_context[mod_id].stats_req = req_msg;
  mac_stats_context[mod_id].prev_stats_reply = NULL;

  mac_stats_context[mod_id].cont_update = 1;
  mac_stats_context[mod_id].xid = xid;

  if (pthread_mutex_unlock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }
  return 0;

 error:
  LOG_E(FLEXRAN_AGENT, "mac_stats_context for eNB %d is not initialized\n", mod_id);
  return -1;
}

err_code_t flexran_agent_disable_cont_mac_stats_update(mid_t mod_id) {
  /*Disable the continuous updates for the MAC*/
  if (pthread_mutex_lock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }
  mac_stats_context[mod_id].cont_update = 0;
  mac_stats_context[mod_id].xid = 0;
  if (mac_stats_context[mod_id].stats_req != NULL) {
    flexran_agent_destroy_flexran_message(mac_stats_context[mod_id].stats_req);
  }
  if (mac_stats_context[mod_id].prev_stats_reply != NULL) {
    flexran_agent_destroy_flexran_message(mac_stats_context[mod_id].prev_stats_reply);
  }
  if (pthread_mutex_unlock(mac_stats_context[mod_id].mutex)) {
    goto error;
  }
  return 0;

 error:
  LOG_E(FLEXRAN_AGENT, "mac_stats_context for eNB %d is not initialized\n", mod_id);
  return -1;

}
