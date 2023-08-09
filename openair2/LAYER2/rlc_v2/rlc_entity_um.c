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

#include "rlc_entity_um.h"
#include "rlc_pdu.h"

#include <stdlib.h>
#include <string.h>

#include "LOG/log.h"

/*************************************************************************/
/* PDU RX functions                                                      */
/*************************************************************************/

static int modulus_rx(rlc_entity_um_t *entity, int a)
{
  /* as per 36.322 7.1, modulus base is vr(uh)-window_size and modulus is
   * 2^sn_field_length (which is 'sn_modulus' in rlc_entity_um_t)
   */
  int r = a - (entity->vr_uh - entity->window_size);
  if (r < 0) r += entity->sn_modulus;
  return r % entity->sn_modulus;
}

static int sn_compare_rx(void *_entity, int a, int b)
{
  rlc_entity_um_t *entity = _entity;
  return modulus_rx(entity, a) - modulus_rx(entity, b);
}

static int sn_in_recv_window(void *_entity, int sn)
{
  rlc_entity_um_t *entity = _entity;
  int mod_sn = modulus_rx(entity, sn);
  /* we simplify (VR(UH) - UM_Window_Size) <= SN < VR(UH), base is
   * (VR(UH) - UM_Window_Size) and VR(UH) = base + window_size
   */
  return mod_sn < entity->window_size;
}

/* return 1 if a PDU with SN == 'sn' is in the rx list, 0 otherwise */
static int rlc_um_pdu_received(rlc_entity_um_t *entity, int sn)
{
  rlc_rx_pdu_segment_t *cur = entity->rx_list;
  while (cur != NULL) {
    if (cur->sn == sn)
      return 1;
    cur = cur->next;
  }
  return 0;
}

static int less_than_vr_ur(rlc_entity_um_t *entity, int sn)
{
  return sn_compare_rx(entity, sn, entity->vr_ur) < 0;
}

static int outside_of_reordering_window(rlc_entity_um_t *entity, int sn)
{
  return !sn_in_recv_window(entity, sn);
}

static int less_than_vr_uh(rlc_entity_um_t *entity, int sn)
{
  return sn_compare_rx(entity, sn, entity->vr_uh) < 0;
}

