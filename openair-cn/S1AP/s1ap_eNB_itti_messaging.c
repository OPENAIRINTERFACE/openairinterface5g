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
#include "intertask_interface.h"

#include "s1ap_eNB_itti_messaging.h"

void s1ap_eNB_itti_send_sctp_data_req(instance_t instance, int32_t assoc_id, uint8_t *buffer,
                                      uint32_t buffer_length, uint16_t stream)
{
  MessageDef      *message_p;
  sctp_data_req_t *sctp_data_req;

  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_DATA_REQ);

  sctp_data_req = &message_p->ittiMsg.sctp_data_req;

  sctp_data_req->assoc_id      = assoc_id;
  sctp_data_req->buffer        = buffer;
  sctp_data_req->buffer_length = buffer_length;
  sctp_data_req->stream        = stream;

  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}

void s1ap_eNB_itti_send_nas_downlink_ind(instance_t instance,
    uint16_t ue_initial_id,
    uint32_t eNB_ue_s1ap_id,
    uint8_t *nas_pdu,
    uint32_t nas_pdu_length)
{
  MessageDef          *message_p;
  s1ap_downlink_nas_t *s1ap_downlink_nas;

  message_p = itti_alloc_new_message(TASK_S1AP, S1AP_DOWNLINK_NAS);

  s1ap_downlink_nas = &message_p->ittiMsg.s1ap_downlink_nas;

  s1ap_downlink_nas->ue_initial_id  = ue_initial_id;
  s1ap_downlink_nas->eNB_ue_s1ap_id = eNB_ue_s1ap_id;
  s1ap_downlink_nas->nas_pdu.buffer = nas_pdu;
  s1ap_downlink_nas->nas_pdu.length = nas_pdu_length;

  itti_send_msg_to_task(TASK_RRC_ENB, instance, message_p);
}

void s1ap_eNB_itti_send_sctp_close_association(instance_t instance, int32_t assoc_id)
{
  MessageDef               *message_p = NULL;
  sctp_close_association_t *sctp_close_association_p = NULL;

  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_CLOSE_ASSOCIATION);
  sctp_close_association_p = &message_p->ittiMsg.sctp_close_association;
  sctp_close_association_p->assoc_id      = assoc_id;

  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}

