/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file 
 * \brief 
 * \author 
 * \date 2016
 * \version 0.1
 */

#include "enb_agent_mac.h"
#include "enb_agent_common.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/RLC/rlc.h"
#include "log.h"

int enb_agent_mac_reply(uint32_t xid, Protocol__ProgranMessage **msg){
  
  void *buffer;
  int size;
  err_code_t err_code;
  // test code 


  // Create and serialize a stats reply message. This would be done by one of the agents
  // Let's assume that we want the power headroom, the pending CEs for UEs 1 & 2 and their
  // DL CQI reports as well as the noise and interference for cell 1
  report_config_t report_config;

  // We set the flags indicating what kind of stats we need for each UE. Both UEs will have
  // the same flags in this example
  uint32_t ue_flags = 0;
  // Set the power headroom flag
  ue_flags |= PROTOCOL__PRP_UE_STATS_TYPE__PRUST_PRH;
  // Set the pending CEs flag
  ue_flags |= PROTOCOL__PRP_UE_STATS_TYPE__PRUST_MAC_CE_BS;
  // Set the DL CQI report flag
  ue_flags |= PROTOCOL__PRP_UE_STATS_TYPE__PRUST_DL_CQI;

  // We do the same with the Cell flags
  uint32_t c_flags = 0;
  // Set the noise and interference flag
  c_flags |= PROTOCOL__PRP_CELL_STATS_TYPE__PRCST_NOISE_INTERFERENCE;

  // We create the appropriate configurations
  ue_report_type_t ue_configs[2];
  cc_report_type_t cell_configs[1];

  // Create the config for UE with RNTI 1
  ue_report_type_t ue1_config;
  ue1_config.ue_rnti = 1;
  ue1_config.ue_report_flags = ue_flags;

  // Do the same for UE with RNTI 2
  ue_report_type_t ue2_config;
  ue2_config.ue_rnti = 2;
  ue2_config.ue_report_flags = ue_flags;

  // Add them to the UE list
  ue_configs[0] = ue1_config;
  ue_configs[1] = ue2_config;
  
  // Do the same for cell with id 1  
  cc_report_type_t c1_config;
  c1_config.cc_id = 1;
  c1_config.cc_report_flags = c_flags;

  // Add them to the cell list
  cell_configs[0] = c1_config;

  //Create the full report configuration
  report_config.nr_ue = 2;
  report_config.nr_cc = 1;
  report_config.ue_report_type = ue_configs;
  report_config.cc_report_type = cell_configs;

  if (enb_agent_mac_stats_reply(xid, &report_config, msg) < 0 ){
    err_code = PROTOCOL__PROGRAN_ERR__MSG_BUILD;
    goto error;
  }

  return 0;

 error :
  LOG_E(ENB_APP, "errno %d occured\n", err_code);
  return err_code;
}

