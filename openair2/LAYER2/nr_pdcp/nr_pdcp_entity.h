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

#ifndef _NR_PDCP_ENTITY_H_
#define _NR_PDCP_ENTITY_H_

#include <stdint.h>

#include "nr_pdcp_sdu.h"

typedef enum {
  NR_PDCP_DRB_AM,
  NR_PDCP_DRB_UM,
  NR_PDCP_SRB
} nr_pdcp_entity_type_t;

typedef struct {
  //nr_pdcp_entity_type_t mode;
  /* PDU stats */
  /* TX */
  uint32_t txpdu_pkts;     /* aggregated number of tx packets */
  uint32_t txpdu_bytes;    /* aggregated bytes of tx packets */
  uint32_t txpdu_sn;       /* current sequence number of last tx packet (or TX_NEXT) */
  /* RX */
  uint32_t rxpdu_pkts;     /* aggregated number of rx packets */
  uint32_t rxpdu_bytes;    /* aggregated bytes of rx packets */
  uint32_t rxpdu_sn;       /* current sequence number of last rx packet (or  RX_NEXT) */
  /* TODO? */
  uint32_t rxpdu_oo_pkts;       /* aggregated number of out-of-order rx pkts  (or RX_REORD) */
  /* TODO? */
  uint32_t rxpdu_oo_bytes; /* aggregated amount of out-of-order rx bytes */
  uint32_t rxpdu_dd_pkts;  /* aggregated number of duplicated discarded packets (or dropped packets because of other reasons such as integrity failure) (or RX_DELIV) */
  uint32_t rxpdu_dd_bytes; /* aggregated amount of discarded packets' bytes */
  /* TODO? */
  uint32_t rxpdu_ro_count; /* this state variable indicates the COUNT value following the COUNT value associated with the PDCP Data PDU which triggered t-Reordering. (RX_REORD) */

  /* SDU stats */
  /* TX */
  uint32_t txsdu_pkts;     /* number of SDUs delivered */
  uint32_t txsdu_bytes;    /* number of bytes of SDUs delivered */

  /* RX */
  uint32_t rxsdu_pkts;     /* number of SDUs received */
  uint32_t rxsdu_bytes;    /* number of bytes of SDUs received */

  uint8_t  mode;               /* 0: PDCP AM, 1: PDCP UM, 2: PDCP TM */
} nr_pdcp_statistics_t;


typedef struct nr_pdcp_entity_t {
  nr_pdcp_entity_type_t type;

  /* functions provided by the PDCP module */
  void (*recv_pdu)(struct nr_pdcp_entity_t *entity, char *buffer, int size);
  void (*recv_sdu)(struct nr_pdcp_entity_t *entity, char *buffer, int size,
                   int sdu_id);
  void (*delete_entity)(struct nr_pdcp_entity_t *entity);
  void (*get_stats)(struct nr_pdcp_entity_t *entity, nr_pdcp_statistics_t *out);

  /* set_security: pass -1 to integrity_algorithm / ciphering_algorithm
   *               to keep the current algorithm
   *               pass NULL to integrity_key / ciphering_key
   *               to keep the current key
   */
  void (*set_security)(struct nr_pdcp_entity_t *entity,
                       int integrity_algorithm,
                       char *integrity_key,
                       int ciphering_algorithm,
                       char *ciphering_key);

  void (*set_time)(struct nr_pdcp_entity_t *entity, uint64_t now);

  /* callbacks provided to the PDCP module */
  void (*deliver_sdu)(void *deliver_sdu_data, struct nr_pdcp_entity_t *entity,
                      char *buf, int size);
  void *deliver_sdu_data;
  void (*deliver_pdu)(void *deliver_pdu_data, struct nr_pdcp_entity_t *entity,
                      char *buf, int size, int sdu_id);
  void *deliver_pdu_data;

  /* configuration variables */
  int rb_id;
  int pdusession_id;
  int has_sdap;
  int has_sdapULheader;
  int has_sdapDLheader;
  int sn_size;                  /* SN size, in bits */
  int t_reordering;             /* unit: ms, -1 for infinity */
  int discard_timer;            /* unit: ms, -1 for infinity */

  int sn_max;                   /* (2^SN_size) - 1 */
  int window_size;              /* 2^(SN_size - 1) */

  /* state variables */
  uint32_t tx_next;
  uint32_t rx_next;
  uint32_t rx_deliv;
  uint32_t rx_reord;

  /* set to the latest know time by the user of the module. Unit: ms */
  uint64_t t_current;

  /* timers (stores the ms of activation, 0 means not active) */
  int t_reordering_start;

  /* security */
  int has_ciphering;
  int has_integrity;
  int ciphering_algorithm;
  int integrity_algorithm;
  unsigned char ciphering_key[16];
  unsigned char integrity_key[16];
  void *security_context;
  void (*cipher)(void *security_context,
                 unsigned char *buffer, int length,
                 int bearer, int count, int direction);
  void (*free_security)(void *security_context);
  void *integrity_context;
  void (*integrity)(void *integrity_context, unsigned char *out,
                 unsigned char *buffer, int length,
                 int bearer, int count, int direction);
  void (*free_integrity)(void *integrity_context);
  /* security/integrity algorithms need to know uplink/downlink information
   * which is reverse for gnb and ue, so we need to know if this
   * pdcp entity is for a gnb or an ue
   */
  int is_gnb;

  /* rx management */
  nr_pdcp_sdu_t *rx_list;
  int           rx_size;
  int           rx_maxsize;
  nr_pdcp_statistics_t stats;
} nr_pdcp_entity_t;

nr_pdcp_entity_t *new_nr_pdcp_entity(
    nr_pdcp_entity_type_t type,
    int is_gnb, int rb_id, int pdusession_id,int has_sdap,
    int has_sdapULheader,int has_sdapDLheader,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size),
    void *deliver_sdu_data,
    void (*deliver_pdu)(void *deliver_pdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size, int sdu_id),
    void *deliver_pdu_data,
    int sn_size,
    int t_reordering,
    int discard_timer,
    int ciphering_algorithm,
    int integrity_algorithm,
    unsigned char *ciphering_key,
    unsigned char *integrity_key);

void nr_DRB_preconfiguration(uint16_t crnti);

#endif /* _NR_PDCP_ENTITY_H_ */
