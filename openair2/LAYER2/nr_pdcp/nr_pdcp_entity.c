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

#include "nr_pdcp_entity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nr_pdcp_security_nea2.h"
#include "nr_pdcp_integrity_nia2.h"
#include "nr_pdcp_integrity_nia1.h"
#include "nr_pdcp_sdu.h"

#include "LOG/log.h"

static void nr_pdcp_entity_recv_pdu(nr_pdcp_entity_t *entity,
                                    char *_buffer, int size)
{
  unsigned char    *buffer = (unsigned char *)_buffer;
  nr_pdcp_sdu_t    *sdu;
  int              rcvd_sn;
  uint32_t         rcvd_hfn;
  uint32_t         rcvd_count;
  int              header_size;
  int              integrity_size;
  int              rx_deliv_sn;
  uint32_t         rx_deliv_hfn;

  if (size < 1) {
    LOG_E(PDCP, "bad PDU received (size = %d)\n", size);
    return;
  }

  if (entity->type != NR_PDCP_SRB && !(buffer[0] & 0x80)) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    /* TODO: This is something of a hack. The most significant bit
       in buffer[0] should be 1 if the packet is a data packet. We are
       processing malformed data packets if the most significant bit
       is 0. Rather than exit(1), this hack allows us to continue for now.
       We need to investigate why this hack is neccessary. */
    buffer[0] |= 128;
  }
  entity->stats.rxpdu_pkts++;
  entity->stats.rxpdu_bytes += size;


  if (entity->sn_size == 12) {
    rcvd_sn = ((buffer[0] & 0xf) <<  8) |
                buffer[1];
    header_size = 2;
  } else {
    rcvd_sn = ((buffer[0] & 0x3) << 16) |
               (buffer[1]        <<  8) |
                buffer[2];
    header_size = 3;
  }
  entity->stats.rxpdu_sn = rcvd_sn;

  /* SRBs always have MAC-I, even if integrity is not active */
  if (entity->has_integrity || entity->type == NR_PDCP_SRB) {
    integrity_size = 4;
  } else {
    integrity_size = 0;
  }

  if (size < header_size + integrity_size + 1) {
    LOG_E(PDCP, "bad PDU received (size = %d)\n", size);

    entity->stats.rxpdu_dd_pkts++;
    entity->stats.rxpdu_dd_bytes += size;

    return;
  }

  rx_deliv_sn  = entity->rx_deliv & entity->sn_max;
  rx_deliv_hfn = entity->rx_deliv >> entity->sn_size;

  if (rcvd_sn < rx_deliv_sn - entity->window_size) {
    rcvd_hfn = rx_deliv_hfn + 1;
  } else if (rcvd_sn >= rx_deliv_sn + entity->window_size) {
    rcvd_hfn = rx_deliv_hfn - 1;
  } else {
    rcvd_hfn = rx_deliv_hfn;
  }

  rcvd_count = (rcvd_hfn << entity->sn_size) | rcvd_sn;

  if (entity->has_ciphering)
    entity->cipher(entity->security_context,
                   buffer+header_size, size-header_size,
                   entity->rb_id, rcvd_count, entity->is_gnb ? 0 : 1);

  if (entity->has_integrity) {
    unsigned char integrity[4] = {0};
    entity->integrity(entity->integrity_context, integrity,
                      buffer, size - integrity_size,
                      entity->rb_id, rcvd_count, entity->is_gnb ? 0 : 1);
    if (memcmp(integrity, buffer + size - integrity_size, 4) != 0) {
      LOG_E(PDCP, "discard NR PDU, integrity failed\n");
      entity->stats.rxpdu_dd_pkts++;
      entity->stats.rxpdu_dd_bytes += size;

      return;
    }
  }

  if (rcvd_count < entity->rx_deliv
      || nr_pdcp_sdu_in_list(entity->rx_list, rcvd_count)) {
    LOG_W(PDCP, "discard NR PDU rcvd_count=%d, entity->rx_deliv %d,sdu_in_list %d\n", rcvd_count,entity->rx_deliv,nr_pdcp_sdu_in_list(entity->rx_list,rcvd_count));
    entity->stats.rxpdu_dd_pkts++;
    entity->stats.rxpdu_dd_bytes += size;

    return;
  }

  sdu = nr_pdcp_new_sdu(rcvd_count,
                        (char *)buffer + header_size,
                        size - header_size - integrity_size);
  entity->rx_list = nr_pdcp_sdu_list_add(entity->rx_list, sdu);
  entity->rx_size += size-header_size;

  if (rcvd_count >= entity->rx_next) {
    entity->rx_next = rcvd_count + 1;
  }

  /* TODO(?): out of order delivery */

  if (rcvd_count == entity->rx_deliv) {
    /* deliver all SDUs starting from rx_deliv up to discontinuity or end of list */
    uint32_t count = entity->rx_deliv;
    while (entity->rx_list != NULL && count == entity->rx_list->count) {
      nr_pdcp_sdu_t *cur = entity->rx_list;
      entity->deliver_sdu(entity->deliver_sdu_data, entity,
                          cur->buffer, cur->size);
      entity->rx_list = cur->next;
      entity->rx_size -= cur->size;
      entity->stats.txsdu_pkts++;
      entity->stats.txsdu_bytes += cur->size;

      nr_pdcp_free_sdu(cur);
      count++;
    }
    entity->rx_deliv = count;
  }

  if (entity->t_reordering_start != 0 && entity->rx_deliv >= entity->rx_reord) {
    /* stop and reset t-Reordering */
    entity->t_reordering_start = 0;
  }

  if (entity->t_reordering_start == 0 && entity->rx_deliv < entity->rx_next) {
    entity->rx_reord = entity->rx_next;
    entity->t_reordering_start = entity->t_current;
  }
}

