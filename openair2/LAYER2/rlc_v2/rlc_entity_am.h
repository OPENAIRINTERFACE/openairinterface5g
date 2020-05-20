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

#ifndef _RLC_ENTITY_AM_H_
#define _RLC_ENTITY_AM_H_

#include <stdint.h>

#include "rlc_entity.h"
#include "rlc_pdu.h"
#include "rlc_sdu.h"

/*
 * Here comes some documentation to understand the reassembly
 * logic in the code and the fields in the structure rlc_am_reassemble_t.
 *
 * Inside RLC, we deal with SDUs, PDUs and PDU segments.
 * SDUs are packets coming from upper layer.
 * A PDU is made of a header and a payload.
 * In the payload there are SDUs.
 * First SDU and last SDU in a PDU may be incomplete.
 * PDU segments exist in case of retransmissions when the MAC
 * layer asks for less data than previously, in which case
 * only part of the previous PDU is sent.
 *
 * This is PDU data (just bytes):
 * ---------------------------------------------------------
 * |  PDU data                                             |
 * ---------------------------------------------------------
 * It contains SDUs, like:
 * ---------------------------------------------------------
 * | SDU 1 | SDU 2     |  [...]                   | SDU n  |
 * ---------------------------------------------------------
 * SDU 1 may be only the end of an SDU from which previous bytes were
 * transmitted in previous PDUs.
 * SDU n may be only the start of an SDU, that is more bytes from
 * this SDU may be sent in successive PDUs.
 *
 * At front of the PDU data, we have a header:
 * ---------------  ---------------------------------------------------------
 * | PDU header  |  | SDU 1 | SDU 2     |  [...]                   | SDU n  |
 * ---------------  ---------------------------------------------------------
 * PDU header describes PDU data (most notably lengths).
 *
 * A PDU segment is a part of a PDU. For example, from this PDU data:
 * ---------------------------------------------------------
 * | SDU 1 | SDU 2     |  [...]                   | SDU n  |
 * ---------------------------------------------------------
 * We can extract the following PDU segment (data part only):
 *                ----------------------
 *                | PDU segment data   |
 *                ----------------------
 * This PDU segment would contain the end of SDU 2 above and some SDUs up to,
 * let's say SDU x (x is 5 below).
 *
 * In front of a transmitted PDU segment, we have a header,
 * containing the important variable 'so' (segment offset) that gives
 * the index of the first byte of the segment in the original PDU.
 * -------------- ----------------------
 * | seg. header| | PDU segment data   |
 * -------------- ----------------------
 *
 * Let's now explain the data structure rlc_am_reassemble_t.
 *
 * In the structure rlc_am_reassemble_t, the fields fi, e, sn and so
 * are coming from the PDU segment header and the semantics is the
 * one of the RLC specs.
 *
 * The currently processed PDU segment is stored in 'start'.
 * We have 'start->s->data_offset' and 'start->s->size'.
 * start->s->data_offset is the index of the start of the data in the
 * PDU segment. That is if the header is of length 3 bytes
 * then start->s->data_offset is 3.
 * start->s->size is the total length of the PDU segment,
 * including header.
 * The size of actual data bytes in the PDU segment is thus
 * start->s->size - start->s->data_offset.
 *
 * The field sdu_len is the length of the current SDU being
 * processed.
 *
 * The field sdu_offset is the starting point of the
 * current SDU being processed (starting from beginning
 * of PDU segment, including header).
 *
 * The field data_pos is the current read pointer. 0 points to
 * the beginning of the PDU segment (including header).
 *
 * The field pdu_byte points to the current byte in the original
 * PDU (not the PDU segment). It starts at 0 when we start
 * processing a new PDU (when a new 'sn' is seen) and always
 * increases after each byte processed. This is tha variable
 * that is used to know if the next PDU segment will be used
 * or not and if yes, starting from which data byte (see
 * function rlc_am_reassemble_next_segment).
 *
 * 'so' is important and points to the byte in the original PDU
 * that is the first byte of the PDU segment.
 *
 * For example, let's take this PDU segment data from above:
 *                ----------------------
 *                | PDU segment data   |
 *                ----------------------
 * Let's say it is decomposed as:
 *                ----------------------
 *                |222|33|4444|55555555|
 *                ----------------------
 * It contains SDUs 2, 3, 4, and 5.
 * SDU 2 is 3 bytes, SDU 3 is 2 bytes, SDU 4 is 4 bytes, SDU 5 is 8 bytes.
 *
 * Let's suppose that the original PDU starts with:
 * ----------------
 * |1111111|222222|
 * ----------------
 *
 * (In this example, in the PDU segment, SDU 2 is not full,
 * we only have its end.)
 *
 * Then 'so' is 13 (SDU 1 is 7 bytes, head of SDU 2 is 6 bytes).
 *
 * Let's continue with our PDU segment data.
 * Let's say we are current processing SDU 4.
 * Let's say the read pointer (variable 'data_pos') is there:
 *                ----------------------
 *                |222|33|4444|55555555|
 *                ----------------------
 *                         ^
 *                      read pointer (data_pos)
 *
 * Then:
 *     - sdu_len is 4
 *     - sdu_offset is 5 + [PDU segment header length]
 *       (it points to the beginning of SDU 4, starting
 *        from the head of the PDU segment, that is
 *        3 bytes for SDU 2, 2 bytes for SDU 3, and the
 *        PDU segment header length)
 *     - start->s->data_offset is [PDU segment header length]
 *     - pdu_byte is 20
 *       (13 bytes from beginning of original PDU,
 *        3 bytes for SDU 2, 2 bytes for SDU 3, then 2 bytes for SDU 4)
 *     - data_pos = read pointer = 7 + [PDU segment header length]
 *
 * To finish this description, in the code, a PDU is simply
 * seen as a PDU segment with 'so' = 0 (and is_last == 1 (lsf in the specs),
 * but this variable is not used by the reassembly logic).
 *
 * And for [PDU segment header length] we use start->s->data_offset.
 *
 * To recap, here is an illustration of the various variables
 * and what starting point they use. In the figures, the start
 * of the variable name is aligned to the byte it refers to.
 * + is used to show the starting point.
 *
 * Let's put the PDU segment back into the original PDU.
 * And let's show the values for when the read pointer
 * is on the second byte of SDU 4 (as above).
 *
 * +++++++++++++++ so
 * +++++++++++++++++++++++ pdu_byte
 * ---------------------------------------------------------
 * | SDU 1| SDU 2..222|33|4444|55555555| [...]   | SDU n   |
 * ---------------------------------------------------------
 *
 * And now the PDU segment with header.
 *
 *
 *                        ++++ sdu_len
 * ++++++++++++++++++++++ sdu_offset
 * +++++++++++++++++++++++ data_pos
 * +++++++++++++++ start->s->data_offset
 * +++++++++++++++++++++++++++++++++++++ start->s->size
 * -------------- ----------------------
 * | seg. header| |222|33|4444|55555555|
 * -------------- ----------------------
 *
 * We see three case for the starting point:
 *     - start of original PDU (without any header)
 *     - start of header of current PDU segment
 *     - start of current SDU (for sdu_len)
 */

