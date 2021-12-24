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

#include "rlc_entity_am.h"
#include "rlc_pdu.h"

#include <stdlib.h>
#include <string.h>

#include "LOG/log.h"

/*************************************************************************/
/* PDU RX functions                                                      */
/*************************************************************************/

static int modulus_rx(rlc_entity_am_t *entity, int a)
{
  /* as per 36.322 7.1, modulus base is vr(r) and modulus is 1024 for rx */
  int r = a - entity->vr_r;
  if (r < 0) r += 1024;
  return r;
}

/* used in both RX and TX processing */
static int modulus_tx(rlc_entity_am_t *entity, int a)
{
  /* as per 36.322 7.1, modulus base is vt(a) and modulus is 1024 for tx */
  int r = a - entity->vt_a;
  if (r < 0) r += 1024;
  return r;
}

static int sn_in_recv_window(void *_entity, int sn)
{
  rlc_entity_am_t *entity = _entity;
  int mod_sn = modulus_rx(entity, sn);
  /* we simplify vr(r)<=sn<vr(mr). base is vr(r) and vr(mr) = vr(r) + 512 */
  return mod_sn < 512;
}

static int sn_compare_rx(void *_entity, int a, int b)
{
  rlc_entity_am_t *entity = _entity;
  return modulus_rx(entity, a) - modulus_rx(entity, b);
}

/* used in both RX and TX processing */
static int sn_compare_tx(void *_entity, int a, int b)
{
  rlc_entity_am_t *entity = _entity;
  return modulus_tx(entity, a) - modulus_tx(entity, b);
}

static int segment_already_received(rlc_entity_am_t *entity,
    int sn, int so, int data_size)
{
  /* TODO: optimize */
  rlc_rx_pdu_segment_t *l = entity->rx_list;

  while (l != NULL) {
    if (l->sn == sn && l->so <= so &&
        l->so + l->size - l->data_offset >= so + data_size)
      return 1;
    l = l->next;
  }

  return 0;
}

static int rlc_am_segment_full(rlc_entity_am_t *entity, int sn)
{
  rlc_rx_pdu_segment_t *l = entity->rx_list;
  int last_byte;
  int new_last_byte;

  last_byte = -1;
  while (l != NULL) {
    if (l->sn == sn)
      break;
    l = l->next;
  }
  while (l != NULL && l->sn == sn) {
    if (l->so > last_byte + 1)
      return 0;
    if (l->is_last)
      return 1;
    new_last_byte = l->so + l->size - l->data_offset - 1;
    if (new_last_byte > last_byte)
      last_byte = new_last_byte;
    l = l->next;
  }
  return 0;
}

/* return 1 if the new segment has some data to consume, 0 if not */
static int rlc_am_reassemble_next_segment(rlc_am_reassemble_t *r)
{
  int rf;
  int sn;

  r->sdu_offset = r->start->data_offset;

  rlc_pdu_decoder_init(&r->dec, r->start->data, r->start->size);

  rlc_pdu_decoder_get_bits(&r->dec, 1);            /* dc */
  rf    = rlc_pdu_decoder_get_bits(&r->dec, 1);
  rlc_pdu_decoder_get_bits(&r->dec, 1);            /* p */
  r->fi = rlc_pdu_decoder_get_bits(&r->dec, 2);
  r->e  = rlc_pdu_decoder_get_bits(&r->dec, 1);
  sn    = rlc_pdu_decoder_get_bits(&r->dec, 10);
  if (rf) {
    rlc_pdu_decoder_get_bits(&r->dec, 1);          /* lsf */
    r->so = rlc_pdu_decoder_get_bits(&r->dec, 15);
  } else {
    r->so = 0;
  }

  if (r->e) {
    r->e       = rlc_pdu_decoder_get_bits(&r->dec, 1);
    r->sdu_len = rlc_pdu_decoder_get_bits(&r->dec, 11);
  } else
    r->sdu_len = r->start->size - r->sdu_offset;

  /* new sn: read starts from PDU byte 0 */
  if (sn != r->sn) {
    r->pdu_byte = 0;
    r->sn = sn;
  }

  r->data_pos = r->start->data_offset + r->pdu_byte - r->so;

  /* TODO: remove this check, it is useless, data has been validated before */
  if (r->pdu_byte < r->so) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  /* if pdu_byte is not in [so .. so+len-1] then all bytes from this segment
   * have already been consumed
   */
  if (r->pdu_byte >= r->so + r->start->size - r->start->data_offset)
    return 0;

  /* go to correct SDU */
  while (r->pdu_byte >= r->so + (r->sdu_offset - r->start->data_offset) + r->sdu_len) {
    r->sdu_offset += r->sdu_len;
    if (r->e) {
      r->e       = rlc_pdu_decoder_get_bits(&r->dec, 1);
      r->sdu_len = rlc_pdu_decoder_get_bits(&r->dec, 11);
    } else {
      r->sdu_len = r->start->size - r->sdu_offset;
    }
  }

  return 1;
}