static int nr_pdcp_entity_process_sdu(nr_pdcp_entity_t *entity,
                                      char *buffer,
                                      int size,
                                      int sdu_id,
                                      char *pdu_buffer,
                                      int pdu_max_size)
{
  uint32_t count;
  int      sn;
  int      header_size;
  int      integrity_size;
  char    *buf = pdu_buffer;
  DevAssert(size + 3 + 4 <= pdu_max_size);
  int      dc_bit;
  entity->stats.rxsdu_pkts++;
  entity->stats.rxsdu_bytes += size;


  count = entity->tx_next;
  sn = entity->tx_next & entity->sn_max;

  /* D/C bit is only to be set for DRBs */
  if (entity->type == NR_PDCP_DRB_AM || entity->type == NR_PDCP_DRB_UM) {
    dc_bit = 0x80;
  } else {
    dc_bit = 0;
  }

  if (entity->sn_size == 12) {
    buf[0] = dc_bit | ((sn >> 8) & 0xf);
    buf[1] = sn & 0xff;
    header_size = 2;
  } else {
    buf[0] = dc_bit | ((sn >> 16) & 0x3);
    buf[1] = (sn >> 8) & 0xff;
    buf[2] = sn & 0xff;
    header_size = 3;
  }

  /* SRBs always have MAC-I, even if integrity is not active */
  if (entity->has_integrity || entity->type == NR_PDCP_SRB) {
    integrity_size = 4;
  } else {
    integrity_size = 0;
  }

  memcpy(buf + header_size, buffer, size);

  if (entity->has_integrity) {
    uint8_t integrity[4] = {0};
    entity->integrity(entity->integrity_context,
                      integrity,
                      (unsigned char *)buf, header_size + size,
                      entity->rb_id, count, entity->is_gnb ? 1 : 0);

    memcpy((unsigned char *)buf + header_size + size, integrity, 4);
  } else if (integrity_size == 4) {
    // set MAC-I to 0 for SRBs with integrity not active
    memset(buf + header_size + size, 0, 4);
  }

  if (entity->has_ciphering && (entity->is_gnb || entity->security_mode_completed)) {
    entity->cipher(entity->security_context,
                   (unsigned char *)buf + header_size, size + integrity_size,
                   entity->rb_id, count, entity->is_gnb ? 1 : 0);
  } else {
    entity->security_mode_completed = true;
  }

  entity->tx_next++;

  entity->stats.txpdu_pkts++;
  entity->stats.txpdu_bytes += header_size + size + integrity_size;
  entity->stats.txpdu_sn = sn;

  return header_size + size + integrity_size;
}

