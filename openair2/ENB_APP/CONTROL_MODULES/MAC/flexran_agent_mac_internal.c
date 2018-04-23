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

/*! \file flexran_agent_mac_internal.c
 * \brief Helper functions for the MAC agent
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include <string.h>
#include <dlfcn.h>

#include "flexran_agent_common_internal.h"
#include "flexran_agent_mac_internal.h"

/* from flexran_agent_mac.c */
extern Protocol__FlexSliceConfig *slice_config[NUM_MAX_ENB];
extern Protocol__FlexSliceConfig *sc_update[NUM_MAX_ENB];
extern int perform_slice_config_update_count;
extern Protocol__FlexUeConfig *ue_slice_assoc_update[NUM_MAX_UE];
extern int n_ue_slice_assoc_updates;

Protocol__FlexranMessage * flexran_agent_generate_diff_mac_stats_report(Protocol__FlexranMessage *new_message,
									Protocol__FlexranMessage *old_message) {

  int i, j;
  
  Protocol__FlexStatsReply *old_report, *new_report;

  Protocol__FlexStatsReply *stats_reply_msg = NULL;
  Protocol__FlexranMessage *msg = NULL;
  
  Protocol__FlexUeStatsReport **ue_report;
  Protocol__FlexUeStatsReport *tmp_ue_report[NUM_MAX_UE];
  Protocol__FlexCellStatsReport **cell_report;
  Protocol__FlexCellStatsReport *tmp_cell_report[NUM_MAX_UE];
  
  old_report = old_message->stats_reply_msg;
  new_report = new_message->stats_reply_msg;

  /*See how many and which UE reports should be included in the final stats message*/
  int n_ue_report = 0;
  int ue_found = 0;
  
  /*Go through each RNTI of the new report and see if it exists in the old one*/
  for (i = 0; i < new_report->n_ue_report; i++) {
    for (j = 0; j < old_report->n_ue_report; i++) {
     if (new_report->ue_report[i]->rnti == old_report->ue_report[j]->rnti) {
  	ue_found = 1;
	/*Need to check if something changed*/
	if (compare_ue_stats_reports(new_report->ue_report[i], old_report->ue_report[j]) != 0) {
	  tmp_ue_report[n_ue_report] = copy_ue_stats_report(new_report->ue_report[i]);
	  n_ue_report++;
	}
	break;
     }
    }
    if (!ue_found) {
      tmp_ue_report[n_ue_report] = copy_ue_stats_report(new_report->ue_report[i]);
      n_ue_report++;
    }
    ue_found = 0;
  }

  /*See how many and which cell reports should be included in the final stats message*/
  int n_cell_report = 0;
  int cell_found = 0;
  
  /*Go through each cell of the new report and see if it exists in the old one*/
  for (i = 0; i < new_report->n_cell_report; i++) {
    for (j = 0; j < old_report->n_cell_report; i++) {
     if (new_report->cell_report[i]->carrier_index == old_report->cell_report[j]->carrier_index) {
  	cell_found = 1;
	/*Need to check if something changed*/
	if (compare_cell_stats_reports(new_report->cell_report[i], old_report->cell_report[j]) != 0) {
	  tmp_cell_report[n_cell_report] = copy_cell_stats_report(new_report->cell_report[i]);
	  n_cell_report++;
	}
	break;
     }
    }
    if (!cell_found) {
      tmp_cell_report[n_cell_report] = copy_cell_stats_report(new_report->cell_report[i]);
      n_cell_report++;
    }
    cell_found = 0;
  }
  
  if (n_cell_report > 0 || n_ue_report > 0) {
    /*Create header*/
    int xid = old_report->header->xid;
    Protocol__FlexHeader *header = NULL;
    if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_STATS_REPLY, &header) != 0) {
    goto error;
    }
    stats_reply_msg = malloc(sizeof(Protocol__FlexStatsReply));
    protocol__flex_stats_reply__init(stats_reply_msg);

    stats_reply_msg->header = header;
    
    /*TODO: create the reply message based on the findings*/
    /*Create ue report list*/
    stats_reply_msg->n_ue_report = n_ue_report;
    if (n_ue_report > 0) {
      ue_report = malloc(sizeof(Protocol__FlexUeStatsReport *));
      for (i = 0; i<n_ue_report; i++) {
	ue_report[i] = tmp_ue_report[i];
      }
      stats_reply_msg->ue_report = ue_report;
    }
    
    /*Create cell report list*/
    stats_reply_msg->n_cell_report = n_cell_report;
    if (n_cell_report > 0) {
      cell_report = malloc(sizeof(Protocol__FlexCellStatsReport *));
      for (i = 0; i<n_cell_report; i++) {
	cell_report[i] = tmp_cell_report[i];
      }
      stats_reply_msg->cell_report = cell_report;
    }

    msg = malloc(sizeof(Protocol__FlexranMessage));
    if(msg == NULL)
      goto error;
    protocol__flexran_message__init(msg);
    msg->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG;
    msg->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
    msg->stats_reply_msg = stats_reply_msg;
  }
  return msg;
  
 error:
   return NULL;
}

int compare_ue_stats_reports(Protocol__FlexUeStatsReport *rep1,
			    Protocol__FlexUeStatsReport *rep2) {
  return 1;
}

int compare_cell_stats_reports(Protocol__FlexCellStatsReport *rep1,
			      Protocol__FlexCellStatsReport *rep2) {
  return 1;
}

Protocol__FlexUeStatsReport * copy_ue_stats_report(Protocol__FlexUeStatsReport * original) {
  int i;
  Protocol__FlexUeStatsReport *copy =  malloc(sizeof(Protocol__FlexUeStatsReport));
  if (copy == NULL)
    goto error;
  protocol__flex_ue_stats_report__init(copy);
  copy->rnti = original->rnti;
  copy->has_rnti = original->has_rnti;
  copy->flags = original->flags;
  copy->has_flags = original->has_flags;
  
  if (copy->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_BSR) {
    copy->n_bsr = original->n_bsr;
    if (copy->n_bsr > 0) {
      uint32_t *elem;
      elem = (uint32_t *) malloc(sizeof(uint32_t)*copy->n_bsr);
      if (elem == NULL)
	goto error;
      for (i = 0; i < original->n_bsr; i++) {
	elem[i] = original->bsr[i];
      }
      copy->bsr = elem;
    }
  }

  

   if (copy->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PHR) { 
     copy->has_phr = original->has_phr;
     copy->phr = original->phr;
   }

  if (copy->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_RLC_BS) {
    copy->n_rlc_report = original->n_rlc_report; 
    if (copy->n_rlc_report > 0) {
      Protocol__FlexRlcBsr ** rlc_reports;
      rlc_reports = malloc(sizeof(Protocol__FlexRlcBsr) * copy->n_rlc_report);
      if (rlc_reports == NULL)
	goto error;
      for (i = 0; i < copy->n_rlc_report; i++) {
	rlc_reports[i] = copy_rlc_report(original->rlc_report[i]);
      }
      copy->rlc_report = rlc_reports;
    }
  }

  if (copy->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_CE_BS) {
    copy->has_pending_mac_ces = original->has_pending_mac_ces;
    copy->pending_mac_ces = original->pending_mac_ces;
  
  }
  
  if (copy->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_DL_CQI) {
    copy->dl_cqi_report = copy_dl_cqi_report(original->dl_cqi_report);
  }

  if (copy->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PBS) {  
    /*Copy the Paging Buffer report*/
    copy->pbr = copy_paging_buffer_report(original->pbr);
  }

  if (copy->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_UL_CQI) {
    /*TODO: Copy the UL report*/  
    copy->ul_cqi_report = copy_ul_cqi_report(original->ul_cqi_report);
  }

  return copy;

 error:
  return NULL;
}

