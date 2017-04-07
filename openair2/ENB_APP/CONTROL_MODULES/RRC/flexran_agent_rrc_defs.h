/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2016 Eurecom

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

/*! \file flexran_agent_rrc_defs.h
 * \brief FlexRAN agent - RRC interface primitives
 * \author shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 * \mail 
 */

#ifndef __FLEXRAN_AGENT_RRC_PRIMITIVES_H__
#define __FLEXRAN_AGENT_RRC_PRIMITIVES_H__

#include "PHY/extern.h"
#include "flexran_agent_defs.h"
#include "flexran.pb-c.h"
#include "header.pb-c.h"
#include "MeasResults.h"

#define RINGBUFFER_SIZE 100

/* FLEXRAN AGENT-RRC Interface */
typedef struct {
  
  /// Inform the controller about the scheduling requests received during the subframe
  //void (*flexran_agent_send_update_rrc_stats)(mid_t mod_id);
  
   /// Notify the controller for a state change of a particular UE, by sending the proper
  /// UE state change message (ACTIVATION, DEACTIVATION, HANDOVER)
  int (*flexran_agent_notify_ue_state_change)(mid_t mod_id, uint32_t rnti,
                 uint32_t state_change);

  void (*flexran_trigger_rrc_measurements)(mid_t mod_id, MeasResults_t*  measResults);
  
} AGENT_RRC_xface;

#endif