static void rlc_am_reassemble(rlc_entity_am_t *entity)
{
  rlc_am_reassemble_t *r = &entity->reassemble;

  while (r->start != NULL) {
    if (r->sdu_pos >= SDU_MAX) {
      /* TODO: proper error handling (discard PDUs with current sn from
       * reassembly queue? something else?)
       */
      LOG_E(RLC, "%s:%d:%s: bad RLC PDU\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    r->sdu[r->sdu_pos] = r->start->data[r->data_pos];
    r->sdu_pos++;
    r->data_pos++;
    r->pdu_byte++;
    if (r->data_pos == r->sdu_offset + r->sdu_len) {
      /* all bytes of SDU are consumed, check if SDU is fully there.
       * It is if the data pointer is not at the end of the PDU segment
       * or if 'fi' & 1 == 0
       */
      if (r->data_pos != r->start->size ||
          (r->fi & 1) == 0) {
        /* SDU is full - deliver to higher layer */
        entity->common.deliver_sdu(entity->common.deliver_sdu_data,
                                   (rlc_entity_t *)entity,
                                   r->sdu, r->sdu_pos);
        r->sdu_pos = 0;
      }
      if (r->data_pos != r->start->size) {
        /* not at the end, process next SDU */
        r->sdu_offset += r->sdu_len;
        if (r->e) {
          r->e       = rlc_pdu_decoder_get_bits(&r->dec, 1);
          r->sdu_len = rlc_pdu_decoder_get_bits(&r->dec, 11);
        } else
          r->sdu_len = r->start->size - r->sdu_offset;
      } else {
        /* all bytes are consumend, go to next segment not already fully
         * processed, if any
         */
        do {
          rlc_rx_pdu_segment_t *e = r->start;
          entity->rx_size -= e->size;
          r->start = r->start->next;
          rlc_rx_free_pdu_segment(e);
        } while (r->start != NULL && !rlc_am_reassemble_next_segment(r));
      }
    }
  }
}

static void rlc_am_reception_actions(rlc_entity_am_t *entity,
    rlc_rx_pdu_segment_t *pdu_segment)
{
  int x = pdu_segment->sn;
  int vr_ms;
  int vr_r;

  if (modulus_rx(entity, x) >= modulus_rx(entity, entity->vr_h))
    entity->vr_h = (x + 1) % 1024;

  vr_ms = entity->vr_ms;
  while (rlc_am_segment_full(entity, vr_ms))
    vr_ms = (vr_ms + 1) % 1024;
  entity->vr_ms = vr_ms;

  if (x == entity->vr_r) {
    vr_r = entity->vr_r;
    while (rlc_am_segment_full(entity, vr_r)) {
      /* move segments with sn=vr(r) from rx list to end of reassembly list */
      while (entity->rx_list != NULL && entity->rx_list->sn == vr_r) {
        rlc_rx_pdu_segment_t *e = entity->rx_list;
        entity->rx_list = e->next;
        e->next = NULL;
        if (entity->reassemble.start == NULL) {
          entity->reassemble.start = e;
          /* the list was empty, we need to init decoder */
          entity->reassemble.sn = -1;
          if (!rlc_am_reassemble_next_segment(&entity->reassemble)) {
            /* TODO: proper error recovery (or remove the test, it should not happen) */
            LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
            exit(1);
          }
        } else {
          entity->reassemble.end->next = e;
        }
        entity->reassemble.end = e;
      }

      /* update vr_r */
      vr_r = (vr_r + 1) % 1024;
    }
    entity->vr_r = vr_r;
  }

  rlc_am_reassemble(entity);

  if (entity->t_reordering_start) {
    int vr_x = entity->vr_x;
    if (vr_x < entity->vr_r) vr_x += 1024;
    if (vr_x == entity->vr_r || vr_x > entity->vr_r + 512)
      entity->t_reordering_start = 0;
  }

  if (entity->t_reordering_start == 0) {
    if (sn_compare_rx(entity, entity->vr_h, entity->vr_r) > 0) {
      entity->t_reordering_start = entity->t_current;
      entity->vr_x = entity->vr_h;
    }
  }
}

static void process_received_ack(rlc_entity_am_t *entity, int sn)
{
  rlc_tx_pdu_segment_t head;
  rlc_tx_pdu_segment_t *cur;
  rlc_tx_pdu_segment_t *prev;

  /* put all PDUs from wait and retransmit lists with SN < 'sn' to ack_list */

  /* process wait list */
  head.next = entity->wait_list;
  prev = &head;
  cur = entity->wait_list;
  while (cur != NULL) {
    if (sn_compare_tx(entity, cur->sn, sn) < 0) {
      /* remove from wait list */
      prev->next = cur->next;
      /* put the PDU in the ack list */
      entity->ack_list = rlc_tx_pdu_list_add(sn_compare_tx, entity,
                                             entity->ack_list, cur);
      cur = prev->next;
    } else {
      prev = cur;
      cur = cur->next;
    }
  }
  entity->wait_list = head.next;

  /* process retransmit list */
  head.next = entity->retransmit_list;
  prev = &head;
  cur = entity->retransmit_list;
  while (cur != NULL) {
    if (sn_compare_tx(entity, cur->sn, sn) < 0) {
      /* dec. retx_count in case we put this segment back in retransmit list
       * in 'process_received_nack'
       */
      cur->retx_count--;
      /* remove from retransmit list */
      prev->next = cur->next;
      /* put the PDU in the ack list */
      entity->ack_list = rlc_tx_pdu_list_add(sn_compare_tx, entity,
                                             entity->ack_list, cur);
      cur = prev->next;
    } else {
      prev = cur;
      cur = cur->next;
    }
  }
  entity->retransmit_list = head.next;

}

static void consider_retransmission(rlc_entity_am_t *entity,
    rlc_tx_pdu_segment_t *cur)
{
  cur->retx_count++;

  /* let's report max RETX reached for all retx_count >= max_retx_threshold
   * (specs say to report if retx_count == max_retx_threshold).
   * Upper layers should react (radio link failure), so no big deal actually.
   */
  if (cur->retx_count >= entity->max_retx_threshold) {
    entity->common.max_retx_reached(entity->common.max_retx_reached_data,
                                    (rlc_entity_t *)entity);
  }

  /* let's put in retransmit list even if we are over max_retx_threshold.
   * upper layers should deal with this condition, internally it's better
   * for the RLC code to keep going with this segment (we only remove
   * a segment that was ACKed)
   */
  entity->retransmit_list = rlc_tx_pdu_list_add(sn_compare_tx, entity,
                                                entity->retransmit_list, cur);
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

static void process_received_nack(rlc_entity_am_t *entity, int sn,
    int so_start, int so_end)
{
  /* put all PDU segments with SN == 'sn' and with an overlapping so start/end
   * to the retransmit list
   * source lists are ack list and wait list.
   * Not sure if we should consider wait list, isn't the other end supposed
   * to only NACK SNs lower than the ACK SN sent in the status PDU, in which
   * case all potential PDU segments should all be in ack list when calling
   * the current function? in doubt let's accept anything and thus process
   * also wait list.
   */
  rlc_tx_pdu_segment_t head;
  rlc_tx_pdu_segment_t *cur;
  rlc_tx_pdu_segment_t *prev;

  /* check that VT(A) <= sn < VT(S) */
  if (!(sn_compare_tx(entity, entity->vt_a, sn) <= 0 &&
        sn_compare_tx(entity, sn, entity->vt_s) < 0))
    return;

  /* process wait list */
  head.next = entity->wait_list;
  prev = &head;
  cur = entity->wait_list;
  while (cur != NULL) {
    if (cur->sn == sn &&
        so_overlap(so_start, so_end, cur->so, cur->so + cur->data_size - 1)) {
      /* remove from wait list */
      prev->next = cur->next;
      /* consider the PDU segment for retransmission */
      consider_retransmission(entity, cur);
      cur = prev->next;
    } else {
      prev = cur;
      cur = cur->next;
    }
  }
  entity->wait_list = head.next;

  /* process ack list */
  head.next = entity->ack_list;
  prev = &head;
  cur = entity->ack_list;
  while (cur != NULL) {
    if (cur->sn == sn &&
        so_overlap(so_start, so_end, cur->so, cur->so + cur->data_size - 1)) {
      /* remove from ack list */
      prev->next = cur->next;
      /* consider the PDU segment for retransmission */
      consider_retransmission(entity, cur);
      cur = prev->next;
    } else {
      prev = cur;
      cur = cur->next;
    }
  }
  entity->ack_list = head.next;
}

int tx_pdu_in_ack_list_full(rlc_tx_pdu_segment_t *pdu)
{
  int sn = pdu->sn;
  int last_byte = -1;
  int new_last_byte;
  int is_last_seen = 0;

  while (pdu != NULL && pdu->sn == sn) {
    if (pdu->so > last_byte + 1) return 0;
    if (pdu->is_last)
      is_last_seen = 1;
    new_last_byte = pdu->so + pdu->data_size - 1;
    if (new_last_byte > last_byte)
      last_byte = new_last_byte;
    pdu = pdu->next;
  }

  return is_last_seen == 1;
}

int tx_pdu_in_ack_list_size(rlc_tx_pdu_segment_t *pdu)
{
  int sn = pdu->sn;
  int ret = 0;

  while (pdu != NULL && pdu->sn == sn) {
    ret += pdu->data_size;
    pdu = pdu->next;
  }

  return ret;
}

void ack_sdu_bytes(rlc_sdu_t *start, int start_byte, int sdu_size)
{
  rlc_sdu_t *cur = start;
  int remaining_size = sdu_size;

  while (remaining_size) {
    int cursize = cur->size - start_byte;
    if (cursize > remaining_size)
      cursize = remaining_size;
    cur->acked_bytes += cursize;
    remaining_size -= cursize;
    /* start_byte is only meaningful for the 1st SDU, then it is 0 */
    start_byte = 0;
    cur = cur->next;
  }
}

rlc_tx_pdu_segment_t *tx_list_remove_sn(rlc_tx_pdu_segment_t *list, int sn)
{
  rlc_tx_pdu_segment_t head;
  rlc_tx_pdu_segment_t *cur;
  rlc_tx_pdu_segment_t *prev;

  head.next = list;
  cur = list;
  prev = &head;

  while (cur != NULL) {
    if (cur->sn == sn) {
      prev->next = cur->next;
      rlc_tx_free_pdu(cur);
      cur = prev->next;
    } else {
      prev = cur;
      cur = cur->next;
    }
  }

  return head.next;
}

void cleanup_sdu_list(rlc_entity_am_t *entity)
{
  rlc_sdu_t head;
  rlc_sdu_t *cur;
  rlc_sdu_t *prev;

  /* remove fully acked SDUs, indicate successful delivery to upper layer */
  head.next = entity->tx_list;
  cur = entity->tx_list;
  prev = &head;

  while (cur != NULL) {
    if (cur->acked_bytes == cur->size) {
      prev->next = cur->next;
      entity->tx_size -= cur->size;
      entity->common.sdu_successful_delivery(
          entity->common.sdu_successful_delivery_data,
          (rlc_entity_t *)entity, cur->upper_layer_id);
      rlc_free_sdu(cur);
      entity->tx_end = prev;
      cur = prev->next;
    } else {
      entity->tx_end = cur;
      prev = cur;
      cur = cur->next;
    }
  }

  entity->tx_list = head.next;

  /* if tx_end == head then it means that the list is now empty */
  if (entity->tx_end == &head)
    entity->tx_end = NULL;
}

static void finalize_ack_nack_processing(rlc_entity_am_t *entity)
{
  int sn;
  rlc_tx_pdu_segment_t *cur = entity->ack_list;
  int pdu_size;

  if (cur == NULL)
    return;

  /* Remove full PDUs and ack the SDU bytes they cover. Start from SN == VT(A)
   * and process increasing SNs until end of list or missing ACK or PDU not
   * fully ACKed.
   */
  while (cur != NULL && cur->sn == entity->vt_a &&
         tx_pdu_in_ack_list_full(cur)) {
    sn = cur->sn;
    entity->vt_a = (entity->vt_a + 1) % 1024;
    pdu_size = tx_pdu_in_ack_list_size(cur);
    ack_sdu_bytes(cur->start_sdu, cur->sdu_start_byte, pdu_size);
    while (cur != NULL && cur->sn == sn)
      cur = cur->next;
    entity->ack_list = tx_list_remove_sn(entity->ack_list, sn);
  }

  cleanup_sdu_list(entity);
}

void rlc_entity_am_recv_pdu(rlc_entity_t *_entity, char *buffer, int size)
{
#define R(d) do { if (rlc_pdu_decoder_in_error(&d)) goto err; } while (0)
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  rlc_pdu_decoder_t decoder;
  rlc_pdu_decoder_t data_decoder;
  rlc_pdu_decoder_t control_decoder;

  int dc;
  int rf;
  int p = 0;
  int fi;
  int e;
  int sn;
  int lsf;
  int so;

  int cpt;
  int e1;
  int e2;
  int ack_sn;
  int nack_sn;
  int so_start;
  int so_end;
  int control_e1;
  int control_e2;

  int data_e;
  int data_li;

  int packet_count;
  int data_size;
  int data_start;
  int indicated_data_size;

  rlc_rx_pdu_segment_t *pdu_segment;

  rlc_pdu_decoder_init(&decoder, buffer, size);
  dc = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  if (dc == 0) goto control;

  /* data PDU */
  rf = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  p  = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  fi = rlc_pdu_decoder_get_bits(&decoder, 2); R(decoder);
  e  = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  sn = rlc_pdu_decoder_get_bits(&decoder, 10); R(decoder);

  /* dicard PDU if rx buffer is full */
  if (entity->rx_size + size > entity->rx_maxsize) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, RX buffer full\n",
          __FILE__, __LINE__, __FUNCTION__);
    goto discard;
  }

  if (!sn_in_recv_window(entity, sn)) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, sn out of window (sn %d vr_r %d)\n",
          __FILE__, __LINE__, __FUNCTION__,
           sn, entity->vr_r);
    goto discard;
  }

  if (rf) {
    lsf = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
    so  = rlc_pdu_decoder_get_bits(&decoder, 15); R(decoder);
  } else {
    lsf = 1;
    so = 0;
  }

  packet_count = 1;

  /* go to start of data */
  indicated_data_size = 0;
  data_decoder = decoder;
  data_e = e;
  while (data_e) {
    data_e = rlc_pdu_decoder_get_bits(&data_decoder, 1); R(data_decoder);
    data_li = rlc_pdu_decoder_get_bits(&data_decoder, 11); R(data_decoder);
    if (data_li == 0) {
      LOG_D(RLC, "%s:%d:%s: warning: discard PDU, li == 0\n",
            __FILE__, __LINE__, __FUNCTION__);
      goto discard;
    }
    indicated_data_size += data_li;
    packet_count++;
  }
  rlc_pdu_decoder_align(&data_decoder);

  data_start = data_decoder.byte;
  data_size = size - data_start;

  if (data_size <= 0) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, wrong data size (sum of LI %d data size %d)\n",
          __FILE__, __LINE__, __FUNCTION__,
           indicated_data_size, data_size);
    goto discard;
  }
  if (indicated_data_size >= data_size) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, bad LIs (sum of LI %d data size %d)\n",
          __FILE__, __LINE__, __FUNCTION__,
           indicated_data_size, data_size);
    goto discard;
  }

  /* discard segment if all the bytes of the segment are already there */
  if (segment_already_received(entity, sn, so, data_size)) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, already received\n",
          __FILE__, __LINE__, __FUNCTION__);
    goto discard;
  }

  char *fi_str[] = {
    "first byte: YES  last byte: YES",
    "first byte: YES  last byte: NO",
    "first byte: NO   last byte: YES",
    "first byte: NO   last byte: NO",
  };

  LOG_D(RLC, "found %d packets, data size %d data start %d [fi %d %s] (sn %d) (p %d)\n",
        packet_count, data_size, data_decoder.byte, fi, fi_str[fi], sn, p);

  /* put in pdu reception list */
  entity->rx_size += size;
  pdu_segment = rlc_rx_new_pdu_segment(sn, so, size, lsf, buffer, data_start);
  entity->rx_list = rlc_rx_pdu_segment_list_add(sn_compare_rx, entity,
                                                entity->rx_list, pdu_segment);

  /* do reception actions (36.322 5.1.3.2.3) */
  rlc_am_reception_actions(entity, pdu_segment);

  if (p) {
    /* 36.322 5.2.3 says status triggering should be delayed
     * until x < VR(MS) or x >= VR(MR). This is not clear (what
     * is x then? we keep the same?). So let's trigger no matter what.
     */
    int vr_mr = (entity->vr_r + 512) % 1024;
    entity->status_triggered = 1;
    if (!(sn_compare_rx(entity, sn, entity->vr_ms) < 0 ||
          sn_compare_rx(entity, sn, vr_mr) >= 0)) {
      LOG_D(RLC, "%s:%d:%s: warning: STATUS trigger should be delayed, according to specs\n",
            __FILE__, __LINE__, __FUNCTION__);
    }
  }

  return;