static void rlc_um_reassemble_pdu(rlc_entity_um_t *entity,
    rlc_rx_pdu_segment_t *pdu)
{
  rlc_um_reassemble_t *r = &entity->reassemble;

  int fi;
  int e;
  int sn;
  int data_pos;
  int sdu_len;
  int sdu_offset;

  sdu_offset = pdu->data_offset;

  rlc_pdu_decoder_init(&r->dec, pdu->data, pdu->size);

  if (entity->sn_field_length == 10)
    rlc_pdu_decoder_get_bits(&r->dec, 3);

  fi = rlc_pdu_decoder_get_bits(&r->dec, 2);
  e  = rlc_pdu_decoder_get_bits(&r->dec, 1);
  sn = rlc_pdu_decoder_get_bits(&r->dec, entity->sn_field_length);

  if (e) {
    e       = rlc_pdu_decoder_get_bits(&r->dec, 1);
    sdu_len = rlc_pdu_decoder_get_bits(&r->dec, 11);
  } else
    sdu_len = pdu->size - sdu_offset;

  /* discard current SDU being reassembled if bad SN or bad FI */
  if (sn != (r->sn + 1) % entity->sn_modulus ||
      !(fi & 0x02)) {
    if (r->sdu_pos)
      LOG_D(RLC, "%s:%d:%s: warning: discard partially reassembled SDU\n",
            __FILE__, __LINE__, __FUNCTION__);
    r->sdu_pos = 0;
  }

  /* if the head of the SDU is missing, still process the PDU
   * but remember to discard the reassembled SDU later on (the
   * head has not been received).
   * The head is missing if sdu_pos == 0 and fi says the PDU does not
   * start an SDU.
   */
  if (r->sdu_pos == 0 && (fi & 0x02))
    r->sdu_head_missing = 1;

  r->sn = sn;
  data_pos = pdu->data_offset;

  while (1) {
    if (r->sdu_pos >= SDU_MAX) {
      /* TODO: proper error handling (discard PDUs with current sn from
       * reassembly queue? something else?)
       */
      LOG_E(RLC, "%s:%d:%s: bad RLC PDU\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    r->sdu[r->sdu_pos] = pdu->data[data_pos];
    r->sdu_pos++;
    data_pos++;
    if (data_pos == sdu_offset + sdu_len) {
      /* all bytes of SDU are consumed, check if SDU is fully there.
       * It is if the data pointer is not at the end of the PDU segment
       * or if 'fi' & 1 == 0
       */
      if (data_pos != pdu->size || (fi & 1) == 0) {
        /* time to discard the SDU if we didn't receive the head */
        if (r->sdu_head_missing) {
          LOG_D(RLC, "%s:%d:%s: warning: discard SDU, head not received\n",
                __FILE__, __LINE__, __FUNCTION__);
          r->sdu_head_missing = 0;
        } else {
          /* SDU is full - deliver to higher layer */
          entity->common.deliver_sdu(entity->common.deliver_sdu_data,
                                     (rlc_entity_t *)entity,
                                     r->sdu, r->sdu_pos);
        }
        r->sdu_pos = 0;
      }
      /* done with PDU? */
      if (data_pos == pdu->size)
        break;
      /* not at the end of PDU, process next SDU */
      sdu_offset += sdu_len;
      if (e) {
        e       = rlc_pdu_decoder_get_bits(&r->dec, 1);
        sdu_len = rlc_pdu_decoder_get_bits(&r->dec, 11);
      } else
        sdu_len = pdu->size - sdu_offset;
    }
  }
}

static void rlc_um_reassemble(rlc_entity_um_t *entity,
    int (*check_sn)(rlc_entity_um_t *entity, int sn))
{
  rlc_rx_pdu_segment_t *cur;

  /* process all PDUs from head of rx list until all is processed or
   * the SN is not valid anymore with respect to 'check_sn'
   */
  while (entity->rx_list != NULL && check_sn(entity, entity->rx_list->sn)) {
    cur = entity->rx_list;
    rlc_um_reassemble_pdu(entity, cur);
    entity->rx_size -= cur->size;
    entity->rx_list = cur->next;
    rlc_rx_free_pdu_segment(cur);
  }
}

static void rlc_um_reception_actions(rlc_entity_um_t *entity,
    rlc_rx_pdu_segment_t *pdu_segment)
{
  if (!sn_in_recv_window(entity, pdu_segment->sn)) {
    entity->vr_uh = (pdu_segment->sn + 1) % entity->sn_modulus;
    rlc_um_reassemble(entity, outside_of_reordering_window);
    if (!sn_in_recv_window(entity, entity->vr_ur))
      entity->vr_ur = (entity->vr_uh - entity->window_size
                         + entity->sn_modulus) % entity->sn_modulus;
  }

  if (rlc_um_pdu_received(entity, entity->vr_ur)) {
    do {
      entity->vr_ur = (entity->vr_ur + 1) % entity->sn_modulus;
    } while (rlc_um_pdu_received(entity, entity->vr_ur));
    rlc_um_reassemble(entity, less_than_vr_ur);
  }

  if (entity->t_reordering_start) {
    if (sn_compare_rx(entity, entity->vr_ux, entity->vr_ur) <= 0 ||
        (!sn_in_recv_window(entity, entity->vr_ux) &&
         entity->vr_ux != entity->vr_uh))
      entity->t_reordering_start = 0;
  }

  if (entity->t_reordering_start == 0) {
    if (sn_compare_rx(entity, entity->vr_uh, entity->vr_ur) > 0) {
      entity->t_reordering_start = entity->t_current;
      entity->vr_ux = entity->vr_uh;
    }
  }
}

void rlc_entity_um_recv_pdu(rlc_entity_t *_entity, char *buffer, int size)
{
#define R(d) do { if (rlc_pdu_decoder_in_error(&d)) goto err; } while (0)
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;
  rlc_pdu_decoder_t decoder;
  rlc_pdu_decoder_t data_decoder;

  int e;
  int sn;

  int data_e;
  int data_li;

  int data_size;
  int data_start;
  int indicated_data_size;

  rlc_rx_pdu_segment_t *pdu_segment;

  rlc_pdu_decoder_init(&decoder, buffer, size);

  if (entity->sn_field_length == 10) {
    rlc_pdu_decoder_get_bits(&decoder, 3); R(decoder);       /* R1 */
  }

  rlc_pdu_decoder_get_bits(&decoder, 2); R(decoder);         /* FI */
  e  = rlc_pdu_decoder_get_bits(&decoder, 1); R(decoder);
  sn = rlc_pdu_decoder_get_bits(&decoder, entity->sn_field_length); R(decoder);

  /* dicard PDU if rx buffer is full */
  if (entity->rx_size + size > entity->rx_maxsize) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, RX buffer full\n",
          __FILE__, __LINE__, __FUNCTION__);
    return;
  }

  /* discard according to 36.322 5.1.2.2.2 */
  if ((sn_compare_rx(entity, entity->vr_ur, sn) < 0 &&
       sn_compare_rx(entity, sn, entity->vr_uh) < 0 &&
       rlc_um_pdu_received(entity, sn)) ||
      (sn_compare_rx(entity, entity->vr_uh - entity->window_size, sn) <= 0 &&
       sn_compare_rx(entity, sn, entity->vr_ur) < 0)) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU (sn %d vr(ur) %d vr(uh) %d)\n",
          __FILE__, __LINE__, __FUNCTION__,
          sn, entity->vr_ur, entity->vr_uh);
    return;
  }


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
      return;
    }
    indicated_data_size += data_li;
  }
  rlc_pdu_decoder_align(&data_decoder);

  data_start = data_decoder.byte;
  data_size = size - data_start;

  if (data_size <= 0) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, wrong data size (sum of LI %d data size %d)\n",
          __FILE__, __LINE__, __FUNCTION__,
          indicated_data_size, data_size);
    return;
  }
  if (indicated_data_size >= data_size) {
    LOG_D(RLC, "%s:%d:%s: warning: discard PDU, bad LIs (sum of LI %d data size %d)\n",
          __FILE__, __LINE__, __FUNCTION__,
          indicated_data_size, data_size);
    return;
  }

  /* put in pdu reception list */
  entity->rx_size += size;
  pdu_segment = rlc_rx_new_pdu_segment(sn, 0, size, 1, buffer, data_start);
  entity->rx_list = rlc_rx_pdu_segment_list_add(sn_compare_rx, entity,
                                                entity->rx_list, pdu_segment);

  /* do reception actions (36.322 5.1.2.2.3) */
  rlc_um_reception_actions(entity, pdu_segment);

  return;