typedef struct {
  rlc_rx_pdu_segment_t *start;      /* start of list */
  rlc_rx_pdu_segment_t *end;        /* end of list (last element) */
  int                  pos;         /* byte to get from current buffer */
  char                 sdu[SDU_MAX]; /* sdu is reassembled here */
  int                  sdu_pos;      /* next byte to put in sdu */

  /* decoder of current PDU */
  rlc_pdu_decoder_t    dec;
  int fi;
  int e;
  int sn;
  int so;
  int sdu_len;
  int sdu_offset;
  int data_pos;
  int pdu_byte;
} rlc_am_reassemble_t;

typedef struct {
  rlc_entity_t common;

  /* configuration */
  int t_reordering;
  int t_status_prohibit;
  int t_poll_retransmit;
  int poll_pdu;              /* -1 means infinity */
  int poll_byte;             /* -1 means infinity */
  int max_retx_threshold;

  /* runtime rx */
  int vr_r;
  int vr_x;
  int vr_ms;
  int vr_h;

  int status_triggered;

  /* runtime tx */
  int vt_a;
  int vt_s;
  int poll_sn;
  int pdu_without_poll;
  int byte_without_poll;
  int force_poll;

  /* set to the latest know time by the user of the module. Unit: ms */
  uint64_t t_current;

  /* timers (stores the TTI of activation, 0 means not active) */
  uint64_t t_reordering_start;
  uint64_t t_status_prohibit_start;
  uint64_t t_poll_retransmit_start;

  /* rx management */
  rlc_rx_pdu_segment_t *rx_list;
  int                  rx_size;
  int                  rx_maxsize;

  /* reassembly management */
  rlc_am_reassemble_t    reassemble;

  /* tx management */
  rlc_sdu_t *tx_list;
  rlc_sdu_t *tx_end;
  int       tx_size;
  int       tx_maxsize;

  rlc_tx_pdu_segment_t *wait_list;
  rlc_tx_pdu_segment_t *retransmit_list;

  rlc_tx_pdu_segment_t *ack_list;
} rlc_entity_am_t;

void rlc_entity_am_recv_sdu(rlc_entity_t *entity, char *buffer, int size,
                            int sdu_id);
void rlc_entity_am_recv_pdu(rlc_entity_t *entity, char *buffer, int size);
rlc_entity_buffer_status_t rlc_entity_am_buffer_status(
    rlc_entity_t *entity, int maxsize);
int rlc_entity_am_generate_pdu(rlc_entity_t *entity, char *buffer, int size);
void rlc_entity_am_set_time(rlc_entity_t *entity, uint64_t now);
void rlc_entity_am_discard_sdu(rlc_entity_t *entity, int sdu_id);
void rlc_entity_am_reestablishment(rlc_entity_t *entity);
void rlc_entity_am_delete(rlc_entity_t *entity);

#endif /* _RLC_ENTITY_AM_H_ */
