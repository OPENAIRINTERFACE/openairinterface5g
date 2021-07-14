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

#include "nr_rlc_entity_um.h"

#include <stdlib.h>
#include <string.h>

#include "nr_rlc_pdu.h"

#include "LOG/log.h"

/*************************************************************************/
/* PDU RX functions                                                      */
/*************************************************************************/

static int modulus_rx(nr_rlc_entity_um_t *entity, int a)
{
  /* as per 38.322 7.1, modulus base is rx_next_highest - window_size */
  int r = a - (entity->rx_next_highest - entity->window_size);
  if (r < 0) r += entity->sn_modulus;
  return r % entity->sn_modulus;
}

static int sn_compare_rx(void *_entity, int a, int b)
{
  nr_rlc_entity_um_t *entity = _entity;
  return modulus_rx(entity, a) - modulus_rx(entity, b);
}

/* checks that all the bytes of the SDU sn have been received (but SDU
 * has not been already processed)
 */
static int sdu_full(nr_rlc_entity_um_t *entity, int sn)
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
static int sdu_delivered(nr_rlc_entity_um_t *entity, int sn)
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
static int sdu_has_missing_bytes(nr_rlc_entity_um_t *entity, int sn)
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

static void reassemble_and_deliver(nr_rlc_entity_um_t *entity, int sn)
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
    pdu->size = 0;
    pdu = pdu->next;
  }

  if (bad_sdu)
    return;

  /* deliver */
  entity->common.deliver_sdu(entity->common.deliver_sdu_data,
                             (nr_rlc_entity_t *)entity,
                             sdu, so);
}

static void reception_actions(nr_rlc_entity_um_t *entity, nr_rlc_pdu_t *pdu)
{
  int x = pdu->sn;

  if (sdu_full(entity, x)) {
    /* SDU full */
    reassemble_and_deliver(entity, x);

    if (x == entity->rx_next_reassembly) {
      int rx_next_reassembly = entity->rx_next_reassembly;
      while (sdu_delivered(entity, rx_next_reassembly))
        rx_next_reassembly = (rx_next_reassembly + 1) % entity->sn_modulus;
      entity->rx_next_reassembly = rx_next_reassembly;
    }
  } else {
    /* SDU not full */
    /* test if x is not in reassembly window, that is x >= rx_next_highest */
    if (sn_compare_rx(entity, x, entity->rx_next_highest) >= 0) {
      entity->rx_next_highest = (x + 1) % entity->sn_modulus;

      /* discard PDUs not in reassembly window */
      while (entity->rx_list != NULL &&
             sn_compare_rx(entity, entity->rx_list->sn,
                           entity->rx_next_highest) >= 0) {
        nr_rlc_pdu_t *p = entity->rx_list;
        entity->rx_size -= p->size;
        entity->rx_list = p->next;
        nr_rlc_free_pdu(p);
      }

      /* if rx_next_reassembly not in reassembly window */
      if (sn_compare_rx(entity, entity->rx_next_reassembly,
                        entity->rx_next_highest) >= 0) {
        int rx_next_reassembly;
        /* set rx_next_reassembly to first SN >= rx_next_highest - window_size
         * not delivered yet
         */
        rx_next_reassembly = (entity->rx_next_highest - entity->window_size
                                 + entity->sn_modulus) % entity->sn_modulus;
        while (sdu_delivered(entity, rx_next_reassembly))
          rx_next_reassembly = (rx_next_reassembly + 1) % entity->sn_modulus;
        entity->rx_next_reassembly = rx_next_reassembly;
      }
    }
  }

  if (entity->t_reassembly_start) {
    if (/* rx_timer_trigger <= rx_next_reassembly */
        sn_compare_rx(entity, entity->rx_timer_trigger,
                      entity->rx_next_reassembly) <= 0 ||
        /* or rx_timer_trigger outside of reassembly window and not equal
         * to rx_next_highest, that is is > rx_next_highest
         */
        sn_compare_rx(entity, entity->rx_timer_trigger,
                      entity->rx_next_highest) > 0 ||
        /* or rx_next_highest == rx_next_reassembly + 1 and no missing byte
         * for rx_next_reassembly
         */
       (entity->rx_next_highest == (entity->rx_next_reassembly + 1) %
           entity->sn_modulus &&
        !sdu_has_missing_bytes(entity, entity->rx_next_reassembly)))
      entity->t_reassembly_start = 0;
  }

  if (entity->t_reassembly_start == 0) {
    if (sn_compare_rx(entity, entity->rx_next_highest,
                      (entity->rx_next_reassembly + 1)
                          % entity->sn_modulus) > 0 ||
        (entity->rx_next_highest == (entity->rx_next_reassembly + 1)
                                        % entity->sn_modulus &&
         sdu_has_missing_bytes(entity, entity->rx_next_reassembly))) {
      entity->t_reassembly_start = entity->t_current;
      entity->rx_timer_trigger = entity->rx_next_highest;
    }
  }
}

