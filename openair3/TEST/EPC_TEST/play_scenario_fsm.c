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

/*
                                play_scenario_fsm.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */
#include <stdio.h>

#include "intertask_interface.h"
#include "platform_types.h"
#include "assertions.h"
#include "play_scenario.h"


et_scenario_t  *g_scenario  = NULL;
et_fsm_state_t  g_fsm_state = ET_FSM_STATE_NULL;
//------------------------------------------------------------------------------
int et_scenario_fsm_notify_event_state_null(et_event_t event)
{
  et_packet_t                    *packet            = NULL;
  const Enb_properties_array_t   *enb_properties_p  = NULL;
  uint32_t                        register_enb_pending;

  switch (event.code){
    case ET_EVENT_INIT:
      AssertFatal(NULL == g_scenario, "Current scenario not ended");
      g_scenario = event.u.init.scenario;
      packet = g_scenario->list_packet;
      while (NULL != packet) {
        switch (packet->sctp_hdr.chunk_type) {

          case SCTP_CID_DATA :
            // no init in this scenario, may be sub-scenario
            if (packet->action == ET_PACKET_ACTION_S1C_SEND) {
            } else if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
              g_scenario->waited_packet = packet;
            } else {
              packet->status = ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT;
              packet = packet->next;
            }
            break;
          case SCTP_CID_INIT:
          case SCTP_CID_INIT_ACK:
            enb_properties_p = enb_config_get();
            /* Try to register each eNB */
            g_fsm_state = ET_FSM_STATE_CONNECTING_SCTP;
            register_enb_pending = et_eNB_app_register (enb_properties_p);
            break;
          case SCTP_CID_HEARTBEAT:
          case SCTP_CID_HEARTBEAT_ACK:
          case SCTP_CID_COOKIE_ECHO:
          case SCTP_CID_COOKIE_ACK:
          case SCTP_CID_ECN_ECNE:
          case SCTP_CID_ECN_CWR:
            packet->status = ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT;
            packet = packet->next;
            break;
          case SCTP_CID_ABORT:
          case SCTP_CID_SHUTDOWN:
          case SCTP_CID_SHUTDOWN_ACK:
          case SCTP_CID_ERROR:
          case SCTP_CID_SHUTDOWN_COMPLETE:
            AssertFatal(0, "The scenario should be cleaned (packet %s cannot be processed at this time)", et_chunk_type_cid2str(packet->sctp_hdr.chunk_type));
            break;

          default:
            packet->status = ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT;
            packet = packet->next;
        }
      }
      fprintf(stderr, "No Packet found in this scenario: %s\n", g_scenario->name);
      return -1;
      break;

    case ET_EVENT_STOP:
      break;

    default:
      AssertFatal(0, "Case event %d not handled in ET_FSM_STATE_NULL", event.code);
  }
  return 0;
}

//------------------------------------------------------------------------------
int et_scenario_fsm_notify_event(et_event_t event)
{
  AssertFatal((event.code >= ET_EVENT_START) && (event.code < ET_EVENT_END), "Unknown et_event_t.code %d", event.code);

  switch (g_fsm_state){
    case ET_FSM_STATE_NULL: return et_scenario_fsm_notify_event_state_null(event); break;
    case ET_FSM_STATE_CONNECTING_SCTP: return et_scenario_fsm_notify_event_state_null(event); break;
    case ET_FSM_STATE_WAITING_TX_EVENT: return et_scenario_fsm_notify_event_state_null(event); break;
    case ET_FSM_STATE_WAITING_RX_EVENT: return et_scenario_fsm_notify_event_state_null(event); break;
    default:
      return -1;
  }
}