/* may be called several times, take care to clean previous settings */
static void nr_pdcp_entity_set_security(nr_pdcp_entity_t *entity,
                                        int integrity_algorithm,
                                        char *integrity_key,
                                        int ciphering_algorithm,
                                        char *ciphering_key)
{
  if (integrity_algorithm != -1)
    entity->integrity_algorithm = integrity_algorithm;
  if (ciphering_algorithm != -1)
    entity->ciphering_algorithm = ciphering_algorithm;
  if (integrity_key != NULL)
    memcpy(entity->integrity_key, integrity_key, 16);
  if (ciphering_key != NULL)
    memcpy(entity->ciphering_key, ciphering_key, 16);

  if (integrity_algorithm == 0) {
    entity->has_integrity = 0;
    if (entity->free_integrity != NULL)
      entity->free_integrity(entity->integrity_context);
    entity->free_integrity = NULL;
  }

  if (integrity_algorithm != 0 && integrity_algorithm != -1) {
    entity->has_integrity = 1;
    if (entity->free_integrity != NULL)
      entity->free_integrity(entity->integrity_context);
    if (integrity_algorithm == 2) {
      entity->integrity_context = nr_pdcp_integrity_nia2_init(entity->integrity_key);
      entity->integrity = nr_pdcp_integrity_nia2_integrity;
      entity->free_integrity = nr_pdcp_integrity_nia2_free_integrity;
    } else if (integrity_algorithm == 1) {
      entity->integrity_context = nr_pdcp_integrity_nia1_init(entity->integrity_key);
      entity->integrity = nr_pdcp_integrity_nia1_integrity;
      entity->free_integrity = nr_pdcp_integrity_nia1_free_integrity;
    } else {
      LOG_E(PDCP, "FATAL: only nia1 and nia2 supported for the moment\n");
      exit(1);
    }
  }

  if (ciphering_algorithm == 0) {
    entity->has_ciphering = 0;
    if (entity->free_security != NULL)
      entity->free_security(entity->security_context);
    entity->free_security = NULL;
  }

  if (ciphering_algorithm != 0 && ciphering_algorithm != -1) {
    if (ciphering_algorithm != 2) {
      LOG_E(PDCP, "FATAL: only nea2 supported for the moment\n");
      exit(1);
    }
    entity->has_ciphering = 1;
    if (entity->free_security != NULL)
      entity->free_security(entity->security_context);
    entity->security_context = nr_pdcp_security_nea2_init(entity->ciphering_key);
    entity->cipher = nr_pdcp_security_nea2_cipher;
    entity->free_security = nr_pdcp_security_nea2_free_security;
  }
}