void nr_rlc_entity_um_recv_pdu(nr_rlc_entity_t *_entity,
                               char *buffer, int size)
{
#define R(d) do { if (nr_rlc_pdu_decoder_in_error(&d)) goto err; } while (0)
  nr_rlc_entity_um_t *entity = (nr_rlc_entity_um_t *)_entity;
  nr_rlc_pdu_decoder_t decoder;
  nr_rlc_pdu_t *pdu;
  int si;
  int sn;
  int so = 0;
  int data_size;
  int is_first;
  int is_last;

  nr_rlc_pdu_decoder_init(&decoder, buffer, size);

  si = nr_rlc_pdu_decoder_get_bits(&decoder, 2); R(decoder);

  is_first = (si & 0x2) == 0;
  is_last = (si & 0x1) == 0;

  /* if full, deliver SDU */
  if (is_first && is_last) {
    if (size < 2) {
      LOG_E(RLC, "%s:%d:%s: warning: discard PDU, no data\n",
            __FILE__, __LINE__, __FUNCTION__);
      goto discard;
    }
    /* deliver */
    entity->common.deliver_sdu(entity->common.deliver_sdu_data,
                               (nr_rlc_entity_t *)entity,
                               buffer + 1, size - 1);
    return;
  }

  if (entity->sn_field_length == 12) {
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

  /* discard PDU if sn < rx_next_reassembly */
  if (sn_compare_rx(entity, sn, entity->rx_next_reassembly) < 0) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, SN (%d) < rx_next_reassembly (%d)\n",
          __FILE__, __LINE__, __FUNCTION__,
          sn, entity->rx_next_reassembly);
    goto discard;
  }

  /* put in pdu reception list */
  entity->rx_size += data_size;
  pdu = nr_rlc_new_pdu(sn, so, is_first, is_last,
                       buffer + size - data_size, data_size);
  entity->rx_list = nr_rlc_pdu_list_add(sn_compare_rx, entity,
                                        entity->rx_list, pdu);

  /* do reception actions (38.322 5.2.2.2.3) */
  reception_actions(entity, pdu);

  return;

err:
  LOG_W(RLC, "%s:%d:%s: error decoding PDU, discarding\n", __FILE__, __LINE__, __FUNCTION__);
  goto discard;

discard:
  return;

#undef R
}

/*************************************************************************/
/* TX functions                                                          */
/*************************************************************************/

static int serialize_sdu(nr_rlc_entity_um_t *entity,
                         nr_rlc_sdu_segment_t *sdu, char *buffer, int bufsize)
{
  nr_rlc_pdu_encoder_t encoder;

  /* generate header */
  nr_rlc_pdu_encoder_init(&encoder, buffer, bufsize);

  nr_rlc_pdu_encoder_put_bits(&encoder, 1-sdu->is_first,1);/* 1st bit of SI */
  nr_rlc_pdu_encoder_put_bits(&encoder, 1-sdu->is_last,1); /* 2nd bit of SI */

  /* SN, if required */
  if (sdu->is_first == 1 && sdu->is_last == 1) {
    nr_rlc_pdu_encoder_put_bits(&encoder, 0, 6);                       /* R */
  } else {
    if (entity->sn_field_length == 12)
      nr_rlc_pdu_encoder_put_bits(&encoder, 0, 2);                     /* R */
    nr_rlc_pdu_encoder_put_bits(&encoder, sdu->sdu->sn,
                                entity->sn_field_length);             /* SN */
  }

  if (!sdu->is_first)
    nr_rlc_pdu_encoder_put_bits(&encoder, sdu->so, 16);               /* SO */

  /* data */
  memcpy(buffer + encoder.byte, sdu->sdu->data + sdu->so, sdu->size);

  return encoder.byte + sdu->size;
}