Protocol__FlexRlcBsr * copy_rlc_report(Protocol__FlexRlcBsr * original) {
  Protocol__FlexRlcBsr * copy = malloc(sizeof(Protocol__FlexRlcBsr));
  if (copy == NULL)
    goto error;
  protocol__flex_rlc_bsr__init(copy);
  copy->lc_id = original->lc_id;
  copy->has_lc_id = original->has_lc_id;
  copy->tx_queue_size = original->tx_queue_size;
  copy->has_tx_queue_size = original->has_tx_queue_size;
  copy->tx_queue_hol_delay = original->tx_queue_hol_delay;
  copy->has_tx_queue_hol_delay = original->has_tx_queue_hol_delay;
  copy->retransmission_queue_size = original->retransmission_queue_size;
  copy->has_retransmission_queue_size = original->has_retransmission_queue_size;
  copy->retransmission_queue_hol_delay = original->retransmission_queue_hol_delay;
  copy->has_retransmission_queue_hol_delay = original->has_retransmission_queue_hol_delay;
  copy->status_pdu_size = original->status_pdu_size;
  copy->has_status_pdu_size = original->has_status_pdu_size;
  
  return copy;

 error:
  return NULL;
}

Protocol__FlexUlCqiReport * copy_ul_cqi_report(Protocol__FlexUlCqiReport * original) {
  int i, j;
  
  //Fill in the full UL CQI report of the UE
  Protocol__FlexUlCqiReport *full_ul_report;
  full_ul_report = malloc(sizeof(Protocol__FlexUlCqiReport));
  if(full_ul_report == NULL) {
    goto error;
  }
  protocol__flex_ul_cqi_report__init(full_ul_report);
  //TODO:Set the SFN and SF of the generated report
  full_ul_report->sfn_sn = original->sfn_sn;
  full_ul_report->has_sfn_sn = original->has_sfn_sn;
  full_ul_report->n_cqi_meas = original->n_cqi_meas;

  Protocol__FlexUlCqi **ul_report;
  ul_report = malloc(sizeof(Protocol__FlexUlCqi *) * full_ul_report->n_cqi_meas);
  if(ul_report == NULL)
    goto error;
  for(i = 0; i < full_ul_report->n_cqi_meas; i++) {
    ul_report[i] = malloc(sizeof(Protocol__FlexUlCqi));
    if(ul_report[i] == NULL)
      goto error;
    protocol__flex_ul_cqi__init(ul_report[i]);
    ul_report[i]->type = original->cqi_meas[i]->type;
    ul_report[i]->has_type = original->cqi_meas[i]->has_type;
    ul_report[i]->n_sinr = original->cqi_meas[i]->n_sinr;
    uint32_t *sinr_meas;
    sinr_meas = (uint32_t *) malloc(sizeof(uint32_t) * ul_report[i]->n_sinr);
    if (sinr_meas == NULL)
      goto error;
    for (j = 0; j < ul_report[i]->n_sinr; j++) {
      sinr_meas[j] = original->cqi_meas[i]->sinr[j];
    }
    ul_report[i]->sinr = sinr_meas;
    ul_report[i]->serv_cell_index = original->cqi_meas[i]->serv_cell_index;
    ul_report[i]->has_serv_cell_index = original->cqi_meas[i]->has_serv_cell_index;
  }
  full_ul_report->cqi_meas = ul_report;

  return full_ul_report;
  
  error:
    return NULL;
}

Protocol__FlexDlCqiReport * copy_dl_cqi_report(Protocol__FlexDlCqiReport * original) {
  int i;
  /*Copy the DL report*/
  Protocol__FlexDlCqiReport * dl_report;
  dl_report = malloc(sizeof(Protocol__FlexDlCqiReport));
  if (dl_report == NULL)
    goto error;
  protocol__flex_dl_cqi_report__init(dl_report);

  dl_report->sfn_sn = original->sfn_sn;
  dl_report->has_sfn_sn = original->has_sfn_sn;
  dl_report->n_csi_report = original->n_csi_report;

  Protocol__FlexDlCsi **csi_reports;
  csi_reports = malloc(sizeof(Protocol__FlexDlCsi *) * dl_report->n_csi_report);
  if (csi_reports == NULL)
    goto error;
  
  for (i = 0; i < dl_report->n_csi_report; i++) {
    csi_reports[i] = copy_csi_report(original->csi_report[i]); 
  }
  dl_report->csi_report = csi_reports;
  return dl_report;

 error:
  /*TODO: Must free memory properly*/
  return NULL;
}

Protocol__FlexPagingBufferReport * copy_paging_buffer_report(Protocol__FlexPagingBufferReport *original) {
  
  int i;
  Protocol__FlexPagingBufferReport *copy;
  copy = malloc(sizeof(Protocol__FlexPagingBufferReport));
  if (copy == NULL)
    goto error;
  
  protocol__flex_paging_buffer_report__init(copy);
  copy->n_paging_info = original->n_paging_info;
  
  Protocol__FlexPagingInfo **p_info;
  p_info = malloc(sizeof(Protocol__FlexPagingInfo *) * copy->n_paging_info);
  if (p_info == NULL)
    goto error;
  for (i = 0; i < copy->n_paging_info; i++) {
    p_info[i] = malloc(sizeof(Protocol__FlexPagingInfo));
    if(p_info[i] == NULL)
      goto error;
    protocol__flex_paging_info__init(p_info[i]);
    p_info[i]->paging_index = original->paging_info[i]->paging_index;
    p_info[i]->has_paging_index = original->paging_info[i]->has_paging_index;;
    p_info[i]->paging_message_size = original->paging_info[i]->paging_message_size;
    p_info[i]->has_paging_message_size =  original->paging_info[i]->has_paging_message_size;
    p_info[i]->paging_subframe = original->paging_info[i]->paging_subframe;
    p_info[i]->has_paging_subframe = original->paging_info[i]->has_paging_subframe;
    p_info[i]->carrier_index = original->paging_info[i]->carrier_index;
    p_info[i]->has_carrier_index = original->paging_info[i]->has_carrier_index;
  }
  copy->paging_info = p_info;
  return copy;

 error:
  /*TODO: free memory properly*/
  return NULL;
}

