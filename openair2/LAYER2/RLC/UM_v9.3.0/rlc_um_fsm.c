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
      AssertFatal(1==0,"RLC-UM FSM WARNING PROTOCOL ERROR - EVENT NOT EXPECTED FROM NULL_STATE");
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
