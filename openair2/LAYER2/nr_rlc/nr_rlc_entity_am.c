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

#include "nr_rlc_entity_am.h"

#include <stdlib.h>
#include <string.h>

#include "nr_rlc_pdu.h"

#include "LOG/log.h"

/*************************************************************************/
/* PDU RX functions                                                      */
/*************************************************************************/

static int modulus_rx(nr_rlc_entity_am_t *entity, int a)
{
  /* as per 38.322 7.1, modulus base is rx_next */
  int r = a - entity->rx_next;
  if (r < 0) r += entity->sn_modulus;
  return r;
}

static int modulus_tx(nr_rlc_entity_am_t *entity, int a)
{
  int r = a - entity->tx_next_ack;
  if (r < 0) r += entity->sn_modulus;
  return r;
}

static int sn_in_recv_window(void *_entity, int sn)
{
  nr_rlc_entity_am_t *entity = _entity;
  int mod_sn = modulus_rx(entity, sn);
  /* we simplify rx_next <= sn < rx_next + am_window_size */
  return mod_sn < entity->window_size;
}

static int sn_compare_rx(void *_entity, int a, int b)
{
  nr_rlc_entity_am_t *entity = _entity;
  return modulus_rx(entity, a) - modulus_rx(entity, b);
}

static int sn_compare_tx(void *_entity, int a, int b)
{
  nr_rlc_entity_am_t *entity = _entity;
  return modulus_tx(entity, a) - modulus_tx(entity, b);
}

static int segment_already_received(nr_rlc_entity_am_t *entity,
    int sn, int so, int size)
{
  nr_rlc_pdu_t *l = entity->rx_list;
  int covered;

  while (l != NULL && size > 0) {
    if (l->sn == sn) {
      if (l->so <= so && so < l->so + l->size) {
        covered = l->size - (so - l->so);
        size -= covered;
        so += covered;
      } else if (l->so <= so+size-1 && so+size-1 < l->so + l->size) {
        covered = size - (l->so - so);
        size -= covered;
      }
    }
    l = l->next;
  }

  return size <= 0;
}

static void consider_retransmission(nr_rlc_entity_am_t *entity,
    nr_rlc_sdu_segment_t *cur, int update_retx)
{
  if (update_retx)
    cur->sdu->retx_count++;

  /* let's report max RETX reached for all retx_count >= max_retx_threshold
   * (specs say to report if retx_count == max_retx_threshold).
   * Upper layers should react (radio link failure), so no big deal actually.
   */
  if (update_retx && cur->sdu->retx_count >= entity->max_retx_threshold) {
    entity->common.max_retx_reached(entity->common.max_retx_reached_data,
                                    (nr_rlc_entity_t *)entity);
  }

  /* let's put in retransmit list even if we are over max_retx_threshold.
   * upper layers should deal with this condition, internally it's better
   * for the RLC code to keep going with this segment (we only remove
   * a segment that was ACKed)
   */ 
  LOG_D(RLC, "RLC segment to be added at the ReTx list \n"); 
  nr_rlc_sdu_segment_list_append(&entity->retransmit_list,
                                 &entity->retransmit_end,
                                 cur);
}

/* checks that all the bytes of the SDU sn have been received (but SDU
 * has not been already processed)
 */
static int sdu_full(nr_rlc_entity_am_t *entity, int sn)
{
  nr_rlc_pdu_t *l = entity->rx_list;
  int last_byte;
  int new_last_byte;

  last_byte = -1;
  while (l != NULL) {
    if (l->sn == sn)
      break;
    l = l->next;
  }

  /* check if the data has already been processed */
  if (l != NULL && l->data == NULL)
    return 0;

  while (l != NULL && l->sn == sn) {
    if (l->so > last_byte + 1)
      return 0;
    if (l->is_last)
      return 1;
    new_last_byte = l->so + l->size - 1;
    if (new_last_byte > last_byte)
      last_byte = new_last_byte;
    l = l->next;
  }

  return 0;
}

/* checks that an SDU has already been delivered */
static int sdu_delivered(nr_rlc_entity_am_t *entity, int sn)
{
  nr_rlc_pdu_t *l = entity->rx_list;

  while (l != NULL) {
    if (l->sn == sn)
      break;
    l = l->next;
  }

  return l != NULL && l->data == NULL;
}

/* check if there is some missing bytes before the last received of SDU sn */
/* todo: be sure that when no byte was received or the SDU has already been
 *       processed then the SDU has no missing byte
 */
static int sdu_has_missing_bytes(nr_rlc_entity_am_t *entity, int sn)
{
  nr_rlc_pdu_t *l = entity->rx_list;
  int last_byte;
  int new_last_byte;

  last_byte = -1;
  while (l != NULL) {
    if (l->sn == sn)
      break;
    l = l->next;
  }

  /* check if the data has already been processed */
  if (l != NULL && l->data == NULL)
    return 0;                    /* data already processed: no missing byte */

  while (l != NULL && l->sn == sn) {
    if (l->so > last_byte + 1)
      return 1;
    new_last_byte = l->so + l->size - 1;
    if (new_last_byte > last_byte)
      last_byte = new_last_byte;
    l = l->next;
  }

  return 0;
}

static void reassemble_and_deliver(nr_rlc_entity_am_t *entity, int sn)
{
  nr_rlc_pdu_t *pdu;
  char sdu[NR_SDU_MAX];
  int so = 0;
  int bad_sdu = 0;

  /* go to first segment of sn */
  pdu = entity->rx_list;
  while (pdu->sn != sn)
    pdu = pdu->next;

  /* reassemble - free 'data' of each segment after processing */
  while (pdu != NULL && pdu->sn == sn) {
    int len = pdu->size - (so - pdu->so);
    if (so + len > NR_SDU_MAX && !bad_sdu) {
      LOG_E(RLC, "%s:%d:%s: bad SDU, too big, discarding\n",
            __FILE__, __LINE__, __FUNCTION__);
      bad_sdu = 1;
    }
    if (!bad_sdu && len > 0) {
      memcpy(sdu + so, pdu->data, len);
      so += len;
    }
    free(pdu->data);
    pdu->data = NULL;
    entity->rx_size -= pdu->size;
    pdu = pdu->next;
  }

  if (bad_sdu)
    return;

  /* deliver */
  entity->common.deliver_sdu(entity->common.deliver_sdu_data,
                             (nr_rlc_entity_t *)entity,
                             sdu, so);
}