Protocol__FlexDlCsi * copy_csi_report(Protocol__FlexDlCsi * original) {
  int i, j;
  Protocol__FlexDlCsi *copy = malloc(sizeof(Protocol__FlexDlCsi));
  if (copy == NULL)
    goto error;
  protocol__flex_dl_csi__init(copy);
  copy->serv_cell_index = original->serv_cell_index;
  copy->has_serv_cell_index = original->has_serv_cell_index;
  copy->ri = original->ri;
  copy->has_ri = original->ri;

  copy->type = original->type;
  copy->has_type = original->has_type;
  copy->report_case = original->report_case;
  
  switch (copy->report_case) {
  case PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI:
    copy->p10csi->wb_cqi = original->p10csi->wb_cqi;
    copy->p10csi->has_wb_cqi = original->p10csi->has_wb_cqi;
    break;
  case PROTOCOL__FLEX_DL_CSI__REPORT_P11CSI:
    copy->p11csi->n_wb_cqi = original->p11csi->n_wb_cqi;
    copy->p11csi->wb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->p11csi->n_wb_cqi);
    for (i = 0; i < copy->p11csi->n_wb_cqi; i++) {
      copy->p11csi->wb_cqi[i] = original->p11csi->wb_cqi[i];
    }
    copy->p11csi->has_wb_pmi = original->p11csi->has_wb_pmi;
    copy->p11csi->wb_pmi = original->p11csi->wb_pmi;
    break;
  case PROTOCOL__FLEX_DL_CSI__REPORT_P20CSI:
    copy->p20csi->has_wb_cqi = original->p20csi->has_wb_cqi;
    copy->p20csi->wb_cqi = original->p20csi->wb_cqi;
    copy->p20csi->has_sb_cqi = original->p20csi->has_sb_cqi;
    copy->p20csi->sb_cqi = original->p20csi->sb_cqi;
    copy->p20csi->has_bandwidth_part_index = original->p20csi->has_bandwidth_part_index;
    copy->p20csi->bandwidth_part_index = original->p20csi->bandwidth_part_index;
    copy->p20csi->has_sb_index = original->p20csi->has_sb_index;
    copy->p20csi->sb_index = original->p20csi->sb_index;
    break;
  case PROTOCOL__FLEX_DL_CSI__REPORT_P21CSI:
    copy->p21csi->n_wb_cqi = original->p21csi->n_wb_cqi;
    copy->p21csi->wb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->p21csi->n_wb_cqi);
    for (i = 0; i < copy->p21csi->n_wb_cqi; i++) {
      copy->p21csi->wb_cqi[i] = original->p21csi->wb_cqi[i];
    }
    copy->p21csi->has_wb_pmi = original->p21csi->has_wb_pmi;
    copy->p21csi->wb_pmi = original->p21csi->wb_pmi;
    copy->p21csi->n_sb_cqi = original->p21csi->n_sb_cqi;
    copy->p21csi->sb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->p21csi->n_sb_cqi);
    for (i = 0; i < copy->p21csi->n_sb_cqi; i++) {
      copy->p21csi->sb_cqi[i] = original->p21csi->sb_cqi[i];
    }
     copy->p21csi->has_badwidth_part_index = original->p21csi->has_badwidth_part_index;
    copy->p21csi->badwidth_part_index = original->p21csi->badwidth_part_index;
    copy->p21csi->has_sb_index = original->p21csi->has_sb_index;
    copy->p21csi->sb_index = original->p21csi->sb_index;
    break;
  case PROTOCOL__FLEX_DL_CSI__REPORT_A12CSI:
    copy->a12csi->n_wb_cqi = original->a12csi->n_wb_cqi;
    copy->a12csi->wb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->a12csi->n_wb_cqi);
    for (i = 0; i < copy->a12csi->n_wb_cqi; i++) {
      copy->a12csi->wb_cqi[i] = original->a12csi->wb_cqi[i];
    }
    copy->a12csi->n_sb_pmi = original->a12csi->n_sb_pmi;
    copy->a12csi->sb_pmi = (uint32_t *) malloc(sizeof(uint32_t) * copy->a12csi->n_sb_pmi);
    for (i = 0; i < copy->a12csi->n_sb_pmi; i++) {
      copy->a12csi->sb_pmi[i] = original->a12csi->sb_pmi[i];
    }
    break;
  case PROTOCOL__FLEX_DL_CSI__REPORT_A22CSI:
    copy->a22csi->n_wb_cqi = original->a22csi->n_wb_cqi;
    copy->a22csi->wb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->a22csi->n_wb_cqi);
    for (i = 0; i < copy->a22csi->n_wb_cqi; i++) {
      copy->a22csi->wb_cqi[i] = original->a22csi->wb_cqi[i];
    }
    copy->a22csi->n_sb_cqi = original->a22csi->n_sb_cqi;
    copy->a22csi->sb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->a22csi->n_sb_cqi);
    for (i = 0; i < copy->a22csi->n_sb_cqi; i++) {
      copy->a22csi->sb_cqi[i] = original->a22csi->sb_cqi[i];
    }
    copy->a22csi->has_wb_pmi = original->a22csi->has_wb_pmi;
    copy->a22csi->wb_pmi = original->a22csi->wb_pmi;
    copy->a22csi->has_sb_pmi = original->a22csi->has_sb_pmi;
    copy->a22csi->sb_pmi = original->a22csi->sb_pmi;
    copy->a22csi->n_sb_list = original->a22csi->n_sb_list;
    copy->a22csi->sb_list = (uint32_t *) malloc(sizeof(uint32_t) * copy->a22csi->n_sb_list);
    for (i = 0; i < copy->a22csi->n_sb_list; i++) {
      copy->a22csi->sb_list[i] = original->a22csi->sb_list[i];
    }
    break;
  case PROTOCOL__FLEX_DL_CSI__REPORT_A20CSI:
    copy->a20csi->has_wb_cqi = original->a20csi->has_wb_cqi;
    copy->a20csi->wb_cqi = original->a20csi->wb_cqi;
    copy->a20csi->has_sb_cqi = original->a20csi->has_sb_cqi;
    copy->a20csi->sb_cqi = original->a20csi->sb_cqi;
    copy->a20csi->n_sb_list = original->a20csi->n_sb_list;
    copy->a20csi->sb_list = (uint32_t *) malloc(sizeof(uint32_t) * copy->a20csi->n_sb_list);
    for (i = 0; i < copy->a20csi->n_sb_list; i++) {
      copy->a20csi->sb_list[i] = original->a20csi->sb_list[i];
    }
    break;
  case PROTOCOL__FLEX_DL_CSI__REPORT_A30CSI:
    copy->a30csi->has_wb_cqi = original->a30csi->has_wb_cqi;
    copy->a30csi->wb_cqi = original->a30csi->wb_cqi;
    copy->a30csi->n_sb_cqi = original->a30csi->n_sb_cqi;
    copy->a30csi->sb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->a30csi->n_sb_cqi);
    for (i = 0; i < copy->a30csi->n_sb_cqi; i++) {
      copy->a30csi->sb_cqi[i] = original->a30csi->sb_cqi[i];
    }
    break;
  case PROTOCOL__FLEX_DL_CSI__REPORT_A31CSI:
    copy->a31csi->n_wb_cqi = original->a31csi->n_wb_cqi;
    copy->a31csi->wb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->a31csi->n_wb_cqi);
    for (i = 0; i < copy->a31csi->n_wb_cqi; i++) {
      copy->a31csi->wb_cqi[i] = original->a31csi->wb_cqi[i];
    }
    copy->a31csi->has_wb_pmi = original->a31csi->has_wb_pmi;
    copy->a31csi->wb_pmi = original->a31csi->wb_pmi;
    copy->a31csi->n_sb_cqi = original->a31csi->n_sb_cqi;
    copy->a31csi->sb_cqi = malloc(sizeof(Protocol__FlexMsbCqi *) * copy->a31csi->n_sb_cqi);
    if (copy->a31csi == NULL) {
      goto error;
    }
    for (i = 0; i < copy->a31csi->n_sb_cqi; i++) {
      copy->a31csi->sb_cqi[i] = malloc(sizeof(Protocol__FlexMsbCqi));
      if (copy->a31csi->sb_cqi[i] == NULL) {
	goto error;
      }
      protocol__flex_msb_cqi__init(copy->a31csi->sb_cqi[i]);
      copy->a31csi->sb_cqi[i]->n_sb_cqi = original->a31csi->sb_cqi[i]->n_sb_cqi;
      copy->a31csi->sb_cqi[i]->sb_cqi = (uint32_t *) malloc(sizeof(uint32_t) * copy->a31csi->sb_cqi[i]->n_sb_cqi);
      for (j = 0; j < copy->a31csi->sb_cqi[i]->n_sb_cqi; j++) {
	copy->a31csi->sb_cqi[i]->sb_cqi[j] = original->a31csi->sb_cqi[i]->sb_cqi[j];
      }
    }
    break;
  default:
    goto error;
  }
  return copy;

 error:
  return NULL;
}

