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
// for flexran_agent_get_mac_xface()
#include "flexran_agent_extern.h"

/* Type for a list of shared objects used in the user plane (schedulers) */
typedef struct flexran_agent_so_handle_s {
  char *name; // name of the object
  void *dl_handle; // handle of associated objects
  SLIST_ENTRY (flexran_agent_so_handle_s) entries;
} flexran_agent_so_handle_t;

/* Initialization function for the agent structures etc */
void flexran_agent_init_mac_agent(mid_t mod_id);

/* Scheduling request information protocol message constructor and estructor */
int flexran_agent_mac_sr_info(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_sr_info(Protocol__FlexranMessage *msg);

/* Subframe trigger protocol msssage constructor and destructor */
int flexran_agent_mac_sf_trigger(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_sf_trigger(Protocol__FlexranMessage *msg);

/* Statistics reply protocol message constructors (for UE and cell stats) and destructor */
int flexran_agent_mac_stats_reply_ue(mid_t mod_id,
                                     Protocol__FlexUeStatsReport **ue_report,
                                     int      n_ue,
                                     uint32_t ue_flags);
int flexran_agent_mac_stats_reply_cell(mid_t mod_id,
                                       Protocol__FlexCellStatsReport **cell_report,
                                       int      n_cc,
                                       uint32_t cc_flags);
int flexran_agent_mac_destroy_stats_reply(Protocol__FlexStatsReply *reply);

/* DL MAC scheduling decision protocol message constructor (empty command) and destructor */ 
int flexran_agent_mac_create_empty_dl_config(mid_t mod_id, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_dl_config(Protocol__FlexranMessage *msg);

/* UL MAC scheduling decision protocol message constructor (empty command) and destructor */ 
int flexran_agent_mac_create_empty_ul_config(mid_t mod_id, Protocol__FlexranMessage **msg);
int flexran_agent_mac_destroy_ul_config(Protocol__FlexranMessage *msg);

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

/* Fill the MAC part of an cell_config message */
void flexran_agent_fill_mac_cell_config(mid_t mod_id, uint8_t cc_id,
    Protocol__FlexCellConfig *conf);

/* Fill the MAC part of a ue_config message */
void flexran_agent_fill_mac_ue_config(mid_t mod_id, mid_t ue_id,
    Protocol__FlexUeConfig *ue_conf);

/* Fill the lc_ue_config->lc_config message */
void flexran_agent_fill_mac_lc_ue_config(mid_t mod_id, mid_t ue_id,
    Protocol__FlexLcUeConfig *lc_ue_conf);

/*Register technology specific interface callbacks*/
int flexran_agent_register_mac_xface(mid_t mod_id);

/*Unregister technology specific callbacks*/
int flexran_agent_unregister_mac_xface(mid_t mod_id);

/***************************************
 * FlexRAN agent - slice configuration *
 ***************************************/

/* Inform controller about possibility to update slice configuration */
void flexran_agent_slice_update(mid_t mod_id);

/* marks slice_config so that it can be applied later. Takes ownership of the
 * FlexSliceConfig message */
void prepare_update_slice_config(mid_t mod_id,
                                 Protocol__FlexSliceConfig **slice,
                                 int request_objects);

/* inserts a new ue_config into the structure keeping ue to slice association
 * updates and marks so it can be applied. Takes ownership of the FlexUeConfig message */
void prepare_ue_slice_assoc_update(mid_t mod_id, Protocol__FlexUeConfig **ue_config);

/* free slice_config part of flexCellConfig, filled in
 * flexran_agent_fill_mac_cell_config() */
void flexran_agent_destroy_mac_slice_config(Protocol__FlexCellConfig *conf);

/* information about a new (potentially relevant) control delegation message */
void flexran_agent_mac_inform_delegation(mid_t mod_id,
                                         Protocol__FlexControlDelegation *cdm);

/* fill the enb_config_reply with the shared objects in use by the MAC sublayer */
void flexran_agent_mac_fill_loaded_mac_objects(
    mid_t mod_id,
    Protocol__FlexEnbConfigReply *reply);

#endif
