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
 
/*! \file ngap_gNB_itti_messaging.c
 * \brief ngap itti messages handlers for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \date 2020
 * \version 0.1
 */
 
#include "intertask_interface.h"

#include "ngap_gNB_itti_messaging.h"

void ngap_gNB_itti_send_sctp_data_req(instance_t instance, int32_t assoc_id, uint8_t *buffer,
                                      uint32_t buffer_length, uint16_t stream)
{
  MessageDef      *message_p;
  sctp_data_req_t *sctp_data_req;

  message_p = itti_alloc_new_message(TASK_NGAP, 0, SCTP_DATA_REQ);

  sctp_data_req = &message_p->ittiMsg.sctp_data_req;

  sctp_data_req->assoc_id      = assoc_id;
  sctp_data_req->buffer        = buffer;
  sctp_data_req->buffer_length = buffer_length;
  sctp_data_req->stream        = stream;

  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}

void ngap_gNB_itti_send_nas_downlink_ind(instance_t instance,
    uint16_t ue_initial_id,
    uint32_t gNB_ue_ngap_id,
    uint8_t *nas_pdu,
    uint32_t nas_pdu_length)
{
  MessageDef          *message_p;
  ngap_downlink_nas_t *ngap_downlink_nas;

  message_p = itti_alloc_new_message(TASK_NGAP, 0, NGAP_DOWNLINK_NAS);

  ngap_downlink_nas = &message_p->ittiMsg.ngap_downlink_nas;

  ngap_downlink_nas->ue_initial_id  = ue_initial_id;
  ngap_downlink_nas->gNB_ue_ngap_id = gNB_ue_ngap_id;
  ngap_downlink_nas->nas_pdu.buffer = malloc(sizeof(uint8_t) * nas_pdu_length);
  memcpy(ngap_downlink_nas->nas_pdu.buffer, nas_pdu, nas_pdu_length);
  ngap_downlink_nas->nas_pdu.length = nas_pdu_length;

  itti_send_msg_to_task(TASK_RRC_GNB, instance, message_p);
}

void ngap_gNB_itti_send_sctp_close_association(instance_t instance, int32_t assoc_id)
{
  MessageDef               *message_p = NULL;
  sctp_close_association_t *sctp_close_association_p = NULL;

  message_p = itti_alloc_new_message(TASK_NGAP, 0, SCTP_CLOSE_ASSOCIATION);
  sctp_close_association_p = &message_p->ittiMsg.sctp_close_association;
  sctp_close_association_p->assoc_id      = assoc_id;

  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}