Protocol__FlexCellStatsReport * copy_cell_stats_report(Protocol__FlexCellStatsReport *original) {
 
  Protocol__FlexCellStatsReport * copy =  malloc(sizeof(Protocol__FlexCellStatsReport));
  
  if(copy == NULL) {
    goto error;
  }
  protocol__flex_cell_stats_report__init(copy);
  copy->carrier_index = original->carrier_index;
  copy->has_carrier_index = original->has_carrier_index;
  copy->flags = original->flags;
  copy->has_flags = original->has_flags;

   if(copy->flags & PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE) {
     copy->noise_inter_report = copy_noise_inter_report(original->noise_inter_report);
   }

   return copy;

 error:
   return NULL;
}

Protocol__FlexNoiseInterferenceReport * copy_noise_inter_report(Protocol__FlexNoiseInterferenceReport *original) {
  Protocol__FlexNoiseInterferenceReport *ni_report;
  ni_report = malloc(sizeof(Protocol__FlexNoiseInterferenceReport));
  if(ni_report == NULL) {
    goto error;
  }
  protocol__flex_noise_interference_report__init(ni_report);
  // Current frame and subframe number
  ni_report->sfn_sf = original->sfn_sf;
  ni_report->has_sfn_sf = original->sfn_sf;
  // Received interference power in dbm
  ni_report->rip = original->rip;
  ni_report->has_rip = original->has_rip;
  // Thermal noise power in dbm
  ni_report->tnp = original->tnp;
  ni_report->has_tnp = original->has_tnp;
  
  return ni_report;
  
 error:
  return NULL;
}


int parse_mac_config(mid_t mod_id, yaml_parser_t *parser) {
  
  yaml_event_t event;
  
  int done = 0;

  int sequence_started = 0;
  int mapping_started = 0;

  while (!done) {

    if (!yaml_parser_parse(parser, &event))
      goto error;
   
    switch (event.type) {
    case YAML_SEQUENCE_START_EVENT:
      LOG_D(ENB_APP, "A sequence just started as expected\n");
      sequence_started = 1;
      break;
    case YAML_SEQUENCE_END_EVENT:
      LOG_D(ENB_APP, "A sequence ended\n");
      sequence_started = 0;
      break;
    case YAML_MAPPING_START_EVENT:
      if (!sequence_started) {
	goto error;
      }
      LOG_D(ENB_APP, "A mapping started\n");
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      if (!mapping_started) {
	goto error;
      }
      LOG_D(ENB_APP, "A mapping ended\n");
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      if (!mapping_started) {
	goto error;
      }
      // Check the types of subsystems offered and handle their values accordingly
      if (strcmp((char *) event.data.scalar.value, "dl_scheduler") == 0) {
	LOG_D(ENB_APP, "This is for the dl_scheduler subsystem\n");
	// Call the proper handler
	if (parse_dl_scheduler_config(mod_id, parser) == -1) {
	  LOG_D(ENB_APP, "An error occured\n");
	  goto error;
	}
      } else if (strcmp((char *) event.data.scalar.value, "ul_scheduler") == 0) {
	// Call the proper handler
	LOG_D(ENB_APP, "This is for the ul_scheduler subsystem\n");
	if (parse_ul_scheduler_config(mod_id, parser) == -1) {
    LOG_D(ENB_APP, "An error occured\n");
    goto error;
  }
	// TODO
      } else if (strcmp((char *) event.data.scalar.value, "ra_scheduler") == 0) {
	// Call the proper handler
	// TODO
      } else if (strcmp((char *) event.data.scalar.value, "page_scheduler") == 0) {
	// Call the proper handler
	// TODO
      } else {
	// Unknown subsystem
	goto error;
      }
      break;
    default: // We expect nothing else at this level of the hierarchy
      goto error;
    }
   
    done = (event.type == YAML_SEQUENCE_END_EVENT);

    yaml_event_delete(&event);
 
  }
  
  return 0;

  error:
  yaml_event_delete(&event);
  return -1;

}

int parse_dl_scheduler_config(mid_t mod_id, yaml_parser_t *parser) {
  
  yaml_event_t event;

  int done = 0;
  int mapping_started = 0;

  while (!done) {
    
    if (!yaml_parser_parse(parser, &event))
      goto error;

    switch (event.type) {
      // We are expecting a mapping (behavior and parameters)
    case YAML_MAPPING_START_EVENT:
      LOG_D(ENB_APP, "The mapping of the subsystem started\n");
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      LOG_D(ENB_APP, "The mapping of the subsystem ended\n");
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      if (!mapping_started) {
	goto error;
      }
      // Check what key needs to be set
      if (strcmp((char *) event.data.scalar.value, "behavior") == 0) {
	LOG_I(ENB_APP, "Time to set the behavior attribute\n");
	yaml_event_delete(&event);
	if (!yaml_parser_parse(parser, &event)) {
	  goto error;
	}
	if (event.type == YAML_SCALAR_EVENT) {
	  if (load_dl_scheduler_function(mod_id, (char *) event.data.scalar.value) == -1) {
	    goto error;
	  } 
	} else {
	  goto error;
	}
      } else if (strcmp((char *) event.data.scalar.value, "parameters") == 0) {
	LOG_D(ENB_APP, "Now it is time to set the parameters for this subsystem\n");
	if (parse_dl_scheduler_parameters(mod_id, parser) == -1) {
	  goto error;
	}
      }
      break;
    default:
      goto error;
    }

    done = (event.type == YAML_MAPPING_END_EVENT);
    yaml_event_delete(&event);
  }

  return 0;

 error:
  yaml_event_delete(&event);
  return -1;
}