static void check_t_reordering(nr_pdcp_entity_t *entity)
{
  uint32_t count;

  /* if t_reordering is set to "infinity" (seen as -1) then do nothing */
  if (entity->t_reordering == -1)
    return;

  if (entity->t_reordering_start == 0
      || entity->t_current <= entity->t_reordering_start + entity->t_reordering)
    return;

  /* stop timer */
  entity->t_reordering_start = 0;

  /* deliver all SDUs with count < rx_reord */
  while (entity->rx_list != NULL && entity->rx_list->count < entity->rx_reord) {
    nr_pdcp_sdu_t *cur = entity->rx_list;
    entity->deliver_sdu(entity->deliver_sdu_data, entity,
                        cur->buffer, cur->size);
    entity->rx_list = cur->next;
    entity->rx_size -= cur->size;
    entity->stats.txsdu_pkts++;
    entity->stats.txsdu_bytes += cur->size;
    nr_pdcp_free_sdu(cur);
  }

  /* deliver all SDUs starting from rx_reord up to discontinuity or end of list */
  count = entity->rx_reord;
  while (entity->rx_list != NULL && count == entity->rx_list->count) {
    nr_pdcp_sdu_t *cur = entity->rx_list;
    entity->deliver_sdu(entity->deliver_sdu_data, entity,
                        cur->buffer, cur->size);
    entity->rx_list = cur->next;
    entity->rx_size -= cur->size;
    entity->stats.txsdu_pkts++;
    entity->stats.txsdu_bytes += cur->size;
    nr_pdcp_free_sdu(cur);
    count++;
  }

  entity->rx_deliv = count;

  if (entity->rx_deliv < entity->rx_next) {
    entity->rx_reord = entity->rx_next;
    entity->t_reordering_start = entity->t_current;
  }
}

static void nr_pdcp_entity_set_time(struct nr_pdcp_entity_t *entity, uint64_t now)
{
  entity->t_current = now;

  check_t_reordering(entity);
}

static void deliver_all_sdus(nr_pdcp_entity_t *entity)
{
  // deliver the PDCP SDUs stored in the receiving PDCP entity to upper layers
  while (entity->rx_list != NULL) {
    nr_pdcp_sdu_t *cur = entity->rx_list;
    entity->deliver_sdu(entity->deliver_sdu_data, entity,
                        cur->buffer, cur->size);
    entity->rx_list = cur->next;
    entity->rx_size -= cur->size;
    entity->stats.txsdu_pkts++;
    entity->stats.txsdu_bytes += cur->size;
    nr_pdcp_free_sdu(cur);
  }
}

static void nr_pdcp_entity_suspend(nr_pdcp_entity_t *entity)
{
  entity->tx_next = 0;
  if (entity->t_reordering_start != 0) {
    entity->t_reordering_start = 0;
    deliver_all_sdus(entity);
  }
  entity->rx_next = 0;
  entity->rx_deliv = 0;
}

static void free_rx_list(nr_pdcp_entity_t *entity)
{
  nr_pdcp_sdu_t *cur = entity->rx_list;
  while (cur != NULL) {
    nr_pdcp_sdu_t *next = cur->next;
    entity->stats.rxpdu_dd_pkts++;
    entity->stats.rxpdu_dd_bytes += cur->size;
    nr_pdcp_free_sdu(cur);
    cur = next;
  }
  entity->rx_list = NULL;
  entity->rx_size = 0;
}

static void nr_pdcp_entity_reestablish_drb_am(nr_pdcp_entity_t *entity)
{
  /* transmitting entity procedures */
  /* todo: deal with ciphering/integrity algos and keys */

  /* receiving entity procedures */
  /* todo: deal with ciphering/integrity algos and keys */
}

static void nr_pdcp_entity_reestablish_drb_um(nr_pdcp_entity_t *entity)
{
  /* transmitting entity procedures */
  entity->tx_next = 0;
  /* todo: deal with ciphering/integrity algos and keys */

  /* receiving entity procedures */
  /* deliver all SDUs if t_reordering is running */
  if (entity->t_reordering_start != 0)
    deliver_all_sdus(entity);
  /* stop t_reordering */
  entity->t_reordering_start = 0;
  /* set rx_next and rx_deliv to the initial value */
  entity->rx_next = 0;
  entity->rx_deliv = 0;
  /* todo: deal with ciphering/integrity algos and keys */
}

