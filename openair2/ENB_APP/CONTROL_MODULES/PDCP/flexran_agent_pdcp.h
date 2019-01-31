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

/*! \file flexran_agent_pdcp.h
 * \brief FlexRAN agent Control Module PDCP header
 * \author shahab SHARIAT BAGHERI 
 * \date 2017
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_PDCP_H_
#define FLEXRAN_AGENT_PDCP_H_

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"


#include "flexran_agent_common.h"
#include "flexran_agent_defs.h"
#include "flexran_agent_pdcp_defs.h"
#include "flexran_agent_ran_api.h"

/**********************************
 * FlexRAN agent - technology PDCP API
 **********************************/

/* Send to the controller all the pdcp stat updates that occured during this subframe*/
int flexran_agent_pdcp_stats_reply(mid_t mod_id,       
          const report_config_t *report_config,
           Protocol__FlexUeStatsReport **ue_report,
           Protocol__FlexCellStatsReport **cell_report);

/* Get the stats from RAN API and aggregate them per USER*/
void flexran_agent_pdcp_aggregate_stats(const mid_t mod_id,
					const mid_t ue_id,
					Protocol__FlexPdcpStats *pdcp_aggr_stats);

/*Register technology specific interface callbacks*/
int flexran_agent_register_pdcp_xface(mid_t mod_id, AGENT_PDCP_xface *xface);

/*Unregister technology specific callbacks*/
int flexran_agent_unregister_pdcp_xface(mid_t mod_id, AGENT_PDCP_xface*xface);

#endif
