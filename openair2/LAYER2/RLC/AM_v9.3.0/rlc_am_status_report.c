/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#define RLC_AM_MODULE 1
#define RLC_AM_STATUS_REPORT_C 1
//-----------------------------------------------------------------------------
#include <string.h>
//-----------------------------------------------------------------------------
#include "platform_types.h"
//-----------------------------------------------------------------------------
#if ENABLE_ITTI
# include "intertask_interface.h"
#endif
#include "assertions.h"
#include "list.h"
#include "rlc_am.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"

#   if ENABLE_ITTI
//-----------------------------------------------------------------------------
void
rlc_am_itti_display_status_ind_infos(
  const protocol_ctxt_t* const            ctxt_pP,
  const rlc_am_entity_t *const            rlc_pP,
  const rlc_am_control_pdu_info_t *const  pdu_info_pP)
{
  char                 message_string[1000];
  size_t               message_string_size = 0;
  MessageDef          *msg_p;

  int num_nack;

  if (!pdu_info_pP->d_c) {
    message_string_size += sprintf(&message_string[message_string_size], "Bearer      : %u\n", rlc_pP->rb_id);
    message_string_size += sprintf(&message_string[message_string_size], "CONTROL PDU ACK SN %04d", pdu_info_pP->ack_sn);

    for (num_nack = 0; num_nack < pdu_info_pP->num_nack; num_nack++) {
      if (pdu_info_pP->nack_list[num_nack].e2) {
        message_string_size += sprintf(&message_string[message_string_size], "\n\tNACK SN %04d SO START %05d SO END %05d",  pdu_info_pP->nack_list[num_nack].nack_sn,
                                       pdu_info_pP->nack_list[num_nack].so_start,
                                       pdu_info_pP->nack_list[num_nack].so_end);
      } else {
        message_string_size += sprintf(&message_string[message_string_size], "\n\tNACK SN %04d",  pdu_info_pP->nack_list[num_nack].nack_sn);
      }
    }

    message_string_size += sprintf(&message_string[message_string_size], "\n");

    msg_p = itti_alloc_new_message_sized (ctxt_pP->enb_flag > 0 ? TASK_RLC_ENB:TASK_RLC_UE , RLC_AM_STATUS_PDU_IND, message_string_size + sizeof (IttiMsgText));
    msg_p->ittiMsg.rlc_am_status_pdu_ind.size = message_string_size;
    memcpy(&msg_p->ittiMsg.rlc_am_status_pdu_ind.text, message_string, message_string_size);

    itti_send_msg_to_task(TASK_UNKNOWN, ctxt_pP->instance, msg_p);
  }
}

#   endif