int enb_agent_mac_stats_reply(uint32_t xid,
			      const report_config_t *report_config, 
			      Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  int i, j, k;
  int cc_id = 0;
  int enb_id = 0;
  eNB_MAC_INST *eNB = &eNB_mac_inst[enb_id];
  UE_list_t *eNB_UE_list= &eNB->UE_list;
  
  
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_STATS_REPLY, &header) != 0)
    goto error;

  Protocol__PrpStatsReply *stats_reply_msg;
  stats_reply_msg = malloc(sizeof(Protocol__PrpStatsReply));
  if (stats_reply_msg == NULL)
    goto error;
  protocol__prp_stats_reply__init(stats_reply_msg);
  stats_reply_msg->header = header;

  stats_reply_msg->n_ue_report = report_config->nr_ue;
  stats_reply_msg->n_cell_report = report_config->nr_cc;

  Protocol__PrpUeStatsReport **ue_report;
  Protocol__PrpCellStatsReport **cell_report;

  
  /* Allocate memory for list of UE reports */
  if (report_config->nr_ue > 0) {
    ue_report = malloc(sizeof(Protocol__PrpUeStatsReport *) * report_config->nr_ue);
    if (ue_report == NULL)
      goto error;
    for (i = 0; i < report_config->nr_ue; i++) {
      ue_report[i] = malloc(sizeof(Protocol__PrpUeStatsReport));
      protocol__prp_ue_stats_report__init(ue_report[i]);
      ue_report[i]->rnti = report_config->ue_report_type[i].ue_rnti;
      ue_report[i]->has_rnti = 1;
      ue_report[i]->flags = report_config->ue_report_type[i].ue_report_flags;
      ue_report[i]->has_flags = 1;
      /* Check the types of reports that need to be constructed based on flag values */

      /* Check flag for creation of buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_BSR) {
	//TODO: Create a report for each LCG (4 elements). See prp_ue_stats_report of
	// progRAN specifications for more details
	ue_report[i]->n_bsr = 4;
	uint32_t *elem;
	elem = (uint32_t *) malloc(sizeof(uint32_t)*ue_report[i]->n_bsr);
	if (elem == NULL)
	  goto error;
	for (j = 0; j++; j < ue_report[i]->n_bsr) {
	  // Set the actual BSR for LCG j of the current UE
	  // NN: we need to know the cc_id here, consider the first one
	  elem[j] = eNB_UE_list->UE_template[UE_PCCID(enb_id,i)][i].bsr_info[j];
	}
	ue_report[i]->bsr = elem;
      }
      
      /* Check flag for creation of PRH report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_PRH) {
	// TODO: Fill in the actual power headroom value for the RNTI
	ue_report[i]->phr = eNB_UE_list->UE_template[UE_PCCID(enb_id,i)][i].phr_info;
	ue_report[i]->has_phr = 1;
      }

      /* Check flag for creation of RLC buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_RLC_BS) {
	// TODO: Fill in the actual RLC buffer status reports
	ue_report[i]->n_rlc_report = 1; // Set this to the number of LCs for this UE
	Protocol__PrpRlcBsr ** rlc_reports;
	rlc_reports = malloc(sizeof(Protocol__PrpRlcBsr) * ue_report[i]->n_rlc_report);
	if (rlc_reports == NULL)
	  goto error;
	
	// Fill the buffer status report for each logical channel of the UE
	// NN: see LAYER2/openair2_proc.c for rlc status
	for (j = 0; j < ue_report[i]->n_rlc_report; j++) {
	  rlc_reports[j] = malloc(sizeof(Protocol__PrpRlcBsr));
	  if (rlc_reports[j] == NULL)
	    goto error;
	  protocol__prp_rlc_bsr__init(rlc_reports[j]);
	  //TODO:Set logical channel id
	  rlc_reports[j]->lc_id = 1;
	  rlc_reports[j]->has_lc_id = 1;
	  //TODO:Set tx queue size in bytes
	  rlc_reports[j]->tx_queue_size = 10;
	  rlc_reports[j]->has_tx_queue_size = 1;
	  //TODO:Set tx queue head of line delay in ms
	  rlc_reports[j]->tx_queue_hol_delay = 100;
	  rlc_reports[j]->has_tx_queue_hol_delay = 1;
	  //TODO:Set retransmission queue size in bytes
	  rlc_reports[j]->retransmission_queue_size = 10;
	  rlc_reports[j]->has_retransmission_queue_size = 1;
	  //TODO:Set retransmission queue head of line delay in ms
	  rlc_reports[j]->retransmission_queue_hol_delay = 100;
	  rlc_reports[j]->has_retransmission_queue_hol_delay = 1;
	  //TODO:Set current size of the pending message in bytes
	  rlc_reports[j]->status_pdu_size = 100;
	  rlc_reports[j]->has_status_pdu_size = 1;
	}
	// Add RLC buffer status reports to the full report
	if (ue_report[i]->n_rlc_report > 0)
	  ue_report[i]->rlc_report = rlc_reports;
      }

      /* Check flag for creation of MAC CE buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_MAC_CE_BS) {
	// TODO: Fill in the actual MAC CE buffer status report
	ue_report[i]->pending_mac_ces = -1; /* Use as bitmap. Set one or more of the
					       PROTOCOL__PRP_CE_TYPE__PRPCET_ values
					       found in stats_common.pb-c.h. See
					       prp_ce_type in progRAN specification */
	ue_report[i]->has_pending_mac_ces = 1;
      }
      
      /* Check flag for creation of DL CQI report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_DL_CQI) {
	// TODO: Fill in the actual DL CQI report for the UE based on its configuration
	Protocol__PrpDlCqiReport * dl_report;
	dl_report = malloc(sizeof(Protocol__PrpDlCqiReport));
	if (dl_report == NULL)
	  goto error;
	protocol__prp_dl_cqi_report__init(dl_report);
	//TODO:Set the SFN and SF of the last report held in the agent.
	dl_report->sfn_sn = eNB->frame*10 + eNB->subframe;
	dl_report->has_sfn_sn = 1;
	//TODO:Set the number of DL CQI reports for this UE. One for each CC
	dl_report->n_csi_report = 1;
	//TODO:Create the actual CSI reports.
	Protocol__PrpDlCsi **csi_reports;
	csi_reports = malloc(sizeof(Protocol__PrpDlCsi *));
	if (csi_reports == NULL)
	  goto error;
	for (j = 0; j < dl_report->n_csi_report; j++) {
	  csi_reports[j] = malloc(sizeof(Protocol__PrpDlCsi));
	  if (csi_reports[j] == NULL)
	    goto error;
	  protocol__prp_dl_csi__init(csi_reports[j]);
	  //TODO: the servCellIndex for this report
	  csi_reports[j]->serv_cell_index = 0;
	  csi_reports[j]->has_serv_cell_index = 1;
	  //TODO: the rank indicator value for this cc
	  csi_reports[j]->ri = 1;
	  csi_reports[j]->has_ri = 1;
	  //TODO: the type of CSI report based on the configuration of the UE
	  //For this example we use type P10, which only needs a wideband value
	  //The full set of types can be found in stats_common.pb-c.h and
	  //in the progRAN specifications
	  csi_reports[j]->type =  PROTOCOL__PRP_CSI_TYPE__PRCSIT_P10;
	  csi_reports[j]->has_type = 1;
	  csi_reports[j]->report_case = PROTOCOL__PRP_DL_CSI__REPORT_P10CSI;
	  Protocol__PrpCsiP10 *csi10;
	  csi10 = malloc(sizeof(Protocol__PrpCsiP10));
	  if (csi10 == NULL)
	    goto error;
	  protocol__prp_csi_p10__init(csi10);
	  //TODO: set the wideband value
	  // NN: this is also depends on cc_id
	  csi10->wb_cqi = eNB_UE_list->eNB_UE_stats[UE_PCCID(enb_id,i)][i].dl_cqi;
	  csi10->has_wb_cqi = 1;
	  //Add the type of measurements to the csi report in the proper union type
	  csi_reports[j]->p10csi = csi10;
	}
	//Add the csi reports to the full DL CQI report
	dl_report->csi_report = csi_reports;
	//Add the DL CQI report to the stats report
	ue_report[i]->dl_cqi_report = dl_report;
      }
      
      /* Check flag for creation of paging buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_PBS) {
	//TODO: Fill in the actual paging buffer status report. For this field to be valid, the RNTI
	//set in the report must be a P-RNTI
	Protocol__PrpPagingBufferReport *paging_report;
	paging_report = malloc(sizeof(Protocol__PrpPagingBufferReport));
	if (paging_report == NULL)
	  goto error;
	protocol__prp_paging_buffer_report__init(paging_report);
	//Set the number of pending paging messages
	paging_report->n_paging_info = 1;
	//Provide a report for each pending paging message
	Protocol__PrpPagingInfo **p_info;
	p_info = malloc(sizeof(Protocol__PrpPagingInfo *));
	if (p_info == NULL)
	  goto error;
	for (j = 0; j < paging_report->n_paging_info; j++) {
	  p_info[j] = malloc(sizeof(Protocol__PrpPagingInfo));
	  if(p_info[j] == NULL)
	    goto error;
	  protocol__prp_paging_info__init(p_info[j]);
	  //TODO: Set paging index. This index is the same that will be used for the scheduling of the
	  //paging message by the controller
	  p_info[j]->paging_index = 10;
	  p_info[j]->has_paging_index = 1;
	  //TODO:Set the paging message size
	  p_info[j]->paging_message_size = 100;
	  p_info[j]->has_paging_message_size = 1;
	  //TODO: Set the paging subframe
	  p_info[j]->paging_subframe = 10;
	  p_info[j]->has_paging_subframe = 1;
	  //TODO: Set the carrier index for the pending paging message
	  p_info[j]->carrier_index = 0;
	  p_info[j]->has_carrier_index = 1;
	}
	//Add all paging info to the paging buffer rerport
	paging_report->paging_info = p_info;
	//Add the paging report to the UE report
	ue_report[i]->pbr = paging_report;
      }
      
      /* Check flag for creation of UL CQI report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_UL_CQI) {
	//Fill in the full UL CQI report of the UE
	Protocol__PrpUlCqiReport *full_ul_report;
	full_ul_report = malloc(sizeof(Protocol__PrpUlCqiReport));
	if(full_ul_report == NULL)
	  goto error;
	protocol__prp_ul_cqi_report__init(full_ul_report);
	//TODO:Set the SFN and SF of the generated report
	full_ul_report->sfn_sn = 100;
	full_ul_report->has_sfn_sn = 1;
	//TODO:Set the number of UL measurement reports based on the types of measurements
	//configured for this UE and on the servCellIndex
	full_ul_report->n_cqi_meas = 1;
	Protocol__PrpUlCqi **ul_report;
	ul_report = malloc(sizeof(Protocol__PrpUlCqi *) * full_ul_report->n_cqi_meas);
	if(ul_report == NULL)
	  goto error;
	//Fill each UL report of the UE for each of the configured report types
	for(j = 0; j++; j < full_ul_report->n_cqi_meas) {
	  ul_report[j] = malloc(sizeof(Protocol__PrpUlCqi));
	  if(ul_report[j] == NULL)
	  goto error;
	  protocol__prp_ul_cqi__init(ul_report[j]);
	  //TODO: Set the type of the UL report. As an example set it to SRS UL report
	  // See enum prp_ul_cqi_type in progRAN specification for more details
	  ul_report[j]->type = PROTOCOL__PRP_UL_CQI_TYPE__PRUCT_SRS;
	  ul_report[j]->has_type = 1;
	  //TODO:Set the number of SINR measurements based on the report type
	  //See struct prp_ul_cqi in progRAN specification for more details
	  ul_report[j]->n_sinr = 100;
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
    cell_report = malloc(sizeof(Protocol__PrpCellStatsReport *) * report_config->nr_cc);
    if (cell_report == NULL)
      goto error;
    // Fill in the Cell reports
    for (i = 0; i < report_config->nr_cc; i++) {
      cell_report[i] = malloc(sizeof(Protocol__PrpCellStatsReport));
      if(ue_report[i] == NULL)
	goto error;
      protocol__prp_cell_stats_report__init(cell_report[i]);
      cell_report[i]->carrier_index = report_config->cc_report_type[i].cc_id;
      cell_report[i]->has_carrier_index = 1;
      cell_report[i]->flags = report_config->cc_report_type[i].cc_report_flags;
      cell_report[i]->has_flags = 1;

      /* Check flag for creation of noise and interference report */
      if(report_config->cc_report_type[i].cc_report_flags & PROTOCOL__PRP_CELL_STATS_TYPE__PRCST_NOISE_INTERFERENCE) {
	// TODO: Fill in the actual noise and interference report for this cell
	Protocol__PrpNoiseInterferenceReport *ni_report;
	ni_report = malloc(sizeof(Protocol__PrpNoiseInterferenceReport));
	if(ni_report == NULL)
	  goto error;
	protocol__prp_noise_interference_report__init(ni_report);
	// Current frame and subframe number
	ni_report->sfn_sf = 0;
	ni_report->has_sfn_sf = 1;
	// Received interference power in dbm
	ni_report->rip = 0;
	ni_report->has_rip = 1;
	// Thermal noise power in dbm
	ni_report->tnp = 0;
	ni_report->has_tnp = 1;
	cell_report[i]->noise_inter_report = ni_report;
      }
    }
    /* Add list of all cell reports to the message */
    stats_reply_msg->cell_report = cell_report;
  }
  
  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REPLY_MSG;
  (*msg)->msg_dir =  PROTOCOL__PROGRAN_DIRECTION__SUCCESSFUL_OUTCOME;
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