control:
  cpt = rlc_pdu_decoder_get_bits(&decoder, 3); R(decoder);
  if (cpt != 0) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, CPT not 0 (%d)\n",
          __FILE__, __LINE__, __FUNCTION__, cpt);
    goto discard;
  }
  ack_sn = rlc_pdu_decoder_get_bits(&decoder, 10); R(decoder);
  e1 = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);

  /* let's try to parse the control PDU once to check consistency */
  control_decoder = decoder;
  control_e1 = e1;
  while (control_e1) {
    rlc_pdu_decoder_get_bits(&control_decoder, 10); R(control_decoder); /* NACK_SN */
    control_e1 = rlc_pdu_decoder_get_bits(&control_decoder, 1); R(control_decoder);
    control_e2 = rlc_pdu_decoder_get_bits(&control_decoder, 1); R(control_decoder);
    if (control_e2) {
      rlc_pdu_decoder_get_bits(&control_decoder, 15); R(control_decoder); /* SOstart */
      rlc_pdu_decoder_get_bits(&control_decoder, 15); R(control_decoder); /* SOend */
    }
  }

  /* 36.322 5.2.2.2 says to stop t_poll_retransmit if a ACK or NACK is
   * received for the SN 'poll_sn'
   */
  if (sn_compare_tx(entity, entity->poll_sn, ack_sn) < 0)
    entity->t_poll_retransmit_start = 0;

  /* at this point, accept the PDU even if the actual values
   * may be incorrect (eg. if so_start > so_end)
   */
  process_received_ack(entity, ack_sn);

  while (e1) {
    nack_sn = rlc_pdu_decoder_get_bits(&decoder, 10); R(decoder);
    e1 = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
    e2 = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
    if (e2) {
      so_start = rlc_pdu_decoder_get_bits(&decoder, 15); R(decoder);
      so_end = rlc_pdu_decoder_get_bits(&decoder, 15); R(decoder);
      if (so_end < so_start) {
        LOG_W(RLC, "%s:%d:%s: warning, bad so start/end, NACK the whole PDU (sn %d)\n",
              __FILE__, __LINE__, __FUNCTION__, nack_sn);
        so_start = 0;
        so_end = -1;
      }
      /* special value 0x7fff indicates 'all bytes to the end' */
      if (so_end == 0x7fff)
        so_end = -1;
    } else {
      so_start = 0;
      so_end = -1;
    }
    process_received_nack(entity, nack_sn, so_start, so_end);

    /* 36.322 5.2.2.2 says to stop t_poll_retransmit if a ACK or NACK is
     * received for the SN 'poll_sn'
     */
    if (entity->poll_sn == nack_sn)
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

static int pdu_size(rlc_entity_am_t *entity, rlc_tx_pdu_segment_t *pdu)
{
  int header_size;
  int sdu_count;
  int data_size;
  int li_bits;
  rlc_sdu_t *sdu;

  header_size = 2;
  if (pdu->is_segment)
    header_size += 2;

  data_size = pdu->data_size;

  sdu = pdu->start_sdu;

  sdu_count = 1;
  data_size -= sdu->size - pdu->sdu_start_byte;
  sdu = sdu->next;

  while (data_size > 0) {
    sdu_count++;
    data_size -= sdu->size;
    sdu = sdu->next;
  }

  li_bits = 12 * (sdu_count - 1);
  header_size += (li_bits + 7) / 8;

  return header_size + pdu->data_size;
}

static int header_size(int sdu_count)
{
  int bits = 16 + 12 * (sdu_count - 1);
  /* padding if we have to */
  return (bits + 7) / 8;
}

typedef struct {
  int sdu_count;
  int data_size;
  int header_size;
} tx_pdu_size_t;

static tx_pdu_size_t compute_new_pdu_size(rlc_entity_am_t *entity, int maxsize)
{
  tx_pdu_size_t ret;
  int sdu_count;
  int sdu_size;
  int pdu_data_size;
  rlc_sdu_t *sdu;

  int vt_ms = (entity->vt_a + 512) % 1024;

  ret.sdu_count = 0;
  ret.data_size = 0;
  ret.header_size = 0;

  /* sn out of window? nothing to do */
  if (!(sn_compare_tx(entity, entity->vt_s, entity->vt_a) >= 0 &&
        sn_compare_tx(entity, entity->vt_s, vt_ms) < 0))
    return ret;

  /* TX PDU - let's make the biggest PDU we can with the SDUs we have */
  sdu_count = 0;
  pdu_data_size = 0;
  sdu = entity->tx_list;
  while (sdu != NULL) {
    /* include SDU only if it has not been fully included in PDUs already */
    if (sdu->next_byte != sdu->size) {
      int new_header_size = header_size(sdu_count + 1);
      /* if we cannot put new header + at least 1 byte of data then over */
      if (new_header_size + pdu_data_size + 1 > maxsize)
        break;
      sdu_count++;
      /* only include the bytes of this SDU not included in PDUs already */
      sdu_size = sdu->size - sdu->next_byte;
      /* don't feed more than 'maxsize' bytes */
      if (new_header_size + pdu_data_size + sdu_size > maxsize)
        sdu_size = maxsize - new_header_size - pdu_data_size;
      pdu_data_size += sdu_size;
      /* if we put more than 2^11-1 bytes then the LI field cannot be used,
       * so this is the last SDU we can put
       */
      if (sdu_size > 2047)
        break;
    }
    sdu = sdu->next;
  }

  if (sdu_count) {
    ret.sdu_count = sdu_count;
    ret.data_size = pdu_data_size;
    ret.header_size = header_size(sdu_count);
  }

  return ret;
}

/* return number of missing parts of a sn
 * if everything is missing (nothing received), return 0
 */
static int count_nack_missing_parts(rlc_entity_am_t *entity, int sn)
{
  rlc_rx_pdu_segment_t *l = entity->rx_list;
  int last_byte;
  int new_last_byte;
  int count;

  while (l != NULL) {
    if (l->sn == sn)
      break;
    l = l->next;
  }
  /* nothing received, everything is missing, return 0 */
  if (l == NULL)
    return 0;

  last_byte = -1;
  count = 0;
  while (l != NULL && l->sn == sn) {
    if (l->so > last_byte + 1)
      /* missing part detected */
      count++;
    if (l->is_last)
      /* end of PDU reached - no more missing part to add */
      return count;
    new_last_byte = l->so + l->size - l->data_offset - 1;
    if (new_last_byte > last_byte)
      last_byte = new_last_byte;
    l = l->next;
  }
  /* at this point: end of PDU not received, one more missing part */
  count++;

  return count;
}

/* return size of nack reporting for sn, in bits */
static int segment_nack_size(rlc_entity_am_t *entity, int sn)
{
  /* nack + e1 + e2 = 12 bits
   * SOstart + SOend = 30 bits
   */
  int count;

  count = count_nack_missing_parts(entity, sn);

  /* count_nack_missing_parts returns 0 when everything is missing
   * in which case we don't have SOstart/SOend, so only 12 bits
   */
  if (count == 0)
    return 12;

  return count * (12 + 30);
}

static int status_size(rlc_entity_am_t *entity, int maxsize)
{
  /* let's count bits */
  int bits = 15;               /* minimum size is 15 (header+ack_sn+e1) */
  int sn;

  maxsize *= 8;

  if (bits > maxsize) {
    LOG_W(RLC, "%s:%d:%s: warning: cannot generate status PDU, not enough room\n",
          __FILE__, __LINE__, __FUNCTION__);
    return 0;
  }

  /* add size of NACKs */
  sn = entity->vr_r;
  while (sn_compare_rx(entity, sn, entity->vr_ms) < 0) {
    if (!(rlc_am_segment_full(entity, sn))) {
      int nack_size = segment_nack_size(entity, sn);
      /* stop there if not enough room */
      if (bits + nack_size > maxsize)
        break;
      bits += nack_size;
    }
    sn = (sn + 1) % 1024;
  }

  return (bits + 7) / 8;
}

static int generate_status(rlc_entity_am_t *entity, char *buffer, int size)
{
  /* let's count bits */
  int bits = 15;               /* minimum size is 15 (header+ack_sn+e1) */
  int sn;
  rlc_pdu_encoder_t encoder;
  rlc_pdu_encoder_t encoder_ack;
  rlc_pdu_encoder_t previous_e1;
  int ack;

  rlc_pdu_encoder_init(&encoder, buffer, size);

  size *= 8;

  if (bits > size) {
    LOG_W(RLC, "%s:%d:%s: warning: cannot generate status PDU, not enough room\n",
          __FILE__, __LINE__, __FUNCTION__);
    return 0;
  }

  /* header */
  rlc_pdu_encoder_put_bits(&encoder, 0, 1);   /* D/C */
  rlc_pdu_encoder_put_bits(&encoder, 0, 3);   /* CPT */

  /* reserve room for ACK (it will be set after putting the NACKs) */
  encoder_ack = encoder;
  rlc_pdu_encoder_put_bits(&encoder, 0, 10);
  /* put 0 for e1, will be set to 1 later in the code if needed */
  previous_e1 = encoder;
  rlc_pdu_encoder_put_bits(&encoder, 0, 1);

  /* at this point, ACK is VR(R) */
  ack = entity->vr_r;

  sn = entity->vr_r;
  while (sn_compare_rx(entity, sn, entity->vr_ms) < 0) {
    if (!(rlc_am_segment_full(entity, sn))) {
      rlc_rx_pdu_segment_t *l;
      int i;
      int count     = count_nack_missing_parts(entity, sn);
      int nack_bits = count == 0 ? 12 : count * (12 + 30);
      int last_byte;
      int new_last_byte;
      int so_start;
      int so_end;
      /* if not enough room, stop putting NACKs */
      if (bits + nack_bits > size)
        break;
      /* set previous e1 to 1 */
      rlc_pdu_encoder_put_bits(&previous_e1, 1, 1);

      if (count == 0) {
        /* simply NACK, no SOstart/SOend */
        rlc_pdu_encoder_put_bits(&encoder, sn, 10);                 /* nack */
        previous_e1 = encoder;
        rlc_pdu_encoder_put_bits(&encoder, 0, 2);                 /* e1, e2 */
      } else {
        /* count is not 0, so we have 'count' NACK+e1+e2+SOstart+SOend */
        l = entity->rx_list;
        last_byte = -1;
        while (l->sn != sn) l = l->next;
        for (i = 0; i < count; i++) {
          rlc_pdu_encoder_put_bits(&encoder, sn, 10);               /* nack */
          previous_e1 = encoder;
          /* all NACKs but the last have a following NACK, set e1 for them */
          rlc_pdu_encoder_put_bits(&encoder, i != count - 1, 1);      /* e1 */
          rlc_pdu_encoder_put_bits(&encoder, 1, 1);                   /* e2 */
          /* look for the next segment with missing data before it */
          while (l != NULL && l->sn == sn && !(l->so > last_byte + 1)) {
            new_last_byte = l->so + l->size - l->data_offset - 1;
            if (new_last_byte > last_byte)
              last_byte = new_last_byte;
            l = l->next;
          }
          so_start = last_byte + 1;
          if (l == NULL)
            so_end = 0x7fff;
          else
            so_end = l->so - 1;
          rlc_pdu_encoder_put_bits(&encoder, so_start, 15);
          rlc_pdu_encoder_put_bits(&encoder, so_end, 15);
          if (l != NULL)
            last_byte = l->so + l->size - l->data_offset - 1;
        }
      }

      bits += nack_bits;
    } else {
      /* this sn is full and we put all NACKs before it, use it for ACK */
      ack = (sn + 1) % 1024;
    }
    sn = (sn + 1) % 1024;
  }

  rlc_pdu_encoder_align(&encoder);

  /* let's put the ACK */
  rlc_pdu_encoder_put_bits(&encoder_ack, ack, 10);

  /* reset the trigger */
  entity->status_triggered = 0;

  /* start t_status_prohibit */
  entity->t_status_prohibit_start = entity->t_current;

  return encoder.byte;
}

int transmission_buffer_empty(rlc_entity_am_t *entity)
{
  rlc_sdu_t *sdu;

  /* is transmission buffer empty? */
  sdu = entity->tx_list;
  while (sdu != NULL) {
    if (sdu->next_byte != sdu->size)
      return 0;
    sdu = sdu->next;
  }
  return 1;
}

int check_poll_after_pdu_assembly(rlc_entity_am_t *entity)
{
  int retransmission_buffer_empty;
  int window_stalling;
  int vt_ms;

  /* is retransmission buffer empty? */
  if (entity->retransmit_list == NULL)
    retransmission_buffer_empty = 1;
  else
    retransmission_buffer_empty = 0;

  /* is window stalling? */
  vt_ms = (entity->vt_a + 512) % 1024;
  if (!(sn_compare_tx(entity, entity->vt_s, entity->vt_a) >= 0 &&
        sn_compare_tx(entity, entity->vt_s, vt_ms) < 0))
    window_stalling = 1;
  else
    window_stalling = 0;

  return (transmission_buffer_empty(entity) && retransmission_buffer_empty) ||
         window_stalling;
}

void include_poll(rlc_entity_am_t *entity, char *buffer)
{
  /* set the P bit to 1 */
  buffer[0] |= 0x20;

  entity->pdu_without_poll = 0;
  entity->byte_without_poll = 0;

  /* set POLL_SN to VT(S) - 1 */
  entity->poll_sn = (entity->vt_s + 1023) % 1024;

  /* start t_poll_retransmit */
  entity->t_poll_retransmit_start = entity->t_current;
}

static int serialize_pdu(rlc_entity_am_t *entity, char *buffer, int bufsize,
                         rlc_tx_pdu_segment_t *pdu, int p)
{
  int                  first_sdu_full;
  int                  last_sdu_full;
  int                  sdu_next_byte;
  rlc_sdu_t            *sdu;
  int                  i;
  int                  cursize;
  rlc_pdu_encoder_t    encoder;
  int                  fi;
  int                  e;
  int                  li;
  char                 *out;
  int                  outpos;
  int                  sdu_count;
  int                  header_size;
  int                  sdu_start_byte;

  first_sdu_full = pdu->sdu_start_byte == 0;

  /* is last SDU full? (and also compute sdu_count) */
  last_sdu_full = 1;
  sdu = pdu->start_sdu;
  sdu_next_byte = pdu->sdu_start_byte;
  cursize = 0;
  sdu_count = 0;
  while (cursize != pdu->data_size) {
    int sdu_size = sdu->size - sdu_next_byte;
    sdu_count++;
    if (cursize + sdu_size > pdu->data_size) {
      last_sdu_full = 0;
      break;
    }
    cursize += sdu_size;
    sdu = sdu->next;
    sdu_next_byte = 0;
  }

  /* generate header */
  rlc_pdu_encoder_init(&encoder, buffer, bufsize);

  rlc_pdu_encoder_put_bits(&encoder, 1, 1);                /* D/C: 1 = data */
  rlc_pdu_encoder_put_bits(&encoder, pdu->is_segment, 1);             /* RF */
  rlc_pdu_encoder_put_bits(&encoder, 0, 1);        /* P: reserve, set later */

  fi = 0;
  if (!first_sdu_full)
    fi |= 0x02;
  if (!last_sdu_full)
    fi |= 0x01;
  rlc_pdu_encoder_put_bits(&encoder, fi, 2);                          /* FI */

  /* to understand the logic for Es and LIs:
   * If we have:
   *   1 SDU:   E=0
   *
   *   2 SDUs:  E=1
   *     then:  E=0 LI(sdu[0])
   *
   *   3 SDUs:  E=1
   *     then:  E=1 LI(sdu[0])
   *     then:  E=0 LI(sdu[1])
   *
   *   4 SDUs:  E=1
   *     then:  E=1 LI(sdu[0])
   *     then:  E=1 LI(sdu[1])
   *     then:  E=0 LI(sdu[2])
   */
  if (sdu_count >= 2)
    e = 1;
  else
    e = 0;
  rlc_pdu_encoder_put_bits(&encoder, e, 1);                            /* E */

  rlc_pdu_encoder_put_bits(&encoder, pdu->sn, 10);                    /* SN */

  if (pdu->is_segment) {
    rlc_pdu_encoder_put_bits(&encoder, pdu->is_last, 1);             /* LSF */
    rlc_pdu_encoder_put_bits(&encoder, pdu->so, 15);                  /* SO */
  }

  /* put LIs */
  sdu = pdu->start_sdu;
  /* first SDU */
  li = sdu->size - pdu->sdu_start_byte;
  /* put E+LI only if at least 2 SDUs */
  if (sdu_count >= 2) {
    /* E is 1 if at least 3 SDUs */
    if (sdu_count >= 3)
      e = 1;
    else
      e = 0;
    rlc_pdu_encoder_put_bits(&encoder, e, 1);                          /* E */
    rlc_pdu_encoder_put_bits(&encoder, li, 11);                       /* LI */
  }
  /* next SDUs, but not the last (no LI for the last) */
  sdu = sdu->next;
  for (i = 2; i < sdu_count; i++, sdu = sdu->next) {
    if (i != sdu_count - 1)
      e = 1;
    else
      e = 0;
    li = sdu->size;
    rlc_pdu_encoder_put_bits(&encoder, e, 1);                          /* E */
    rlc_pdu_encoder_put_bits(&encoder, li, 11);                       /* LI */
  }

  rlc_pdu_encoder_align(&encoder);

  header_size = encoder.byte;

  /* generate data */
  out = buffer + header_size;
  sdu = pdu->start_sdu;
  sdu_start_byte = pdu->sdu_start_byte;
  outpos = 0;
  for (i = 0; i < sdu_count; i++, sdu = sdu->next) {
    li = sdu->size - sdu_start_byte;
    if (outpos + li >= pdu->data_size)
      li = pdu->data_size - outpos;
    memcpy(out+outpos, sdu->data + sdu_start_byte, li);
    outpos += li;
    sdu_start_byte = 0;
  }

  if (p)
    include_poll(entity, buffer);

  return header_size + pdu->data_size;
}

static int generate_tx_pdu(rlc_entity_am_t *entity, char *buffer, int bufsize)
{
  int                  vt_ms;
  tx_pdu_size_t        pdu_size;
  rlc_sdu_t            *sdu;
  int                  i;
  int                  cursize;
  int                  p;
  rlc_tx_pdu_segment_t *pdu;

  /* sn out of window? do nothing */
  vt_ms = (entity->vt_a + 512) % 1024;
  if (!(sn_compare_tx(entity, entity->vt_s, entity->vt_a) >= 0 &&
        sn_compare_tx(entity, entity->vt_s, vt_ms) < 0))
    return 0;

  pdu_size = compute_new_pdu_size(entity, bufsize);
  if (pdu_size.sdu_count == 0)
    return 0;

  pdu = rlc_tx_new_pdu();

  pdu->sn = entity->vt_s;
  entity->vt_s = (entity->vt_s + 1) % 1024;

  /* go to first SDU (skip those already fully processed) */
  sdu = entity->tx_list;
  while (sdu->next_byte == sdu->size)
    sdu = sdu->next;

  pdu->start_sdu = sdu;

  pdu->sdu_start_byte = sdu->next_byte;

  pdu->so = 0;
  pdu->is_segment = 0;
  pdu->is_last = 1;
  /* to conform to specs' logic, put -1 (specs say "for 1st retransmission
   * put 0 otherwise increase", let's put -1 and always increase when the
   * segment goes to retransmit list)
   */
  pdu->retx_count = -1;

  /* reserve SDU bytes */
  cursize = 0;
  for (i = 0; i < pdu_size.sdu_count; i++, sdu = sdu->next) {
    int sdu_size = sdu->size - sdu->next_byte;
    if (cursize + sdu_size > pdu_size.data_size)
      sdu_size = pdu_size.data_size - cursize;
    sdu->next_byte += sdu_size;
    cursize += sdu_size;
  }

  pdu->data_size = cursize;

  /* put PDU at the end of the wait list */
  entity->wait_list = rlc_tx_pdu_list_append(entity->wait_list, pdu);

  /* polling actions for a new PDU */
  entity->pdu_without_poll++;
  entity->byte_without_poll += pdu_size.data_size;
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

  return serialize_pdu(entity, buffer, bufsize, pdu, p);
}

static void resegment(rlc_tx_pdu_segment_t *pdu, int size)
{
  rlc_tx_pdu_segment_t *new_pdu;
  rlc_sdu_t *sdu;
  int sdu_count;
  int pdu_header_size;
  int pdu_data_size;
  int sdu_pos;
  int sdu_bytes_to_take;

  /* PDU segment too big, cut in two parts so that first part fits into
   * size bytes (including header)
   */
  sdu = pdu->start_sdu;
  pdu_data_size = 0;
  sdu_pos = pdu->sdu_start_byte;
  sdu_count = 0;
  while (1) {
    /* can we put a new header and at least one byte of data? */
    /* header has 2 more bytes for SO */
    pdu_header_size = 2 + header_size(sdu_count + 1);
    if (pdu_header_size + pdu_data_size + 1 > size) {
      /* no we can't, stop here */
      break;
    }
    /* yes we can, go ahead */
    sdu_count++;
    sdu_bytes_to_take = sdu->size - sdu_pos;
    if (pdu_header_size + pdu_data_size + sdu_bytes_to_take > size) {
      sdu_bytes_to_take = size - (pdu_header_size + pdu_data_size);
    }
    sdu_pos += sdu_bytes_to_take;
    if (sdu_pos == sdu->size) {
      sdu = sdu->next;
      sdu_pos = 0;
    }
    pdu_data_size += sdu_bytes_to_take;
  }

  new_pdu = rlc_tx_new_pdu();
  pdu->is_segment = 1;
  *new_pdu = *pdu;

  new_pdu->so = pdu->so + pdu_data_size;
  new_pdu->data_size = pdu->data_size - pdu_data_size;
  new_pdu->start_sdu = sdu;
  new_pdu->sdu_start_byte = sdu_pos;

  pdu->is_last = 0;
  pdu->data_size = pdu_data_size;
  pdu->next = new_pdu;
}

static int generate_retx_pdu(rlc_entity_am_t *entity, char *buffer, int size)
{
  rlc_tx_pdu_segment_t *pdu;
  int orig_size;
  int p;

  pdu = entity->retransmit_list;
  orig_size = pdu_size(entity, pdu);

  if (orig_size > size) {
    /* we can't resegment if size is less than 5
     * (4 bytes for header, 1 byte for data)
     */
    if (size < 5)
      return 0;
    resegment(pdu, size);
  }

  /* remove from retransmit list and put in wait list */
  entity->retransmit_list = pdu->next;
  entity->wait_list = rlc_tx_pdu_list_add(sn_compare_tx, entity,
                                          entity->wait_list, pdu);

  p = check_poll_after_pdu_assembly(entity);

  if (entity->force_poll) {
    p = 1;
    entity->force_poll = 0;
  }

  return serialize_pdu(entity, buffer, orig_size, pdu, p);
}

static int status_to_report(rlc_entity_am_t *entity)
{
  return entity->status_triggered &&
         (entity->t_status_prohibit_start == 0 ||
          entity->t_current - entity->t_status_prohibit_start >
              entity->t_status_prohibit);
}

static int retx_pdu_size(rlc_entity_am_t *entity, int maxsize)
{
  int size;

  if (entity->retransmit_list == NULL)
    return 0;

  size = pdu_size(entity, entity->retransmit_list);
  if (size <= maxsize)
    return size;

  /* we can segment head of retransmist list if maxsize is large enough
   * to hold a PDU segment with at least 1 data byte (so 5 bytes: 4 bytes
   * header + 1 byte data)
   */
  if (maxsize < 5)
    return 0;

  /* a later segmentation of the head of retransmit list will generate a pdu
   * of maximum size 'maxsize' (can be less)
   */
  return maxsize;
}

rlc_entity_buffer_status_t rlc_entity_am_buffer_status(
    rlc_entity_t *_entity, int maxsize)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  rlc_entity_buffer_status_t ret;
  tx_pdu_size_t tx_size;

  /* status PDU, if we have to */
  if (status_to_report(entity))
    ret.status_size = status_size(entity, maxsize);
  else
    ret.status_size = 0;

  /* TX PDU */
  /* todo: if an SDU has size >2047 in the tx list then processing
   * stops and computed size will not be accurate. Change the computation
   * to be more accurate (if needed).
   */
  tx_size = compute_new_pdu_size(entity, maxsize);
  ret.tx_size = tx_size.data_size + tx_size.header_size;

  /* reTX PDU */
  /* todo: report size of all available data, not just first PDU */
  ret.retx_size = retx_pdu_size(entity, maxsize);

  return ret;
}