//-----------------------------------------------------------------------------
uint16_t rlc_am_read_bit_field(
  uint8_t** data_ppP,
  unsigned int* bit_pos_pP,
  const signed int bits_to_readP)
{
  uint16_t        value     = 0;
  unsigned int bits_read = 0;
  signed int bits_to_read = bits_to_readP;

  do {
    // bits read > bits to read
    if ((8 - *bit_pos_pP) > bits_to_read) {
      bits_read = 8 - *bit_pos_pP;
      value = (value << bits_to_read) | ((((uint16_t)(**data_ppP)) & (uint16_t)(0x00FF >> *bit_pos_pP)) >> (bits_read -
                                         bits_to_read));
      *bit_pos_pP = *bit_pos_pP + bits_to_read;
      return value;
      // bits read == bits to read
    } else if ((8 - *bit_pos_pP) == bits_to_read) {
      value = (value << bits_to_read) | (((uint16_t)(**data_ppP)) & (uint16_t)(0x00FF >> *bit_pos_pP));
      *bit_pos_pP = 0;
      *data_ppP = *data_ppP + 1;
      return value;
      // bits read < bits to read
    } else {
      bits_read = 8 - *bit_pos_pP;
      value = (value << bits_read) | ((((uint16_t)(**data_ppP)) & (uint16_t)(0x00FF >> *bit_pos_pP)));
      *bit_pos_pP = 0;
      *data_ppP = *data_ppP + 1;
      bits_to_read = bits_to_read - bits_read;
    }
  } while (bits_to_read > 0);

  return value;
}
//-----------------------------------------------------------------------------
void
rlc_am_write8_bit_field(
  uint8_t** data_ppP,
  unsigned int* bit_pos_pP,
  const signed int bits_to_writeP,
  const uint8_t valueP)
{
  unsigned int available_bits;
  signed int bits_to_write= bits_to_writeP;

  do {
    available_bits = 8 - *bit_pos_pP;

    // available_bits > bits to write
    if (available_bits > bits_to_write) {
      **data_ppP = **data_ppP | (((valueP & (((uint8_t)0xFF) >> (available_bits - bits_to_write)))) << (available_bits -
                                 bits_to_write));
      *bit_pos_pP = *bit_pos_pP + bits_to_write;
      return;
      // bits read == bits to read
    } else if (available_bits == bits_to_write) {
      **data_ppP = **data_ppP | (valueP & (((uint8_t)0xFF) >> (8 - bits_to_write)));
      *bit_pos_pP = 0;
      *data_ppP = *data_ppP + 1;
      return;
      // available_bits < bits to write
    } else {
      **data_ppP = **data_ppP  | (valueP >> (bits_to_write - available_bits));
      *bit_pos_pP = 0;
      *data_ppP = *data_ppP + 1;
      bits_to_write = bits_to_write - available_bits;
    }
  } while (bits_to_write > 0);
}
//-----------------------------------------------------------------------------
void
rlc_am_write16_bit_field(
  uint8_t** data_ppP,
  unsigned int* bit_pos_pP,
  signed int bits_to_writeP,
  const uint16_t valueP)
{
  assert(bits_to_writeP <= 16);

  if (bits_to_writeP > 8) {
    rlc_am_write8_bit_field(data_ppP,bit_pos_pP,  bits_to_writeP - 8, (uint8_t)(valueP >> 8));
    rlc_am_write8_bit_field(data_ppP,bit_pos_pP,  8, (uint8_t)(valueP & 0x00FF));
  } else {
    rlc_am_write8_bit_field(data_ppP,bit_pos_pP,  bits_to_writeP, (uint8_t)(valueP & 0x00FF));
  }
}
//-----------------------------------------------------------------------------
signed int
rlc_am_get_control_pdu_infos(
  rlc_am_pdu_sn_10_t* const        header_pP,
  sdu_size_t * const               total_size_pP,
  rlc_am_control_pdu_info_t* const pdu_info_pP)
{
  memset(pdu_info_pP, 0, sizeof (rlc_am_control_pdu_info_t));

  pdu_info_pP->d_c = header_pP->b1 >> 7;


  if (!pdu_info_pP->d_c) {
    pdu_info_pP->cpt    = (header_pP->b1 >> 4) & 0x07;

    if (pdu_info_pP->cpt != 0x00) {
      return -3;
    }

    pdu_info_pP->ack_sn = ((header_pP->b2 >> 2) & 0x3F) | (((uint16_t)(header_pP->b1 & 0x0F)) << 6);
    pdu_info_pP->e1     = (header_pP->b2 >> 1) & 0x01;
    //*total_size_pP -= 1;

    if (pdu_info_pP->e1) {
      unsigned int nack_to_read  = 1;
      unsigned int bit_pos       = 7; // range from 0 (MSB/left) to 7 (LSB/right)
      uint8_t*        byte_pos_p      = &header_pP->b2;

      while (nack_to_read)  {
        pdu_info_pP->nack_list[pdu_info_pP->num_nack].nack_sn = rlc_am_read_bit_field(&byte_pos_p, &bit_pos, 10);
        pdu_info_pP->nack_list[pdu_info_pP->num_nack].e1      = rlc_am_read_bit_field(&byte_pos_p, &bit_pos, 1);
        pdu_info_pP->nack_list[pdu_info_pP->num_nack].e2      = rlc_am_read_bit_field(&byte_pos_p, &bit_pos, 1);

        // READ SOstart, SOend field
        if (pdu_info_pP->nack_list[pdu_info_pP->num_nack].e2) {
          pdu_info_pP->nack_list[pdu_info_pP->num_nack].so_start = rlc_am_read_bit_field(&byte_pos_p, &bit_pos, 15);
          pdu_info_pP->nack_list[pdu_info_pP->num_nack].so_end   = rlc_am_read_bit_field(&byte_pos_p, &bit_pos, 15);
        } else {
          pdu_info_pP->nack_list[pdu_info_pP->num_nack].so_start = 0;
          // all 15 bits set to 1 (indicate that the missing portion of the AMD PDU includes all bytes
          // to the last byte of the AMD PDU)
          pdu_info_pP->nack_list[pdu_info_pP->num_nack].so_end   = 0x7FFF;
        }

        pdu_info_pP->num_nack = pdu_info_pP->num_nack + 1;

        if (!pdu_info_pP->nack_list[pdu_info_pP->num_nack - 1].e1) {
          nack_to_read = 0;
          *total_size_pP = *total_size_pP - (sdu_size_t)((uint64_t)byte_pos_p + (uint64_t)((bit_pos + 7)/8) - (uint64_t)header_pP);
          return 0;
        }

        if (pdu_info_pP->num_nack == RLC_AM_MAX_NACK_IN_STATUS_PDU) {
          *total_size_pP = *total_size_pP - (sdu_size_t)((uint64_t)byte_pos_p + (uint64_t)((bit_pos + 7)/8) - (uint64_t)header_pP);
          return -2;
        }
      }

      *total_size_pP = *total_size_pP - (sdu_size_t)((uint64_t)byte_pos_p + (uint64_t)((bit_pos + 7)/8) - (uint64_t)header_pP);
    } else {
      *total_size_pP = *total_size_pP - 2;
    }

    return 0;
  } else {
    return -1;
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_display_control_pdu_infos(
  const rlc_am_control_pdu_info_t* const pdu_info_pP
)
{
  int num_nack;

  if (!pdu_info_pP->d_c) {
    LOG_T(RLC, "CONTROL PDU ACK SN %04d", pdu_info_pP->ack_sn);

    for (num_nack = 0; num_nack < pdu_info_pP->num_nack; num_nack++) {
      if (pdu_info_pP->nack_list[num_nack].e2) {
        LOG_T(RLC, "\n\tNACK SN %04d SO START %05d SO END %05d",  pdu_info_pP->nack_list[num_nack].nack_sn,
              pdu_info_pP->nack_list[num_nack].so_start,
              pdu_info_pP->nack_list[num_nack].so_end);
      } else {
        LOG_T(RLC, "\n\tNACK SN %04d",  pdu_info_pP->nack_list[num_nack].nack_sn);
      }
    }

    LOG_T(RLC, "\n");
  } else {
    LOG_E(RLC, "CAN'T DISPLAY CONTROL INFO: PDU IS DATA PDU\n");
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_receive_process_control_pdu(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  mem_block_t* const tb_pP,
  uint8_t** first_byte_ppP,
  sdu_size_t * const tb_size_in_bytes_pP)
{
  rlc_am_pdu_sn_10_t *rlc_am_pdu_sn_10_p = (rlc_am_pdu_sn_10_t*)*first_byte_ppP;
  sdu_size_t          initial_pdu_size   = *tb_size_in_bytes_pP;

  if (rlc_am_get_control_pdu_infos(rlc_am_pdu_sn_10_p, tb_size_in_bytes_pP, &rlc_pP->control_pdu_info) >= 0) {

    rlc_am_tx_buffer_display(ctxt_pP, rlc_pP, " TX BUFFER BEFORE PROCESS OF STATUS PDU");
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" RX CONTROL PDU VT(A) %04d VT(S) %04d POLL_SN %04d ACK_SN %04d\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->vt_a,
          rlc_pP->vt_s,
          rlc_pP->poll_sn,
          rlc_pP->control_pdu_info.ack_sn);
    rlc_am_display_control_pdu_infos(&rlc_pP->control_pdu_info);

    rlc_sn_t        ack_sn    = rlc_pP->control_pdu_info.ack_sn;
    rlc_sn_t        sn_cursor = rlc_pP->vt_a;
    rlc_sn_t        nack_sn;
    unsigned int nack_index;

    // 5.2.1 Retransmission
    //
    // The transmitting side of an AM RLC entity can receive a negative acknowledgement (notification of reception failure
    // by its peer AM RLC entity) for an AMD PDU or a portion of an AMD PDU by the following:
    //     - STATUS PDU from its peer AM RLC entity.
    //
    // When receiving a negative acknowledgement for an AMD PDU or a portion of an AMD PDU by a STATUS PDU from
    // its peer AM RLC entity, the transmitting side of the AM RLC entity shall:
    //     - if the SN of the corresponding AMD PDU falls within the range VT(A) <= SN < VT(S):
    //         - consider the AMD PDU or the portion of the AMD PDU for which a negative acknowledgement was
    //           received for retransmission.

    // 5.2.2.2    Reception of a STATUS report
    // Upon reception of a STATUS report from the receiving RLC AM entity the
    // transmitting side of an AM RLC entity shall:
    // - if the STATUS report comprises a positive or negative
    //     acknowledgement for the RLC data PDU with sequence number equal to
    //     POLL_SN:
    //     - if t-PollRetransmit is running:
    //         - stop and reset t-PollRetransmit.
    assert(ack_sn < RLC_AM_SN_MODULO);
    assert(rlc_pP->control_pdu_info.num_nack < RLC_AM_MAX_NACK_IN_STATUS_PDU);

    if (rlc_am_in_tx_window(ctxt_pP, rlc_pP, ack_sn) > 0) {
      rlc_pP->num_nack_so = 0;
      rlc_pP->num_nack_sn = 0;

      if (rlc_pP->control_pdu_info.num_nack == 0) {
        while (sn_cursor != ack_sn) {
          if (sn_cursor == rlc_pP->poll_sn) {
            rlc_am_stop_and_reset_timer_poll_retransmit(ctxt_pP, rlc_pP);
          }

          rlc_am_ack_pdu(ctxt_pP, rlc_pP, sn_cursor);
          sn_cursor = (sn_cursor + 1)  & RLC_AM_SN_MASK;
        }
      } else {
        nack_index = 0;
        nack_sn   = rlc_pP->control_pdu_info.nack_list[nack_index].nack_sn;

        while (sn_cursor != ack_sn) {
          if (sn_cursor == rlc_pP->poll_sn) {
            rlc_am_stop_and_reset_timer_poll_retransmit(ctxt_pP, rlc_pP);
          }

          if (sn_cursor != nack_sn) {
            rlc_am_ack_pdu(ctxt_pP,
                           rlc_pP,
                           sn_cursor);
          } else {
            rlc_am_nack_pdu (ctxt_pP,
                             rlc_pP,
                             sn_cursor,
                             rlc_pP->control_pdu_info.nack_list[nack_index].so_start,
                             rlc_pP->control_pdu_info.nack_list[nack_index].so_end);

            nack_index = nack_index + 1;

            if (nack_index == rlc_pP->control_pdu_info.num_nack) {
              nack_sn = 0xFFFF; // value never reached by sn
            } else {
              nack_sn = rlc_pP->control_pdu_info.nack_list[nack_index].nack_sn;
            }
          }

          if ((nack_index <  rlc_pP->control_pdu_info.num_nack) && (nack_index > 0)) {
            if (rlc_pP->control_pdu_info.nack_list[nack_index].nack_sn != rlc_pP->control_pdu_info.nack_list[nack_index-1].nack_sn) {
              sn_cursor = (sn_cursor + 1)  & RLC_AM_SN_MASK;
            }
          } else {
            sn_cursor = (sn_cursor + 1)  & RLC_AM_SN_MASK;
          }
        }
      }
    } else {
      LOG_N(RLC, PROTOCOL_RLC_AM_CTXT_FMT" WARNING CONTROL PDU ACK SN OUT OF WINDOW\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
    }
  } else {
    LOG_W(RLC, PROTOCOL_RLC_AM_CTXT_FMT" ERROR IN DECODING CONTROL PDU\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
  }

  *first_byte_ppP = (uint8_t*)((uint64_t)*first_byte_ppP + initial_pdu_size - *tb_size_in_bytes_pP);

  free_mem_block(tb_pP);
  rlc_am_tx_buffer_display(ctxt_pP, rlc_pP, NULL);
}
//-----------------------------------------------------------------------------
int
rlc_am_write_status_pdu(
  const protocol_ctxt_t* const     ctxt_pP,
  rlc_am_entity_t *const           rlc_pP,
  rlc_am_pdu_sn_10_t* const        rlc_am_pdu_sn_10_pP,
  rlc_am_control_pdu_info_t* const pdu_info_pP)
{
  unsigned int bit_pos       = 4; // range from 0 (MSB/left) to 7 (LSB/right)
  uint8_t*        byte_pos_p    = &rlc_am_pdu_sn_10_pP->b1;
  unsigned int index         = 0;
  unsigned int num_bytes     = 0;

  rlc_am_write16_bit_field(&byte_pos_p, &bit_pos, 10, pdu_info_pP->ack_sn);

  if (pdu_info_pP->num_nack > 0) {
    rlc_am_write8_bit_field(&byte_pos_p, &bit_pos, 1, 1);
  } else {
    rlc_am_write8_bit_field(&byte_pos_p, &bit_pos, 1, 0);
  }

  for (index = 0; index < pdu_info_pP->num_nack ; index++) {
    rlc_am_write16_bit_field(&byte_pos_p, &bit_pos, 10, pdu_info_pP->nack_list[index].nack_sn);
    rlc_am_write8_bit_field(&byte_pos_p, &bit_pos, 1,  pdu_info_pP->nack_list[index].e1);
    rlc_am_write8_bit_field(&byte_pos_p, &bit_pos, 1,  pdu_info_pP->nack_list[index].e2);

    // if SO_START SO_END fields
    if (pdu_info_pP->nack_list[index].e2 > 0) {
      rlc_am_write16_bit_field(&byte_pos_p, &bit_pos, 15, pdu_info_pP->nack_list[index].so_start);
      rlc_am_write16_bit_field(&byte_pos_p, &bit_pos, 15, pdu_info_pP->nack_list[index].so_end);
    }
  }

  ptrdiff_t diff = byte_pos_p - &rlc_am_pdu_sn_10_pP->b1; // this is the difference in terms of typeof(byte_pos_p), which is uint8_t
  num_bytes = diff;

  if (bit_pos > 0) {
    num_bytes += 1;
  }

#if TRACE_RLC_AM_STATUS_CREATION
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" WROTE STATUS PDU %d BYTES\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        num_bytes);
#endif
  return num_bytes;
}
//-----------------------------------------------------------------------------
void
rlc_am_send_status_pdu(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP
)
{
  // When STATUS reporting has been triggered, the receiving side of an AM RLC entity shall:
  // - if t-StatusProhibit is not running:
  //     - at the first transmission opportunity indicated by lower layer, construct a STATUS PDU and deliver it to lower layer;
  // - else:
  //     - at the first transmission opportunity indicated by lower layer after t-StatusProhibit expires, construct a single
  //       STATUS PDU even if status reporting was triggered several times while t-StatusProhibit was running and
  //       deliver it to lower layer;
  //
  // When a STATUS PDU has been delivered to lower layer, the receiving side of an AM RLC entity shall:
  //     - start t-StatusProhibit.
  //
  // When constructing a STATUS PDU, the AM RLC entity shall:
  //     - for the AMD PDUs with SN such that VR(R) <= SN < VR(MR) that has not been completely received yet, in
  //       increasing SN of PDUs and increasing byte segment order within PDUs, starting with SN = VR(R) up to
  //       the point where the resulting STATUS PDU still fits to the total size of RLC PDU(s) indicated by lower layer:
  //         - for an AMD PDU for which no byte segments have been received yet::
  //             - include in the STATUS PDU a NACK_SN which is set to the SN of the AMD PDU;
  //         - for a continuous sequence of byte segments of a partly received AMD PDU that have not been received yet:
  //             - include in the STATUS PDU a set of NACK_SN, SOstart and SOend
  //     - set the ACK_SN to the SN of the next not received RLC Data PDU which is not indicated as missing in the
  //       resulting STATUS PDU.

  signed int                    nb_bits_to_transmit   = rlc_pP->nb_bytes_requested_by_mac << 3;
  rlc_am_control_pdu_info_t     control_pdu_info;
  rlc_am_pdu_info_t            *pdu_info_cursor_p     = NULL;
  rlc_sn_t                      previous_sn_cursor    = (rlc_pP->vr_r - 1) & RLC_AM_SN_MASK;
  rlc_sn_t                      sn_cursor             = 0;
  mem_block_t                  *cursor_p              = rlc_pP->receiver_buffer.head;
  int                           all_segments_received = 0;
  int                           waited_so             = 0;
  mem_block_t                  *tb_p                  = NULL;
  sdu_size_t                    pdu_size              = 0;

  memset(&control_pdu_info, 0, sizeof(rlc_am_control_pdu_info_t));
  // header size
  nb_bits_to_transmit = nb_bits_to_transmit - 15;
#if TRACE_RLC_AM_STATUS_CREATION
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] nb_bits_to_transmit %d (15 already allocated for header)\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        nb_bits_to_transmit);
  rlc_am_rx_list_display(rlc_pP, " DISPLAY BEFORE CONSTRUCTION OF STATUS REPORT");
#endif

  if (cursor_p != NULL) {
    pdu_info_cursor_p       = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
    sn_cursor             = pdu_info_cursor_p->sn;

    while (rlc_am_in_rx_window(ctxt_pP, rlc_pP, sn_cursor) == 0) {
      cursor_p                = cursor_p->next;
      previous_sn_cursor    = sn_cursor;

      pdu_info_cursor_p       = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
      sn_cursor             = pdu_info_cursor_p->sn;
#if TRACE_RLC_AM_STATUS_CREATION
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d FIND VR(R) <= SN sn_cursor %04d -> %04d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            __LINE__,
            previous_sn_cursor,
            sn_cursor);
#endif
    }

    // 12 bits = size of NACK_SN field + E1, E2 bits
    // 42 bits = size of NACK_SN field + SO_START, SO_END fields, E1, E2 bits
    while ((cursor_p != NULL) && (nb_bits_to_transmit >= 12)) {
      pdu_info_cursor_p       = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
      all_segments_received = ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->all_segments_received;
      sn_cursor             = pdu_info_cursor_p->sn;
#if TRACE_RLC_AM_STATUS_CREATION
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d LOOPING sn_cursor %04d previous sn_cursor %04d \n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            __LINE__,
            sn_cursor,
            previous_sn_cursor);
#endif

      // -------------------------------------------------------------------------------
      // case resegmentation : several PDUs related to the same SN queued in list
      // -------------------------------------------------------------------------------
      if (sn_cursor == previous_sn_cursor) {
        do {
          cursor_p = cursor_p->next;

          if (cursor_p != NULL) {
            pdu_info_cursor_p       = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
            all_segments_received = ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->all_segments_received;
            sn_cursor             = pdu_info_cursor_p->sn;
#if TRACE_RLC_AM_STATUS_CREATION
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d NOW sn_cursor %04d \n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  sn_cursor);
#endif
          } else {
            if (all_segments_received) {
              control_pdu_info.ack_sn = (sn_cursor + 1) & RLC_AM_SN_MASK;
#if TRACE_RLC_AM_STATUS_CREATION
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING ACK SN %04d \n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    control_pdu_info.ack_sn);
#endif
            } else {
              control_pdu_info.ack_sn = (previous_sn_cursor + 1) & RLC_AM_SN_MASK;
#if TRACE_RLC_AM_STATUS_CREATION
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING ACK SN %04d (CASE PREVIOUS SN)\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    control_pdu_info.ack_sn);
#endif
            }

            goto end_push_nack;
          }
        } while ((cursor_p != NULL) && (sn_cursor == previous_sn_cursor));
      }

      // -------------------------------------------------------------------------------
      // simple case, PDU(s) is/are missing
      // -------------------------------------------------------------------------------
      while (((previous_sn_cursor + 1) & RLC_AM_SN_MASK) != sn_cursor) {
        if (nb_bits_to_transmit > 12) {
          previous_sn_cursor = (previous_sn_cursor + 1) & RLC_AM_SN_MASK;
          control_pdu_info.nack_list[control_pdu_info.num_nack].nack_sn   = previous_sn_cursor;
          control_pdu_info.nack_list[control_pdu_info.num_nack].so_start  = 0;
          control_pdu_info.nack_list[control_pdu_info.num_nack].so_end    = 0x7ff;
          control_pdu_info.nack_list[control_pdu_info.num_nack].e1        = 1;
          control_pdu_info.nack_list[control_pdu_info.num_nack].e2        = 0;
          control_pdu_info.num_nack += 1;
          nb_bits_to_transmit = nb_bits_to_transmit - 12;
#if TRACE_RLC_AM_STATUS_CREATION
          LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING NACK %04d\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                __LINE__,
                previous_sn_cursor);
#endif
        } else {
          control_pdu_info.ack_sn = (previous_sn_cursor + 1) & RLC_AM_SN_MASK;
#if TRACE_RLC_AM_STATUS_CREATION
          LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d NO MORE BITS FOR SENDING NACK %04d -> ABORT AND SET FINAL ACK %04d\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                __LINE__,
                previous_sn_cursor,
                control_pdu_info.ack_sn);
#endif
          goto end_push_nack;
        }
      }

      // -------------------------------------------------------------------------------
      // not so simple case, a resegmented PDU(s) is missing
      // -------------------------------------------------------------------------------
      if (all_segments_received == 0) {
        waited_so = 0;
#if TRACE_RLC_AM_STATUS_CREATION
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] if (all_segments_received == 0) \n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
#endif

        do {
          if (pdu_info_cursor_p->so > waited_so) {
            if (nb_bits_to_transmit >= 42) {
              control_pdu_info.nack_list[control_pdu_info.num_nack].nack_sn   = sn_cursor;
              control_pdu_info.nack_list[control_pdu_info.num_nack].so_start  = waited_so;
              control_pdu_info.nack_list[control_pdu_info.num_nack].so_end    = pdu_info_cursor_p->so - 1;
              control_pdu_info.nack_list[control_pdu_info.num_nack].e1        = 1;
              control_pdu_info.nack_list[control_pdu_info.num_nack].e2        = 1;
              control_pdu_info.num_nack += 1;
              nb_bits_to_transmit = nb_bits_to_transmit - 42;
#if TRACE_RLC_AM_STATUS_CREATION
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING NACK %04d SO START %05d SO END %05d (CASE SO %d > WAITED SO %d)\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    __LINE__,
                    sn_cursor,
                    waited_so,
                    pdu_info_cursor_p->so - 1,
                    pdu_info_cursor_p->so,
                    waited_so);
#endif

              if (pdu_info_cursor_p->lsf == 1) { // last segment flag
                //waited_so = 0x7FF;
                waited_so = 0x7FFF;
#if TRACE_RLC_AM_STATUS_CREATION
                LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d SN %04d SET WAITED SO 0x7FFF)\n",
                      PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                      __LINE__, sn_cursor);
#endif
                //break;
              } else {
                waited_so = pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size;
#if TRACE_RLC_AM_STATUS_CREATION
                LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d SN %04d SET WAITED SO %d @1\n",
                      PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                      __LINE__,
                      sn_cursor,
                      waited_so);