int enb_agent_mac_destroy_stats_reply(Protocol__ProgranMessage *msg) {
  //TODO: Need to deallocate memory for the stats reply message
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REPLY_MSG)
    goto error;
  free(msg->stats_reply_msg->header);
  int i, j, k;
  
  Protocol__PrpStatsReply *reply = msg->stats_reply_msg;
  Protocol__PrpDlCqiReport *dl_report;
  Protocol__PrpUlCqiReport *ul_report;
  Protocol__PrpPagingBufferReport *paging_report;

  // Free the memory for the UE reports
  for (i = 0; i < reply->n_ue_report; i++) {
    free(reply->ue_report[i]->bsr);
    for (j = 0; j < reply->ue_report[i]->n_rlc_report; j++) {
      free(reply->ue_report[i]->rlc_report[j]);
    }
    free(reply->ue_report[i]->rlc_report);
    // If DL CQI report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_DL_CQI) {
      dl_report = reply->ue_report[i]->dl_cqi_report;
      // Delete all CSI reports
      for (j = 0; j < dl_report->n_csi_report; j++) {
	//Must free memory based on the type of report
	switch(dl_report->csi_report[j]->report_case) {
	case PROTOCOL__PRP_DL_CSI__REPORT_P10CSI:
	  free(dl_report->csi_report[j]->p10csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_P11CSI:
	  free(dl_report->csi_report[j]->p11csi->wb_cqi);
	  free(dl_report->csi_report[j]->p11csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_P20CSI:
	  free(dl_report->csi_report[j]->p20csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_P21CSI:
	  free(dl_report->csi_report[j]->p21csi->wb_cqi);
	  free(dl_report->csi_report[j]->p21csi->sb_cqi);
	  free(dl_report->csi_report[j]->p21csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A12CSI:
	  free(dl_report->csi_report[j]->a12csi->wb_cqi);
	  free(dl_report->csi_report[j]->a12csi->sb_pmi);
	  free(dl_report->csi_report[j]->a12csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A22CSI:
	  free(dl_report->csi_report[j]->a22csi->wb_cqi);
	  free(dl_report->csi_report[j]->a22csi->sb_cqi);
	  free(dl_report->csi_report[j]->a22csi->sb_list);
	  free(dl_report->csi_report[j]->a22csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A20CSI:
	  free(dl_report->csi_report[j]->a20csi->sb_list);
	  free(dl_report->csi_report[j]->a20csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A30CSI:
	  free(dl_report->csi_report[j]->a30csi->sb_cqi);
	  free(dl_report->csi_report[j]->a30csi);
	  break;
	case PROTOCOL__PRP_DL_CSI__REPORT_A31CSI:
	  free(dl_report->csi_report[j]->a31csi->wb_cqi);
	  for (k = 0; k < dl_report->csi_report[j]->a31csi->n_sb_cqi; k++) {
	    free(dl_report->csi_report[j]->a31csi->sb_cqi[k]);
	  }
	  free(dl_report->csi_report[j]->a31csi->sb_cqi);
	  break;
	}  
	
	free(dl_report->csi_report[j]);
      }
      free(dl_report->csi_report);
      free(dl_report);
    }
    // If Paging buffer report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_PBS) {
      paging_report = reply->ue_report[i]->pbr;
      // Delete all paging buffer reports
      for (j = 0; j < paging_report->n_paging_info; j++) {
	free(paging_report->paging_info[j]);
      }
      free(paging_report->paging_info);
      free(paging_report);
    }
    // If UL CQI report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__PRP_UE_STATS_TYPE__PRUST_UL_CQI) {
      ul_report = reply->ue_report[i]->ul_cqi_report;
      for (j = 0; j < ul_report->n_cqi_meas; j++) {
	free(ul_report->cqi_meas[j]->sinr);
	free(ul_report->cqi_meas[j]);
      }
      free(ul_report->cqi_meas);
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
