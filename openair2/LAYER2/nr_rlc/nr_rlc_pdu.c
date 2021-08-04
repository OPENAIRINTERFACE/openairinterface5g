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

#include "nr_rlc_pdu.h"

#include <stdlib.h>
#include <string.h>

#include "LOG/log.h"

/**************************************************************************/
/* RX PDU management                                                      */
/**************************************************************************/

nr_rlc_pdu_t *nr_rlc_new_pdu(int sn, int so, int is_first,
    int is_last, char *data, int size)
{
  nr_rlc_pdu_t *ret = malloc(sizeof(nr_rlc_pdu_t));
  if (ret == NULL) {
    LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  ret->sn = sn;
  ret->so = so;
  ret->size = size;
  ret->is_first = is_first;
  ret->is_last = is_last;
  ret->next = NULL;

  ret->data = malloc(size);
  if (ret->data == NULL) {
    LOG_E(RLC, "%s:%d:%s: out of memory\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  memcpy(ret->data, data, size);

  return ret;
}

void nr_rlc_free_pdu(nr_rlc_pdu_t *pdu)
{
  free(pdu->data);
  free(pdu);
}

nr_rlc_pdu_t *nr_rlc_pdu_list_add(
    int (*sn_compare)(void *, int, int), void *sn_compare_data,
    nr_rlc_pdu_t *list, nr_rlc_pdu_t *pdu)
{
  nr_rlc_pdu_t head;
  nr_rlc_pdu_t *cur;
  nr_rlc_pdu_t *prev;

  head.next = list;
  cur = list;
  prev = &head;

  /* order is by 'sn', if 'sn' is the same then order is by 'so' */
  while (cur != NULL) {
    /* check if 'pdu' is before 'cur' in the list */
    if (sn_compare(sn_compare_data, cur->sn, pdu->sn) > 0 ||
        (cur->sn == pdu->sn && cur->so > pdu->so)) {
      break;
    }
    prev = cur;
    cur = cur->next;
  }
  prev->next = pdu;
  pdu->next = cur;
  return head.next;
}

/**************************************************************************/
/* PDU decoder                                                            */
/**************************************************************************/

void nr_rlc_pdu_decoder_init(nr_rlc_pdu_decoder_t *decoder,
                             char *buffer, int size)
{
  decoder->error = 0;
  decoder->byte = 0;
  decoder->bit = 0;
  decoder->buffer = buffer;
  decoder->size = size;
}

static int get_bit(nr_rlc_pdu_decoder_t *decoder)
{
  int ret;
  LOG_I(RLC, "Melissa Elkadi, in %s():. bytes = %d size = %d,  bit %d, shifted bit %d \n",
        __FUNCTION__, decoder->byte, decoder->size, decoder->bit, (7-decoder->bit));
  if (decoder->byte >= decoder->size) {
    decoder->error = 1;
    return 0;
  }
  LOG_I(RLC, "Melissa Elkadi, in %s():. (%d >> (7 - decoder->bit)) & 1 = %d (returned val) \n",
        __FUNCTION__,
        decoder->buffer[decoder->byte],
        ((decoder->buffer[decoder->byte] >> (7 - decoder->bit)) & 1));

  ret = (decoder->buffer[decoder->byte] >> (7 - decoder->bit)) & 1;

  decoder->bit++;
  if (decoder->bit == 8) {
    decoder->bit = 0;
    decoder->byte++;
  }

  return ret;
}

int nr_rlc_pdu_decoder_get_bits(nr_rlc_pdu_decoder_t *decoder, int count)
{
  int ret = 0;
  int i;

  if (count > 31) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  for (i = 0; i < count; i++) {
    ret <<= 1;
    ret |= get_bit(decoder);
    if (decoder->error) return -1;
  }

  return ret;
}

/**************************************************************************/
/* PDU encoder                                                            */
/**************************************************************************/

void nr_rlc_pdu_encoder_init(nr_rlc_pdu_encoder_t *encoder,
                             char *buffer, int size)
{
  encoder->byte = 0;
  encoder->bit = 0;
  encoder->buffer = buffer;
  encoder->size = size;
}

static void put_bit(nr_rlc_pdu_encoder_t *encoder, int bit)
{
  if (encoder->byte == encoder->size) {
    LOG_E(RLC, "%s:%d:%s: fatal, buffer full\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  encoder->buffer[encoder->byte] <<= 1;
  if (bit)
    encoder->buffer[encoder->byte] |= 1;

  encoder->bit++;
  if (encoder->bit == 8) {
    encoder->bit = 0;
    encoder->byte++;
  }
}

void nr_rlc_pdu_encoder_put_bits(nr_rlc_pdu_encoder_t *encoder,
                                 int value, int count)
{
  int i;
  int x;

  if (count > 31) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  x = 1 << (count - 1);
  for (i = 0; i < count; i++, x >>= 1)
    put_bit(encoder, value & x);
}