static void reception_actions(nr_rlc_entity_am_t *entity, nr_rlc_pdu_t *pdu)
{
  int x = pdu->sn;

  if (sn_compare_rx(entity, x, entity->rx_next_highest) >= 0)
    entity->rx_next_highest = (x + 1) % entity->sn_modulus;

  /* todo: room for optimization: we can run through rx_list only once */
  if (sdu_full(entity, x)) {
    reassemble_and_deliver(entity, x);

    if (x == entity->rx_highest_status) {
      int rx_highest_status = entity->rx_highest_status;
      while (sdu_delivered(entity, rx_highest_status))
        rx_highest_status = (rx_highest_status + 1) % entity->sn_modulus;
      entity->rx_highest_status = rx_highest_status;
    }

    if (x == entity->rx_next) {
      /* update rx_next and free all delivered SDUs at the head of the
       * rx_list
       */
      int rx_next = entity->rx_next;
      while (entity->rx_list != NULL && entity->rx_list->data == NULL &&
             entity->rx_list->sn == rx_next) {
        /* free all segments of this SDU */
        do {
          nr_rlc_pdu_t *p = entity->rx_list;
          entity->rx_list = p->next;
          free(p);
        } while (entity->rx_list != NULL &&
                 entity->rx_list->sn == rx_next);
        rx_next = (rx_next + 1) % entity->sn_modulus;
      }
      entity->rx_next = rx_next;
    }
  }

  if (entity->t_reassembly_start) {
    if (entity->rx_next_status_trigger == entity->rx_next ||
        (entity->rx_next_status_trigger == (entity->rx_next + 1)
                                             % entity->sn_modulus &&
         !sdu_has_missing_bytes(entity, entity->rx_next)) ||
        (!sn_in_recv_window(entity, entity->rx_next_status_trigger) &&
         entity->rx_next_status_trigger !=
           (entity->rx_next + entity->window_size) % entity->sn_modulus)) {
      entity->t_reassembly_start = 0;
    }
  }

  if (entity->t_reassembly_start == 0) {
    if (sn_compare_rx(entity, entity->rx_next_highest,
                      (entity->rx_next + 1) % entity->sn_modulus) > 0 ||
        (entity->rx_next_highest == (entity->rx_next + 1)
                                      % entity->sn_modulus &&
         sdu_has_missing_bytes(entity, entity->rx_next))) {
      entity->t_reassembly_start = entity->t_current;
      entity->rx_next_status_trigger = entity->rx_next_highest;
    }
  }
}

static void process_received_ack(nr_rlc_entity_am_t *entity, int ack_sn)
{
  nr_rlc_sdu_segment_t head;
  nr_rlc_sdu_segment_t *cur;
  nr_rlc_sdu_segment_t *prev;
  unsigned char sn_set[32768];  /* used to dec retx_count only once per sdu */

  memset(sn_set, 0, 32768);

#define IS_SN_SET(b) (sn_set[(b)/8] & (1 << ((b) % 8)))
#define SET_SN(b) do { sn_set[(b)/8] |= (1 << ((b) % 8)); } while (0)

  /* put SDUs from wait and retransmit lists with SN < 'ack_sn' to ack_list */

  /* process wait list */
  head.next = entity->wait_list;
  prev = &head;
  cur = entity->wait_list;
  while (cur != NULL) {
    if (sn_compare_tx(entity, cur->sdu->sn, ack_sn) < 0) {
      /* remove from wait list */
      prev->next = cur->next;
      /* put the PDU in the ack list */
      entity->ack_list = nr_rlc_sdu_segment_list_add(sn_compare_tx, entity,
                                                     entity->ack_list, cur);
      entity->wait_end = prev;
      cur = prev->next;
    } else {
      entity->wait_end = cur;
      prev = cur;
      cur = cur->next;
    }
  }
  entity->wait_list = head.next;
  if (entity->wait_list == NULL)
    entity->wait_end = NULL;

  /* process retransmit list */
  head.next = entity->retransmit_list;
  prev = &head;
  cur = entity->retransmit_list;
  while (cur != NULL) {
    if (sn_compare_tx(entity, cur->sdu->sn, ack_sn) < 0) {
      /* dec. retx_count in case we put this segment back in retransmit list
       * in 'process_received_nack'
       * do it only once per SDU
       */
      if (!IS_SN_SET(cur->sdu->sn)) {
        cur->sdu->retx_count--;
        SET_SN(cur->sdu->sn);
      }
      /* remove from retransmit list */
      prev->next = cur->next;
      /* put the PDU in the ack list */
      entity->ack_list = nr_rlc_sdu_segment_list_add(sn_compare_tx, entity,
                                                     entity->ack_list, cur);
      entity->retransmit_end = prev;
      cur = prev->next;
    } else {
      entity->retransmit_end = cur;
      prev = cur;
      cur = cur->next;
    }
  }
  entity->retransmit_list = head.next;
  if (entity->retransmit_list == NULL)
    entity->retransmit_end = NULL;

#undef IS_BIT_SET
#undef SET_BIT
}

static int so_overlap(int s1, int e1, int s2, int e2)
{
  if (s1 < s2) {
    if (e1 == -1 || e1 >= s2)
      return 1;
    return 0;
  }
  if (e2 == -1 || s1 <= e2)
    return 1;
  return 0;
}

static void process_nack_sn(nr_rlc_entity_am_t *entity, int nack_sn,
                            int so_start, int so_end, unsigned char *sn_set)
{
  /* put all SDU segments with SN == 'sn' and with an overlapping so start/end
   * to the retransmit list
   * source lists are ack list and wait list.
   * Not sure if we should consider wait list, isn't the other end supposed
   * to only NACK SNs lower than the ACK SN sent in the status PDU, in which
   * case all potential SDU segments should all be in ack list when calling
   * the current function? in doubt let's accept anything and thus process
   * also wait list.
   */
  nr_rlc_sdu_segment_t head;
  nr_rlc_sdu_segment_t *cur;
  nr_rlc_sdu_segment_t *prev;

#define IS_SN_SET(b) (sn_set[(b)/8] & (1 << ((b) % 8)))
#define SET_SN(b) do { sn_set[(b)/8] |= (1 << ((b) % 8)); } while (0)

  /* check that tx_next_ack <= sn < tx_next */
  if (!(sn_compare_tx(entity, entity->tx_next_ack, nack_sn) <= 0 &&
        sn_compare_tx(entity, nack_sn, entity->tx_next) < 0))
    return;

  /* process wait list */
  head.next = entity->wait_list;
  prev = &head;
  cur = entity->wait_list;
  while (cur != NULL) {
    if (cur->sdu->sn == nack_sn &&
        so_overlap(so_start, so_end, cur->so, cur->so + cur->size - 1)) {
      /* remove from wait list */
      prev->next = cur->next;
      cur->next = NULL;
      /* consider the SDU segment for retransmission */
      consider_retransmission(entity, cur, !IS_SN_SET(cur->sdu->sn));
      SET_SN(cur->sdu->sn);
      entity->wait_end = prev;
      cur = prev->next;
    } else {
      entity->wait_end = cur;
      prev = cur;
      cur = cur->next;
    }
  }
  entity->wait_list = head.next;
  if (entity->wait_list == NULL)
    entity->wait_end = NULL;

  /* process ack list */
  head.next = entity->ack_list;
  prev = &head;
  cur = entity->ack_list;
  while (cur != NULL) {
    if (cur->sdu->sn == nack_sn &&
        so_overlap(so_start, so_end, cur->so, cur->so + cur->size - 1)) {
      /* remove from ack list */
      prev->next = cur->next;
      cur->next = NULL;
      /* consider the SDU segment for retransmission */
      consider_retransmission(entity, cur, !IS_SN_SET(cur->sdu->sn));
      SET_SN(cur->sdu->sn);
      cur = prev->next;
    } else {
      prev = cur;
      cur = cur->next;
    }
  }
  entity->ack_list = head.next;

#undef IS_BIT_SET
#undef SET_BIT
}

static void process_received_nack(nr_rlc_entity_am_t *entity, int nack_sn,
                                  int so_start, int so_end, int range,
                                  unsigned char *sn_set)
{
  int i;

  for (i = 0; i < range; i++)
    process_nack_sn(entity, (nack_sn + i) % entity->sn_modulus,
                    i == 0 ?         so_start : 0,
                    i == range - 1 ? so_end : -1,
                    sn_set);
}

static int sdu_segment_in_ack_list_full(nr_rlc_sdu_segment_t *sdu)
{
  int target_count = sdu->sdu->ref_count;
  int actual_count = 0;
  int sn = sdu->sdu->sn;

  while (sdu != NULL && sdu->sdu->sn == sn) {
    actual_count++;
    sdu = sdu->next;
  }

  return actual_count == target_count;
}

