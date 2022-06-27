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

#include "nr_rlc_entity.h"

#include <stdlib.h>

#include "nr_rlc_entity_am.h"
#include "nr_rlc_entity_um.h"
#include "nr_rlc_entity_tm.h"

#include "LOG/log.h"

#include "common/utils/time_stat.h"

static void nr_rlc_entity_get_stats(
    nr_rlc_entity_t *entity,
    nr_rlc_statistics_t *out)
{
// printf("Stats from the RLC entity asked\n");
  *out = entity->stats;
  out->txsdu_avg_time_to_tx = time_average_get_average(entity->txsdu_avg_time_to_tx,
                                  time_average_now());
}

nr_rlc_entity_t *new_nr_rlc_entity_am(
    int rx_maxsize,
    int tx_maxsize,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_rlc_entity_t *entity,
                      char *buf, int size),
    void *deliver_sdu_data,
    void (*sdu_successful_delivery)(void *sdu_successful_delivery_data,
                                    struct nr_rlc_entity_t *entity,
                                    int sdu_id),
    void *sdu_successful_delivery_data,
    void (*max_retx_reached)(void *max_retx_reached_data,
                             struct nr_rlc_entity_t *entity),
    void *max_retx_reached_data,
    int t_poll_retransmit,
    int t_reassembly,
    int t_status_prohibit,
    int poll_pdu,
    int poll_byte,
    int max_retx_threshold,
    int sn_field_length)
{
  nr_rlc_entity_am_t *ret;

  ret = calloc(1, sizeof(nr_rlc_entity_am_t));
  if (ret == NULL) {
    LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  ret->tx_maxsize = tx_maxsize;
  ret->rx_maxsize = rx_maxsize;

  ret->t_poll_retransmit  = t_poll_retransmit;
  ret->t_reassembly       = t_reassembly;
  ret->t_status_prohibit  = t_status_prohibit;
  ret->poll_pdu           = poll_pdu;
  ret->poll_byte          = poll_byte;
  ret->max_retx_threshold = max_retx_threshold;
  ret->sn_field_length    = sn_field_length;

  if (!(sn_field_length == 12 || sn_field_length == 18)) {
    LOG_E(RLC, "%s:%d:%s: wrong SN field_lenght (%d), must be 12 or 18\n",
          __FILE__, __LINE__, __FUNCTION__, sn_field_length);
    exit(1);
  }
  ret->sn_modulus = 1 << ret->sn_field_length;
  ret->window_size = ret->sn_modulus / 2;

  ret->common.recv_pdu           = nr_rlc_entity_am_recv_pdu;
  ret->common.buffer_status      = nr_rlc_entity_am_buffer_status;
  ret->common.generate_pdu       = nr_rlc_entity_am_generate_pdu;
  ret->common.recv_sdu           = nr_rlc_entity_am_recv_sdu;
  ret->common.set_time           = nr_rlc_entity_am_set_time;
  ret->common.discard_sdu        = nr_rlc_entity_am_discard_sdu;
  ret->common.reestablishment    = nr_rlc_entity_am_reestablishment;
  ret->common.delete             = nr_rlc_entity_am_delete;
  ret->common.available_tx_space = nr_rlc_entity_am_available_tx_space;
  ret->common.get_stats       = nr_rlc_entity_get_stats;

  ret->common.deliver_sdu                  = deliver_sdu;
  ret->common.deliver_sdu_data             = deliver_sdu_data;
  ret->common.sdu_successful_delivery      = sdu_successful_delivery;
  ret->common.sdu_successful_delivery_data = sdu_successful_delivery_data;
  ret->common.max_retx_reached             = max_retx_reached;
  ret->common.max_retx_reached_data        = max_retx_reached_data;

  ret->common.stats.mode = 0;  /* 0 for AM */

  /* let's take average over the last 100 milliseconds
   * initial_size of 1024 is arbitrary
   */
  ret->common.txsdu_avg_time_to_tx = time_average_new(100 * 1000, 1024);

  return (nr_rlc_entity_t *)ret;
}

nr_rlc_entity_t *new_nr_rlc_entity_um(
    int rx_maxsize,
    int tx_maxsize,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_rlc_entity_t *entity,
                      char *buf, int size),
    void *deliver_sdu_data,
    int t_reassembly,
    int sn_field_length)
{
  nr_rlc_entity_um_t *ret;

  ret = calloc(1, sizeof(nr_rlc_entity_um_t));
  if (ret == NULL) {
    LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  ret->tx_maxsize = tx_maxsize;
  ret->rx_maxsize = rx_maxsize;

  ret->t_reassembly    = t_reassembly;
  ret->sn_field_length = sn_field_length;

  if (!(sn_field_length == 6 || sn_field_length == 12)) {
    LOG_E(RLC, "%s:%d:%s: wrong SN field_lenght (%d), must be 6 or 12\n",
          __FILE__, __LINE__, __FUNCTION__, sn_field_length);
    exit(1);
  }
  ret->sn_modulus = 1 << ret->sn_field_length;
  ret->window_size = ret->sn_modulus / 2;

  ret->common.recv_pdu           = nr_rlc_entity_um_recv_pdu;
  ret->common.buffer_status      = nr_rlc_entity_um_buffer_status;
  ret->common.generate_pdu       = nr_rlc_entity_um_generate_pdu;
  ret->common.recv_sdu           = nr_rlc_entity_um_recv_sdu;
  ret->common.set_time           = nr_rlc_entity_um_set_time;
  ret->common.discard_sdu        = nr_rlc_entity_um_discard_sdu;
  ret->common.reestablishment    = nr_rlc_entity_um_reestablishment;
  ret->common.delete             = nr_rlc_entity_um_delete;
  ret->common.available_tx_space = nr_rlc_entity_um_available_tx_space;
  ret->common.get_stats       = nr_rlc_entity_get_stats;

  ret->common.deliver_sdu                  = deliver_sdu;
  ret->common.deliver_sdu_data             = deliver_sdu_data;

  ret->common.stats.mode = 1;  /* 1 for UM */

  /* let's take average over the last 100 milliseconds
   * initial_size of 1024 is arbitrary
   */
  ret->common.txsdu_avg_time_to_tx = time_average_new(100 * 1000, 1024);

  return (nr_rlc_entity_t *)ret;
}

nr_rlc_entity_t *new_nr_rlc_entity_tm(
    int tx_maxsize,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_rlc_entity_t *entity,
                      char *buf, int size),
    void *deliver_sdu_data)
{
  nr_rlc_entity_tm_t *ret;

  ret = calloc(1, sizeof(nr_rlc_entity_tm_t));
  if (ret == NULL) {
    LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  ret->tx_maxsize = tx_maxsize;

  ret->common.recv_pdu           = nr_rlc_entity_tm_recv_pdu;
  ret->common.buffer_status      = nr_rlc_entity_tm_buffer_status;
  ret->common.generate_pdu       = nr_rlc_entity_tm_generate_pdu;
  ret->common.recv_sdu           = nr_rlc_entity_tm_recv_sdu;
  ret->common.set_time           = nr_rlc_entity_tm_set_time;
  ret->common.discard_sdu        = nr_rlc_entity_tm_discard_sdu;
  ret->common.reestablishment    = nr_rlc_entity_tm_reestablishment;
  ret->common.delete             = nr_rlc_entity_tm_delete;
  ret->common.available_tx_space = nr_rlc_entity_tm_available_tx_space;
  ret->common.get_stats       = nr_rlc_entity_get_stats;

  ret->common.deliver_sdu                  = deliver_sdu;
  ret->common.deliver_sdu_data             = deliver_sdu_data;

  ret->common.stats.mode = 2;  /* 2 for TM */

  /* let's take average over the last 100 milliseconds
   * initial_size of 1024 is arbitrary
   */
  ret->common.txsdu_avg_time_to_tx = time_average_new(100 * 1000, 1024);

  return (nr_rlc_entity_t *)ret;
}