int parse_ul_scheduler_config(mid_t mod_id, yaml_parser_t *parser) {
  
  yaml_event_t event;

  int done = 0;
  int mapping_started = 0;

  while (!done) {
    
    if (!yaml_parser_parse(parser, &event))
      goto error;

    switch (event.type) {
      // We are expecting a mapping (behavior and parameters)
    case YAML_MAPPING_START_EVENT:
      LOG_D(ENB_APP, "The mapping of the subsystem started\n");
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      LOG_D(ENB_APP, "The mapping of the subsystem ended\n");
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      if (!mapping_started) {
  goto error;
      }
      // Check what key needs to be set
      if (strcmp((char *) event.data.scalar.value, "parameters") == 0) {
  LOG_D(ENB_APP, "Now it is time to set the parameters for this subsystem\n");
  if (parse_ul_scheduler_parameters(mod_id, parser) == -1) {
    goto error;
  }
      }
      break;
    default:
      goto error;
    }

    done = (event.type == YAML_MAPPING_END_EVENT);
    yaml_event_delete(&event);
  }

  return 0;

 error:
  yaml_event_delete(&event);
  return -1;
}


int parse_dl_scheduler_parameters(mid_t mod_id, yaml_parser_t *parser) {
  yaml_event_t event;
  
  void *param;
  
  int done = 0;
  int mapping_started = 0;

  while (!done) {
    
    if (!yaml_parser_parse(parser, &event))
      goto error;

    switch (event.type) {
      // We are expecting a mapping of parameters
    case YAML_MAPPING_START_EVENT:
      LOG_D(ENB_APP, "The mapping of the parameters started\n");
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      LOG_D(ENB_APP, "The mapping of the parameters ended\n");
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      if (!mapping_started) {
	goto error;
      }
      // Check what key needs to be set
      if (mac_agent_registered[mod_id]) {
	LOG_D(ENB_APP, "Setting parameter %s\n", event.data.scalar.value);
	param = dlsym(agent_mac_xface[mod_id]->dl_scheduler_loaded_lib,
		      (char *) event.data.scalar.value);
	if (param == NULL) {
	  goto error;
	}
	apply_parameter_modification(param, parser);
      } else {
	goto error;
      }
      break;
    default:
      goto error;
    }

    done = (event.type == YAML_MAPPING_END_EVENT);
    yaml_event_delete(&event);
  }

  return 0;
  
 error:
  yaml_event_delete(&event);
  return -1;
}

int parse_ul_scheduler_parameters(mid_t mod_id, yaml_parser_t *parser) {
  yaml_event_t event;
  
  void *param;
  
  int done = 0;
  int mapping_started = 0;

  while (!done) {
    
    if (!yaml_parser_parse(parser, &event))
      goto error;

    switch (event.type) {
      // We are expecting a mapping of parameters
    case YAML_MAPPING_START_EVENT:
      LOG_D(ENB_APP, "The mapping of the parameters started\n");
      mapping_started = 1;
      break;
    case YAML_MAPPING_END_EVENT:
      LOG_D(ENB_APP, "The mapping of the parameters ended\n");
      mapping_started = 0;
      break;
    case YAML_SCALAR_EVENT:
      if (!mapping_started) {
  goto error;
      }
      // Check what key needs to be set
      if (mac_agent_registered[mod_id]) {
  LOG_D(ENB_APP, "Setting parameter %s\n", event.data.scalar.value);
  param = dlsym(agent_mac_xface[mod_id]->ul_scheduler_loaded_lib,
          (char *) event.data.scalar.value);
  if (param == NULL) {
    goto error;
  }
  apply_parameter_modification(param, parser);
      } else {
  goto error;
      }
      break;
    default:
      goto error;
    }

    done = (event.type == YAML_MAPPING_END_EVENT);
    yaml_event_delete(&event);
  }

  return 0;
  
 error:
  yaml_event_delete(&event);
  return -1;
}

int load_dl_scheduler_function(mid_t mod_id, const char *function_name) {
  void *lib;

  char lib_name[120];
  char target[512];
  snprintf(lib_name, sizeof(lib_name), "/%s.so", function_name);
  strcpy(target, RC.flexran[mod_id]->cache_name);
  strcat(target, lib_name);
  
  LOG_I(FLEXRAN_AGENT, "Opening pushed code: %s\n", target);
  lib = dlopen(target, RTLD_NOW);
  if (lib == NULL) {
    LOG_I(FLEXRAN_AGENT, "Could not load library\n");
    goto error;
  }
  
  LOG_I(FLEXRAN_AGENT, "Loading function: %s\n", function_name);
  void *loaded_scheduler = dlsym(lib, function_name);
  if (loaded_scheduler) {
    if (mac_agent_registered[mod_id]) {
      if (agent_mac_xface[mod_id]->dl_scheduler_loaded_lib != NULL) {
	dlclose(agent_mac_xface[mod_id]->dl_scheduler_loaded_lib);
      }
      agent_mac_xface[mod_id]->dl_scheduler_loaded_lib = lib;
      LOG_I(FLEXRAN_AGENT, "New DL UE scheduler: %s\n", function_name);
    }
  } else {
    LOG_I(FLEXRAN_AGENT, "Scheduler could not be loaded\n");
  }

  return 0;

 error:
  return -1;
  
}

Protocol__FlexSliceConfig *flexran_agent_create_slice_config(int n_dl, int m_ul)
{
  int i;
  Protocol__FlexSliceConfig *fsc = malloc(sizeof(Protocol__FlexSliceConfig));
  if (!fsc) return NULL;
  protocol__flex_slice_config__init(fsc);

  /* say there are n_dl slices but reserve memory for up to MAX_NUM_SLICES so
   * we don't need to reserve again later */
  fsc->n_dl = n_dl;
  fsc->dl = calloc(MAX_NUM_SLICES, sizeof(Protocol__FlexDlSlice *));
  if (!fsc->dl) fsc->n_dl = 0;
  for (i = 0; i < MAX_NUM_SLICES; i++) {
    fsc->dl[i] = malloc(sizeof(Protocol__FlexDlSlice));
    if (!fsc->dl[i]) continue;
    protocol__flex_dl_slice__init(fsc->dl[i]);
  }

  /* as above */
  fsc->n_ul = m_ul;
  fsc->ul = calloc(MAX_NUM_SLICES, sizeof(Protocol__FlexUlSlice *));
  if (!fsc->ul) fsc->n_ul = 0;
  for (i = 0; i < MAX_NUM_SLICES; i++) {
    fsc->ul[i] = malloc(sizeof(Protocol__FlexUlSlice));
    if (!fsc->ul[i]) continue;
    protocol__flex_ul_slice__init(fsc->ul[i]);
  }
  return fsc;
}