static void finalize_ack_nack_processing(nr_rlc_entity_am_t *entity)
{
  nr_rlc_sdu_segment_t *cur = entity->ack_list;
  int sn;

  /* - send indication of successful delivery for all consecutive acked SDUs
   *   starting from tx_next_ack. Also free them.
   * - update tx_next_ack to the next SN not acked yet
   */
  /* todo: send indication of successful delivery as soon as possible as
   *       the specs say (38.322 5.2.3.1.1). As the code is, if we receive
   *       ack for SN+2 we won't indicate successful delivery before
   *       SN+1 has been indicated.
   */
  while (cur != NULL && cur->sdu->sn == entity->tx_next_ack &&
         sdu_segment_in_ack_list_full(cur)) {
    entity->tx_size -= cur->sdu->size;
    sn = cur->sdu->sn;
    entity->common.sdu_successful_delivery(
        entity->common.sdu_successful_delivery_data,
        (nr_rlc_entity_t *)entity, cur->sdu->upper_layer_id);
    while (cur != NULL && cur->sdu->sn == sn) {
      nr_rlc_sdu_segment_t *s = cur;
      cur = cur->next;
      nr_rlc_free_sdu_segment(s);
    }
    entity->ack_list = cur;
    entity->tx_next_ack = (entity->tx_next_ack + 1) % entity->sn_modulus;
  }
}

void nr_rlc_entity_am_recv_pdu(nr_rlc_entity_t *_entity,
                               char *buffer, int size)
{
#define R(d) do { if (nr_rlc_pdu_decoder_in_error(&d)) goto err; } while (0)
  nr_rlc_entity_am_t *entity = (nr_rlc_entity_am_t *)_entity;
  nr_rlc_pdu_decoder_t decoder;
  nr_rlc_pdu_decoder_t control_decoder;
  nr_rlc_pdu_t *pdu;
  int dc;
  int p = 0;
  int si;
  int sn;
  int so = 0;
  int data_size;
  int is_first;
  int is_last;

  int cpt;
  int e1;
  int e2;
  int e3;
  int ack_sn;
  int nack_sn;
  int so_start;
  int so_end;
  int range;
  int control_e1;
  int control_e2;
  int control_e3;
  unsigned char sn_set[32768];  /* used to dec retx_count only once per sdu */

  nr_rlc_pdu_decoder_init(&decoder, buffer, size);
  dc = nr_rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  if (dc == 0) goto control;

  /* data PDU */
  p  = nr_rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  si = nr_rlc_pdu_decoder_get_bits(&decoder, 2); R(decoder);

  is_first = (si & 0x2) == 0;
  is_last = (si & 0x1) == 0;

  if (entity->sn_field_length == 18) {
    nr_rlc_pdu_decoder_get_bits(&decoder, 2); R(decoder);
  }

  sn = nr_rlc_pdu_decoder_get_bits(&decoder, entity->sn_field_length);
  R(decoder);

  if (!is_first) {
    so = nr_rlc_pdu_decoder_get_bits(&decoder, 16); R(decoder);
    if (so == 0) {
      LOG_E(RLC, "%s:%d:%s: warning: discard PDU, bad so\n",
            __FILE__, __LINE__, __FUNCTION__);
      goto discard;
    }
  }

  data_size = size - decoder.byte;

  /* dicard PDU if no data */
  if (data_size <= 0) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, no data\n",
          __FILE__, __LINE__, __FUNCTION__);
    goto discard;
  }

  /* dicard PDU if rx buffer is full */
  if (entity->rx_size + data_size > entity->rx_maxsize) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, RX buffer full\n",
          __FILE__, __LINE__, __FUNCTION__);
    goto discard;
  }

  if (!sn_in_recv_window(entity, sn)) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, sn out of window (sn %d rx_next %d)\n",
          __FILE__, __LINE__, __FUNCTION__,
           sn, entity->rx_next);
    goto discard;
  }

  /* discard segment if all the bytes of the segment are already there */
  if (segment_already_received(entity, sn, so, data_size)) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, already received\n",
          __FILE__, __LINE__, __FUNCTION__);
    goto discard;
  }

  /* put in pdu reception list */
  entity->rx_size += data_size;
  pdu = nr_rlc_new_pdu(sn, so, is_first, is_last,
                       buffer + size - data_size, data_size);
  entity->rx_list = nr_rlc_pdu_list_add(sn_compare_rx, entity,
                                        entity->rx_list, pdu);

  /* do reception actions (38.322 5.2.3.2.3) */
  reception_actions(entity, pdu);

  if (p) {
    /* 38.322 5.3.4 says status triggering should be delayed
     * until x < rx_highest_status or x >= rx_next + am_window_size.
     * This is not clear (what is x then? we keep the same?). So let's
     * trigger no matter what.
     * todo: delay status triggering properly
     */
    int v = (entity->rx_next + entity->window_size) % entity->sn_modulus;
    entity->status_triggered = 1;
    if (!(sn_compare_rx(entity, sn, entity->rx_highest_status) < 0 ||
          sn_compare_rx(entity, sn, v) >= 0)) {
      LOG_D(RLC, "%s:%d:%s: warning: STATUS trigger should be delayed, according to specs\n",
            __FILE__, __LINE__, __FUNCTION__);
    }
  }

  return;

control:
  cpt = nr_rlc_pdu_decoder_get_bits(&decoder, 3); R(decoder);
  if (cpt != 0) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, CPT not 0 (%d)\n",
          __FILE__, __LINE__, __FUNCTION__, cpt);
    goto discard;
  }
  ack_sn = nr_rlc_pdu_decoder_get_bits(&decoder, entity->sn_field_length); R(decoder);
  e1 = nr_rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  /* r bits */
  if (entity->sn_field_length == 18) {
    nr_rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  } else {
    nr_rlc_pdu_decoder_get_bits(&decoder, 7); R(decoder);
  }

  /* let's try to parse the control PDU once to check consistency */
  control_decoder = decoder;
  control_e1 = e1;
  while (control_e1) {
    nack_sn = nr_rlc_pdu_decoder_get_bits(&control_decoder, entity->sn_field_length); R(control_decoder);
    control_e1 = nr_rlc_pdu_decoder_get_bits(&control_decoder, 1); R(control_decoder);
    control_e2 = nr_rlc_pdu_decoder_get_bits(&control_decoder, 1); R(control_decoder);
    control_e3 = nr_rlc_pdu_decoder_get_bits(&control_decoder, 1); R(control_decoder);
    /* r bits */
    if (entity->sn_field_length == 18) {
      nr_rlc_pdu_decoder_get_bits(&control_decoder, 3); R(control_decoder);
    } else {
      nr_rlc_pdu_decoder_get_bits(&control_decoder, 1); R(control_decoder);
    }
    /* check range and so_start/so_end consistency */
    if (control_e2) {
      so_start = nr_rlc_pdu_decoder_get_bits(&control_decoder, 16); R(control_decoder);
      so_end = nr_rlc_pdu_decoder_get_bits(&control_decoder, 16); R(control_decoder);
    } else {
      so_start = 0;
      so_end = 0xffff;
    }
    if (control_e3) {
      range = nr_rlc_pdu_decoder_get_bits(&control_decoder, 8); R(control_decoder);
    } else {
      range = 1;
    }
    if (range < 1) {
      LOG_E(RLC, "%s:%d:%s: error, bad 'range' in RLC NACK (sn %d)\n",
            __FILE__, __LINE__, __FUNCTION__, nack_sn);
      goto err;
    }
    /* so_start can be > so_end if more than one range; they don't refer
     * to the same PDU then
     */
    if (range == 1 && so_end < so_start) {
      LOG_E(RLC, "%s:%d:%s: error, bad so start/end (sn %d)\n",
            __FILE__, __LINE__, __FUNCTION__, nack_sn);
      goto err;
    }
  }

  /* 38.322 5.3.3.3 says to stop t_poll_retransmit if a ACK or NACK is
   * received for the SN 'poll_sn' - check ACK case (NACK done below)
   */
  if (sn_compare_tx(entity, entity->poll_sn, ack_sn) < 0)
    entity->t_poll_retransmit_start = 0;

  /* at this point, accept the PDU even if the actual values
   * may be incorrect (eg. if so_start > so_end)
   */
  process_received_ack(entity, ack_sn);

  if (e1)
    memset(sn_set, 0, 32768);

  while (e1) {
    nack_sn = nr_rlc_pdu_decoder_get_bits(&decoder, entity->sn_field_length); R(decoder);
    e1 = nr_rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
    e2 = nr_rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
    e3 = nr_rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
    /* r bits */
    if (entity->sn_field_length == 18) {
      nr_rlc_pdu_decoder_get_bits(&decoder, 3); R(decoder);
    } else {
      nr_rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
    }
    if (e2) {
      so_start = nr_rlc_pdu_decoder_get_bits(&decoder, 16); R(decoder);
      so_end = nr_rlc_pdu_decoder_get_bits(&decoder, 16); R(decoder);
    } else {
      so_start = 0;
      so_end = 0xffff;
    }
    if (e3) {
      range = nr_rlc_pdu_decoder_get_bits(&decoder, 8); R(decoder);
    } else {
      range = 1;
    }
    /* special value 0xffff indicates 'all bytes to the end' */
    if (so_end == 0xffff)
      so_end = -1;
    process_received_nack(entity, nack_sn, so_start, so_end, range, sn_set);

    /* 38.322 5.3.3.3 says to stop t_poll_retransmit if a ACK or NACK is
     * received for the SN 'poll_sn' - check NACK case (ACK done above)
     */
    if (sn_compare_tx(entity, nack_sn, entity->poll_sn) <= 0 &&
        sn_compare_tx(entity, entity->poll_sn, (nack_sn + range) % entity->sn_modulus) < 0)
      entity->t_poll_retransmit_start = 0;
  }

  finalize_ack_nack_processing(entity);

  return;