#endif
              }
            } else {
              control_pdu_info.ack_sn = sn_cursor;
              goto end_push_nack;
            }
          } else { //else { // pdu_info_cursor_p->so <= waited_so
            waited_so = pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size;

            if (pdu_info_cursor_p->lsf == 1) { // last segment flag
              //waited_so = 0x7FF;
              waited_so = 0x7FFF;
            }

#if TRACE_RLC_AM_STATUS_CREATION
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d SN %04d SET WAITED SO %d @2\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  sn_cursor, waited_so);
#endif
          }

          cursor_p = cursor_p->next;

          if (cursor_p != NULL) {
            pdu_info_cursor_p       = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
            all_segments_received = ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->all_segments_received;
            previous_sn_cursor    = sn_cursor;
            sn_cursor             = pdu_info_cursor_p->sn;
          } else {
            // LG control_pdu_info.ack_sn = (previous_sn_cursor + 1) & RLC_AM_SN_MASK;
            control_pdu_info.ack_sn = previous_sn_cursor;
            goto end_push_nack;
          }
        } while ((cursor_p != NULL) && (sn_cursor == previous_sn_cursor));

        // may be last segment of PDU not received
        //if ((sn_cursor != previous_sn_cursor) && (waited_so != 0x7FF)) {
        if ((sn_cursor != previous_sn_cursor) && (waited_so != 0x7FFF)) {
          if (nb_bits_to_transmit >= 42) {
            control_pdu_info.nack_list[control_pdu_info.num_nack].nack_sn   = previous_sn_cursor;
            control_pdu_info.nack_list[control_pdu_info.num_nack].so_start  = waited_so;
            //control_pdu_info.nack_list[control_pdu_info.num_nack].so_end    = 0x7FF;
            control_pdu_info.nack_list[control_pdu_info.num_nack].so_end    = 0x7FFF;
            control_pdu_info.nack_list[control_pdu_info.num_nack].e1        = 1;
            control_pdu_info.nack_list[control_pdu_info.num_nack].e2        = 1;
            control_pdu_info.num_nack += 1;
            nb_bits_to_transmit = nb_bits_to_transmit - 42;
#if TRACE_RLC_AM_STATUS_CREATION
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING NACK %04d SO START %05d SO END %05d\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  previous_sn_cursor,
                  waited_so,
                  0x7FFF);
#endif
          } else {
            control_pdu_info.ack_sn = previous_sn_cursor;
            goto end_push_nack;
          }
        }

        waited_so = 0;
      } else {
        waited_so = 0;
        cursor_p = cursor_p->next;
        previous_sn_cursor = sn_cursor;
      }
    }

    control_pdu_info.ack_sn = (previous_sn_cursor + 1) & RLC_AM_SN_MASK;
  } else {
    control_pdu_info.ack_sn = rlc_pP->vr_r;
#if TRACE_RLC_AM_STATUS_CREATION
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING ACK %04d  = VR(R)\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          __LINE__,
          control_pdu_info.ack_sn);
