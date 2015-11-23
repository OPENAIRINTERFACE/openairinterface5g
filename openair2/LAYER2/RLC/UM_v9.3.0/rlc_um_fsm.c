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
#define RLC_UM_MODULE 1
#define RLC_UM_FSM_C 1
#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "rlc_um.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"


//-----------------------------------------------------------------------------
int
rlc_um_fsm_notify_event (
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t *rlc_pP, uint8_t eventP)
{

  switch (rlc_pP->protocol_state) {
    //-------------------------------
    // RLC_NULL_STATE
    //-------------------------------
  case RLC_NULL_STATE:
    switch (eventP) {
    case RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_DATA_TRANSFER_READY_STATE_EVENT:
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM RLC_NULL_STATE -> RLC_DATA_TRANSFER_READY_STATE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      rlc_pP->protocol_state = RLC_DATA_TRANSFER_READY_STATE;
      return 1;
      break;

    default:
      LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM WARNING PROTOCOL ERROR - EVENT %02X hex NOT EXPECTED FROM NULL_STATE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            eventP);
      mac_xface->macphy_exit("RLC-UM FSM WARNING PROTOCOL ERROR - EVENT NOT EXPECTED FROM NULL_STATE");
      return 0;
    }

    break;

    //-------------------------------
    // RLC_DATA_TRANSFER_READY_STATE
    //-------------------------------
  case RLC_DATA_TRANSFER_READY_STATE:
    switch (eventP) {
    case RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_NULL_STATE_EVENT:
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM RLC_DATA_TRANSFER_READY_STATE -> RLC_NULL_STATE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      rlc_pP->protocol_state = RLC_NULL_STATE;
      return 1;
      break;

    case RLC_UM_RECEIVE_CRLC_SUSPEND_REQ_EVENT:
    case RLC_UM_TRANSMIT_CRLC_SUSPEND_CNF_EVENT:
      LOG_N(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM RLC_DATA_TRANSFER_READY_STATE -> RLC_LOCAL_SUSPEND_STATE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      rlc_pP->protocol_state = RLC_LOCAL_SUSPEND_STATE;
      return 1;
      break;

    default:
      LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM WARNING PROTOCOL ERROR - EVENT %02X hex NOT EXPECTED FROM DATA_TRANSFER_READY_STATE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            eventP);
      return 0;
    }

    break;

    //-------------------------------
    // RLC_LOCAL_SUSPEND_STATE
    //-------------------------------
  case RLC_LOCAL_SUSPEND_STATE:
    switch (eventP) {
    case RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_NULL_STATE_EVENT:
      LOG_N(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM RLC_LOCAL_SUSPEND_STATE -> RLC_NULL_STATE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      rlc_pP->protocol_state = RLC_NULL_STATE;
      return 1;
      break;

    case RLC_UM_RECEIVE_CRLC_RESUME_REQ_EVENT:
      LOG_N(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM RLC_LOCAL_SUSPEND_STATE -> RLC_DATA_TRANSFER_READY_STATE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      rlc_pP->protocol_state = RLC_DATA_TRANSFER_READY_STATE;
      return 1;
      break;

    default:
      LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM  WARNING PROTOCOL ERROR - EVENT %02X hex NOT EXPECTED FROM RLC_LOCAL_SUSPEND_STATE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            eventP);
      return 0;
    }

    break;

  default:
    LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" FSM ERROR UNKNOWN STATE %d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          rlc_pP->protocol_state);
    return 0;
  }
}