int rlc_entity_am_generate_pdu(rlc_entity_t *_entity, char *buffer, int size)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
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

void rlc_entity_am_recv_sdu(rlc_entity_t *_entity, char *buffer, int size,
                            int sdu_id)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  rlc_sdu_t *sdu;

  if (size > SDU_MAX) {
    LOG_E(RLC, "%s:%d:%s: fatal: SDU size too big (%d bytes)\n",
          __FILE__, __LINE__, __FUNCTION__, size);
    exit(1);
  }

  if (entity->tx_size + size > entity->tx_maxsize) {
    LOG_D(RLC, "%s:%d:%s: warning: SDU rejected, SDU buffer full\n",
          __FILE__, __LINE__, __FUNCTION__);
    return;
  }

  entity->tx_size += size;

  sdu = rlc_new_sdu(buffer, size, sdu_id);
  rlc_sdu_list_add(&entity->tx_list, &entity->tx_end, sdu);
}

/*************************************************************************/
/* time/timers                                                           */
/*************************************************************************/

static void check_t_poll_retransmit(rlc_entity_am_t *entity)
{
  rlc_tx_pdu_segment_t head;
  rlc_tx_pdu_segment_t *cur;
  rlc_tx_pdu_segment_t *prev;
  int sn;

  /* 36.322 5.2.2.3 */
  /* did t_poll_retransmit expire? */
  if (entity->t_poll_retransmit_start == 0 ||
      entity->t_current <= entity->t_poll_retransmit_start +
                               entity->t_poll_retransmit)
    return;

  /* stop timer */
  entity->t_poll_retransmit_start = 0;

  /* 36.322 5.2.2.3 says:
   *
   *     - include a poll in a RLC data PDU as described in section 5.2.2.1
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

  /* do we meet conditions of 36.322 5.2.2.3? */
  if (!check_poll_after_pdu_assembly(entity))
    return;

  /* search wait list for PDU with SN = VT(S)-1 */
  sn = (entity->vt_s + 1023) % 1024;

  head.next = entity->wait_list;
  cur = entity->wait_list;
  prev = &head;

  while (cur != NULL) {
    if (cur->sn == sn)
      break;
    prev = cur;
    cur = cur->next;
  }

  /* PDU with SN = VT(S)-1 not found?, take the head of wait list */
  if (cur == NULL) {
    cur = entity->wait_list;
    prev = &head;
    sn = cur->sn;
  }

  /* 36.322 says "PDU", not "PDU segment", so let's retransmit all
   * PDU segments with this SN
   */
  while (cur != NULL && cur->sn == sn) {
    prev->next = cur->next;
    entity->wait_list = head.next;
    /* put in retransmit list */
    consider_retransmission(entity, cur);
    cur = prev->next;
  }
}