#endif
  }

end_push_nack:

  if (control_pdu_info.num_nack > 0) {
    control_pdu_info.nack_list[control_pdu_info.num_nack - 1].e1  = 0;
  }

  //msg ("[FRAME %5u][%s][RLC_AM][MOD %u/%u][RB %u] nb_bits_to_transmit %d\n",
  //     rlc_pP->module_id, rlc_pP->rb_id, ctxt_pP->frame,nb_bits_to_transmit);

#if TRACE_RLC_AM_STATUS_CREATION
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING ACK %04d NUM NACK %d\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        __LINE__,
        control_pdu_info.ack_sn,
        control_pdu_info.num_nack);
#endif
  // encode the control pdu
  pdu_size = rlc_pP->nb_bytes_requested_by_mac - ((nb_bits_to_transmit - 7 )>> 3);

#if TRACE_RLC_AM_STATUS_CREATION
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d forecast pdu_size %d\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        __LINE__,
        pdu_size);
#endif
  tb_p = get_free_mem_block(sizeof(struct mac_tb_req) + pdu_size);
  memset(tb_p->data, 0, sizeof(struct mac_tb_req) + pdu_size);
  //estimation only ((struct mac_tb_req*)(tb_p->data))->tb_size  = pdu_size;
  ((struct mac_tb_req*)(tb_p->data))->data_ptr         = (uint8_t*)&(tb_p->data[sizeof(struct mac_tb_req)]);

  // warning reuse of pdu_size
  pdu_size = rlc_am_write_status_pdu(ctxt_pP, rlc_pP,(rlc_am_pdu_sn_10_t*)(((struct mac_tb_req*)(tb_p->data))->data_ptr), &control_pdu_info);
  ((struct mac_tb_req*)(tb_p->data))->tb_size  = pdu_size;
  //assert((((struct mac_tb_req*)(tb_p->data))->tb_size) < 3000);

#if TRACE_RLC_AM_STATUS_CREATION
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] SEND STATUS PDU SIZE %d, rlc_pP->nb_bytes_requested_by_mac %d, nb_bits_to_transmit>>3 %d\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        pdu_size,
        rlc_pP->nb_bytes_requested_by_mac,
        nb_bits_to_transmit >> 3);
#endif
  assert(pdu_size == (rlc_pP->nb_bytes_requested_by_mac - (nb_bits_to_transmit >> 3)));

  // remaining bytes to transmit for RLC (retrans pdus and new data pdus)
  rlc_pP->nb_bytes_requested_by_mac = rlc_pP->nb_bytes_requested_by_mac - pdu_size;
  // put pdu in trans
  list_add_head(tb_p, &rlc_pP->control_pdu_list);
  rlc_pP->stat_tx_control_pdu   += 1;
  rlc_pP->stat_tx_control_bytes += pdu_size;

}
