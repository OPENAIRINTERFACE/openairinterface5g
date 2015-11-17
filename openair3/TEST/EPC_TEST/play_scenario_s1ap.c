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
                                play_scenario_s1ap.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tree.h"
#include "queue.h"


#include "intertask_interface.h"
#include "messages_types.h"
#include "platform_types.h"
#include "s1ap_common.h"
#include "s1ap_eNB_defs.h"
#include "s1ap_eNB_default_values.h"
#include "s1ap_eNB_management_procedures.h"
#include "s1ap_eNB.h"
#include "play_scenario.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"


//------------------------------------------------------------------------------
extern void s1ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp);
extern void s1ap_eNB_handle_register_eNB(instance_t instance, s1ap_register_enb_req_t *s1ap_register_eNB);
//------------------------------------------------------------------------------
void et_s1ap_eNB_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind)
{
  int            result = 0;
  et_event_t     event;

  DevAssert(sctp_data_ind != NULL);

  memset((void*)&event, 0, sizeof(event));

  event.code = ET_EVENT_RX_S1AP;
  event.u.s1ap_data_ind.sctp_datahdr.tsn                       = 0;
  event.u.s1ap_data_ind.sctp_datahdr.stream                    = sctp_data_ind->stream;
  event.u.s1ap_data_ind.sctp_datahdr.ssn                       = 0;
  event.u.s1ap_data_ind.sctp_datahdr.ppid                      = 18; // find constant

  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_pos = 0;
  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_allocated_size = sctp_data_ind->buffer_length;
  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream = NULL;
  if ((sctp_data_ind->buffer_length > 0) && (NULL != sctp_data_ind->buffer)) {
    event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream   = calloc(1, sctp_data_ind->buffer_length);
    memcpy(event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream,
           sctp_data_ind->buffer,
           sctp_data_ind->buffer_length);

    if (et_s1ap_decode_pdu(
           &event.u.s1ap_data_ind.sctp_datahdr.payload.pdu,
           &event.u.s1ap_data_ind.sctp_datahdr.payload.message,
           event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream,
           event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_allocated_size) < 0) {
      AssertFatal (0, "ERROR %s() Cannot decode RX S1AP message!\n", __FUNCTION__);
    }

  }

  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

  et_scenario_fsm_notify_event(event);

}
//------------------------------------------------------------------------------
void *et_s1ap_eNB_task(void *arg)
{
  MessageDef *received_msg = NULL;
  int         result;

  S1AP_DEBUG("Starting S1AP layer\n");

  s1ap_eNB_prepare_internal_data();

  itti_mark_task_ready(TASK_S1AP);
  MSC_START_USE();

  while (1) {
    itti_receive_msg(TASK_S1AP, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
    case TERMINATE_MESSAGE:
      itti_exit_task();
      break;

    case S1AP_REGISTER_ENB_REQ: {
      /* Register a new eNB.
       * in Virtual mode eNBs will be distinguished using the mod_id/
       * Each eNB has to send an S1AP_REGISTER_ENB message with its
       * own parameters.
       */
      s1ap_eNB_handle_register_eNB(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                   &S1AP_REGISTER_ENB_REQ(received_msg));
    }
    break;

    case SCTP_NEW_ASSOCIATION_RESP: {
      s1ap_eNB_handle_sctp_association_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                            &received_msg->ittiMsg.sctp_new_association_resp);
    }
    break;

    case SCTP_DATA_IND: {
      et_s1ap_eNB_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
    }
    break;


    default:
      S1AP_ERROR("Received unhandled message: %d:%s\n",
                 ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

    received_msg = NULL;
  }

  return NULL;
}