/* for a given SDU/SDU segment, computes the corresponding PDU header size */
static int compute_pdu_header_size(nr_rlc_entity_um_t *entity,
                                   nr_rlc_sdu_segment_t *sdu)
{
  int header_size = 1;

  /* if SN to be included then one more byte if SN field length is 12 */
  if (!(sdu->is_first && sdu->is_last) && entity->sn_field_length == 12)
    header_size++;
  /* two more bytes for SO if SDU segment is not the first */
  if (!sdu->is_first) header_size += 2;
  return header_size;
}

/* resize SDU/SDU segment for the corresponding PDU to fit into 'pdu_size'
 * bytes
 * - modifies SDU/SDU segment to become an SDU segment
 * - returns a new SDU segment covering the remaining data bytes
 * returns NULL if pdu_size is too small to contain the new segment
 */
static nr_rlc_sdu_segment_t *resegment(nr_rlc_sdu_segment_t *sdu,
                                       nr_rlc_entity_um_t *entity,
                                       int pdu_size)
{
  nr_rlc_sdu_segment_t *next;
  int pdu_header_size;
  int over_size;
  int old_is_last;

  sdu->sdu->ref_count++;

  /* clear is_last to compute header size */
  old_is_last = sdu->is_last;
  sdu->is_last = 0;
  pdu_header_size = compute_pdu_header_size(entity, sdu);
  sdu->is_last = old_is_last;

  /* if no room for at least 1 data byte, do nothing */
  if (pdu_header_size + 1 > pdu_size)
    return NULL;

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

static int generate_tx_pdu(nr_rlc_entity_um_t *entity, char *buffer, int size)
{
  nr_rlc_sdu_segment_t *sdu;
  int pdu_header_size;
  int pdu_size;
  int ret;

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

  /* assign SN to SDU */
  sdu->sdu->sn = entity->tx_next;

  /* segment if necessary */
  pdu_size = pdu_header_size + sdu->size;
  if (pdu_size > size) {
    nr_rlc_sdu_segment_t *next_sdu;
    next_sdu = resegment(sdu, entity, size);
    if (next_sdu == NULL)
      return 0;
    /* put the second SDU back at the head of the TX list */
    next_sdu->next = entity->tx_list;
    entity->tx_list = next_sdu;
    if (entity->tx_end == NULL)
      entity->tx_end = entity->tx_list;
  }

  /* update tx_next if the SDU is an SDU segment and is the last */
  if (!sdu->is_first && sdu->is_last)
    entity->tx_next = (entity->tx_next + 1) % entity->sn_modulus;

  ret = serialize_sdu(entity, sdu, buffer, size);

  entity->tx_size -= sdu->size;
  nr_rlc_free_sdu_segment(sdu);

  return ret;
}

/* Pretend to serialize all the SDUs in a list and return the size
 * of all the PDUs it would produce, limited to 'maxsize'.
 * Used for buffer status reporting.
 */
static int tx_list_size(nr_rlc_entity_um_t *entity,
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

nr_rlc_entity_buffer_status_t nr_rlc_entity_um_buffer_status(
    nr_rlc_entity_t *_entity, int maxsize)
{
  nr_rlc_entity_um_t *entity = (nr_rlc_entity_um_t *)_entity;
  nr_rlc_entity_buffer_status_t ret;

  ret.status_size = 0;
  ret.tx_size = tx_list_size(entity, entity->tx_list, maxsize);
  ret.retx_size = 0;

  return ret;
}

int nr_rlc_entity_um_generate_pdu(nr_rlc_entity_t *_entity,
                                  char *buffer, int size)
{
  nr_rlc_entity_um_t *entity = (nr_rlc_entity_um_t *)_entity;

  return generate_tx_pdu(entity, buffer, size);
}

/*************************************************************************/
/* SDU RX functions                                                      */
/*************************************************************************/

void nr_rlc_entity_um_recv_sdu(nr_rlc_entity_t *_entity,
                               char *buffer, int size,
                               int sdu_id)
{
  nr_rlc_entity_um_t *entity = (nr_rlc_entity_um_t *)_entity;
  nr_rlc_sdu_segment_t *sdu;

  if (size > NR_SDU_MAX) {
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

  sdu = nr_rlc_new_sdu(buffer, size, sdu_id);

  nr_rlc_sdu_segment_list_append(&entity->tx_list, &entity->tx_end, sdu);
}

/*************************************************************************/
/* time/timers                                                           */
/*************************************************************************/

static void check_t_reassembly(nr_rlc_entity_um_t *entity)
{
  nr_rlc_pdu_t *cur;

  /* is t_reassembly running and if yes has it expired? */
  if (entity->t_reassembly_start == 0 ||
      entity->t_current <= entity->t_reassembly_start + entity->t_reassembly)
    return;

  /* stop timer */
  entity->t_reassembly_start = 0;

  LOG_D(RLC, "%s:%d:%s: t_reassembly expired\n",
        __FILE__, __LINE__, __FUNCTION__);

  /* update rx_next_reassembly to first SN >= rx_timer_trigger not reassembled
   * (ie. not delivered yet)
   */
  entity->rx_next_reassembly = entity->rx_timer_trigger;
  while (sdu_delivered(entity, entity->rx_next_reassembly))
    entity->rx_next_reassembly = (entity->rx_next_reassembly + 1)
                                     % entity->sn_modulus;

  /* discard all segments < entity->rx_next_reassembly */
  cur = entity->rx_list;
  while (cur != NULL &&
         sn_compare_rx(entity, cur->sn, entity->rx_next_reassembly) < 0) {
    nr_rlc_pdu_t *p = cur;
    cur = cur->next;
    entity->rx_list = cur;
    nr_rlc_free_pdu(p);
  }

  if (sn_compare_rx(entity, entity->rx_next_highest,
                    (entity->rx_next_reassembly + 1)
                        % entity->sn_modulus) > 0 ||
      (entity->rx_next_highest == entity->rx_next_reassembly + 1 &&
       sdu_has_missing_bytes(entity, entity->rx_next_reassembly))) {
    entity->t_reassembly_start = entity->t_current;
    entity->rx_timer_trigger = entity->rx_next_highest;
  }
}

void nr_rlc_entity_um_set_time(nr_rlc_entity_t *_entity, uint64_t now)
{
  nr_rlc_entity_um_t *entity = (nr_rlc_entity_um_t *)_entity;

  entity->t_current = now;

  check_t_reassembly(entity);
}

/*************************************************************************/
/* discard/re-establishment/delete                                       */
/*************************************************************************/

void nr_rlc_entity_um_discard_sdu(nr_rlc_entity_t *_entity, int sdu_id)
{
  /* implements 38.322 5.4 */
  nr_rlc_entity_um_t *entity = (nr_rlc_entity_um_t *)_entity;
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

static void clear_entity(nr_rlc_entity_um_t *entity)
{
  nr_rlc_pdu_t *cur_rx;

  entity->rx_next_highest    = 0;
  entity->rx_next_reassembly = 0;
  entity->rx_timer_trigger   = 0;


  entity->tx_next           = 0;

  entity->t_current = 0;

  entity->t_reassembly_start      = 0;

  cur_rx = entity->rx_list;
  while (cur_rx != NULL) {
    nr_rlc_pdu_t *p = cur_rx;
    cur_rx = cur_rx->next;
    nr_rlc_free_pdu(p);
  }
  entity->rx_list = NULL;
  entity->rx_size = 0;

  nr_rlc_free_sdu_segment_list(entity->tx_list);

  entity->tx_list         = NULL;
  entity->tx_end          = NULL;
  entity->tx_size         = 0;
}

void nr_rlc_entity_um_reestablishment(nr_rlc_entity_t *_entity)
{
  nr_rlc_entity_um_t *entity = (nr_rlc_entity_um_t *)_entity;
  clear_entity(entity);
}

void nr_rlc_entity_um_delete(nr_rlc_entity_t *_entity)
{
  nr_rlc_entity_um_t *entity = (nr_rlc_entity_um_t *)_entity;
  clear_entity(entity);
  free(entity);
}