void flexran_agent_read_slice_dl_config(mid_t mod_id, int slice_idx, Protocol__FlexDlSlice *dl_slice)
{
  dl_slice->id = flexran_get_dl_slice_id(mod_id, slice_idx);
  dl_slice->has_id = 1;
  /* read label from corresponding sc_update entry or give default */
  dl_slice->label = PROTOCOL__FLEX_SLICE_LABEL__xMBB;
  dl_slice->has_label = 1;
  for (int i = 0; i < sc_update[mod_id]->n_dl; i++) {
    if (sc_update[mod_id]->dl[i]->id == dl_slice->id
        && sc_update[mod_id]->dl[i]->has_label) {
      dl_slice->label = sc_update[mod_id]->dl[i]->label;
      break;
    }
  }
  dl_slice->percentage = flexran_get_dl_slice_percentage(mod_id, slice_idx);
  dl_slice->has_percentage = 1;
  dl_slice->isolation = flexran_get_dl_slice_isolation(mod_id, slice_idx);
  dl_slice->has_isolation = 1;
  dl_slice->priority = flexran_get_dl_slice_priority(mod_id, slice_idx);
  dl_slice->has_priority = 1;
  dl_slice->position_low = flexran_get_dl_slice_position_low(mod_id, slice_idx);
  dl_slice->has_position_low = 1;
  dl_slice->position_high = flexran_get_dl_slice_position_high(mod_id, slice_idx);
  dl_slice->has_position_high = 1;
  dl_slice->maxmcs = flexran_get_dl_slice_maxmcs(mod_id, slice_idx);
  dl_slice->has_maxmcs = 1;
  dl_slice->n_sorting = flexran_get_dl_slice_sorting(mod_id, slice_idx, &dl_slice->sorting);
  if (dl_slice->n_sorting < 1) dl_slice->sorting = NULL;
  dl_slice->accounting = flexran_get_dl_slice_accounting_policy(mod_id, slice_idx);
  dl_slice->has_accounting = 1;
  dl_slice->scheduler_name = flexran_get_dl_slice_scheduler_name(mod_id, slice_idx);
}

void flexran_agent_read_slice_ul_config(mid_t mod_id, int slice_idx, Protocol__FlexUlSlice *ul_slice)
{
  ul_slice->id = flexran_get_ul_slice_id(mod_id, slice_idx);
  ul_slice->has_id = 1;
  /* read label from corresponding sc_update entry or give default */
  ul_slice->label = PROTOCOL__FLEX_SLICE_LABEL__xMBB;
  ul_slice->has_label = 1;
  for (int i = 0; i < sc_update[mod_id]->n_ul; i++) {
    if (sc_update[mod_id]->ul[i]->id == ul_slice->id
        && sc_update[mod_id]->ul[i]->has_label) {
      ul_slice->label = sc_update[mod_id]->ul[i]->label;
      break;
    }
  }
  ul_slice->percentage = flexran_get_ul_slice_percentage(mod_id, slice_idx);
  ul_slice->has_percentage = 1;
  /*ul_slice->isolation = flexran_get_ul_slice_isolation(mod_id, slice_idx);*/
  ul_slice->has_isolation = 0;
  /*ul_slice->priority = flexran_get_ul_slice_priority(mod_id, slice_idx);*/
  ul_slice->has_priority = 0;
  ul_slice->first_rb = flexran_get_ul_slice_first_rb(mod_id, slice_idx);
  ul_slice->has_first_rb = 1;
  /*ul_slice-> = flexran_get_ul_slice_length_rb(mod_id, slice_idx);
  ul_slice->has_length_rb = 0;*/
  ul_slice->maxmcs = flexran_get_ul_slice_maxmcs(mod_id, slice_idx);
  ul_slice->has_maxmcs = 1;
  ul_slice->n_sorting = 0;
  /*if (ul_slice->sorting) {
    free(ul_slice->sorting);
    ul_slice->sorting = NULL;
  }
  ul_slice->n_sorting = flexran_get_ul_slice_sorting(mod_id, slice_idx, &ul_slice->sorting);
  if (ul_slice->n_sorting < 1) ul_slice->sorting = NULL;*/
  /*ul_slice->accounting = flexran_get_ul_slice_accounting_policy(mod_id, slice_idx);*/
  ul_slice->has_accounting = 0;
  ul_slice->scheduler_name = flexran_get_ul_slice_scheduler_name(mod_id, slice_idx);
}

void overwrite_slice_config_dl(Protocol__FlexDlSlice *exist, Protocol__FlexDlSlice *update)
{
  if (update->has_label && update->label != exist->label) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update label: %d -> %d\n",
          update->id, exist->label, update->label);
    exist->label = update->label;
  }
  if (update->has_percentage && update->percentage != exist->percentage) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update percentage: %d -> %d\n",
          update->id, exist->percentage, update->percentage);
    exist->percentage = update->percentage;
  }
  if (update->has_isolation && update->isolation != exist->isolation) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update isolation: %d -> %d\n",
          update->id, exist->isolation, update->isolation);
    exist->isolation = update->isolation;
  }
  if (update->has_priority && update->priority != exist->priority) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update priority: %d -> %d\n",
          update->id, exist->priority, update->priority);
    exist->priority = update->priority;
  }
  if (update->has_position_low && update->position_low != exist->position_low) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update position_low: %d -> %d\n",
          update->id, exist->position_low, update->position_low);
    exist->position_low = update->position_low;
  }
  if (update->has_position_high && update->position_high != exist->position_high) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update position_high: %d -> %d\n",
          update->id, exist->position_high, update->position_high);
    exist->position_high = update->position_high;
  }
  if (update->has_maxmcs && update->maxmcs != exist->maxmcs) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update maxmcs: %d -> %d\n",
          update->id, exist->maxmcs, update->maxmcs);
    exist->maxmcs = update->maxmcs;
  }
  int sorting_update = 0;
  int n = min(exist->n_sorting, update->n_sorting);
  /* check whether something has changed. If update->n_sorting is 0,
   * sorting_update will remain false, so no update will be done */
  int i = 0;
  while(i < n && !sorting_update) {
    sorting_update = sorting_update || (update->sorting[i] != exist->sorting[i]);
    i++;
  }
  if (sorting_update) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update sorting array\n", update->id);
    if (exist->n_sorting != update->n_sorting)
      LOG_W(FLEXRAN_AGENT, "[slice %d] only writing %d elements\n",
          update->id, n);
    for (int i = 0; i < n; i++)
      exist->sorting[i] = update->sorting[i];
  }
  if (update->has_accounting && update->accounting != exist->accounting) {
    LOG_I(FLEXRAN_AGENT, "[DL slice %d] update accounting: %d -> %d\n",
          update->id, exist->accounting, update->accounting);
    exist->accounting = update->accounting;
  }
  if (update->scheduler_name
      && strcmp(update->scheduler_name, exist->scheduler_name) != 0) {
    LOG_W(FLEXRAN_AGENT, "[DL slice %d] ignoring new scheduler name \"%s\"!\n",
        update->id, update->scheduler_name);
  }
}

