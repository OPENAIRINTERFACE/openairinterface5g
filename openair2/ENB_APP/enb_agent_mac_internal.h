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

/*! \file enb_agent_mac_internal.h
 * \brief Implementation specific definitions for the eNB MAC agent
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#ifndef ENB_AGENT_MAC_INTERNAL_H_
#define ENB_AGENT_MAC_INTERNAL_H_

#include <pthread.h>

#include <yaml.h>

#include "enb_agent_mac.h"
#include "enb_agent_common.h"
#include "enb_agent_defs.h"

/*This will be used for producing continuous status updates for the MAC
 *Needs to be thread-safe
 */
typedef struct {
  /*Flag showing if continuous mac stats update is enabled*/
  uint8_t is_initialized;
  volatile uint8_t cont_update;
  xid_t xid;
  Protocol__FlexranMessage *stats_req;
  Protocol__FlexranMessage *prev_stats_reply;

  pthread_mutex_t *mutex;
} mac_stats_updates_context_t;

/*Array holding the last stats reports for each eNB. Used for continuous reporting*/
mac_stats_updates_context_t mac_stats_context[NUM_MAX_ENB];

/*Functions to initialize and destroy the struct required for the
 *continuous stats update report*/
err_code_t enb_agent_init_cont_mac_stats_update(mid_t mod_id);

err_code_t enb_agent_destroy_cont_mac_stats_update(mid_t mod_id);


/*Enable/Disable the continuous stats update service for the MAC*/
err_code_t enb_agent_enable_cont_mac_stats_update(mid_t mod_id, xid_t xid,
						  stats_request_config_t *stats_req);

err_code_t enb_agent_disable_cont_mac_stats_update(mid_t mod_id);

Protocol__FlexranMessage * enb_agent_generate_diff_mac_stats_report(Protocol__FlexranMessage *new_report,
								    Protocol__FlexranMessage *old_report);

Protocol__FlexUeStatsReport * copy_ue_stats_report(Protocol__FlexUeStatsReport * original);

Protocol__FlexCellStatsReport * copy_cell_stats_report(Protocol__FlexCellStatsReport *original);

Protocol__FlexRlcBsr * copy_rlc_report(Protocol__FlexRlcBsr * original);

Protocol__FlexUlCqiReport * copy_ul_cqi_report(Protocol__FlexUlCqiReport * original);

Protocol__FlexDlCqiReport * copy_dl_cqi_report(Protocol__FlexDlCqiReport * original);

Protocol__FlexPagingBufferReport * copy_paging_buffer_report(Protocol__FlexPagingBufferReport *original);

Protocol__FlexDlCsi * copy_csi_report(Protocol__FlexDlCsi * original);

Protocol__FlexNoiseInterferenceReport * copy_noise_inter_report(Protocol__FlexNoiseInterferenceReport *original);

int compare_ue_stats_reports(Protocol__FlexUeStatsReport *rep1,
			    Protocol__FlexUeStatsReport *rep2);

int compare_cell_stats_reports(Protocol__FlexCellStatsReport *rep1,
			    Protocol__FlexCellStatsReport *rep2);


/* Functions for parsing the MAC agent policy reconfiguration command */

int parse_mac_config(mid_t mod_id, yaml_parser_t *parser);

int parse_dl_scheduler_config(mid_t mod_id, yaml_parser_t *parser);

int parse_dl_scheduler_parameters(mid_t mod_id, yaml_parser_t *parser);

int load_dl_scheduler_function(mid_t mod_id, const char *function_name);

#endif /*ENB_AGENT_MAC_INTERNAL_H_*/
