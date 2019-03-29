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

/*! \file flexran_agent_pdcp.c
 * \brief FlexRAN agent Control Module PDCP 
 * \author Navid Nikaein and shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

#include "flexran_agent_pdcp.h"

/*Array containing the Agent-PDCP interfaces*/
AGENT_PDCP_xface *agent_pdcp_xface[NUM_MAX_ENB];

// MAX_MOBILES_PER_ENB

void flexran_agent_pdcp_aggregate_stats(const mid_t mod_id,
					uint16_t uid,
					Protocol__FlexPdcpStats *pdcp_aggr_stats){

  int lcid=0;
  /* only calculate the DRBs */ 
  //LOG_I(FLEXRAN_AGENT, "enb %d ue %d \n", mod_id, uid);
  
  for (lcid=NUM_MAX_SRB ; lcid < NUM_MAX_SRB + NUM_MAX_DRB; lcid++){
    
    pdcp_aggr_stats->pkt_tx += flexran_get_pdcp_tx(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_tx_bytes += flexran_get_pdcp_tx_bytes(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_tx_w += flexran_get_pdcp_tx_w(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_tx_bytes_w += flexran_get_pdcp_tx_bytes_w(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_tx_aiat += flexran_get_pdcp_tx_aiat(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_tx_aiat_w += flexran_get_pdcp_tx_aiat_w(mod_id, uid, lcid);
    
      
    pdcp_aggr_stats->pkt_rx += flexran_get_pdcp_rx(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_rx_bytes += flexran_get_pdcp_rx_bytes(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_rx_w += flexran_get_pdcp_rx_w(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_rx_bytes_w += flexran_get_pdcp_rx_bytes_w(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_rx_aiat += flexran_get_pdcp_rx_aiat(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_rx_aiat_w += flexran_get_pdcp_rx_aiat_w(mod_id, uid, lcid);
    pdcp_aggr_stats->pkt_rx_oo += flexran_get_pdcp_rx_oo(mod_id, uid, lcid);
    
  }
  
}


int flexran_agent_pdcp_stats_reply(mid_t mod_id,       
				   const report_config_t *report_config,
				   Protocol__FlexUeStatsReport **ue_report,
				   Protocol__FlexCellStatsReport **cell_report) {
  
  
  // Protocol__FlexHeader *header;
  int i;
  // int cc_id = 0;
 
  
  /* Allocate memory for list of UE reports */
  if (report_config->nr_ue > 0) {
    
    for (i = 0; i < report_config->nr_ue; i++) {
      const rnti_t rnti = report_config->ue_report_type[i].ue_rnti;
      const uint16_t uid = flexran_get_pdcp_uid_from_rnti(mod_id, rnti);

      /* Check flag for creation of buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PDCP_STATS) {
	
	Protocol__FlexPdcpStats *pdcp_aggr_stats;
	pdcp_aggr_stats = malloc(sizeof(Protocol__FlexPdcpStats));
	if (pdcp_aggr_stats == NULL)
	  goto error;
	protocol__flex_pdcp_stats__init(pdcp_aggr_stats);
	
	flexran_agent_pdcp_aggregate_stats(mod_id, uid, pdcp_aggr_stats);

	pdcp_aggr_stats->has_pkt_tx=1;
	pdcp_aggr_stats->has_pkt_tx_bytes =1;
	pdcp_aggr_stats->has_pkt_tx_w=1; 
	pdcp_aggr_stats->has_pkt_tx_bytes_w =1;
	pdcp_aggr_stats->has_pkt_tx_aiat =1; 
	pdcp_aggr_stats->has_pkt_tx_aiat_w =1;

	pdcp_aggr_stats->pkt_tx_sn = flexran_get_pdcp_tx_sn(mod_id, uid, DEFAULT_DRB);
	pdcp_aggr_stats->has_pkt_tx_sn =1;
	
	pdcp_aggr_stats->has_pkt_rx =1;
	pdcp_aggr_stats->has_pkt_rx_bytes =1; 
	pdcp_aggr_stats->has_pkt_rx_w =1;
	pdcp_aggr_stats->has_pkt_rx_bytes_w =1;
	pdcp_aggr_stats->has_pkt_rx_aiat =1;
	pdcp_aggr_stats->has_pkt_rx_aiat_w =1;
	pdcp_aggr_stats->has_pkt_rx_oo =1;

	pdcp_aggr_stats->pkt_rx_sn = flexran_get_pdcp_rx_sn(mod_id, uid, DEFAULT_DRB);
	pdcp_aggr_stats->has_pkt_rx_sn =1;

	pdcp_aggr_stats->sfn = flexran_get_pdcp_sfn(mod_id);
	pdcp_aggr_stats->has_sfn =1;

	ue_report[i]->pdcp_stats = pdcp_aggr_stats;
        ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PDCP_STATS;
      }
    }
  }  else {
    LOG_D(FLEXRAN_AGENT, "no UE\n");
  }     
  
  return 0;
  
 error:
  LOG_W(FLEXRAN_AGENT, "Can't allocate PDCP stats\n");
  
  /* if (cell_report != NULL)
        free(cell_report);
  if (ue_report != NULL)
        free(ue_report);
  */
  return -1;
}

int flexran_agent_pdcp_destroy_stats_reply(Protocol__FlexStatsReply *reply)
{
  for (int i = 0; i < reply->n_ue_report; ++i) {
    if (reply->ue_report[i]->pdcp_stats)
      free(reply->ue_report[i]->pdcp_stats);
  }
  return 0;
}


int flexran_agent_register_pdcp_xface(mid_t mod_id)
{
  if (agent_pdcp_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "PDCP agent CM for eNB %d is already registered\n", mod_id);
    return -1;
  }
  AGENT_PDCP_xface *xface = malloc(sizeof(AGENT_PDCP_xface));
  if (!xface) {
    LOG_E(FLEXRAN_AGENT, "could not allocate memory for PDCP agent xface %d\n", mod_id);
    return -1;
  }

  //xface->flexran_pdcp_stats_measurement = NULL;

  agent_pdcp_xface[mod_id] = xface;

  return 0;
}

int flexran_agent_unregister_pdcp_xface(mid_t mod_id)
{
  if (!agent_pdcp_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "PDCP agent CM for eNB %d is not registered\n", mod_id);
    return -1;
  }
  //xface->agent_ctxt = NULL;
  //xface->flexran_pdcp_stats_measurement = NULL;

  free(agent_pdcp_xface[mod_id]);
  agent_pdcp_xface[mod_id] = NULL;

  return 0;
}

AGENT_PDCP_xface *flexran_agent_get_pdcp_xface(mid_t mod_id)
{
  return agent_pdcp_xface[mod_id];
}