err:
  LOG_W(RLC, "%s:%d:%s: error decoding PDU, discarding\n", __FILE__, __LINE__, __FUNCTION__);
  goto discard;

discard:
  if (p)
    entity->status_triggered = 1;

#undef R
}

/*************************************************************************/
/* TX functions                                                          */
/*************************************************************************/

static int is_window_stalling(nr_rlc_entity_am_t *entity)
{
  /* we are stalling if tx_next is not:
   *   tx_next_ack <= tx_next < tx_next_ack + window_size
   */
  return !(sn_compare_tx(entity, entity->tx_next_ack, entity->tx_next) <= 0 &&
           sn_compare_tx(entity, entity->tx_next,
                         (entity->tx_next_ack + entity->window_size) %
                           entity->sn_modulus) < 0);
}

static void include_poll(nr_rlc_entity_am_t *entity, char *buffer)
{
  /* set the P bit to 1 */
  buffer[0] |= 0x40;

  entity->pdu_without_poll = 0;
  entity->byte_without_poll = 0;

  /* set POLL_SN to highest SN submitted to lower layer
   * (this is: entity->tx_next - 1) (todo: be sure of this)
   */
  entity->poll_sn = (entity->tx_next - 1 + entity->sn_modulus)
                      % entity->sn_modulus;

  /* start/restart t_poll_retransmit */
  entity->t_poll_retransmit_start = entity->t_current;
}

static int check_poll_after_pdu_assembly(nr_rlc_entity_am_t *entity)
{
  int retransmission_buffer_empty;
  int transmission_buffer_empty;

  /* is transmission buffer empty? */
  if (entity->tx_list == NULL)
    transmission_buffer_empty = 1;
  else
    transmission_buffer_empty = 0;

  /* is retransmission buffer empty? */
  if (entity->retransmit_list == NULL)
    retransmission_buffer_empty = 1;
  else
    retransmission_buffer_empty = 0;

  return (transmission_buffer_empty && retransmission_buffer_empty) ||
         is_window_stalling(entity);
}

static int serialize_sdu(nr_rlc_entity_am_t *entity,
                         nr_rlc_sdu_segment_t *sdu, char *buffer, int bufsize,
                         int p)
{
  nr_rlc_pdu_encoder_t encoder;

  /* generate header */
  nr_rlc_pdu_encoder_init(&encoder, buffer, bufsize);

  nr_rlc_pdu_encoder_put_bits(&encoder, 1, 1);             /* D/C: 1 = data */
  nr_rlc_pdu_encoder_put_bits(&encoder, 0, 1);     /* P: reserve, set later */

  nr_rlc_pdu_encoder_put_bits(&encoder, 1-sdu->is_first,1);/* 1st bit of SI */
  nr_rlc_pdu_encoder_put_bits(&encoder, 1-sdu->is_last,1); /* 2nd bit of SI */

  if (entity->sn_field_length == 18)
    nr_rlc_pdu_encoder_put_bits(&encoder, 0, 2);                       /* R */

  nr_rlc_pdu_encoder_put_bits(&encoder, sdu->sdu->sn,
                                        entity->sn_field_length);     /* SN */

  if (!sdu->is_first)
    nr_rlc_pdu_encoder_put_bits(&encoder, sdu->so, 16);               /* SO */

  /* data */
  memcpy(buffer + encoder.byte, sdu->sdu->data + sdu->so, sdu->size);

  if (p)
    include_poll(entity, buffer);

  return encoder.byte + sdu->size;
}

/* for a given SDU/SDU segment, computes the corresponding PDU header size */
static int compute_pdu_header_size(nr_rlc_entity_am_t *entity,
                                   nr_rlc_sdu_segment_t *sdu)
{
  int header_size = 2;
  /* one more byte if SN field length is 18 */
  if (entity->sn_field_length == 18)
    header_size++;
  /* two more bytes for SO if SDU segment is not the first */
  if (!sdu->is_first) header_size += 2;
  return header_size;
}

/* resize SDU/SDU segment for the corresponding PDU to fit into 'pdu_size'
 * bytes
 * - modifies SDU/SDU segment to become an SDU segment
 * - returns a new SDU segment covering the remaining data bytes
 */
static nr_rlc_sdu_segment_t *resegment(nr_rlc_sdu_segment_t *sdu,
                                       nr_rlc_entity_am_t *entity,
                                       int pdu_size)
{
  nr_rlc_sdu_segment_t *next;
  int pdu_header_size;
  int over_size;

  sdu->sdu->ref_count++;

  pdu_header_size = compute_pdu_header_size(entity, sdu);

  next = calloc(1, sizeof(nr_rlc_sdu_segment_t));
  if (next == NULL) {
    LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__,  __FUNCTION__);
    exit(1);
  }
  *next = *sdu;

  over_size = pdu_header_size + sdu->size - pdu_size;

  /* update SDU */
  sdu->size -= over_size;
  sdu->is_last = 0;

  /* create new segment */
  next->size = over_size;
  next->so = sdu->so + sdu->size;
  next->is_first = 0;

  return next;
}

/*************************************************************************/
/* TX functions - status reporting [begin]                               */
/*************************************************************************/

typedef struct {
  /* data for missing bytes */
  int sn_start;    /* set to -1 when no more missing part to report */
  int so_start;
  int sn_end;
  int so_end;
  /* data for maximum ack */
  int ack_sn;                               /* -1 if not to be used */
  /* pdu to use for next call to 'next_missing' */
  nr_rlc_pdu_t *next;
} missing_data_t;

