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

/*! \file flexran_agent_s1ap.h
 * \brief FlexRAN agent S1AP Control Module
 * \author navid nikaein
 * \date 2017
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_S1AP_H_
#define FLEXRAN_AGENT_S1AP_H_

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"


#include "flexran_agent_common.h"
#include "flexran_agent_defs.h"
#include "flexran_agent_s1ap_defs.h"
#include "flexran_agent_ran_api.h"

/***************************************
 * FlexRAN agent - technology S1AP API *
 ***************************************/

/* Send to the controller all the S1AP configs */
void flexran_agent_fill_s1ap_cell_config(mid_t mod_id,
                                         Protocol__FlexS1apConfig **s1ap_config);

/* Free allocated S1AP cell configs */
void flexran_agent_free_s1ap_cell_config(Protocol__FlexS1apConfig **s1ap);

/* Fill the stats message for S1AP */
int flexran_agent_s1ap_stats_reply(mid_t mod_id,
                                   Protocol__FlexUeStatsReport **ue_report,
                                   int n_ue,
                                   uint32_t ue_flags);

/* Free allocated S1AP stats message */
void flexran_agent_s1ap_destroy_stats_reply(Protocol__FlexStatsReply *reply);

/* Add or remove MME updates */
void flexran_agent_handle_mme_update(mid_t mod_id,
                                     size_t n_mme,
                                     Protocol__FlexS1apMme **mme);

/* Set a new PLMN configuration */
void flexran_agent_handle_plmn_update(mid_t mod_id,
                                      int CC_id,
                                      size_t n_plmn,
                                      Protocol__FlexPlmn **plmn_id);

/* Register technology specific interface callbacks */
int flexran_agent_register_s1ap_xface(mid_t mod_id);

/* Unregister technology specific callbacks */
int flexran_agent_unregister_s1ap_xface(mid_t mod_id);

#endif
