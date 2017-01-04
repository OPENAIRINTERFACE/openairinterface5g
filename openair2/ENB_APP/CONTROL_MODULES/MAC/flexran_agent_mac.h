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

/*! \file flexran_agent_mac.h
 * \brief FlexRAN agent message handler APIs for MAC layer
 * \author  Xenofon Foukas, Mohamed Kassem and Navid Nikaein
 * \date 2016
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_MAC_H_
#define FLEXRAN_AGENT_MAC_H_

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"

#include "flexran_agent_common.h"
#include "flexran_agent_extern.h"

/* These types will be used to give
   instructions for the type of stats reports
   we need to create */
typedef struct {
  uint16_t ue_rnti;
  uint32_t ue_report_flags; /* Indicates the report elements
			       required for this UE id. See
			       FlexRAN specification 1.2.4.2 */
} ue_report_type_t;

typedef struct {
  uint16_t cc_id;
  uint32_t cc_report_flags; /* Indicates the report elements
			      required for this CC index. See
			      FlexRAN specification 1.2.4.3 */
} cc_report_type_t;

typedef struct {
  int nr_ue;
  ue_report_type_t *ue_report_type;
  int nr_cc;
  cc_report_type_t *cc_report_type;
} report_config_t;

typedef struct stats_request_config_s{
  uint8_t report_type;
  uint8_t report_frequency;
  uint16_t period; /*In number of subframes*/
  report_config_t *config;
} stats_request_config_t;

/* Initialization function for the agent structures etc */
void flexran_agent_init_mac_agent(mid_t mod_id);

int flexran_agent_mac_handle_stats(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);

/* Statistics request protocol message constructor and destructor */
int flexran_agent_mac_stats_request(mid_t mod_id, xid_t xid, const stats_request_config_t *report_config, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_stats_request(Protocol__FlexranMessage *msg);

/* Statistics reply protocol message constructor and destructor */
int flexran_agent_mac_stats_reply(mid_t mod_id, xid_t xid, const report_config_t *report_config, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_stats_reply(Protocol__FlexranMessage *msg);

/* Scheduling request information protocol message constructor and estructor */
int flexran_agent_mac_sr_info(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_sr_info(Protocol__FlexranMessage *msg);

/* Subframe trigger protocol msssage constructor and destructor */
int flexran_agent_mac_sf_trigger(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_sf_trigger(Protocol__FlexranMessage *msg);

/* DL MAC scheduling decision protocol message constructor (empty command) and destructor */ 
int flexran_agent_mac_create_empty_dl_config(mid_t mod_id, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_dl_config(Protocol__FlexranMessage *msg);

int flexran_agent_mac_handle_dl_mac_config(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);


/**********************************
 * FlexRAN agent - technology mac API
 **********************************/

/*Inform controller about received scheduling requests during a subframe*/
void flexran_agent_send_sr_info(mid_t mod_id);

/*Inform the controller about the current UL/DL subframe*/
void flexran_agent_send_sf_trigger(mid_t mod_id);

/// Send to the controller all the mac stat updates that occured during this subframe
/// based on the stats request configuration
void flexran_agent_send_update_mac_stats(mid_t mod_id);

/// Provide to the scheduler a pending dl_mac_config message
void flexran_agent_get_pending_dl_mac_config(mid_t mod_id, Protocol__FlexranMessage **msg);

/*Register technology specific interface callbacks*/
int flexran_agent_register_mac_xface(mid_t mod_id, AGENT_MAC_xface *xface);

/*Unregister technology specific callbacks*/
int flexran_agent_unregister_mac_xface(mid_t mod_id, AGENT_MAC_xface*xface);

#endif