static void check_t_reordering(rlc_entity_am_t *entity)
{
  int sn;

  /* is t_reordering running and if yes has it expired? */
  if (entity->t_reordering_start == 0 ||
      entity->t_current <= entity->t_reordering_start + entity->t_reordering)
    return;

  /* stop timer */
  entity->t_reordering_start = 0;

  LOG_D(RLC, "%s:%d:%s: t_reordering expired\n", __FILE__, __LINE__, __FUNCTION__);

  /* update VR(MS) to first SN >= VR(X) for which not all PDU segments
   * have been received
   */
  sn = entity->vr_x;
  while (rlc_am_segment_full(entity, sn))
    sn = (sn + 1) % 1024;
  entity->vr_ms = sn;

  if (sn_compare_rx(entity, entity->vr_h, entity->vr_ms) > 0) {
    entity->t_reordering_start = entity->t_current;
    entity->vr_x = entity->vr_h;
  }

  /* trigger STATUS report */
  entity->status_triggered = 1;
}

void rlc_entity_am_set_time(rlc_entity_t *_entity, uint64_t now)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;

  entity->t_current = now;

  check_t_poll_retransmit(entity);

  check_t_reordering(entity);

  /* t_status_prohibit is handled by generate_status */
}

/*************************************************************************/
/* discard/re-establishment/delete                                       */
/*************************************************************************/

