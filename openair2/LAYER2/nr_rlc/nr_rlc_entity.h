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

#ifndef _NR_RLC_ENTITY_H_
#define _NR_RLC_ENTITY_H_

#include <stdint.h>

#define NR_SDU_MAX 16000   /* max NR PDCP SDU size is 9000, let's take more */

typedef struct {
  int status_size;
  int tx_size;
  int retx_size;
} nr_rlc_entity_buffer_status_t;

typedef struct nr_rlc_entity_t {
  /* functions provided by the RLC module */
  void (*recv_pdu)(struct nr_rlc_entity_t *entity, char *buffer, int size);
  nr_rlc_entity_buffer_status_t (*buffer_status)(
      struct nr_rlc_entity_t *entity, int maxsize);
  int (*generate_pdu)(struct nr_rlc_entity_t *entity, char *buffer, int size);

  void (*recv_sdu)(struct nr_rlc_entity_t *entity, char *buffer, int size,
                   int sdu_id);

  void (*set_time)(struct nr_rlc_entity_t *entity, uint64_t now);

  void (*discard_sdu)(struct nr_rlc_entity_t *entity, int sdu_id);

  void (*reestablishment)(struct nr_rlc_entity_t *entity);

  void (*delete)(struct nr_rlc_entity_t *entity);

  /* callbacks provided to the RLC module */
  void (*deliver_sdu)(void *deliver_sdu_data, struct nr_rlc_entity_t *entity,
                      char *buf, int size);
  void *deliver_sdu_data;

  void (*sdu_successful_delivery)(void *sdu_successful_delivery_data,
                                  struct nr_rlc_entity_t *entity,
                                  int sdu_id);
  void *sdu_successful_delivery_data;

  void (*max_retx_reached)(void *max_retx_reached_data,
                           struct nr_rlc_entity_t *entity);
  void *max_retx_reached_data;
  /* buffer status computation */
  nr_rlc_entity_buffer_status_t bstatus;
} nr_rlc_entity_t;

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
    int sn_field_length);

nr_rlc_entity_t *new_nr_rlc_entity_um(
    int rx_maxsize,
    int tx_maxsize,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_rlc_entity_t *entity,
                      char *buf, int size),
    void *deliver_sdu_data,
    int t_reassembly,
    int sn_field_length);

nr_rlc_entity_t *new_nr_rlc_entity_tm(
    int tx_maxsize,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_rlc_entity_t *entity,
                      char *buf, int size),
    void *deliver_sdu_data);

#endif /* _NR_RLC_ENTITY_H_ */