err:
  LOG_D(RLC, "%s:%d:%s: error decoding PDU, discarding\n", __FILE__, __LINE__, __FUNCTION__);

#undef R
}

/*************************************************************************/
/* TX functions                                                          */
/*************************************************************************/

typedef struct {
  int sdu_count;
  int data_size;
  int header_size;
  int last_sdu_is_full;
  int first_sdu_length;
} tx_pdu_size_t;

static int header_size(int sn_field_length, int sdu_count)
{
  int bits = 8 + 8 * (sn_field_length == 10) + 12 * (sdu_count - 1);
  /* padding if we have to */
  return (bits + 7) / 8;
}

static tx_pdu_size_t tx_pdu_size(rlc_entity_um_t *entity, int maxsize)
{
  tx_pdu_size_t ret;
  int sdu_count;
  int sdu_size;
  int pdu_data_size;
  rlc_sdu_t *sdu;

  ret.sdu_count = 0;
  ret.data_size = 0;
  ret.header_size = 0;
  ret.last_sdu_is_full = 1;
  ret.first_sdu_length = 0;

  /* TX PDU - let's make the biggest PDU we can with the SDUs we have */
  sdu_count = 0;
  pdu_data_size = 0;
  sdu = entity->tx_list;
  while (sdu != NULL) {
    int new_header_size = header_size(entity->sn_field_length, sdu_count+1);
    /* if we cannot put new header + at least 1 byte of data then over */
    if (new_header_size + pdu_data_size >= maxsize)
      break;
    sdu_count++;
    /* only include the bytes of this SDU not included in PDUs already */
    sdu_size = sdu->size - sdu->next_byte;
    /* don't feed more than 'maxsize' bytes */
    if (new_header_size + pdu_data_size + sdu_size > maxsize) {
      sdu_size = maxsize - new_header_size - pdu_data_size;
      ret.last_sdu_is_full = 0;
    }
    if (sdu_count == 1)
      ret.first_sdu_length = sdu_size;
    pdu_data_size += sdu_size;
    /* if we put more than 2^11-1 bytes then the LI field cannot be used,
     * so this is the last SDU we can put
     */
    if (sdu_size > 2047)
      break;
    sdu = sdu->next;
  }

  if (sdu_count) {
    ret.sdu_count = sdu_count;
    ret.data_size = pdu_data_size;
    ret.header_size = header_size(entity->sn_field_length, sdu_count);
  }

  return ret;
}

