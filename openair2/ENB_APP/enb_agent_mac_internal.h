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

#include "enb_agent_mac.h"
#include "enb_agent_common.h"


/*This will be used for producing continuous status updates for the MAC
 *Needs to be thread-safe
 */
typedef struct {
  /*Flag showing if continuous mac stats update is enabled*/
  uint8_t is_initialized;
  volatile uint8_t cont_update;
  xid_t xid;
  Protocol__ProgranMessage *stats_req;
  Protocol__ProgranMessage *prev_stats_reply;

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

Protocol__ProgranMessage * enb_agent_generate_diff_mac_stats_report(Protocol__ProgranMessage *new_report,
								    Protocol__ProgranMessage *old_report);

Protocol__PrpUeStatsReport * copy_ue_stats_report(Protocol__PrpUeStatsReport * original);

Protocol__PrpCellStatsReport * copy_cell_stats_report(Protocol__PrpCellStatsReport *original);

Protocol__PrpRlcBsr * copy_rlc_report(Protocol__PrpRlcBsr * original);

Protocol__PrpUlCqiReport * copy_ul_cqi_report(Protocol__PrpUlCqiReport * original);

Protocol__PrpDlCqiReport * copy_dl_cqi_report(Protocol__PrpDlCqiReport * original);

Protocol__PrpPagingBufferReport * copy_paging_buffer_report(Protocol__PrpPagingBufferReport *original);

Protocol__PrpDlCsi * copy_csi_report(Protocol__PrpDlCsi * original);

Protocol__PrpNoiseInterferenceReport * copy_noise_inter_report(Protocol__PrpNoiseInterferenceReport *original);

int compare_ue_stats_reports(Protocol__PrpUeStatsReport *rep1,
			    Protocol__PrpUeStatsReport *rep2);

int compare_cell_stats_reports(Protocol__PrpCellStatsReport *rep1,
			    Protocol__PrpCellStatsReport *rep2);

#endif /*ENB_AGENT_MAC_INTERNAL_H_*/
