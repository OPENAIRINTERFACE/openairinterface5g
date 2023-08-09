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

#include "rlc_entity.h"

#include <stdlib.h>

#include "rlc_entity_am.h"
#include "rlc_entity_um.h"

#include "LOG/log.h"

rlc_entity_t *new_rlc_entity_am(
    int rx_maxsize,
    int tx_maxsize,
    void (*deliver_sdu)(void *deliver_sdu_data, struct rlc_entity_t *entity,
                      char *buf, int size),
    void *deliver_sdu_data,
    void (*sdu_successful_delivery)(void *sdu_successful_delivery_data,
                                    struct rlc_entity_t *entity,
                                    int sdu_id),
    void *sdu_successful_delivery_data,
    void (*max_retx_reached)(void *max_retx_reached_data,
                             struct rlc_entity_t *entity),
    void *max_retx_reached_data,
    int t_reordering,
    int t_status_prohibit,
    int t_poll_retransmit,
    int poll_pdu,
    int poll_byte,
    int max_retx_threshold)
{
  rlc_entity_am_t *ret;

  ret = calloc(1, sizeof(rlc_entity_am_t));
  if (ret == NULL) {
    LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  ret->common.recv_pdu      = rlc_entity_am_recv_pdu;
  ret->common.buffer_status = rlc_entity_am_buffer_status;
  ret->common.generate_pdu  = rlc_entity_am_generate_pdu;

  ret->common.recv_sdu         = rlc_entity_am_recv_sdu;

  ret->common.set_time = rlc_entity_am_set_time;

  ret->common.discard_sdu = rlc_entity_am_discard_sdu;

  ret->common.reestablishment = rlc_entity_am_reestablishment;

  ret->common.delete = rlc_entity_am_delete;

  ret->common.deliver_sdu      = deliver_sdu;
  ret->common.deliver_sdu_data = deliver_sdu_data;

  ret->common.sdu_successful_delivery      = sdu_successful_delivery;
  ret->common.sdu_successful_delivery_data = sdu_successful_delivery_data;

  ret->common.max_retx_reached      = max_retx_reached;
  ret->common.max_retx_reached_data = max_retx_reached_data;

  ret->rx_maxsize         = rx_maxsize;
  ret->tx_maxsize         = tx_maxsize;
  ret->t_reordering       = t_reordering;
  ret->t_status_prohibit  = t_status_prohibit;
  ret->t_poll_retransmit  = t_poll_retransmit;
  ret->poll_pdu           = poll_pdu;
  ret->poll_byte          = poll_byte;
  ret->max_retx_threshold = max_retx_threshold;

  return (rlc_entity_t *)ret;
}

rlc_entity_t *new_rlc_entity_um(
    int rx_maxsize,
    int tx_maxsize,
    void (*deliver_sdu)(void *deliver_sdu_data, struct rlc_entity_t *entity,
                      char *buf, int size),
    void *deliver_sdu_data,
    int t_reordering,
    int sn_field_length)
{
  rlc_entity_um_t *ret;

  ret = calloc(1, sizeof(rlc_entity_um_t));
  if (ret == NULL) {
    LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  ret->common.recv_pdu      = rlc_entity_um_recv_pdu;
  ret->common.buffer_status = rlc_entity_um_buffer_status;
  ret->common.generate_pdu  = rlc_entity_um_generate_pdu;

  ret->common.recv_sdu         = rlc_entity_um_recv_sdu;

  ret->common.set_time = rlc_entity_um_set_time;

  ret->common.discard_sdu = rlc_entity_um_discard_sdu;

  ret->common.reestablishment = rlc_entity_um_reestablishment;

  ret->common.delete = rlc_entity_um_delete;

  ret->common.deliver_sdu      = deliver_sdu;
  ret->common.deliver_sdu_data = deliver_sdu_data;

  ret->sn_field_length    = sn_field_length;
  ret->rx_maxsize         = rx_maxsize;
  ret->tx_maxsize         = tx_maxsize;
  ret->t_reordering       = t_reordering;

  if (sn_field_length == 5)
    ret->sn_modulus = 32;
  else if (sn_field_length == 10)
    ret->sn_modulus = 1024;
  else {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  ret->window_size = ret->sn_modulus / 2;

  return (rlc_entity_t *)ret;
}