/* todo: rewrite this function, too messy */
static missing_data_t next_missing(nr_rlc_entity_am_t *entity,
                                        nr_rlc_pdu_t *cur, int check_head)
{
  missing_data_t ret;
  int cur_max_so;
  int sn;
  int max_so       = 0;
  int last_reached = 0;

  ret.ack_sn = -1;

  /* special case: missing part before the head of RX list */
  if (check_head) {
    if (cur->sn != entity->rx_next || !cur->is_first) {
      /* don't report if out of reporting window */
      if (sn_compare_rx(entity, entity->rx_highest_status, cur->sn) <= 0) {
        ret.sn_start = -1;
        return ret;
      }
      /* the missing part is starting from rx_next(0)
       * going to min of:
       *     - cur->sn(cur->so-1) [if cur->sn is not first]
       *       or (cur->sn-1)(0xffff) [if cur->sn is first]
       *     - (entity->rx_highest_status-1)(0xffff)
       */
      ret.sn_start = entity->rx_next;
      ret.so_start = 0;
      ret.next = cur;
      goto set_end_different_sdu;
    }
  }

next_pdu:
  sn = cur->sn;
  cur_max_so = cur->so + cur->size - 1;
  if (cur_max_so > max_so)
    max_so = cur_max_so;
  last_reached = last_reached | cur->is_last;

  /* if cur already processed, it can be the acked SDU */
  if (cur->data == NULL)
    ret.ack_sn = (cur->sn + 1) % entity->sn_modulus;

  /* no next? */
  if (cur->next == NULL) {
    /* inform the caller that work is over */
    ret.next = NULL;

    /* already processed => next SDU to rx_highest_status - 1 to be nacked */
    if (cur->data == NULL) {
      ret.sn_start = (cur->sn + 1) % entity->sn_modulus;
      /* don't report if out of reporting window */
      if (sn_compare_rx(entity, entity->rx_highest_status,
                        ret.sn_start) <= 0) {
        ret.sn_start = -1;
        return ret;
      }
      ret.so_start = 0;
      ret.sn_end   = (entity->rx_highest_status - 1 + entity->sn_modulus) %
                        entity->sn_modulus;
      ret.so_end   = 0xffff;
      return ret;
    }
    /* not already processed => all bytes after max_so (if any) then all SDU
     * to rx_highest_status-1 to be nacked
     */
    if (last_reached) {
      ret.sn_start = (cur->sn + 1) % entity->sn_modulus;
      ret.so_start = 0;
    } else {
      ret.sn_start = cur->sn;
      ret.so_start = max_so + 1;
    }
    /* don't report if out of reporting window */
    if (sn_compare_rx(entity, entity->rx_highest_status,
                      ret.sn_start) <= 0) {
      ret.sn_start = -1;
      return ret;
    }
    ret.sn_end   = (entity->rx_highest_status - 1 + entity->sn_modulus) %
                      entity->sn_modulus;
    ret.so_end   = 0xffff;
    return ret;
  }

  cur = cur->next;

  /* no discontinuity in data => process to next PDU */
  if (cur->sn == sn && max_so >= cur->so - 1)
    goto next_pdu;
  if (cur->sn == (sn + 1) % entity->sn_modulus && last_reached &&
      cur->is_first) {
    last_reached = 0;
    max_so       = 0;
    goto next_pdu;
  }

  /* discontinuity in data */

  /* remember where to start from for the next call */
  ret.next = cur;

  /* discontinuity in same SDU */
  if (cur->sn == sn) {
    ret.sn_start = sn;
    /* don't report if out of reporting window */
    if (sn_compare_rx(entity, entity->rx_highest_status,
                      ret.sn_start) <= 0) {
      ret.sn_start = -1;
      return ret;
    }
    ret.so_start = max_so + 1;
    ret.sn_end = sn;
    ret.so_end = cur->so - 1;
    return ret;
  }

  /* discontinuity between different SDUs */
  ret.sn_start = sn;
  /* don't report if out of reporting window */
  if (sn_compare_rx(entity, entity->rx_highest_status, ret.sn_start) <= 0) {
    ret.sn_start = -1;
    return ret;
  }
  ret.so_start = max_so + 1;

set_end_different_sdu:
  /* don't go more than rx_highest_status - 1 */
  if (sn_compare_rx(entity, entity->rx_highest_status, cur->sn) <= 0) {
    ret.so_end = (entity->rx_highest_status - 1 + entity->sn_modulus) %
                      entity->sn_modulus;
    ret.so_end   = 0xffff;
    return ret;
  }

  /* if cur is the head of a SDU, then use cur-1 */
  if (cur->is_first) {
    ret.sn_end = (cur->sn - 1 + entity->sn_modulus) % entity->sn_modulus;
    ret.so_end = 0xffff;
    return ret;
  }

  ret.sn_end = cur->sn;
  ret.so_end = cur->so - 1;
  return ret;
}

static int nack_size(nr_rlc_entity_am_t *entity, missing_data_t *m)
{
  int nack_length = 2 + (entity->sn_field_length == 18);

  if (m->sn_start == m->sn_end) {
    /* only nack_sn, no so_start/end, no nack range */
    if (m->so_start == 0 && m->so_end == 0xffff)
      return nack_length;
    /* nack_sn + so_start/end */
    return nack_length + 4;
  }

  /* nack_sn + nack range, no so_start/end */
  if (m->so_start == 0 && m->so_end == 0xffff)
    return nack_length + 1;

  /* nack_sn + so_start/end + nack range */
  return nack_length + 5;
}

/* returns the e1 byte/bit position supposing the encoder points at
 * the beginning of a nack_sn block
 */
static void get_e1_position(nr_rlc_entity_am_t *entity,
                            nr_rlc_pdu_encoder_t *encoder,
                            int *e1_byte, int *e1_bit)
{
  if (entity->sn_field_length == 18) {
    *e1_byte = encoder->byte + 2;
    *e1_bit = 5;
  } else {
    *e1_byte = encoder->byte + 1;
    *e1_bit = 3;
  }
}

/* returns the number of nacks serialized.
 * In most cases it is 1, it can be more if the
 * missing data consists of a range that is more
 * than 255 SNs in which case it has to be cut in
 * smaller ranges.
 * If there is no more room in the status buffer,
 * will set m->next = NULL (and may serialize
 * less nacks than required by 'm').
 */