void overwrite_slice_config_ul(Protocol__FlexUlSlice *exist, Protocol__FlexUlSlice *update)
{
  if (update->has_label && update->label != exist->label) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update label: %d -> %d\n",
          update->id, exist->label, update->label);
    exist->label = update->label;
  }
  if (update->has_percentage && update->percentage != exist->percentage) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update percentage: %d -> %d\n",
          update->id, exist->percentage, update->percentage);
    exist->percentage = update->percentage;
  }
  if (update->has_isolation && update->isolation != exist->isolation) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update isolation: %d -> %d\n",
          update->id, exist->isolation, update->isolation);
    exist->isolation = update->isolation;
  }
  if (update->has_priority && update->priority != exist->priority) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update priority: %d -> %d\n",
          update->id, exist->priority, update->priority);
    exist->priority = update->priority;
  }
  if (update->has_first_rb && update->first_rb != exist->first_rb) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update first_rb: %d -> %d\n",
	  update->id, exist->first_rb, update->first_rb);
    exist->first_rb = update ->first_rb;
  }
  /*if (update->has_length_rb && update->lenght_rb != exist->lenght_rb) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update lenght_rb: %d -> %d\n",
          update->id, exist->lenght_rb, update->lenght_rb);
    exist->lenght_rb = update->lenght_rb;
  }*/
  if (update->has_maxmcs && update->maxmcs != exist->maxmcs) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update maxmcs: %d -> %d\n",
          update->id, exist->maxmcs, update->maxmcs);
    exist->maxmcs = update->maxmcs;
  }
  int sorting_update = 0;
  int n = min(exist->n_sorting, update->n_sorting);
  /* check whether something has changed. If update-> n_sorting is 0,
   * sorting_update will remain false, so no update will be done */
  int i = 0;
  while (i < n && !sorting_update) {
    sorting_update = sorting_update || (update->sorting[i] != exist->sorting[i]);
    i++;
  }
  if (sorting_update) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update sorting array\n", update->id);
    if (exist->n_sorting != update->n_sorting)
      LOG_W(FLEXRAN_AGENT, "[UL slice %d] only writing %d elements\n",
          update->id, n);
    for (i = 0; i < n; i++)
      exist->sorting[i] = update->sorting[i];
  }
  if (update->has_accounting && update->accounting != exist->accounting) {
    LOG_I(FLEXRAN_AGENT, "[UL slice %d] update accounting: %d -> %d\n",
          update->id, exist->accounting, update->accounting);
    exist->accounting = update->accounting;
  }
  if (update->scheduler_name
      && strcmp(update->scheduler_name, exist->scheduler_name) != 0) {
    LOG_W(FLEXRAN_AGENT, "[UL slice %d] ignoring new scheduler name \"%s\"!\n",
        update->id, update->scheduler_name);
  }
}

void prepare_update_slice_config_dl(mid_t mod_id, Protocol__FlexDlSlice *dls)
{
  if (!dls->has_id) {
    LOG_E(FLEXRAN_AGENT, "[%d] Incoming DL slice configuration has no ID\n", mod_id);
    return;
  }
  /* a percentage of zero will be interpreted as removal command */
  if (sc_update[mod_id]->n_dl >= MAX_NUM_SLICES
      && (!dls->has_percentage || dls->percentage > 0)) {
    LOG_E(FLEXRAN_AGENT, "[%d] Cannot create more than %ld slices in DL\n",
          mod_id, sc_update[mod_id]->n_dl);
    return;
  }
  if (sc_update[mod_id]->n_dl == 1 && dls->has_percentage && dls->percentage == 0) {
    LOG_E(FLEXRAN_AGENT, "[%d] Cannot delete last slice ID %d in DL\n",
          mod_id, sc_update[mod_id]->dl[0]->id);
    return;
  }

  Protocol__FlexDlSlice *to = NULL;
  for (int i = 0; i < sc_update[mod_id]->n_dl; ++i) {
    if (dls->id == sc_update[mod_id]->dl[i]->id) {
      to = sc_update[mod_id]->dl[i];
      break;
    }
  }

  /* create new slice -> read contents from existing slice config index 0 */
  if (!to) {
    LOG_I(FLEXRAN_AGENT,
          "[%d] Creating DL slice with ID %d, taking default values from DL slice 0\n",
          mod_id, dls->id);
    to = sc_update[mod_id]->dl[sc_update[mod_id]->n_dl];
    sc_update[mod_id]->n_dl++;
    memcpy(to, slice_config[mod_id]->dl[0], sizeof(*to));
    to->id = dls->id;
  }

  overwrite_slice_config_dl(to, dls);
}

void prepare_update_slice_config_ul(mid_t mod_id, Protocol__FlexUlSlice *uls)
{
  if (!uls->has_id) {
    LOG_E(FLEXRAN_AGENT, "[%d] Incoming UL slice configuration has no ID\n", mod_id);
    return;
  }
  /* a percentage of zero will be interpreted as removal command */
  if (sc_update[mod_id]->n_ul >= MAX_NUM_SLICES
      && (!uls->has_percentage || uls->percentage > 0)) {
    LOG_E(FLEXRAN_AGENT, "[%d] Cannot create more than %ld slices in UL\n",
          mod_id, sc_update[mod_id]->n_ul);
    return;
  }
  if (sc_update[mod_id]->n_ul == 1 && uls->has_percentage && uls->percentage == 0) {
    LOG_E(FLEXRAN_AGENT, "[%d] Cannot delete last slice ID %d in UL\n",
          mod_id, sc_update[mod_id]->ul[0]->id);
    return;
  }

  Protocol__FlexUlSlice *to = NULL;
  for (int i = 0; i < sc_update[mod_id]->n_ul; ++i) {
    if (uls->id == sc_update[mod_id]->ul[i]->id) {
      to = sc_update[mod_id]->ul[i];
      break;
    }
  }

  /* create new slice -> read contents from existing slice config index 0 */
  if (!to) {
    LOG_I(FLEXRAN_AGENT,
          "[%d] Creating UL slice with ID %d, taking default values from UL slice 0\n",
          mod_id, uls->id);
    to = sc_update[mod_id]->ul[sc_update[mod_id]->n_ul];
    sc_update[mod_id]->n_ul++;
    memcpy(to, slice_config[mod_id]->ul[0], sizeof(*to));
    to->id = uls->id;
  }

  overwrite_slice_config_ul(to, uls);
}

void prepare_update_slice_config(mid_t mod_id, Protocol__FlexSliceConfig *slice)
{
  if (!sc_update[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "Can not update slice policy (no existing slice profile)\n");
    return;
  }
  if (slice->n_dl == 0)
    LOG_I(FLEXRAN_AGENT, "[%d] no DL slice configuration in flex_slice_config message\n", mod_id);
  for (int i = 0; i < slice->n_dl; i++)
    prepare_update_slice_config_dl(mod_id, slice->dl[i]);

  if (slice->n_ul == 0)
    LOG_I(FLEXRAN_AGENT, "[%d] no UL slice configuration in flex_slice_config message\n", mod_id);
  for (int i = 0; i < slice->n_ul; i++)
    prepare_update_slice_config_ul(mod_id, slice->ul[i]);

  /* perform the slice configuration reads a couple of times. If there are
   * inconsistencies (i.e. the MAC refuses a configuration), we will have a
   * couple of warnings because the current configuration and the one in
   * sc_update will be diverged */
  perform_slice_config_update_count = 3;
}