static void nr_pdcp_entity_reestablish_srb(nr_pdcp_entity_t *entity)
{
  /* transmitting entity procedures */
  entity->tx_next = 0;
  /* todo: deal with ciphering/integrity algos and keys */

  /* receiving entity procedures */
  free_rx_list(entity);
  /* stop t_reordering */
  entity->t_reordering_start = 0;
  /* set rx_next and rx_deliv to the initial value */
  entity->rx_next = 0;
  entity->rx_deliv = 0;
  /* todo: deal with ciphering/integrity algos and keys */
}

static void nr_pdcp_entity_release(nr_pdcp_entity_t *entity)
{
  deliver_all_sdus(entity);
}

static void nr_pdcp_entity_delete(nr_pdcp_entity_t *entity)
{
  free_rx_list(entity);
  if (entity->free_security != NULL)
    entity->free_security(entity->security_context);
  if (entity->free_integrity != NULL)
    entity->free_integrity(entity->integrity_context);
  free(entity);
}

static void nr_pdcp_entity_get_stats(nr_pdcp_entity_t *entity,
                                     nr_pdcp_statistics_t *out)
{
  *out = entity->stats;
}


nr_pdcp_entity_t *new_nr_pdcp_entity(
    nr_pdcp_entity_type_t type,
    int is_gnb,
    int rb_id,
    int pdusession_id,
    bool has_sdap_rx,
    bool has_sdap_tx,
    void (*deliver_sdu)(void *deliver_sdu_data, struct nr_pdcp_entity_t *entity,
                        char *buf, int size),
    void *deliver_sdu_data,
    void (*deliver_pdu)(void *deliver_pdu_data, ue_id_t ue_id, int rb_id,
                        char *buf, int size, int sdu_id),
    void *deliver_pdu_data,
    int sn_size,
    int t_reordering,
    int discard_timer,
    int ciphering_algorithm,
    int integrity_algorithm,
    unsigned char *ciphering_key,
    unsigned char *integrity_key)
{
  nr_pdcp_entity_t *ret;

  ret = calloc(1, sizeof(nr_pdcp_entity_t));
  if (ret == NULL) {
    LOG_E(PDCP, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  ret->type = type;

  ret->recv_pdu     = nr_pdcp_entity_recv_pdu;
  ret->process_sdu  = nr_pdcp_entity_process_sdu;
  ret->set_security = nr_pdcp_entity_set_security;
  ret->set_time     = nr_pdcp_entity_set_time;

  ret->delete_entity = nr_pdcp_entity_delete;
  ret->release_entity = nr_pdcp_entity_release;
  ret->suspend_entity = nr_pdcp_entity_suspend;

  switch (type) {
    case NR_PDCP_DRB_AM:
      ret->reestablish_entity = nr_pdcp_entity_reestablish_drb_am;
      break;
    case NR_PDCP_DRB_UM:
      ret->reestablish_entity = nr_pdcp_entity_reestablish_drb_um;
      break;
    case NR_PDCP_SRB:
      ret->reestablish_entity = nr_pdcp_entity_reestablish_srb;
      break;
  }
  
  ret->get_stats = nr_pdcp_entity_get_stats;
  ret->deliver_sdu = deliver_sdu;
  ret->deliver_sdu_data = deliver_sdu_data;

  ret->deliver_pdu = deliver_pdu;
  ret->deliver_pdu_data = deliver_pdu_data;

  ret->rb_id         = rb_id;
  ret->pdusession_id = pdusession_id;
  ret->has_sdap_rx   = has_sdap_rx;
  ret->has_sdap_tx   = has_sdap_tx;
  ret->sn_size       = sn_size;
  ret->t_reordering  = t_reordering;
  ret->discard_timer = discard_timer;

  ret->sn_max        = (1 << sn_size) - 1;
  ret->window_size   = 1 << (sn_size - 1);

  ret->is_gnb = is_gnb;

  nr_pdcp_entity_set_security(ret,
                              integrity_algorithm, (char *)integrity_key,
                              ciphering_algorithm, (char *)ciphering_key);

  return ret;
}
