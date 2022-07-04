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

#define RLC_AM_MODULE 1
#define RLC_AM_STATUS_REPORT_C 1
//-----------------------------------------------------------------------------
#include <string.h>
//-----------------------------------------------------------------------------
#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "intertask_interface.h"
#include "assertions.h"
#include "list.h"
#include "rlc_am.h"
#include "common/utils/LOG/log.h"


//-----------------------------------------------------------------------------
uint16_t rlc_am_read_bit_field(
  uint8_t **data_ppP,
  unsigned int *bit_pos_pP,
  const signed int bits_to_readP) {
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
  uint8_t **data_ppP,
  unsigned int *bit_pos_pP,
  const signed int bits_to_writeP,
  const uint8_t valueP) {
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
  uint8_t **data_ppP,
  unsigned int *bit_pos_pP,
  signed int bits_to_writeP,
  const uint16_t valueP) {
  //assert(bits_to_writeP <= 16);
  if(bits_to_writeP > 16) {
    LOG_E(RLC, "bits_to_writeP error. %d\n", bits_to_writeP);
  }

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
  rlc_am_pdu_sn_10_t *const        header_pP,
  sdu_size_t *const               total_size_pP,
  rlc_am_control_pdu_info_t *const pdu_info_pP) {
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
      uint8_t        *byte_pos_p      = &header_pP->b2;

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

          if (*total_size_pP != 0) {
            LOG_E(RLC, "[RLC_AM_GET_CONTROL_PDU_INFOS][FIRST]header_pP->b1=%d,header_pP->b2=%d\n",header_pP->b1,header_pP->b2);
          }

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

    if (*total_size_pP != 0) {
      LOG_E(RLC, "[RLC_AM_GET_CONTROL_PDU_INFOS][SECOND]header_pP->b1=%d,header_pP->b2=%d\n",header_pP->b1,header_pP->b2);
    }

    return 0;
  } else {
    return -1;
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_display_control_pdu_infos(
  const rlc_am_control_pdu_info_t *const pdu_info_pP
) {
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
  const protocol_ctxt_t *const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  mem_block_t *const tb_pP,
  uint8_t **first_byte_ppP,
  sdu_size_t *const tb_size_in_bytes_pP) {
  rlc_am_pdu_sn_10_t *rlc_am_pdu_sn_10_p = (rlc_am_pdu_sn_10_t *)*first_byte_ppP;
  sdu_size_t          initial_pdu_size   = *tb_size_in_bytes_pP;
  rlc_sn_t        ack_sn    = RLC_AM_NEXT_SN(rlc_pP->vt_a);
  rlc_sn_t        sn_cursor = rlc_pP->vt_a;
  rlc_sn_t    vt_a_new  = rlc_pP->vt_a;
  rlc_sn_t    sn_data_cnf = (rlc_sn_t) 0;
  rlc_sn_t        nack_sn,prev_nack_sn;
  sdu_size_t    data_cnf_so_stop = 0x7FFF;
  unsigned int nack_index;
  bool status = true;

  if (rlc_am_get_control_pdu_infos(rlc_am_pdu_sn_10_p, tb_size_in_bytes_pP, &rlc_pP->control_pdu_info) >= 0) {
    rlc_am_tx_buffer_display(ctxt_pP, rlc_pP, " TX BUFFER BEFORE PROCESS OF STATUS PDU");
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" RX CONTROL PDU VT(A) %04d VT(S) %04d POLL_SN %04d ACK_SN %04d\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->vt_a,
          rlc_pP->vt_s,
          rlc_pP->poll_sn,
          rlc_pP->control_pdu_info.ack_sn);
    rlc_am_display_control_pdu_infos(&rlc_pP->control_pdu_info);
    ack_sn    = rlc_pP->control_pdu_info.ack_sn;
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
    //assert(ack_sn < RLC_AM_SN_MODULO);
    //assert(rlc_pP->control_pdu_info.num_nack < RLC_AM_MAX_NACK_IN_STATUS_PDU);
    if(ack_sn >= RLC_AM_SN_MODULO || rlc_pP->control_pdu_info.num_nack >= RLC_AM_MAX_NACK_IN_STATUS_PDU) {
      LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT" illegal ack_sn %d, num_nack %d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP), ack_sn, rlc_pP->control_pdu_info.num_nack);
      return;
    }

    /* Note : ackSn can be equal to current vtA only in case the status pdu contains a list of nack_sn with same value = vtA with SOStart/SOEnd */
    /* and meaning the report is not complete due to not enough ressources to fill all SOStart/SOEnd of this NACK_SN */
    if (RLC_AM_DIFF_SN(rlc_pP->vt_s,rlc_pP->vt_a) >= RLC_AM_DIFF_SN(ack_sn,rlc_pP->vt_a)) {
      if (rlc_pP->control_pdu_info.num_nack == 0) {
        while (sn_cursor != ack_sn) {
          rlc_am_ack_pdu(ctxt_pP, rlc_pP, sn_cursor, true);
          sn_cursor = RLC_AM_NEXT_SN(sn_cursor);
        }

        vt_a_new = ack_sn;
        sn_data_cnf = RLC_AM_PREV_SN(vt_a_new);
      } else {
        nack_index = 0;
        nack_sn   = rlc_pP->control_pdu_info.nack_list[nack_index].nack_sn;
        prev_nack_sn = 0x3FFF;

        while (sn_cursor != nack_sn) {
          rlc_am_ack_pdu(ctxt_pP, rlc_pP, sn_cursor, true);
          sn_cursor = RLC_AM_NEXT_SN(sn_cursor);
        }

        vt_a_new = nack_sn;
        // catch DataCfn
        rlc_am_tx_data_pdu_management_t *tx_data_pdu_buffer_p = &rlc_pP->tx_data_pdu_buffer[nack_sn % RLC_AM_WINDOW_SIZE];

        if (tx_data_pdu_buffer_p->retx_payload_size == tx_data_pdu_buffer_p->payload_size) {
          sn_data_cnf   = RLC_AM_PREV_SN(nack_sn);
        } else if (tx_data_pdu_buffer_p->nack_so_start != 0) {
          sn_data_cnf       = nack_sn;
          data_cnf_so_stop    = tx_data_pdu_buffer_p->nack_so_start - 1;
        } else {
          sn_data_cnf       = RLC_AM_PREV_SN(nack_sn);
        }

        while ((sn_cursor != ack_sn) && (status)) {
          if (sn_cursor != nack_sn) {
            rlc_am_ack_pdu(ctxt_pP,
                           rlc_pP,
                           sn_cursor,
                           false);
          } else {
            status = rlc_am_nack_pdu (ctxt_pP,
                                      rlc_pP,
                                      nack_sn,
                                      prev_nack_sn,
                                      rlc_pP->control_pdu_info.nack_list[nack_index].so_start,
                                      rlc_pP->control_pdu_info.nack_list[nack_index].so_end);
            nack_index = nack_index + 1;
            prev_nack_sn = nack_sn;

            if (nack_index < rlc_pP->control_pdu_info.num_nack) {
              nack_sn = rlc_pP->control_pdu_info.nack_list[nack_index].nack_sn;
            } else if (nack_sn != ack_sn) {
              /* general case*/
              nack_sn = ack_sn;
            } else {
              /*specific case when the sender did not have enough TBS to fill all SOStart SOEnd for this NACK_SN */
              break;
            }
          }

          if (prev_nack_sn != nack_sn) {
            /* do not increment sn_cursor in case of several informations for the same nack_sn */
            sn_cursor = (sn_cursor + 1)  & RLC_AM_SN_MASK;
          }
        }
      }
    } else {
      LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT" WARNING CONTROL PDU ACK SN %d OUT OF WINDOW vtA=%d vtS=%d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),ack_sn,rlc_pP->vt_a,rlc_pP->vt_s);
      *tb_size_in_bytes_pP = 0;
      status = false;
    }
  } else {
    LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT" ERROR IN DECODING CONTROL PDU\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
    *tb_size_in_bytes_pP = 0;
    status = false;
  }

  if (status) {
    /* Check for Stopping TpollReTx */
    if ((rlc_pP->poll_sn != RLC_SN_UNDEFINED) &&
        (RLC_AM_DIFF_SN(ack_sn,rlc_pP->vt_a) > RLC_AM_DIFF_SN(rlc_pP->poll_sn,rlc_pP->vt_a))) {
      rlc_am_stop_and_reset_timer_poll_retransmit(ctxt_pP, rlc_pP);
      rlc_pP->poll_sn = RLC_SN_UNDEFINED;
    }

    //TODO : this part does not cover all cases of Data Cnf and move it at the end of Status PDU processing
    sn_cursor = rlc_pP->vt_a;

    // Fix Issue 238 : check sn_data_cnf has been transmitted
    if ((rlc_pP->tx_data_pdu_buffer[sn_data_cnf % RLC_AM_WINDOW_SIZE].flags.transmitted) &&
        (rlc_pP->tx_data_pdu_buffer[sn_data_cnf % RLC_AM_WINDOW_SIZE].sn == sn_data_cnf)) {
      /* Handle all acked PDU up to and excluding sn_data_cnf */
      while (sn_cursor != sn_data_cnf) {
        rlc_am_pdu_sdu_data_cnf(ctxt_pP,rlc_pP,sn_cursor);
        sn_cursor = RLC_AM_NEXT_SN(sn_cursor);
      }

      // Handle last SN. TO DO : case of PDU partially ACKED with SDU to be data conf
      if (data_cnf_so_stop == 0x7FFF) {
        rlc_am_pdu_sdu_data_cnf(ctxt_pP,rlc_pP,sn_data_cnf);
      }

      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" RECEIVE STATUS PDU ACK_SN=%d NewvtA=%d OldvtA=%d SnDataCnf=%d DataCnfSOStop=%d vtS=%d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),ack_sn,vt_a_new,rlc_pP->vt_a,sn_data_cnf,data_cnf_so_stop,rlc_pP->vt_s);
    } else {
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" RECEIVE STATUS PDU WITH NO SDU CNF ACK_SN=%d NewvtA=%d OldvtA=%d vtS=%d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),ack_sn,vt_a_new,rlc_pP->vt_a,rlc_pP->vt_s);
    }

    /* Update vtA and vtMS */
    rlc_pP->vt_a = vt_a_new;
    rlc_pP->vt_ms = (rlc_pP->vt_a + RLC_AM_WINDOW_SIZE) & RLC_AM_SN_MASK;
  }

  *first_byte_ppP = (uint8_t *)((uint64_t)*first_byte_ppP + initial_pdu_size - *tb_size_in_bytes_pP);
  free_mem_block(tb_pP, __func__);
  rlc_am_tx_buffer_display(ctxt_pP, rlc_pP, NULL);
}
//-----------------------------------------------------------------------------
int
rlc_am_write_status_pdu(
  const protocol_ctxt_t *const     ctxt_pP,
  rlc_am_entity_t *const           rlc_pP,
  rlc_am_pdu_sn_10_t *const        rlc_am_pdu_sn_10_pP,
  rlc_am_control_pdu_info_t *const pdu_info_pP) {
  unsigned int bit_pos       = 4; // range from 0 (MSB/left) to 7 (LSB/right)
  uint8_t        *byte_pos_p    = &rlc_am_pdu_sn_10_pP->b1;
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

  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" WROTE STATUS PDU %d BYTES\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        num_bytes);
  return num_bytes;
}
//-----------------------------------------------------------------------------
void
rlc_am_send_status_pdu(
  const protocol_ctxt_t *const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP
) {
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
  // minimum header size in bits to be transmitted: D/C + CPT + ACK_SN + E1
  signed int                    nb_bits_transmitted   = RLC_AM_PDU_D_C_BITS + RLC_AM_STATUS_PDU_CPT_LENGTH + RLC_AM_SN_BITS + RLC_AM_PDU_E_BITS;
  rlc_am_control_pdu_info_t     control_pdu_info;
  rlc_am_pdu_info_t            *pdu_info_cursor_p     = NULL;
  rlc_sn_t                      sn_cursor             = 0;
  rlc_sn_t                      sn_nack               = rlc_pP->vr_r;
  mem_block_t                  *cursor_p              = rlc_pP->receiver_buffer.head;
  int                           all_segments_received = 0;
  int                           waited_so             = 0;
  mem_block_t                  *tb_p                  = NULL;
  sdu_size_t                    pdu_size              = 0;
  bool                          status_report_completed = false;
  bool                          segment_loop_end    = false;
  memset(&control_pdu_info, 0, sizeof(rlc_am_control_pdu_info_t));
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] nb_bits_to_transmit %d (15 already allocated for header)\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        nb_bits_to_transmit);
  rlc_am_rx_list_display(rlc_pP, " DISPLAY BEFORE CONSTRUCTION OF STATUS REPORT");

  /* Handle no NACK first */
  if (rlc_pP->vr_r == rlc_pP->vr_ms) {
    control_pdu_info.ack_sn = rlc_pP->vr_ms;
    status_report_completed = true;
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d ALL ACK WITH ACK_SN %04d\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          __LINE__,
          rlc_pP->vr_ms);
  } else if ((cursor_p != NULL) && ((nb_bits_transmitted + RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1)) <= nb_bits_to_transmit)) {
    pdu_info_cursor_p       = &((rlc_am_rx_pdu_management_t *)(cursor_p->data))->pdu_info;
    sn_cursor             = pdu_info_cursor_p->sn;
    /* Set E1 bit for the presence of first NACK_SN/E1/E2 */
    control_pdu_info.e1 = 1;

    // 12 bits = size of NACK_SN field + E1, E2 bits
    // 42 bits = size of NACK_SN field + SO_START, SO_END fields, E1, E2 bits
    while ((!status_report_completed) && (RLC_AM_DIFF_SN(sn_nack,rlc_pP->vr_r) < RLC_AM_DIFF_SN(rlc_pP->vr_ms,rlc_pP->vr_r))
           && (cursor_p != NULL) && (nb_bits_transmitted <= nb_bits_to_transmit)) {
      pdu_info_cursor_p       = &((rlc_am_rx_pdu_management_t *)(cursor_p->data))->pdu_info;
      sn_cursor             = pdu_info_cursor_p->sn;
      all_segments_received = ((rlc_am_rx_pdu_management_t *)(cursor_p->data))->all_segments_received;

      /* First fill NACK_SN with each missing PDU between current sn_nack and sn_cursor */
      while ((sn_nack != sn_cursor) && (RLC_AM_DIFF_SN(sn_nack,rlc_pP->vr_r) < RLC_AM_DIFF_SN(rlc_pP->vr_ms,rlc_pP->vr_r))) {
        if (nb_bits_transmitted + RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) <= nb_bits_to_transmit) {
          /* Fill NACK_SN infos */
          control_pdu_info.nack_list[control_pdu_info.num_nack].nack_sn   = sn_nack;
          control_pdu_info.nack_list[control_pdu_info.num_nack].so_start  = 0;
          control_pdu_info.nack_list[control_pdu_info.num_nack].so_end    = RLC_AM_STATUS_PDU_SO_END_ALL_BYTES;
          control_pdu_info.nack_list[control_pdu_info.num_nack].e2        = 0;
          /* Set E1 for next NACK_SN. The last one will be cleared */
          control_pdu_info.nack_list[control_pdu_info.num_nack].e1  = 1;
          control_pdu_info.num_nack += 1;
          nb_bits_transmitted += (RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1));
          LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING NACK %04d\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                __LINE__,
                sn_nack);
          sn_nack = RLC_AM_NEXT_SN(sn_nack);
        } else {
          /* Not enough UL TBS*/
          /* latest value of sn_nack shall be used as ACK_SN */
          control_pdu_info.ack_sn = sn_nack;
          status_report_completed = true;
          LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d NOT ENOUGH TBS STOP WITH ACK_SN %04d\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                __LINE__,
                sn_nack);
          break;
        }
      }

      if (sn_nack == rlc_pP->vr_ms) {
        break;
      }

      /* Now process all Segments of sn_cursor if PDU not fully received */
      if ((!status_report_completed) && (all_segments_received == 0) && (sn_cursor != rlc_pP->vr_ms)) {
        //AssertFatal (sn_nack == sn_cursor, "RLC AM Tx Status PDU Data sn_nack=%d and sn_cursor=%d should be equal LcId=%d\n",sn_nack,sn_cursor, rlc_pP->channel_id);
        if(sn_nack != sn_cursor) {
          LOG_E(RLC, "RLC AM Tx Status PDU Data sn_nack=%d and sn_cursor=%d should be equal LcId=%d\n",sn_nack,sn_cursor, rlc_pP->channel_id);
        }
        /* First ensure there is enough TBS for at least 1 SOStart/SOEnd, else break */
        else if ((nb_bits_transmitted + RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1)) <= nb_bits_to_transmit) {
          /* Init loop flags */
          /* Check lsf */
          segment_loop_end = (pdu_info_cursor_p->lsf == 1);

          /* Init first SO Start according to first segment */
          if (pdu_info_cursor_p->so) {
            /* Fill the first SO */
            control_pdu_info.nack_list[control_pdu_info.num_nack].nack_sn   = sn_cursor;
            control_pdu_info.nack_list[control_pdu_info.num_nack].so_start  = 0;
            control_pdu_info.nack_list[control_pdu_info.num_nack].so_end    = pdu_info_cursor_p->so - 1;
            control_pdu_info.nack_list[control_pdu_info.num_nack].e2        = 1;
            /* Set E1 for next NACK_SN. The last one will be cleared */
            control_pdu_info.nack_list[control_pdu_info.num_nack].e1  = 1;
            control_pdu_info.num_nack += 1;
            nb_bits_transmitted += (RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1));
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING NACK %04d SO START %05d SO END %05d\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  __LINE__,
                  sn_cursor,
                  0,
                  pdu_info_cursor_p->so - 1);
            waited_so = pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size;
          } else {
            waited_so = pdu_info_cursor_p->payload_size;
          }

          /* Go to next segment */
          cursor_p = cursor_p->next;

          if (cursor_p != NULL) {
            pdu_info_cursor_p     = &((rlc_am_rx_pdu_management_t *)(cursor_p->data))->pdu_info;
          }

          /* Find the first discontinuity and then fill SOStart/SOEnd */
          while (!segment_loop_end) {
            if ((cursor_p != NULL) && (pdu_info_cursor_p->sn == sn_cursor)) {
              /* PDU segment is for the same SN*/
              /* Check lsf */
              segment_loop_end = (pdu_info_cursor_p->lsf == 1);

              if (waited_so < pdu_info_cursor_p->so) {
                /* SO is greater than previous received portion : gap identified to fill */
                if ((nb_bits_transmitted + RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1)) <= nb_bits_to_transmit) {
                  control_pdu_info.nack_list[control_pdu_info.num_nack].nack_sn   = sn_cursor;
                  control_pdu_info.nack_list[control_pdu_info.num_nack].so_start  = waited_so;
                  control_pdu_info.nack_list[control_pdu_info.num_nack].so_end    = pdu_info_cursor_p->so - 1;
                  control_pdu_info.nack_list[control_pdu_info.num_nack].e2        = 1;
                  /* Set E1 for next NACK_SN. The last one will be cleared */
                  control_pdu_info.nack_list[control_pdu_info.num_nack].e1  = 1;
                  control_pdu_info.num_nack += 1;
                  nb_bits_transmitted += (RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1));
                  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING NACK %04d SO START %05d SO END %05d\n",
                        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                        __LINE__,
                        sn_cursor,
                        waited_so,
                        pdu_info_cursor_p->so);
                } else {
                  /* Not enough resources to set a SOStart/SEnd, then set ACK_SN to current NACK_SN and stop Status PDU build */
                  control_pdu_info.ack_sn = sn_cursor;
                  status_report_completed = true;
                  segment_loop_end = true;
                  break;
                }
              } else {
                /* contiguous segment: only update waited_so */
                /* Assuming so and payload_size updated according to duplication removal done at reception ... */
                waited_so += pdu_info_cursor_p->payload_size;
              }

              /* Go to next received PDU or PDU Segment */
              cursor_p = cursor_p->next;

              if (cursor_p != NULL) {
                pdu_info_cursor_p     = &((rlc_am_rx_pdu_management_t *)(cursor_p->data))->pdu_info;
              }
            } //end if (cursor_p != NULL) && (pdu_info_cursor_p->sn == sn_cursor)
            else {
              /* Previous PDU segment was the last one and did not have lsf indication : fill the latest gap */
              if ((nb_bits_transmitted + RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1)) <= nb_bits_to_transmit) {
                control_pdu_info.nack_list[control_pdu_info.num_nack].nack_sn   = sn_cursor;
                control_pdu_info.nack_list[control_pdu_info.num_nack].so_start  = waited_so;
                control_pdu_info.nack_list[control_pdu_info.num_nack].so_end    = RLC_AM_STATUS_PDU_SO_END_ALL_BYTES;
                control_pdu_info.nack_list[control_pdu_info.num_nack].e2        = 1;
                /* Set E1 for next NACK_SN. The last one will be cleared */
                control_pdu_info.nack_list[control_pdu_info.num_nack].e1  = 1;
                control_pdu_info.num_nack += 1;
                nb_bits_transmitted += (RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1));
                LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING LAST NACK %04d SO START %05d SO END %05d\n",
                      PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                      __LINE__,
                      sn_cursor,
                      waited_so,
                      RLC_AM_STATUS_PDU_SO_END_ALL_BYTES);
              } else {
                /* Not enough resources to set a SOStart/SEnd, then set ACK_SN to current NACK_SN and stop Status PDU build */
                control_pdu_info.ack_sn = sn_cursor;
                status_report_completed = true;
              }

              segment_loop_end = true;
            }
          } //end  while (!segment_loop_end)
        } // end if enough resource for transmitting at least one SOStart/SOEnd
        else {
          /* Not enough UL TBS to set at least one SOStart/SOEnd */
          /* latest value of sn_nack shall be used as ACK_SN */
          control_pdu_info.ack_sn = sn_nack;
          status_report_completed = true;
        }
      } // end while on all PDU segments of sn_cursor
      else {
        /* Go to next received PDU or PDU segment with different SN */
        do {
          cursor_p = cursor_p->next;
        } while ((cursor_p != NULL) && (((rlc_am_rx_pdu_management_t *)(cursor_p->data))->pdu_info.sn == sn_cursor));
      }

      /* Increment sn_nack except if sn_cursor = vrMS and if current SN was not fully received */
      if (RLC_AM_DIFF_SN(sn_cursor,rlc_pP->vr_r) < RLC_AM_DIFF_SN(rlc_pP->vr_ms,rlc_pP->vr_r)) {
        sn_nack = RLC_AM_NEXT_SN(sn_cursor);
      } else {
        sn_nack = rlc_pP->vr_ms;
      }
    } // End main while NACK_SN

    /* Clear E1 of last nack_sn entry */
    //  AssertFatal ((control_pdu_info.num_nack) || (all_segments_received == 0), "RLC AM Tx Status PDU Data Error no NACK_SN vrR=%d vrMS=%d lastSN_NACK=%d Completed=%d NbBytesAvailable=%d LcId=%d\n",
    //          rlc_pP->vr_r,rlc_pP->vr_ms,sn_nack,status_report_completed,(nb_bits_to_transmit >> 3),rlc_pP->channel_id);
    if (!((control_pdu_info.num_nack) || (all_segments_received == 0))) {
      LOG_E(RLC, "RLC AM Tx Status PDU Data Error no NACK_SN vrR=%d vrMS=%d lastSN_NACK=%d Completed=%d NbBytesAvailable=%d LcId=%d\n",
            rlc_pP->vr_r,rlc_pP->vr_ms,sn_nack,status_report_completed,(nb_bits_to_transmit >> 3),rlc_pP->channel_id);
      return;
    }

    if (control_pdu_info.num_nack) {
      control_pdu_info.nack_list[control_pdu_info.num_nack - 1].e1  = 0;
    }

    /* Set ACK_SN unless it was set before */
    if (!status_report_completed) {
      control_pdu_info.ack_sn = sn_nack;
    }
  } else {
    /* reception buffer empty or not enough TBS for filling at least 1 NACK_SN + E1 + E2 */
    control_pdu_info.ack_sn = rlc_pP->vr_r;
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING ACK %04d  = VR(R)\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          __LINE__,
          control_pdu_info.ack_sn);
  }

  //msg ("[FRAME %5u][%s][RLC_AM][MOD %u/%u][RB %u] nb_bits_to_transmit %d\n",
  //     rlc_pP->module_id, rlc_pP->rb_id, ctxt_pP->frame,nb_bits_to_transmit);
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d PREPARE SENDING ACK %04d NUM NACK %d\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        __LINE__,
        control_pdu_info.ack_sn,
        control_pdu_info.num_nack);
  /* encode the control pdu */
  pdu_size = (nb_bits_transmitted + 7) >> 3;

  //  AssertFatal (pdu_size <= rlc_pP->nb_bytes_requested_by_mac, "RLC AM Tx Status PDU Data size=%d bigger than remaining TBS=%d nb_bits_transmitted=%d LcId=%d\n",
  //      pdu_size,rlc_pP->nb_bytes_requested_by_mac,nb_bits_transmitted, rlc_pP->channel_id);
  if(pdu_size > rlc_pP->nb_bytes_requested_by_mac) {
    LOG_E(RLC, "RLC AM Tx Status PDU Data size=%d bigger than remaining TBS=%d nb_bits_transmitted=%d LcId=%d\n",
          pdu_size,rlc_pP->nb_bytes_requested_by_mac,nb_bits_transmitted, rlc_pP->channel_id);
    return;
  }

  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] LINE %d forecast pdu_size %d\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        __LINE__,
        pdu_size);
  tb_p = get_free_mem_block(sizeof(struct mac_tb_req) + pdu_size, __func__);

  if(tb_p == NULL) return;

  memset(tb_p->data, 0, sizeof(struct mac_tb_req) + pdu_size);
  //estimation only ((struct mac_tb_req*)(tb_p->data))->tb_size  = pdu_size;
  ((struct mac_tb_req *)(tb_p->data))->data_ptr         = (uint8_t *)&(tb_p->data[sizeof(struct mac_tb_req)]);
  // warning reuse of pdu_size
  // TODO : rlc_am_write_status_pdu should be rewritten as not very tested ...
  pdu_size = rlc_am_write_status_pdu(ctxt_pP, rlc_pP,(rlc_am_pdu_sn_10_t *)(((struct mac_tb_req *)(tb_p->data))->data_ptr), &control_pdu_info);
  ((struct mac_tb_req *)(tb_p->data))->tb_size  = pdu_size;
  //assert((((struct mac_tb_req*)(tb_p->data))->tb_size) < 3000);
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND-STATUS] SEND STATUS PDU SIZE %d, rlc_pP->nb_bytes_requested_by_mac %d, nb_bits_to_transmit>>3 %d\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        pdu_size,
        rlc_pP->nb_bytes_requested_by_mac,
        nb_bits_to_transmit >> 3);

  //  AssertFatal (pdu_size == ((nb_bits_transmitted + 7) >> 3), "RLC AM Tx Status PDU Data encoding size=%d different than expected=%d LcId=%d\n",
  //        pdu_size,((nb_bits_transmitted + 7) >> 3), rlc_pP->channel_id);
  if(pdu_size != ((nb_bits_transmitted + 7) >> 3)) {
    LOG_E(RLC, "RLC AM Tx Status PDU Data encoding size=%d different than expected=%d LcId=%d\n",
          pdu_size,((nb_bits_transmitted + 7) >> 3), rlc_pP->channel_id);
    pdu_size = 0;
    return;
  }

  // remaining bytes to transmit for RLC (retrans pdus and new data pdus)
  rlc_pP->nb_bytes_requested_by_mac = rlc_pP->nb_bytes_requested_by_mac - pdu_size;
  // put pdu in trans
  list_add_head(tb_p, &rlc_pP->control_pdu_list);
  rlc_pP->stat_tx_control_pdu   += 1;
  rlc_pP->stat_tx_control_bytes += pdu_size;
}


