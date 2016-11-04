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

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file flexran_agent_scheduler_dlsch_ue_remote.h
 * \brief Local stub for remote scheduler used by the controller
 * \author Xenofon Foukas
 * \date 2016
 * \email: x.foukas@sms.ed.ac.uk
 * \version 0.1
 * @ingroup _mac

 */

#ifndef __LAYER2_MAC_FLEXRAN_AGENT_SCHEDULER_DLSCH_UE_REMOTE_H__
#define __LAYER2_MAC_FLEXRAN_AGENT_SCHEDULER_DLSCH_UE_REMOTE_H___

#include "flexran.pb-c.h"
#include "header.pb-c.h"

#include "ENB_APP/flexran_agent_defs.h"
#include "flexran_agent_mac.h"
#include "LAYER2/MAC/flexran_agent_mac_proto.h"

#include <sys/queue.h>

// Maximum value of schedule ahead of time
// Required to identify if a dl_command is for the future or not
#define SCHED_AHEAD_SUBFRAMES 20

typedef struct dl_mac_config_element_s {
  Protocol__FlexranMessage *dl_info;
  TAILQ_ENTRY(dl_mac_config_element_s) configs;
} dl_mac_config_element_t;

TAILQ_HEAD(DlMacConfigHead, dl_mac_config_element_s);

/*
 * Default scheduler used by the eNB agent
 */
void flexran_schedule_ue_spec_remote(mid_t mod_id, uint32_t frame, uint32_t subframe,
			     int *mbsfn_flag, Protocol__FlexranMessage **dl_info);


// Find the difference in subframes from the given subframe
// negative for older value
// 0 for equal
// positive for future value
// Based on  
int get_sf_difference(mid_t mod_id, uint32_t sfn_sf);

#endif
