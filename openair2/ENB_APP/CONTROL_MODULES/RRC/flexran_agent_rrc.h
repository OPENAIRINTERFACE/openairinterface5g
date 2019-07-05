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

/*! \file flexran_agent_rrc.h
 * \brief FlexRAN agent Control Module RRC header
 * \author shahab SHARIAT BAGHERI 
 * \date 2017
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_RRC_H_
#define FLEXRAN_AGENT_RRC_H_

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"


#include "flexran_agent_common.h"
#include "flexran_agent_rrc_defs.h"
// for flexran_agent_get_rrc_xface()
#include "flexran_agent_extern.h"


/* UE state change message constructor and destructor */
void flexran_agent_ue_state_change(mid_t mod_id, uint32_t rnti, uint8_t state_change);
int flexran_agent_destroy_ue_state_change(Protocol__FlexranMessage *msg);


/**********************************
 * FlexRAN agent - technology RRC API
 **********************************/

/* Send to the controller all the rrc stat updates that occured during this subframe*/
// void flexran_agent_send_update_rrc_stats(mid_t mod_id);

/* this is called by RRC as a part of rrc xface  . The controller previously requested  this*/ 
void flexran_trigger_rrc_measurements (mid_t mod_id, LTE_MeasResults_t *);

/* Statistics reply protocol message constructor and destructor */
int flexran_agent_rrc_stats_reply(mid_t mod_id, const report_config_t *report_config, Protocol__FlexUeStatsReport **ue_report, Protocol__FlexCellStatsReport **cell_report);
int flexran_agent_rrc_destroy_stats_reply(Protocol__FlexStatsReply *reply);

/* Statistic reply for GTP statistics which OAI stores also in the RRC layer.
 * This might be moved to a separate GTP module in the future */
int flexran_agent_rrc_gtp_stats_reply(mid_t mod_id, const report_config_t *report_config, Protocol__FlexUeStatsReport **ue_report, Protocol__FlexCellStatsReport **cell_report);
int flexran_agent_rrc_gtp_destroy_stats_reply(Protocol__FlexStatsReply *reply);

/* Fill the RRC part of a ue_config message */
void flexran_agent_fill_rrc_ue_config(mid_t mod_id, rnti_t rnti,
    Protocol__FlexUeConfig *ue_conf);

/* Fill the RRC part of an cell_config message */
void flexran_agent_fill_rrc_cell_config(mid_t mod_id, uint8_t cc_id,
    Protocol__FlexCellConfig *conf);

/*Register technology specific interface callbacks*/
int flexran_agent_register_rrc_xface(mid_t mod_id);

/*Unregister technology specific callbacks*/
int flexran_agent_unregister_rrc_xface(mid_t mod_id);

#endif