static int generate_missing(nr_rlc_entity_am_t *entity,
                            nr_rlc_pdu_encoder_t *encoder,
                            missing_data_t *m, int *e1_byte, int *e1_bit)
{
  int r_bits = entity->sn_field_length == 18 ? 3 : 1;
  int range_count = 0;
  int sn_start;
  int so_start;
  int sn_end;
  int so_end;
  int sn_count;
  missing_data_t m_nack;
  int e2;
  int e3;

  /* be careful to limit a range to 255 SNs, that is: cut if needed */
  sn_count = (m->sn_end - m->sn_start + entity->sn_modulus)
              % entity->sn_modulus + 1;

  sn_start = m->sn_start;

  while (sn_count) {
    int cur_sn_count = sn_count;
    if (cur_sn_count > 255)
      cur_sn_count = 255;

    /* for first range, so_start is the one of the initial range
     * for the following ones, it is 0
     */
    if (sn_start == m->sn_start) {
      /* first range */
      so_start = m->so_start;
    } else {
      /* following ranges */
      so_start = 0;
    }

    /* for the last range, sn_end/so_end are the ones of the initial range
     * for the previous ones, it is sn_start+254/0xffff
     */
    if (cur_sn_count == sn_count) {
      /* last range */
      sn_end = m->sn_end;
      so_end = m->so_end;
    } else {
      /* previous ranges */
      sn_end = (sn_start + 254) % entity->sn_modulus;
      so_end = 0xffff;
    }

    /* check that there is room for a nack */
    m_nack.sn_start = sn_start;
    m_nack.so_start = so_start;
    m_nack.sn_end = sn_end;
    m_nack.so_end = so_end;
    if (encoder->byte + nack_size(entity, &m_nack) > encoder->size) {
      m->next = NULL;
      break;
    }

    /* set the previous e1 bit to 1 */
    encoder->buffer[*e1_byte] |= 1 << *e1_bit;

    get_e1_position(entity, encoder, e1_byte, e1_bit);

    if (sn_start == sn_end) {
      if (so_start == 0 && so_end == 0xffff) {
        /* only nack_sn, no so_start/end, no nack range */
        e2 = 0;
        e3 = 0;
      } else {
        /* nack_sn + so_start/end, no nack range */
        e2 = 1;
        e3 = 0;
      }
    } else {
      if (so_start == 0 && so_end == 0xffff) {
        /* nack_sn + nack range, no so_start/end */
        e2 = 0;
        e3 = 1;
      } else {
        /* nack_sn + so_start/end + nack range */
        e2 = 1;
        e3 = 1;
      }
    }

    /* nack_sn */
    nr_rlc_pdu_encoder_put_bits(encoder, sn_start,
                                entity->sn_field_length);
    /* e1 = 0 (set later if needed) */
    nr_rlc_pdu_encoder_put_bits(encoder, 0, 1);
    /* e2 */
    nr_rlc_pdu_encoder_put_bits(encoder, e2, 1);
    /* e3 */
    nr_rlc_pdu_encoder_put_bits(encoder, e3, 1);
    /* r */
    nr_rlc_pdu_encoder_put_bits(encoder, 0, r_bits);
    /* so_start/so_end */
    if (e2) {
      nr_rlc_pdu_encoder_put_bits(encoder, so_start, 16);
      nr_rlc_pdu_encoder_put_bits(encoder, so_end, 16);
    }
    /* nack range */
    if (e3)
      nr_rlc_pdu_encoder_put_bits(encoder, cur_sn_count, 8);

    sn_count -= cur_sn_count;
    sn_start = (sn_start + cur_sn_count) % entity->sn_modulus;
    range_count++;
  }

  return range_count;
}

static int generate_status(nr_rlc_entity_am_t *entity, char *buffer, int size)
{
  int                  ack_sn = entity->rx_next;
  missing_data_t  m;
  nr_rlc_pdu_t         *cur;
  int                  nack_count = 0;
  nr_rlc_pdu_encoder_t encoder;
  int                  e1_byte;
  int                  e1_bit;

  /* if not enough room, do nothing */
  if (size < 3)
    return 0;

  nr_rlc_pdu_encoder_init(&encoder, buffer, size);

  /* first 3 bytes, ack_sn and e1 will be set later */
  nr_rlc_pdu_encoder_put_bits(&encoder, 0, 8*3);

  cur = entity->rx_list;

  /* store the position of the e1 bit to be set if
   * there is a nack following
   */
  e1_byte = 2;
  e1_bit = entity->sn_field_length == 18 ? 1 : 7;

  while (cur != NULL) {
    m = next_missing(entity, cur, nack_count == 0);

    /* update ack_sn if the returned value is valid */
    if (m.ack_sn != -1)
      ack_sn = m.ack_sn;

    /* stop here if no more nack to report */
    if (m.sn_start == -1)
      break;

    nack_count += generate_missing(entity, &encoder, &m, &e1_byte, &e1_bit);

    cur = m.next;
  }

  /* put ack_sn */
  if (entity->sn_field_length == 12) {
    buffer[0] = ack_sn >> 8;
    buffer[1] = ack_sn & 255;
  } else {
    buffer[0] = ack_sn >> 14;
    buffer[1] = (ack_sn >> 6) & 255;
    buffer[2] |= (ack_sn & 0x3f) << 2;
  }

  /* reset the trigger */
  entity->status_triggered = 0;

  /* start t_status_prohibit */
  entity->t_status_prohibit_start = entity->t_current;

  return encoder.byte;
}

static int status_to_report(nr_rlc_entity_am_t *entity)
{
  return entity->status_triggered &&
         (entity->t_status_prohibit_start == 0 ||
          entity->t_current - entity->t_status_prohibit_start >
              entity->t_status_prohibit);
}

static int missing_size(nr_rlc_entity_am_t *entity, missing_data_t *m,
                        int *size, int maxsize)
{
  int r_bits = entity->sn_field_length == 18 ? 3 : 1;
  int range_count = 0;
  int sn_start;
  int so_start;
  int sn_end;
  int so_end;
  int sn_count;
  missing_data_t m_nack;

  /* be careful to limit a range to 255 SNs, that is: cut if needed */
  sn_count = m->sn_end - m->sn_start + 1;
  if (sn_count < 0)
    sn_count += entity->sn_modulus;

  sn_start = m->sn_start;

  while (sn_count) {
    int cur_sn_count = sn_count;
    if (cur_sn_count > 255)
      cur_sn_count = 255;

    /* for first range, so_start is the one of the initial range
     * for the following ones, it is 0
     */
    if (sn_start == m->sn_start) {
      /* first range */
      so_start = m->so_start;
    } else {
      /* following ranges */
      so_start = 0;
    }

    /* for the last range, sn_end/so_end are the ones of the initial range
     * for the previous ones, it is sn_start+254/0xffff
     */
    if (cur_sn_count == sn_count) {
      /* last range */
      sn_end = m->sn_end;
      so_end = m->so_end;
    } else {
      /* previous ranges */
      sn_end = (sn_start + 254) % entity->sn_modulus;
      so_end = 0xffff;
    }

    /* check that there is room for a nack */
    m_nack.sn_start = sn_start;
    m_nack.so_start = so_start;
    m_nack.sn_end = sn_end;
    m_nack.so_end = so_end;
    if (*size + nack_size(entity, &m_nack) > maxsize) {
      m->next = NULL;
      break;
    }

    if (sn_start == sn_end) {
      if (so_start == 0 && so_end == 0xffff) {
        /* only nack_sn, no so_start/end, no nack range */
        *size += (entity->sn_field_length + 3 + r_bits) / 8;
      } else {
        /* nack_sn + so_start/end, no nack range */
        *size += (entity->sn_field_length + 3 + r_bits + 16*2) / 8;
      }
    } else {
      if (so_start == 0 && so_end == 0xffff) {
        /* nack_sn + nack range, no so_start/end */
        *size += (entity->sn_field_length + 3 + r_bits + 8) / 8;
      } else {
        /* nack_sn + so_start/end + nack range */
        *size += (entity->sn_field_length + 3 + r_bits + 16*2 + 8) / 8;
      }
    }

    sn_count -= cur_sn_count;
    sn_start = (sn_start + cur_sn_count) % entity->sn_modulus;
    range_count++;
  }

  return range_count;
}

static int status_size(nr_rlc_entity_am_t *entity, int maxsize)
{
  missing_data_t  m;
  nr_rlc_pdu_t    *cur;
  int             nack_count = 0;
  int             size;

  /* if not enough room, do nothing */
  if (maxsize < 3)
    return 0;

  /* minimum 3 bytes */
  size = 3;

  cur = entity->rx_list;

  while (cur != NULL) {
    m = next_missing(entity, cur, nack_count == 0);

    /* stop here if no more nack to report */
    if (m.sn_start == -1)
      break;

    nack_count += missing_size(entity, &m, &size, maxsize);

    cur = m.next;
  }

  return size;
}

