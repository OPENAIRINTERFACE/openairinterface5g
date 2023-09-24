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

#include "common/utils/time_stat.h"

#define NR_SDU_MAX 16000   /* max NR PDCP SDU size is 9000, let's take more */

typedef enum {
  NR_RLC_AM,
  NR_RLC_UM,
  NR_RLC_TM,
} nr_rlc_mode_t;

typedef struct {
  nr_rlc_mode_t mode;          /* AM, UM, or TM */

  /* PDU stats */
  /* TX */
  uint32_t txpdu_pkts;         /* aggregated number of transmitted RLC PDUs */
  uint32_t txpdu_bytes;        /* aggregated amount of transmitted bytes in RLC PDUs */
  uint32_t txpdu_wt_ms;        /* TODO: aggregated head-of-line tx packet waiting time to be transmitted (i.e. send to the MAC layer) */
  uint32_t txpdu_dd_pkts;      /* aggregated number of dropped or discarded tx packets by RLC */
  uint32_t txpdu_dd_bytes;     /* aggregated amount of bytes dropped or discarded tx packets by RLC */
  uint32_t txpdu_retx_pkts;    /* aggregated number of tx pdus/pkts to be re-transmitted (only applicable to RLC AM) */
  uint32_t txpdu_retx_bytes;   /* aggregated amount of bytes to be re-transmitted (only applicable to RLC AM) */
  uint32_t txpdu_segmented;    /* aggregated number of segmentations */
  uint32_t txpdu_status_pkts;  /* aggregated number of tx status pdus/pkts (only applicable to RLC AM) */
  uint32_t txpdu_status_bytes; /* aggregated amount of tx status bytes  (only applicable to RLC AM) */
  uint32_t txbuf_occ_bytes;    /* (IMPLEMENTED) transmitting bytes currently in buffer */
  uint32_t txbuf_occ_pkts;     /* TODO: current tx buffer occupancy in terms of number of packets (average: NOT IMPLEMENTED) */
  /* txbuf_wd_ms: the time window for which the txbuf  occupancy value is obtained - NOT IMPLEMENTED */

  /* RX */
  uint32_t rxpdu_pkts;         /* aggregated number of received RLC PDUs */
  uint32_t rxpdu_bytes;        /* amount of bytes received by the RLC */
  uint32_t rxpdu_dup_pkts;     /* aggregated number of duplicate packets */
  uint32_t rxpdu_dup_bytes;    /* aggregated amount of duplicated bytes */
  uint32_t rxpdu_dd_pkts;      /* aggregated number of rx packets dropped or discarded by RLC */
  uint32_t rxpdu_dd_bytes;     /* aggregated amount of rx bytes dropped or discarded by RLC */
  uint32_t rxpdu_ow_pkts;      /* aggregated number of out of window received RLC pdu */
  uint32_t rxpdu_ow_bytes;     /* aggregated number of out of window bytes received RLC pdu */
  uint32_t rxpdu_status_pkts;  /* aggregated number of rx status pdus/pkts (only applicable to RLC AM) */
  uint32_t rxpdu_status_bytes; /* aggregated amount of rx status bytes  (only applicable to RLC AM) */
  /* rxpdu_rotout_ms: flag indicating rx reordering  timeout in ms - NOT IMPLEMENTED */
  /* rxpdu_potout_ms: flag indicating the poll retransmit time out in ms - NOT IMPLEMENTED */
  /* rxpdu_sptout_ms: flag indicating status prohibit timeout in ms - NOT IMPLEMENTED */
  uint32_t rxbuf_occ_bytes;    /* (IMPLEMENTED) received bytes currently in buffer */
  uint32_t rxbuf_occ_pkts;     /* TODO: current rx buffer occupancy in terms of number of packets (average: NOT IMPLEMENTED) */

  /* SDU stats */
  /* TX */
  uint32_t txsdu_pkts;         /* number of SDUs delivered */
  uint64_t txsdu_bytes;        /* (UPDATED) number of SDUs bytes successfully transmitted so far (counter) */
  /* Average time for an SDU to be passed to MAC:
   * Actually measures the time it takes for THE FULL SDU to be passed to MAC FOR THE FIRST TIME.
   * Since the MAC schedules in advance, it does not measure the time of
   * transmission over the air, just the time to reach the MAC layer.
   * => to control it, 'txsdu_wt_us' that measures the current HOL-delay is added.
   */
  double txsdu_avg_time_to_tx; /* (100ms-windowed) per-packet sojourn (SDU to PDU) in microseconds */
  uint32_t txsdu_wt_us;        /* HOL delay of the current radio bearer, in microseconds */

  /* RX */
  uint32_t rxsdu_pkts;         /* number of SDUs received */
  uint64_t rxsdu_bytes;        /* (UPDATED) number of SDUs bytes arrived so far (counter) */
  uint32_t rxsdu_dd_pkts;      /* number of dropped or discarded SDUs */
  uint32_t rxsdu_dd_bytes;     /* number of bytes of SDUs dropped or discarded */

} nr_rlc_statistics_t;

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

  void (*delete_entity)(struct nr_rlc_entity_t *entity);

  int (*available_tx_space)(struct nr_rlc_entity_t *entity);

  void (*get_stats)(struct nr_rlc_entity_t *entity, nr_rlc_statistics_t *out);

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

  nr_rlc_statistics_t stats;
  time_average_t *txsdu_avg_time_to_tx;
  int             avg_time_is_on;
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

void nr_rlc_entity_um_reconfigure(nr_rlc_entity_t *_entity, int t_reassembly, int *sn_field_length);

void nr_rlc_entity_am_reconfigure(nr_rlc_entity_t *_entity,
                                  int t_poll_retransmit,
                                  int t_reassembly,
                                  int t_status_prohibit,
                                  int poll_pdu,
                                  int poll_byte,
                                  int max_retx_threshold,
                                  int *sn_field_length);

#endif /* _NR_RLC_ENTITY_H_ */