rlc_entity_buffer_status_t rlc_entity_um_buffer_status(
    rlc_entity_t *_entity, int maxsize)
{
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;
  rlc_entity_buffer_status_t ret;
  tx_pdu_size_t tx_size;

  ret.status_size = 0;

  /* todo: if an SDU has size >2047 in the tx list then processing
   * stops and computed size will not be accurate. Change the computation
   * to be more accurate (if needed).
   */
  tx_size = tx_pdu_size(entity, maxsize);
  ret.tx_size = tx_size.data_size + tx_size.header_size;

  ret.retx_size = 0;

  return ret;
}

int rlc_entity_um_generate_pdu(rlc_entity_t *_entity, char *buffer, int size)
{
  rlc_entity_um_t      *entity = (rlc_entity_um_t *)_entity;
  tx_pdu_size_t        pdu_size;
  rlc_sdu_t            *sdu;
  int                  i;
  int                  cursize;
  int                  first_sdu_full;
  int                  last_sdu_full;
  rlc_pdu_encoder_t    encoder;
  int                  fi;
  int                  e;
  int                  li;
  char                 *out;
  int                  outpos;
  int                  first_sdu_start_byte;
  int                  sdu_start_byte;

  pdu_size = tx_pdu_size(entity, size);
  if (pdu_size.sdu_count == 0)
    return 0;

  sdu = entity->tx_list;

  first_sdu_start_byte = sdu->next_byte;

  /* reserve SDU bytes */
  cursize = 0;
  for (i = 0; i < pdu_size.sdu_count; i++, sdu = sdu->next) {
    int sdu_size = sdu->size - sdu->next_byte;
    if (cursize + sdu_size > pdu_size.data_size)
      sdu_size = pdu_size.data_size - cursize;
    sdu->next_byte += sdu_size;
    cursize += sdu_size;
  }

  first_sdu_full = first_sdu_start_byte == 0;
  last_sdu_full = pdu_size.last_sdu_is_full;

  /* generate header */
  rlc_pdu_encoder_init(&encoder, buffer, size);

  if (entity->sn_field_length == 10)
    rlc_pdu_encoder_put_bits(&encoder, 0, 3);                         /* R1 */

  fi = 0;
  if (!first_sdu_full)
    fi |= 0x02;
  if (!last_sdu_full)
    fi |= 0x01;
  rlc_pdu_encoder_put_bits(&encoder, fi, 2);                          /* FI */

  /* see the AM code to understand the logic for Es and LIs */
  if (pdu_size.sdu_count >= 2)
    e = 1;
  else
    e = 0;
  rlc_pdu_encoder_put_bits(&encoder, e, 1);                            /* E */

  if (entity->sn_field_length == 10)
    rlc_pdu_encoder_put_bits(&encoder, entity->vt_us, 10);            /* SN */
  else
    rlc_pdu_encoder_put_bits(&encoder, entity->vt_us, 5);             /* SN */

  /* put LIs */
  sdu = entity->tx_list;
  /* first SDU */
  li = pdu_size.first_sdu_length;
  /* put E+LI only if at least 2 SDUs */
  if (pdu_size.sdu_count >= 2) {
    /* E is 1 if at least 3 SDUs */
    if (pdu_size.sdu_count >= 3)
      e = 1;
    else
      e = 0;
    rlc_pdu_encoder_put_bits(&encoder, e, 1);                          /* E */
    rlc_pdu_encoder_put_bits(&encoder, li, 11);                       /* LI */
  }
  /* next SDUs, but not the last (no LI for the last) */
  sdu = sdu->next;
  for (i = 2; i < pdu_size.sdu_count; i++, sdu = sdu->next) {
    if (i != pdu_size.sdu_count - 1)
      e = 1;
    else
      e = 0;
    li = sdu->size;
    rlc_pdu_encoder_put_bits(&encoder, e, 1);                          /* E */
    rlc_pdu_encoder_put_bits(&encoder, li, 11);                       /* LI */
  }

  rlc_pdu_encoder_align(&encoder);

  /* generate data */
  out = buffer + pdu_size.header_size;
  sdu = entity->tx_list;
  sdu_start_byte = first_sdu_start_byte;
  outpos = 0;
  for (i = 0; i < pdu_size.sdu_count; i++, sdu = sdu->next) {
    li = sdu->size - sdu_start_byte;
    if (outpos + li >= pdu_size.data_size)
      li = pdu_size.data_size - outpos;
    memcpy(out+outpos, sdu->data + sdu_start_byte, li);
    outpos += li;
    sdu_start_byte = 0;
  }

  /* cleanup sdu list */
  while (entity->tx_list != NULL &&
         entity->tx_list->size == entity->tx_list->next_byte) {
    rlc_sdu_t *c = entity->tx_list;
    /* release SDU bytes */
    entity->tx_size -= c->size;
    entity->tx_list = c->next;
    rlc_free_sdu(c);
  }
  if (entity->tx_list == NULL)
    entity->tx_end = NULL;

  /* update VT(US) */
  entity->vt_us = (entity->vt_us + 1) % entity->sn_modulus;

  return pdu_size.header_size + pdu_size.data_size;
}