/*************************************************************************/
/* TX functions - status reporting [end]                                 */
/*************************************************************************/

static int generate_retx_pdu(nr_rlc_entity_am_t *entity, char *buffer,
                             int size)
{
  nr_rlc_sdu_segment_t *sdu;
  int pdu_header_size;
  int pdu_size;
  int p;

  sdu = entity->retransmit_list;

  pdu_header_size = compute_pdu_header_size(entity, sdu);

  /* not enough room for at least one byte of data? do nothing */
  if (pdu_header_size + 1 > size)
    return 0;

  entity->retransmit_list = entity->retransmit_list->next;
  if (entity->retransmit_list == NULL)
    entity->retransmit_end = NULL;

  sdu->next = NULL;

  /* segment if necessary */
  pdu_size = pdu_header_size + sdu->size;
  if (pdu_size > size) {
    nr_rlc_sdu_segment_t *next_sdu;
    next_sdu = resegment(sdu, entity, size);
    /* put the second SDU back at the head of the retransmit list */
    next_sdu->next = entity->retransmit_list;
    entity->retransmit_list = next_sdu;
    if (entity->retransmit_end == NULL)
      entity->retransmit_end = entity->retransmit_list;
  }

  /* put SDU/SDU segment in the wait list */
  nr_rlc_sdu_segment_list_append(&entity->wait_list, &entity->wait_end, sdu);

  p = check_poll_after_pdu_assembly(entity);

  if (entity->force_poll) {
    p = 1;
    entity->force_poll = 0;
  }

  return serialize_sdu(entity, sdu, buffer, size, p);
}

static int generate_tx_pdu(nr_rlc_entity_am_t *entity, char *buffer, int size)
{
  nr_rlc_sdu_segment_t *sdu;
  int pdu_header_size;
  int pdu_size;
  int p;

  /* sn out of window (that is: we have window stalling)? do nothing */
  if (is_window_stalling(entity))
    return 0;

  if (entity->tx_list == NULL)
    return 0;

  sdu = entity->tx_list;

  pdu_header_size = compute_pdu_header_size(entity, sdu);

  /* not enough room for at least one byte of data? do nothing */
  if (pdu_header_size + 1 > size)
    return 0;

  entity->tx_list = entity->tx_list->next;
  if (entity->tx_list == NULL)
    entity->tx_end = NULL;

  sdu->next = NULL;

  /* assign SN to SDU */
  sdu->sdu->sn = entity->tx_next;

  /* segment if necessary */
  pdu_size = pdu_header_size + sdu->size;
  if (pdu_size > size) {
    nr_rlc_sdu_segment_t *next_sdu;
    next_sdu = resegment(sdu, entity, size);
    /* put the second SDU back at the head of the TX list */
    next_sdu->next = entity->tx_list;
    entity->tx_list = next_sdu;
    if (entity->tx_end == NULL)
      entity->tx_end = entity->tx_list;
  }

  /* update tx_next if the SDU segment is the last */
  if (sdu->is_last)
    entity->tx_next = (entity->tx_next + 1) % entity->sn_modulus;

  /* put SDU/SDU segment in the wait list */
  nr_rlc_sdu_segment_list_append(&entity->wait_list, &entity->wait_end, sdu);

  /* polling actions for a new PDU */
  entity->pdu_without_poll++;
  entity->byte_without_poll += sdu->size;
  if ((entity->poll_pdu != -1 &&
       entity->pdu_without_poll >= entity->poll_pdu) ||
      (entity->poll_byte != -1 &&
       entity->byte_without_poll >= entity->poll_byte))
    p = 1;
  else
    p = check_poll_after_pdu_assembly(entity);

  if (entity->force_poll) {
    p = 1;
    entity->force_poll = 0;
  }

  return serialize_sdu(entity, sdu, buffer, size, p);
}

/* Pretend to serialize all the SDUs in a list and return the size
 * of all the PDUs it would produce, limited to 'maxsize'.
 * Used for buffer status reporting.
 */
static int tx_list_size(nr_rlc_entity_am_t *entity,
                        nr_rlc_sdu_segment_t *l, int maxsize)
{
  int ret = 0;

  while (l != NULL && ret < maxsize) {
    ret += compute_pdu_header_size(entity, l) + l->size;
    l = l->next;
  }

  if (ret > maxsize) ret = maxsize;
  return ret;
}

nr_rlc_entity_buffer_status_t nr_rlc_entity_am_buffer_status(
    nr_rlc_entity_t *_entity, int maxsize)
{
  nr_rlc_entity_am_t *entity = (nr_rlc_entity_am_t *)_entity;
  nr_rlc_entity_buffer_status_t ret;

  if (status_to_report(entity))
    ret.status_size = status_size(entity, maxsize);
  else
    ret.status_size = 0;

  ret.tx_size = tx_list_size(entity, entity->tx_list, maxsize);
  ret.retx_size = tx_list_size(entity, entity->retransmit_list, maxsize);

  return ret;
}

int nr_rlc_entity_am_generate_pdu(nr_rlc_entity_t *_entity,
                                  char *buffer, int size)
{
  nr_rlc_entity_am_t *entity = (nr_rlc_entity_am_t *)_entity;
  int ret;

  if (status_to_report(entity)) {
    ret = generate_status(entity, buffer, size);
    if (ret != 0)
      return ret;
  }

  if (entity->retransmit_list != NULL) {
    ret = generate_retx_pdu(entity, buffer, size);
    if (ret != 0)
      return ret;
  }

  return generate_tx_pdu(entity, buffer, size);
}

/*************************************************************************/
/* SDU RX functions                                                      */
/*************************************************************************/

void nr_rlc_entity_am_recv_sdu(nr_rlc_entity_t *_entity,
                               char *buffer, int size,
                               int sdu_id)
{
  nr_rlc_entity_am_t *entity = (nr_rlc_entity_am_t *)_entity;
  nr_rlc_sdu_segment_t *sdu;

  if (size > NR_SDU_MAX) {
    LOG_E(RLC, "%s:%d:%s: fatal: SDU size too big (%d bytes)\n",
          __FILE__, __LINE__, __FUNCTION__, size);
    exit(1);
  }

  if (entity->tx_size + size > entity->tx_maxsize) {
    LOG_E(RLC, "%s:%d:%s: warning: SDU rejected, SDU buffer full\n",
          __FILE__, __LINE__, __FUNCTION__);
    return;
  }

  entity->tx_size += size;

  sdu = nr_rlc_new_sdu(buffer, size, sdu_id);

  LOG_D(RLC, "Created new RLC SDU and append it to the RLC list \n");

  nr_rlc_sdu_segment_list_append(&entity->tx_list, &entity->tx_end, sdu);
}

/*************************************************************************/
/* time/timers                                                           */
/*************************************************************************/