int apply_new_slice_dl_config(mid_t mod_id, Protocol__FlexDlSlice *oldc, Protocol__FlexDlSlice *newc)
{
  /* not setting the old configuration is intentional, as it will be picked up
   * later when reading the configuration. There is thus a direct feedback
   * whether it has been set. */
  int changes = 0;
  int n;
  int slice_idx = flexran_find_dl_slice(mod_id, newc->id);
  if (slice_idx < 0) {
    LOG_W(FLEXRAN_AGENT, "cannot find index for slice ID %d\n", newc->id);
    return 0;
  }
  if (oldc->percentage != newc->percentage) {
    flexran_set_dl_slice_percentage(mod_id, slice_idx, newc->percentage);
    changes++;
  }
  if (oldc->isolation != newc->isolation) {
    flexran_set_dl_slice_isolation(mod_id, slice_idx, newc->isolation);
    changes++;
  }
  if (oldc->priority != newc->priority) {
    flexran_set_dl_slice_priority(mod_id, slice_idx, newc->priority);
    changes++;
  }
  if (oldc->position_low != newc->position_low) {
    flexran_set_dl_slice_position_low(mod_id, slice_idx, newc->position_low);
    changes++;
  }
  if (oldc->position_high != newc->position_high) {
    flexran_set_dl_slice_position_high(mod_id, slice_idx, newc->position_high);
    changes++;
  }
  if (oldc->maxmcs != newc->maxmcs) {
    flexran_set_dl_slice_maxmcs(mod_id, slice_idx, newc->maxmcs);
    changes++;
  }
  n = min(oldc->n_sorting, newc->n_sorting);
  for (int i = 0; i < n; i++) {
    if (oldc->sorting[i] != newc->sorting[i]) {
      flexran_set_dl_slice_sorting(mod_id, slice_idx, newc->sorting, n);
      changes++;
      break;
    }
  }
  if (oldc->accounting != newc->accounting) {
    flexran_set_dl_slice_accounting_policy(mod_id, slice_idx, newc->accounting);
    changes++;
  }
  if (strcmp(oldc->scheduler_name, newc->scheduler_name) != 0) {
    LOG_E(FLEXRAN_AGENT, "[%d][DL slice %d] setting the DL scheduler is not supported, reverting\n",
          mod_id, newc->id);
    newc->scheduler_name = oldc->scheduler_name;
  }
  return changes;
}

int apply_new_slice_ul_config(mid_t mod_id, Protocol__FlexUlSlice *oldc, Protocol__FlexUlSlice *newc)
{
  /* not setting the old configuration is intentional, as it will be picked up
   * later when reading the configuration. There is thus a direct feedback
   * whether it has been set. */
  int changes = 0;
  int n;
  int slice_idx = flexran_find_ul_slice(mod_id, newc->id);
  if (slice_idx < 0) {
    LOG_W(FLEXRAN_AGENT, "cannot find index for slice ID %d\n", newc->id);
    return 0;
  }
  if (oldc->percentage != newc->percentage) {
    flexran_set_ul_slice_percentage(mod_id, slice_idx, newc->percentage);
    changes++;
  }
  if (oldc->isolation != newc->isolation) {
    /*flexran_set_ul_slice_isolation(mod_id, slice_idx, newc->isolation);
     *changes++;*/
    LOG_W(FLEXRAN_AGENT, "[%d][UL slice %d] setting isolation is not supported\n",
          mod_id, slice_idx);
  }
  if (oldc->priority != newc->priority) {
    /*flexran_set_ul_slice_priority(mod_id, slice_idx, newc->priority);
     *changes++;*/
    LOG_W(FLEXRAN_AGENT, "[%d][UL slice %d] setting the priority is not supported\n",
          mod_id, slice_idx);
  }
  if (oldc->first_rb != newc->first_rb) {
    flexran_set_ul_slice_first_rb(mod_id, slice_idx, newc->first_rb);
    changes++;
  }
  /*if (oldc->length_rb != newc->length_rb) {
    flexran_set_ul_slice_length_rb(mod_id, slice_idx, newc->length_rb);
    changes++;
    LOG_W(FLEXRAN_AGENT, "[%d][UL slice %d] setting length_rb is not supported\n",
          mod_id, slice_idx);
  }*/
  if (oldc->maxmcs != newc->maxmcs) {
    flexran_set_ul_slice_maxmcs(mod_id, slice_idx, newc->maxmcs);
    changes++;
  }
  n = min(oldc->n_sorting, newc->n_sorting);
  for (int i = 0; i < n; i++) {
    if (oldc->sorting[i] != newc->sorting[i]) {
      /*flexran_set_ul_slice_sorting(mod_id, slice_idx, newc->sorting, n);
      changes++;*/
      LOG_W(FLEXRAN_AGENT, "[%d][UL slice %d] setting the sorting is not supported\n",
          mod_id, slice_idx);
      break;
    }
  }
  if (oldc->accounting != newc->accounting) {
    /*flexran_set_ul_slice_accounting_policy(mod_id, slice_idx, newc->accounting);
     *changes++;*/
    LOG_W(FLEXRAN_AGENT, "[%d][UL slice %d] setting the accounting is not supported\n",
          mod_id, slice_idx);
  }
  if (strcmp(oldc->scheduler_name, newc->scheduler_name) != 0) {
    LOG_E(FLEXRAN_AGENT, "[%d][UL slice %d] setting the UL scheduler is not supported, reverting\n",
          mod_id, slice_idx);
    newc->scheduler_name = oldc->scheduler_name;
  }
  return changes;
}

void prepare_ue_slice_assoc_update(mid_t mod_id, Protocol__FlexUeConfig *ue_config)
{
  if (n_ue_slice_assoc_updates == NUM_MAX_UE) {
    LOG_E(FLEXRAN_AGENT,
          "[%d] can not handle flex_ue_config message, buffer is full; try again later\n",
          mod_id);
    return;
  }
  if (!ue_config->has_rnti) {
    LOG_E(FLEXRAN_AGENT,
          "[%d] cannot update UE to slice association, no RNTI in flex_ue_config message\n",
          mod_id);
    return;
  }
  if (ue_config->has_dl_slice_id)
    LOG_I(FLEXRAN_AGENT, "[%d] associating UE RNTI %#x to DL slice ID %d\n",
          mod_id, ue_config->rnti, ue_config->dl_slice_id);
  if (ue_config->has_ul_slice_id)
    LOG_I(FLEXRAN_AGENT, "[%d] associating UE RNTI %#x to UL slice ID %d\n",
          mod_id, ue_config->rnti, ue_config->ul_slice_id);
  ue_slice_assoc_update[n_ue_slice_assoc_updates++] = ue_config;
  perform_slice_config_update_count = 2;
}

int apply_ue_slice_assoc_update(mid_t mod_id)
{
  int i;
  int changes = 0;
  /* TODO should lock structure */
  for (i = 0; i < n_ue_slice_assoc_updates; i++) {
    int ue_id = find_UE_id(mod_id, ue_slice_assoc_update[i]->rnti);
    if (ue_slice_assoc_update[i]->has_dl_slice_id) {
      int slice_idx = flexran_find_dl_slice(mod_id, ue_slice_assoc_update[i]->dl_slice_id);
      if (flexran_dl_slice_exists(mod_id, slice_idx)) {
        flexran_set_ue_dl_slice_idx(mod_id, ue_id, slice_idx);
        changes++;
      } else {
        LOG_W(FLEXRAN_AGENT, "[%d] DL slice %d does not exist, refusing change\n",
              mod_id, ue_slice_assoc_update[i]->dl_slice_id);
      }
    }
    if (ue_slice_assoc_update[i]->has_ul_slice_id) {
      int slice_idx = flexran_find_ul_slice(mod_id, ue_slice_assoc_update[i]->ul_slice_id);
      if (flexran_ul_slice_exists(mod_id, slice_idx)) {
        flexran_set_ue_ul_slice_idx(mod_id, ue_id, slice_idx);
        changes++;
      } else {
        LOG_W(FLEXRAN_AGENT, "[%d] UL slice %d does not exist, refusing change\n",
              mod_id, ue_slice_assoc_update[i]->ul_slice_id);
      }
    }
  }
  n_ue_slice_assoc_updates = 0;
  return changes;
}