/*************************************************************************/
/* SDU RX functions                                                      */
/*************************************************************************/

void rlc_entity_um_recv_sdu(rlc_entity_t *_entity, char *buffer, int size,
                            int sdu_id)
{
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;
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

static void check_t_reordering(rlc_entity_um_t *entity)
{
  int sn;

  /* is t_reordering running and if yes has it expired? */
  if (entity->t_reordering_start == 0 ||
      entity->t_current <= entity->t_reordering_start + entity->t_reordering)
    return;

  /* stop timer */
  entity->t_reordering_start = 0;

  LOG_D(RLC, "%s:%d:%s: t_reordering expired\n", __FILE__, __LINE__, __FUNCTION__);

  /* update VR(UR) to first SN >= VR(UX) of PDU not received
   */
  sn = entity->vr_ux;
  while (rlc_um_pdu_received(entity, sn))
    sn = (sn + 1) % entity->sn_modulus;
  entity->vr_ur = sn;

  rlc_um_reassemble(entity, less_than_vr_ur);

  if (sn_compare_rx(entity, entity->vr_uh, entity->vr_ur) > 0) {
    entity->t_reordering_start = entity->t_current;
    entity->vr_ux = entity->vr_uh;
  }
}

void rlc_entity_um_set_time(rlc_entity_t *_entity, uint64_t now)
{
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;

  entity->t_current = now;

  check_t_reordering(entity);
}

/*************************************************************************/
/* discard/re-establishment/delete                                       */
/*************************************************************************/

void rlc_entity_um_discard_sdu(rlc_entity_t *_entity, int sdu_id)
{
  /* implements 36.322 5.3 */
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;
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

static void clear_entity(rlc_entity_um_t *entity)
{
  rlc_rx_pdu_segment_t *cur_rx;
  rlc_sdu_t            *cur_tx;

  entity->vr_ur = 0;
  entity->vr_ux = 0;
  entity->vr_uh = 0;

  entity->vt_us = 0;

  entity->t_current = 0;

  entity->t_reordering_start = 0;

  cur_rx = entity->rx_list;
  while (cur_rx != NULL) {
    rlc_rx_pdu_segment_t *p = cur_rx;
    cur_rx = cur_rx->next;
    rlc_rx_free_pdu_segment(p);
  }
  entity->rx_list = NULL;
  entity->rx_size = 0;

  memset(&entity->reassemble, 0, sizeof(rlc_um_reassemble_t));

  cur_tx = entity->tx_list;
  while (cur_tx != NULL) {
    rlc_sdu_t *p = cur_tx;
    cur_tx = cur_tx->next;
    rlc_free_sdu(p);
  }
  entity->tx_list = NULL;
  entity->tx_end = NULL;
  entity->tx_size = 0;
}

void rlc_entity_um_reestablishment(rlc_entity_t *_entity)
{
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;

  rlc_um_reassemble(entity, less_than_vr_uh);

  clear_entity(entity);
}

void rlc_entity_um_delete(rlc_entity_t *_entity)
{
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;
  clear_entity(entity);
  free(entity);
}
