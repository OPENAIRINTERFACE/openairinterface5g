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

#include "intertask_interface.h"

#include "sctp_common.h"
#include "sctp_eNB_itti_messaging.h"

int sctp_itti_send_new_message_ind(task_id_t task_id, uint32_t assoc_id, uint8_t *buffer,
                                   uint32_t buffer_length, uint16_t stream)
{
  MessageDef      *message_p;
  sctp_data_ind_t *sctp_data_ind_p;

  message_p = itti_alloc_new_message(TASK_SCTP, SCTP_DATA_IND);

  sctp_data_ind_p = &message_p->ittiMsg.sctp_data_ind;

  sctp_data_ind_p->buffer = itti_malloc(TASK_SCTP, task_id, sizeof(uint8_t) * buffer_length);

  /* Copy the buffer */
  memcpy((void *)sctp_data_ind_p->buffer, (void *)buffer, buffer_length);

  sctp_data_ind_p->stream        = stream;
  sctp_data_ind_p->buffer_length = buffer_length;
  sctp_data_ind_p->assoc_id      = assoc_id;

  return itti_send_msg_to_task(task_id, INSTANCE_DEFAULT, message_p);
}

int sctp_itti_send_association_resp(task_id_t task_id, instance_t instance,
                                    int32_t assoc_id,
                                    uint16_t cnx_id, enum sctp_state_e state,
                                    uint16_t out_streams, uint16_t in_streams)
{
  MessageDef                  *message_p;
  sctp_new_association_resp_t *sctp_new_association_resp_p;

  message_p = itti_alloc_new_message(TASK_SCTP, SCTP_NEW_ASSOCIATION_RESP);

  sctp_new_association_resp_p = &message_p->ittiMsg.sctp_new_association_resp;

  sctp_new_association_resp_p->in_streams  = in_streams;
  sctp_new_association_resp_p->out_streams = out_streams;
  sctp_new_association_resp_p->sctp_state  = state;
  sctp_new_association_resp_p->assoc_id    = assoc_id;
  sctp_new_association_resp_p->ulp_cnx_id  = cnx_id;

  return itti_send_msg_to_task(task_id, instance, message_p);
}

int sctp_itti_send_association_ind(task_id_t task_id, instance_t instance,
                                   int32_t assoc_id, uint16_t port,
                                   uint16_t out_streams, uint16_t in_streams)
{
  MessageDef                 *message_p;
  sctp_new_association_ind_t *sctp_new_association_ind_p;

  message_p = itti_alloc_new_message(TASK_SCTP, SCTP_NEW_ASSOCIATION_IND);

  sctp_new_association_ind_p = &message_p->ittiMsg.sctp_new_association_ind;

  sctp_new_association_ind_p->assoc_id    = assoc_id;
  sctp_new_association_ind_p->port        = port;
  sctp_new_association_ind_p->out_streams = out_streams;
  sctp_new_association_ind_p->in_streams  = in_streams;

  return itti_send_msg_to_task(task_id, instance, message_p);
}