static void check_t_poll_retransmit(nr_rlc_entity_am_t *entity)
{
  nr_rlc_sdu_segment_t head;
  nr_rlc_sdu_segment_t *cur;
  nr_rlc_sdu_segment_t *prev;
  int sn;
  int old_retx_count;

  /* 38.322 5.3.3.4 */
  /* did t_poll_retransmit expire? */
  if (entity->t_poll_retransmit_start == 0 ||
      entity->t_current <= entity->t_poll_retransmit_start +
                               entity->t_poll_retransmit)
    return;

  /* stop timer */
  entity->t_poll_retransmit_start = 0;

  /* 38.322 5.3.3.4 says:
   *
   *     - include a poll in a RLC data PDU as described in section 5.3.3.2
   *
   * That does not seem to be conditional. So we forcefully will send
   * a poll as soon as we generate a PDU.
   * Hopefully this interpretation is correct. In the worst case we generate
   * more polling than necessary, but it's not a big deal. When
   * 't_poll_retransmit' expires it means we didn't receive a status report,
   * meaning a bad radio link, so things are quite bad at this point and
   * asking again for a poll won't hurt much more.
   */
  entity->force_poll = 1;

  LOG_D(RLC, "%s:%d:%s: warning: t_poll_retransmit expired\n",
        __FILE__, __LINE__, __FUNCTION__);

  /* do we meet conditions of 38.322 5.3.3.4? */
  if (!check_poll_after_pdu_assembly(entity))
    return;

  /* search wait list for SDU with highest SN */
  /* this code may be incorrect: in LTE we had to look for PDU
   * with SN = VT(S) - 1, but for NR the specs say "highest SN among the
   * ones submitted to lower layers" not 'tx_next - 1'. So we should look
   * for the highest SN in the wait list. But that's no big deal. If the
   * program runs this code, then the connection is in a bad state and we
   * can retransmit whatever we want. At some point we will receive a status
   * report and retransmit what we really have to. Actually we could just
   * retransmit the head of wait list (the specs have this 'or').
   * (Actually, maybe this interpretation is not correct and what the code
   * does is correct. The specs are confusing.)
   */
  sn = (entity->tx_next - 1 + entity->sn_modulus) % entity->sn_modulus;

  head.next = entity->wait_list;
  cur = entity->wait_list;
  prev = &head;

  while (cur != NULL) {
    if (cur->sdu->sn == sn)
      break;
    prev = cur;
    cur = cur->next;
  }

  /* SDU with highest SN not found? take the head of wait list */
  if (cur == NULL) {
    cur = entity->wait_list;
    prev = &head;
    sn = cur->sdu->sn;
  }

  /* todo: do we need to for check cur == NULL?
   * It seems that no, the wait list should not be empty here, but not sure.
   */

  old_retx_count = cur->sdu->retx_count;

  /* 38.322 says "SDU", not "SDU segment", so let's retransmit all
   * SDU segments with this SN
   */
  /* todo: maybe we could simply retransmit the current SDU segment,
   * so that we don't have to run through the full wait list.
   */
  while (cur != NULL) {
    if (cur->sdu->sn == sn) {
      prev->next = cur->next;
      cur->next = NULL;
      /* put in retransmit list */
      consider_retransmission(entity, cur,
                              old_retx_count == cur->sdu->retx_count);
    } else {
      prev = cur;
    }
    cur = prev->next;
  }
  entity->wait_list = head.next;
  /* reset wait_end (todo: optimize?) */
  entity->wait_end = entity->wait_list;
  while (entity->wait_end != NULL && entity->wait_end->next != NULL)
    entity->wait_end = entity->wait_end->next;
}

static void check_t_reassembly(nr_rlc_entity_am_t *entity)
{
  int sn;

  /* is t_reassembly running and if yes has it expired? */
  if (entity->t_reassembly_start == 0 ||
      entity->t_current <= entity->t_reassembly_start + entity->t_reassembly)
    return;

  /* stop timer */
  entity->t_reassembly_start = 0;

  LOG_D(RLC, "%s:%d:%s: t_reassembly expired\n",
        __FILE__, __LINE__, __FUNCTION__);

  /* update RX_Highest_Status */
  sn = entity->rx_next_status_trigger;
  while (sdu_delivered(entity, sn))
    sn = (sn + 1) % entity->sn_modulus;
  entity->rx_highest_status = sn;

  if (sn_compare_rx(entity, entity->rx_next_highest,
                    (entity->rx_highest_status+1) % entity->sn_modulus) > 0 ||
      (entity->rx_next_highest ==
         (entity->rx_highest_status+1) % entity->sn_modulus &&
       sdu_has_missing_bytes(entity, entity->rx_highest_status))) {
    entity->t_reassembly_start = entity->t_current;
    entity->rx_next_status_trigger = entity->rx_next_highest;
  }
}

void nr_rlc_entity_am_set_time(nr_rlc_entity_t *_entity, uint64_t now)
{
  nr_rlc_entity_am_t *entity = (nr_rlc_entity_am_t *)_entity;

  entity->t_current = now;

  check_t_poll_retransmit(entity);

  check_t_reassembly(entity);
}

/*************************************************************************/
/* discard/re-establishment/delete                                       */
/*************************************************************************/

void nr_rlc_entity_am_discard_sdu(nr_rlc_entity_t *_entity, int sdu_id)
{
  /* implements 38.322 5.4 */
  nr_rlc_entity_am_t *entity = (nr_rlc_entity_am_t *)_entity;
  nr_rlc_sdu_segment_t head;
  nr_rlc_sdu_segment_t *cur;
  nr_rlc_sdu_segment_t *prev;

  head.next = entity->tx_list;
  cur = entity->tx_list;
  prev = &head;

  while (cur != NULL && cur->sdu->upper_layer_id != sdu_id) {
    prev = cur;
    cur = cur->next;
  }

  /* if sdu_id not found or some bytes have already been 'PDU-ized'
   * then do nothing
   */
  if (cur == NULL || !cur->is_first || !cur->is_last)
    return;

  /* remove SDU from tx_list */
  prev->next = cur->next;
  entity->tx_list = head.next;
  if (entity->tx_end == cur) {
    if (prev != &head)
      entity->tx_end = prev;
    else
      entity->tx_end = NULL;
  }

  nr_rlc_free_sdu_segment(cur);
}

static void clear_entity(nr_rlc_entity_am_t *entity)
{
  nr_rlc_pdu_t *cur_rx;

  entity->rx_next                = 0;
  entity->rx_next_status_trigger = 0;
  entity->rx_highest_status      = 0;
  entity->rx_next_highest        = 0;

  entity->status_triggered = 0;

  entity->tx_next           = 0;
  entity->tx_next_ack       = 0;
  entity->poll_sn           = 0;
  entity->pdu_without_poll  = 0;
  entity->byte_without_poll = 0;
  entity->force_poll        = 0;

  entity->t_current = 0;

  entity->t_poll_retransmit_start = 0;
  entity->t_reassembly_start      = 0;
  entity->t_status_prohibit_start = 0;

  cur_rx = entity->rx_list;
  while (cur_rx != NULL) {
    nr_rlc_pdu_t *p = cur_rx;
    cur_rx = cur_rx->next;
    nr_rlc_free_pdu(p);
  }
  entity->rx_list = NULL;
  entity->rx_size = 0;

  nr_rlc_free_sdu_segment_list(entity->tx_list);
  nr_rlc_free_sdu_segment_list(entity->wait_list);
  nr_rlc_free_sdu_segment_list(entity->retransmit_list);
  nr_rlc_free_sdu_segment_list(entity->ack_list);

  entity->tx_list         = NULL;
  entity->tx_end          = NULL;
  entity->tx_size         = 0;

  entity->wait_list       = NULL;
  entity->wait_end        = NULL;

  entity->retransmit_list = NULL;
  entity->retransmit_end  = NULL;

  entity->ack_list        = NULL;
}

void nr_rlc_entity_am_reestablishment(nr_rlc_entity_t *_entity)
{
  nr_rlc_entity_am_t *entity = (nr_rlc_entity_am_t *)_entity;
  clear_entity(entity);
}

void nr_rlc_entity_am_delete(nr_rlc_entity_t *_entity)
{
  nr_rlc_entity_am_t *entity = (nr_rlc_entity_am_t *)_entity;
  clear_entity(entity);
  free(entity);
}