void rlc_entity_am_discard_sdu(rlc_entity_t *_entity, int sdu_id)
{
  /* implements 36.322 5.3 */
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  rlc_sdu_t head;
  rlc_sdu_t *cur;
  rlc_sdu_t *prev;

  head.next = entity->tx_list;
  cur = entity->tx_list;
  prev = &head;

  while (cur != NULL && cur->upper_layer_id != sdu_id) {
    prev = cur;
    cur = cur->next;
  }

  /* if sdu_id not found or some bytes have already been 'PDU-ized'
   * then do nothing
   */
  if (cur == NULL || cur->next_byte != 0)
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

  rlc_free_sdu(cur);
}

static void free_pdu_segment_list(rlc_tx_pdu_segment_t *l)
{
  rlc_tx_pdu_segment_t *cur;

  while (l != NULL) {
    cur = l;
    l = l->next;
    rlc_tx_free_pdu(cur);
  }
}

static void clear_entity(rlc_entity_am_t *entity)
{
  rlc_rx_pdu_segment_t *cur_rx;
  rlc_sdu_t            *cur_tx;

  entity->vr_r = 0;
  entity->vr_x = 0;
  entity->vr_ms = 0;
  entity->vr_h = 0;

  entity->status_triggered = 0;

  entity->vt_a = 0;
  entity->vt_s = 0;
  entity->poll_sn = 0;
  entity->pdu_without_poll = 0;
  entity->byte_without_poll = 0;
  entity->force_poll = 0;

  entity->t_current = 0;

  entity->t_reordering_start = 0;
  entity->t_status_prohibit_start = 0;
  entity->t_poll_retransmit_start = 0;

  cur_rx = entity->rx_list;
  while (cur_rx != NULL) {
    rlc_rx_pdu_segment_t *p = cur_rx;
    cur_rx = cur_rx->next;
    rlc_rx_free_pdu_segment(p);
  }
  entity->rx_list = NULL;
  entity->rx_size = 0;

  memset(&entity->reassemble, 0, sizeof(rlc_am_reassemble_t));

  cur_tx = entity->tx_list;
  while (cur_tx != NULL) {
    rlc_sdu_t *p = cur_tx;
    cur_tx = cur_tx->next;
    rlc_free_sdu(p);
  }
  entity->tx_list = NULL;
  entity->tx_end = NULL;
  entity->tx_size = 0;

  free_pdu_segment_list(entity->wait_list);
  free_pdu_segment_list(entity->retransmit_list);
  free_pdu_segment_list(entity->ack_list);
  entity->wait_list = NULL;
  entity->retransmit_list = NULL;
  entity->ack_list = NULL;
}

void rlc_entity_am_reestablishment(rlc_entity_t *_entity)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;

  /* 36.322 5.4 says to deliver SDUs if possible.
   * Let's not do that, it makes the code simpler.
   * TODO: change this behavior if wanted/needed.
   */

  clear_entity(entity);
}

void rlc_entity_am_delete(rlc_entity_t *_entity)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  clear_entity(entity);
  free(entity);
}
