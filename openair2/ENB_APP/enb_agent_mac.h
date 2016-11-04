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

/*! \file enb_agent_mac.h
 * \brief enb agent message handler APIs for MAC layer
 * \author  Xenofon Foukas, Mohamed Kassem and Navid Nikaein
 * \date 2016
 * \version 0.1
 */

#ifndef ENB_AGENT_MAC_H_
#define ENB_AGENT_MAC_H_

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"

#include "enb_agent_common.h"
#include "enb_agent_extern.h"

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
void enb_agent_init_mac_agent(mid_t mod_id);

int enb_agent_mac_handle_stats(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);

/* Statistics request protocol message constructor and destructor */
int enb_agent_mac_stats_request(mid_t mod_id, xid_t xid, const stats_request_config_t *report_config, Protocol__FlexranMessage **msg);
int enb_agent_mac_destroy_stats_request(Protocol__FlexranMessage *msg);

/* Statistics reply protocol message constructor and destructor */
int enb_agent_mac_stats_reply(mid_t mod_id, xid_t xid, const report_config_t *report_config, Protocol__FlexranMessage **msg);
int enb_agent_mac_destroy_stats_reply(Protocol__FlexranMessage *msg);

/* Scheduling request information protocol message constructor and estructor */
int enb_agent_mac_sr_info(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_mac_destroy_sr_info(Protocol__FlexranMessage *msg);

/* Subframe trigger protocol msssage constructor and destructor */
int enb_agent_mac_sf_trigger(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_mac_destroy_sf_trigger(Protocol__FlexranMessage *msg);

/* 
int enb_agent_mac_create_empty_dl_config(mid_t mod_id, Protocol__FlexranMessage **msg);
int enb_agent_mac_destroy_dl_config(Protocol__FlexranMessage *msg);

int enb_agent_mac_handle_dl_mac_config(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);


/**********************************
 * eNB agent - technology mac API
 **********************************/

/*Inform controller about received scheduling requests during a subframe*/
void enb_agent_send_sr_info(mid_t mod_id);

/*Inform the controller about the current UL/DL subframe*/
void enb_agent_send_sf_trigger(mid_t mod_id);

/// Send to the controller all the mac stat updates that occured during this subframe
/// based on the stats request configuration
void enb_agent_send_update_mac_stats(mid_t mod_id);

/// Provide to the scheduler a pending dl_mac_config message
void enb_agent_get_pending_dl_mac_config(mid_t mod_id, Protocol__FlexranMessage **msg);

/*Register technology specific interface callbacks*/
int enb_agent_register_mac_xface(mid_t mod_id, AGENT_MAC_xface *xface);

/*Unregister technology specific callbacks*/
int enb_agent_unregister_mac_xface(mid_t mod_id, AGENT_MAC_xface*xface);

#endif
