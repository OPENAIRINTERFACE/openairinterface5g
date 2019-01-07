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

#define RLC_UM_MODULE 1
#define RLC_UM_SEGMENT_C 1
//-----------------------------------------------------------------------------
#include "platform_types.h"
#include "platform_constants.h"
//-----------------------------------------------------------------------------
#include <assert.h>
#include "assertions.h"
#include "msc.h"
#include "list.h"
#include "rlc_um.h"
#include "rlc_primitives.h"
#include "common/utils/LOG/log.h"

//-----------------------------------------------------------------------------
void
rlc_um_segment_10 (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t *rlc_pP)
{
  list_t              pdus;
  signed int          pdu_remaining_size;
  signed int          test_pdu_remaining_size;

  int                 nb_bytes_to_transmit = rlc_pP->nb_bytes_requested_by_mac;
  rlc_um_pdu_sn_10_t *pdu_p;
  struct mac_tb_req  *pdu_tb_req_p;
  mem_block_t        *pdu_mem_p;
  char               *data;
  char               *data_sdu_p;
  rlc_um_e_li_t      *e_li_p;
  struct rlc_um_tx_sdu_management *sdu_mngt_p;
  unsigned int       li_length_in_bytes;
  unsigned int       test_li_length_in_bytes;
  unsigned int       test_remaining_size_to_substract;
  unsigned int       test_remaining_num_li_to_substract;
  unsigned int       continue_fill_pdu_with_sdu;
  unsigned int       num_fill_sdu;
  unsigned int       test_num_li;
  unsigned int       fill_num_li;
  mem_block_t        *sdu_in_buffer = NULL;
  unsigned int       data_pdu_size;

  unsigned int       fi_first_byte_pdu_is_first_byte_sdu;
  unsigned int       fi_last_byte_pdu_is_last_byte_sdu;
  unsigned int       fi;
  unsigned int       max_li_overhead;

  if (nb_bytes_to_transmit < 3) {
#if TRACE_RLC_UM_SEGMENT
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" NO SEGMENTATION nb_bytes to transmit = %d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          nb_bytes_to_transmit);
#endif
    return;
  }

  list_init (&pdus, NULL);    // param string identifying the list is NULL
  pdu_mem_p = NULL;

  // not fine locking
  RLC_UM_MUTEX_LOCK(&rlc_pP->lock_input_sdus, ctxt_pP, rlc_pP);

  while ((list_get_head(&rlc_pP->input_sdus)) && (nb_bytes_to_transmit > 0)) {

#if TRACE_RLC_UM_SEGMENT
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" nb_bytes_to_transmit %d BO %d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          nb_bytes_to_transmit,
          rlc_pP->buffer_occupancy);
#endif

    // pdu_p management
    if (!pdu_mem_p) {
      if (rlc_pP->input_sdus.nb_elements <= 1) {
        max_li_overhead = 0;
      } else {
        max_li_overhead = (((rlc_pP->input_sdus.nb_elements - 1) * 3) / 2) + ((rlc_pP->input_sdus.nb_elements - 1) % 2);
      }

      if  (nb_bytes_to_transmit >= (rlc_pP->buffer_occupancy + rlc_pP->tx_header_min_length_in_bytes + max_li_overhead)) {
        data_pdu_size = rlc_pP->buffer_occupancy + rlc_pP->tx_header_min_length_in_bytes + max_li_overhead;
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" alloc PDU size %d bytes to contain not all bytes requested by MAC but all BO of RLC@1\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              data_pdu_size);
#endif
      } else {
        data_pdu_size = nb_bytes_to_transmit;
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" alloc PDU size %d bytes to contain all bytes requested by MAC@1\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              data_pdu_size);
#endif
      }

      if (!(pdu_mem_p = get_free_mem_block (data_pdu_size + sizeof(struct mac_tb_req), __func__))) {
#if TRACE_RLC_UM_SEGMENT
        LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" ERROR COULD NOT GET NEW PDU, EXIT\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
#endif
        RLC_UM_MUTEX_UNLOCK(&rlc_pP->lock_input_sdus);
        return;
      }

#if TRACE_RLC_UM_SEGMENT
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" get new PDU %d bytes\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
            data_pdu_size);
#endif
      pdu_remaining_size = data_pdu_size - 2;
      pdu_p        = (rlc_um_pdu_sn_10_t*) (&pdu_mem_p->data[sizeof(struct mac_tb_req)]);
      pdu_tb_req_p = (struct mac_tb_req*) (pdu_mem_p->data);

      memset (pdu_mem_p->data, 0, sizeof (rlc_um_pdu_sn_10_t)+sizeof(struct mac_tb_req));
      li_length_in_bytes = 1;
    }

    //----------------------------------------
    // compute how many SDUS can fill the PDU
    //----------------------------------------
    continue_fill_pdu_with_sdu = 1;
    num_fill_sdu               = 0;
    test_num_li                = 0;
    sdu_in_buffer              = list_get_head(&rlc_pP->input_sdus);
    test_pdu_remaining_size    = pdu_remaining_size;
    test_li_length_in_bytes    = 1;
    test_remaining_size_to_substract   = 0;
    test_remaining_num_li_to_substract = 0;


    while ((sdu_in_buffer) && (continue_fill_pdu_with_sdu > 0)) {
      sdu_mngt_p = ((struct rlc_um_tx_sdu_management *) (sdu_in_buffer->data));

      if (sdu_mngt_p->sdu_remaining_size > test_pdu_remaining_size) {
        // no LI
        continue_fill_pdu_with_sdu = 0;
        num_fill_sdu += 1;
        test_pdu_remaining_size = 0;
        test_remaining_size_to_substract = 0;
        test_remaining_num_li_to_substract = 0;
      } else if (sdu_mngt_p->sdu_remaining_size == test_pdu_remaining_size) {
        // fi will indicate end of PDU is end of SDU, no need for LI
        continue_fill_pdu_with_sdu = 0;
        num_fill_sdu += 1;
        test_pdu_remaining_size = 0;
        test_remaining_size_to_substract = 0;
        test_remaining_num_li_to_substract = 0;
      } else if ((sdu_mngt_p->sdu_remaining_size + (test_li_length_in_bytes ^ 3)) == test_pdu_remaining_size ) {
        // no LI
        continue_fill_pdu_with_sdu = 0;
        num_fill_sdu += 1;
        test_pdu_remaining_size = 0;
        test_remaining_size_to_substract = 0;
        test_remaining_num_li_to_substract = 0;
        //pdu_remaining_size = pdu_remaining_size - (test_li_length_in_bytes ^ 3);
      } else if ((sdu_mngt_p->sdu_remaining_size + (test_li_length_in_bytes ^ 3)) < test_pdu_remaining_size ) {
        test_num_li += 1;
        num_fill_sdu += 1;
        test_pdu_remaining_size = test_pdu_remaining_size - (sdu_mngt_p->sdu_remaining_size + (test_li_length_in_bytes ^ 3));
        test_remaining_size_to_substract = test_li_length_in_bytes ^ 3;
        test_remaining_num_li_to_substract = 1;
        test_li_length_in_bytes = test_li_length_in_bytes ^ 3;
      } else {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" sdu_mngt_p->sdu_remaining_size=%d test_pdu_remaining_size=%d test_li_length_in_bytes=%d\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p->sdu_remaining_size,
              test_pdu_remaining_size,
              test_li_length_in_bytes ^ 3);
#endif
        // reduce the size of the PDU
        continue_fill_pdu_with_sdu = 0;
        num_fill_sdu += 1;
        test_pdu_remaining_size = 0;
        test_remaining_size_to_substract = 0;
        test_remaining_num_li_to_substract = 0;
        pdu_remaining_size = pdu_remaining_size - 1;
      }

      sdu_in_buffer = sdu_in_buffer->next;
    }

    if (test_remaining_num_li_to_substract > 0) {
      // there is a LI that is not necessary
      test_num_li = test_num_li - 1;
      pdu_remaining_size = pdu_remaining_size - test_remaining_size_to_substract;
    }

    //----------------------------------------
    // Do the real filling of the pdu_p
    //----------------------------------------
#if TRACE_RLC_UM_SEGMENT
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" data shift %d Bytes num_li %d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          ((test_num_li*3) +1) >> 1,
          test_num_li);
#endif

    data = ((char*)(&pdu_p->data[((test_num_li*3) +1) >> 1]));
    e_li_p = (rlc_um_e_li_t*)(pdu_p->data);
    continue_fill_pdu_with_sdu          = 1;
    li_length_in_bytes                  = 1;
    fill_num_li                         = 0;
    fi_first_byte_pdu_is_first_byte_sdu = 0;
    fi_last_byte_pdu_is_last_byte_sdu   = 0;

    sdu_in_buffer = list_get_head(&rlc_pP->input_sdus);

    if (
      ((struct rlc_um_tx_sdu_management *) (sdu_in_buffer->data))->sdu_remaining_size ==
      ((struct rlc_um_tx_sdu_management *) (sdu_in_buffer->data))->sdu_size) {
      fi_first_byte_pdu_is_first_byte_sdu = 1;
    }

    while ((sdu_in_buffer) && (continue_fill_pdu_with_sdu > 0)) {
      sdu_mngt_p = ((struct rlc_um_tx_sdu_management *) (sdu_in_buffer->data));

      if (sdu_mngt_p->sdu_segmented_size == 0) {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" GET NEW SDU %p AVAILABLE SIZE %d Bytes\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p,
              sdu_mngt_p->sdu_remaining_size);
#endif
      } else {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" GET AGAIN SDU %p REMAINING AVAILABLE SIZE %d Bytes / %d Bytes \n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p,
              sdu_mngt_p->sdu_remaining_size,
              sdu_mngt_p->sdu_size);
#endif
      }

      data_sdu_p = (char *) &(sdu_in_buffer->data[sizeof (struct rlc_um_tx_sdu_management) + sdu_mngt_p->sdu_segmented_size]);

      if (sdu_mngt_p->sdu_remaining_size > pdu_remaining_size) {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Filling all remaining PDU with %d bytes\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              pdu_remaining_size);
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" pdu_mem_p %p pdu_mem_p->data %p pdu_p %p pdu_p->data %p data %p data_sdu_p %p pdu_remaining_size %d\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              pdu_mem_p,
              pdu_mem_p->data,
              pdu_p,
              pdu_p->data,
              data,
              data_sdu_p,
              pdu_remaining_size);
#endif

        memcpy(data, data_sdu_p, pdu_remaining_size);
        sdu_mngt_p->sdu_remaining_size = sdu_mngt_p->sdu_remaining_size - pdu_remaining_size;
        sdu_mngt_p->sdu_segmented_size = sdu_mngt_p->sdu_segmented_size + pdu_remaining_size;
        fi_last_byte_pdu_is_last_byte_sdu = 0;
        // no LI
        rlc_pP->buffer_occupancy -= pdu_remaining_size;
        continue_fill_pdu_with_sdu = 0;
        pdu_remaining_size = 0;
      } else if (sdu_mngt_p->sdu_remaining_size == pdu_remaining_size) {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Exactly Filling remaining PDU with %d remaining bytes of SDU\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              pdu_remaining_size);
#endif
        memcpy(data, data_sdu_p, pdu_remaining_size);

        // free SDU
        rlc_pP->buffer_occupancy -= sdu_mngt_p->sdu_remaining_size;
        sdu_in_buffer = list_remove_head(&rlc_pP->input_sdus);
        free_mem_block (sdu_in_buffer, __func__);
        sdu_in_buffer = list_get_head(&rlc_pP->input_sdus);
        sdu_mngt_p    = NULL;


        fi_last_byte_pdu_is_last_byte_sdu = 1;
        // fi will indicate end of PDU is end of SDU, no need for LI
        continue_fill_pdu_with_sdu = 0;
        pdu_remaining_size = 0;
      } else if ((sdu_mngt_p->sdu_remaining_size + (li_length_in_bytes ^ 3)) < pdu_remaining_size ) {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Filling  PDU with %d all remaining bytes of SDU\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p->sdu_remaining_size);
#endif
        memcpy(data, data_sdu_p, sdu_mngt_p->sdu_remaining_size);
        data = &data[sdu_mngt_p->sdu_remaining_size];
        li_length_in_bytes = li_length_in_bytes ^ 3;
        fill_num_li += 1;

        if (li_length_in_bytes  == 2) {
          if (fill_num_li == test_num_li) {
            //e_li_p->e1  = 0;
            e_li_p->b1 = 0;
          } else {
            //e_li_p->e1  = 1;
            e_li_p->b1 =  0x80;
          }

          //e_li_p->li1 = sdu_mngt_p->sdu_remaining_size;
          e_li_p->b1 = e_li_p->b1 | (sdu_mngt_p->sdu_remaining_size >> 4);
          e_li_p->b2 = sdu_mngt_p->sdu_remaining_size << 4;
#if TRACE_RLC_UM_SEGMENT
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" set e_li_p->b1=%02X set e_li_p->b2=%02X fill_num_li=%d test_num_li=%d\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
                e_li_p->b1,
                e_li_p->b2,
                fill_num_li,
                test_num_li);
#endif
        } else {
          if (fill_num_li != test_num_li) {
            //e_li_p->e2  = 1;
            e_li_p->b2  = e_li_p->b2 | 0x08;
          }

          //e_li_p->li2 = sdu_mngt_p->sdu_remaining_size;
          e_li_p->b2 = e_li_p->b2 | (sdu_mngt_p->sdu_remaining_size >> 8);
          e_li_p->b3 = sdu_mngt_p->sdu_remaining_size & 0xFF;
#if TRACE_RLC_UM_SEGMENT
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" set e_li_p->b2=%02X set e_li_p->b3=%02X fill_num_li=%d test_num_li=%d\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
                e_li_p->b2,
                e_li_p->b3,
                fill_num_li,
                test_num_li);
#endif
          e_li_p++;
        }

        pdu_remaining_size = pdu_remaining_size - (sdu_mngt_p->sdu_remaining_size + li_length_in_bytes);

        // free SDU
        rlc_pP->buffer_occupancy -= sdu_mngt_p->sdu_remaining_size;
        sdu_in_buffer = list_remove_head(&rlc_pP->input_sdus);
        free_mem_block (sdu_in_buffer, __func__);
        sdu_in_buffer = list_get_head(&rlc_pP->input_sdus);
        sdu_mngt_p    = NULL;

      } else {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Filling  PDU with %d all remaining bytes of SDU and reduce TB size by %d bytes\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p->sdu_remaining_size,
              pdu_remaining_size - sdu_mngt_p->sdu_remaining_size);
#endif
//#if !EXMIMO
//        assert(1!=1);
//#endif
        memcpy(data, data_sdu_p, sdu_mngt_p->sdu_remaining_size);
        // reduce the size of the PDU
        continue_fill_pdu_with_sdu = 0;
        fi_last_byte_pdu_is_last_byte_sdu = 1;
        pdu_remaining_size = pdu_remaining_size - sdu_mngt_p->sdu_remaining_size;
        // free SDU
        rlc_pP->buffer_occupancy -= sdu_mngt_p->sdu_remaining_size;
        sdu_in_buffer = list_remove_head(&rlc_pP->input_sdus);
        free_mem_block (sdu_in_buffer, __func__);
        sdu_in_buffer = list_get_head(&rlc_pP->input_sdus);
        sdu_mngt_p    = NULL;

      }
    }

    // set framing info
    if (fi_first_byte_pdu_is_first_byte_sdu) {
      fi = 0;
    } else {
      fi = 2;
    }

    if (!fi_last_byte_pdu_is_last_byte_sdu) {
      fi = fi + 1;
    }

    pdu_p->b1 =  (fi << 3); //pdu_p->b1 |

    // set fist e bit
    if (fill_num_li > 0) {
      pdu_p->b1 = pdu_p->b1 | 0x04;
    }

    pdu_p->b1 = pdu_p->b1 | ((rlc_pP->vt_us >> 8) & 0x03);
    pdu_p->b2 = rlc_pP->vt_us & 0xFF;
    rlc_pP->vt_us = rlc_pP->vt_us+1;

    pdu_tb_req_p->data_ptr        = (unsigned char*)pdu_p;
    pdu_tb_req_p->tb_size = data_pdu_size - pdu_remaining_size;
    list_add_tail_eurecom (pdu_mem_p, &rlc_pP->pdus_to_mac_layer);
#if TRACE_RLC_PAYLOAD
    rlc_util_print_hex_octets(RLC, pdu_mem_p->data, data_pdu_size);
#endif
    //AssertFatal( pdu_tb_req_p->tb_size > 0 , "SEGMENT10: FINAL RLC UM PDU LENGTH %d", pdu_tb_req_p->tb_size);
    if(pdu_tb_req_p->tb_size <= 0) {
      LOG_E(RLC, "SEGMENT10: FINAL RLC UM PDU LENGTH %d\n", pdu_tb_req_p->tb_size);
      break;
    }
    pdu_p = NULL;
    pdu_mem_p = NULL;

    //nb_bytes_to_transmit = nb_bytes_to_transmit - data_pdu_size;
    nb_bytes_to_transmit = 0; // 1 PDU only
  }

  RLC_UM_MUTEX_UNLOCK(&rlc_pP->lock_input_sdus);
}
//-----------------------------------------------------------------------------
void
rlc_um_segment_5 (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t *rlc_pP)
{
  list_t              pdus;
  signed int          pdu_remaining_size      = 0;
  signed int          test_pdu_remaining_size = 0;

  int                 nb_bytes_to_transmit = rlc_pP->nb_bytes_requested_by_mac;
  rlc_um_pdu_sn_5_t  *pdu_p                = NULL;
  struct mac_tb_req  *pdu_tb_req_p         = NULL;
  mem_block_t        *pdu_mem_p            = NULL;
  char               *data                 = NULL;
  char               *data_sdu_p           = NULL;
  rlc_um_e_li_t      *e_li_p               = NULL;
  struct rlc_um_tx_sdu_management *sdu_mngt_p           = NULL;
  unsigned int       li_length_in_bytes                 = 0;
  unsigned int       test_li_length_in_bytes            = 0;
  unsigned int       test_remaining_size_to_substract   = 0;
  unsigned int       test_remaining_num_li_to_substract = 0;
  unsigned int       continue_fill_pdu_with_sdu         = 0;
  unsigned int       num_fill_sdu                       = 0;
  unsigned int       test_num_li                        = 0;
  unsigned int       fill_num_li                        = 0;
  mem_block_t        *sdu_in_buffer                     = NULL;
  unsigned int       data_pdu_size                      = 0;

  unsigned int       fi_first_byte_pdu_is_first_byte_sdu = 0;
  unsigned int       fi_last_byte_pdu_is_last_byte_sdu   = 0;
  unsigned int       fi                                  = 0;
  unsigned int       max_li_overhead                     = 0;

  if (nb_bytes_to_transmit < 2) {
#if TRACE_RLC_UM_SEGMENT
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" NO SEGMENTATION5 nb_bytes to transmit = %d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          nb_bytes_to_transmit);
#endif
    return;
  }

  list_init (&pdus, NULL);    // param string identifying the list is NULL
  pdu_mem_p = NULL;

  RLC_UM_MUTEX_LOCK(&rlc_pP->lock_input_sdus, ctxt_pP, rlc_pP);

  while ((list_get_head(&rlc_pP->input_sdus)) && (nb_bytes_to_transmit > 0)) {
#if TRACE_RLC_UM_SEGMENT
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT"nb_bytes_to_transmit %d BO %d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          nb_bytes_to_transmit,
          rlc_pP->buffer_occupancy);
#endif

    // pdu_p management
    if (!pdu_mem_p) {
      if (rlc_pP->input_sdus.nb_elements <= 1) {
        max_li_overhead = 0;
      } else {
        max_li_overhead = (((rlc_pP->input_sdus.nb_elements - 1) * 3) / 2) + ((rlc_pP->input_sdus.nb_elements - 1) % 2);
      }

      if  (nb_bytes_to_transmit >= (rlc_pP->buffer_occupancy + rlc_pP->tx_header_min_length_in_bytes + max_li_overhead)) {
        data_pdu_size = rlc_pP->buffer_occupancy + rlc_pP->tx_header_min_length_in_bytes + max_li_overhead;
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" alloc PDU size %d bytes to contain not all bytes requested by MAC but all BO of RLC@1\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              data_pdu_size);
#endif
      } else {
        data_pdu_size = nb_bytes_to_transmit;
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" alloc PDU size %d bytes to contain all bytes requested by MAC@1\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              data_pdu_size);
#endif
      }

      if (!(pdu_mem_p = get_free_mem_block (data_pdu_size + sizeof(struct mac_tb_req), __func__))) {
#if TRACE_RLC_UM_SEGMENT
        LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" ERROR COULD NOT GET NEW PDU, EXIT\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
#endif
        RLC_UM_MUTEX_UNLOCK(&rlc_pP->lock_input_sdus);
        return;
      }

#if TRACE_RLC_UM_SEGMENT
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" get new PDU %d bytes\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
            data_pdu_size);
#endif
      pdu_remaining_size = data_pdu_size - 1;
      pdu_p        = (rlc_um_pdu_sn_5_t*) (&pdu_mem_p->data[sizeof(struct mac_tb_req)]);
      pdu_tb_req_p = (struct mac_tb_req*) (pdu_mem_p->data);

      memset (pdu_mem_p->data, 0, sizeof (rlc_um_pdu_sn_5_t)+sizeof(struct mac_tb_req));
      li_length_in_bytes = 1;
    }

    //----------------------------------------
    // compute how many SDUS can fill the PDU
    //----------------------------------------
    continue_fill_pdu_with_sdu = 1;
    num_fill_sdu               = 0;
    test_num_li                = 0;
    sdu_in_buffer              = list_get_head(&rlc_pP->input_sdus);
    test_pdu_remaining_size    = pdu_remaining_size;
    test_li_length_in_bytes    = 1;
    test_remaining_size_to_substract   = 0;
    test_remaining_num_li_to_substract = 0;


    while ((sdu_in_buffer) && (continue_fill_pdu_with_sdu > 0)) {
      sdu_mngt_p = ((struct rlc_um_tx_sdu_management *) (sdu_in_buffer->data));

      if (sdu_mngt_p->sdu_remaining_size > test_pdu_remaining_size) {
        // no LI
        continue_fill_pdu_with_sdu = 0;
        num_fill_sdu += 1;
        test_pdu_remaining_size = 0;
        test_remaining_size_to_substract = 0;
        test_remaining_num_li_to_substract = 0;
      } else if (sdu_mngt_p->sdu_remaining_size == test_pdu_remaining_size) {
        // fi will indicate end of PDU is end of SDU, no need for LI
        continue_fill_pdu_with_sdu = 0;
        num_fill_sdu += 1;
        test_pdu_remaining_size = 0;
        test_remaining_size_to_substract = 0;
        test_remaining_num_li_to_substract = 0;
      } else if ((sdu_mngt_p->sdu_remaining_size + (test_li_length_in_bytes ^ 3)) == test_pdu_remaining_size ) {
        // no LI
        continue_fill_pdu_with_sdu = 0;
        num_fill_sdu += 1;
        test_pdu_remaining_size = 0;
        test_remaining_size_to_substract = 0;
        test_remaining_num_li_to_substract = 0;
        pdu_remaining_size = pdu_remaining_size - (test_li_length_in_bytes ^ 3);
        data_pdu_size -=  (test_li_length_in_bytes ^ 3);//modifier pour duy
      } else if ((sdu_mngt_p->sdu_remaining_size + (test_li_length_in_bytes ^ 3)) < test_pdu_remaining_size ) {
        test_num_li += 1;
        num_fill_sdu += 1;
        test_pdu_remaining_size = test_pdu_remaining_size - (sdu_mngt_p->sdu_remaining_size + (test_li_length_in_bytes ^ 3));
        test_remaining_size_to_substract = test_li_length_in_bytes ^ 3;
        test_remaining_num_li_to_substract = 1;
        test_li_length_in_bytes = test_li_length_in_bytes ^ 3;
      } else {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" sdu_mngt_p->sdu_remaining_size=%d test_pdu_remaining_size=%d test_li_length_in_bytes=%d\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p->sdu_remaining_size,
              test_pdu_remaining_size,
              test_li_length_in_bytes ^ 3);
#endif
        // reduce the size of the PDU
        continue_fill_pdu_with_sdu = 0;
        num_fill_sdu += 1;
        test_pdu_remaining_size = 0;
        test_remaining_size_to_substract = 0;
        test_remaining_num_li_to_substract = 0;
        pdu_remaining_size = pdu_remaining_size - 1;
        data_pdu_size -= 1;//modifier pour duy
      }

      sdu_in_buffer = sdu_in_buffer->next;
    }

    if (test_remaining_num_li_to_substract > 0) {
      // there is a LI that is not necessary
      test_num_li = test_num_li - 1;
      pdu_remaining_size = pdu_remaining_size - test_remaining_size_to_substract;
    }

    //----------------------------------------
    // Do the real filling of the pdu_p
    //----------------------------------------
#if TRACE_RLC_UM_SEGMENT
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" data shift %d Bytes num_li %d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          ((test_num_li*3) +1) >> 1,
          test_num_li);
#endif

    data = ((char*)(&pdu_p->data[((test_num_li*3) +1) >> 1]));
    e_li_p = (rlc_um_e_li_t*)(pdu_p->data);
    continue_fill_pdu_with_sdu          = 1;
    li_length_in_bytes                  = 1;
    fill_num_li                         = 0;
    fi_first_byte_pdu_is_first_byte_sdu = 0;
    fi_last_byte_pdu_is_last_byte_sdu   = 0;

    sdu_in_buffer = list_get_head(&rlc_pP->input_sdus);

    if (
      ((struct rlc_um_tx_sdu_management *) (sdu_in_buffer->data))->sdu_remaining_size ==
      ((struct rlc_um_tx_sdu_management *) (sdu_in_buffer->data))->sdu_size) {
      fi_first_byte_pdu_is_first_byte_sdu = 1;
    }

    while ((sdu_in_buffer) && (continue_fill_pdu_with_sdu > 0)) {
      sdu_mngt_p = ((struct rlc_um_tx_sdu_management *) (sdu_in_buffer->data));

      if (sdu_mngt_p->sdu_segmented_size == 0) {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" GET NEW SDU %p AVAILABLE SIZE %d Bytes\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p,
              sdu_mngt_p->sdu_remaining_size);
#endif
      } else {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" GET AGAIN SDU %p REMAINING AVAILABLE SIZE %d Bytes / %d Bytes \n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p,
              sdu_mngt_p->sdu_remaining_size,
              sdu_mngt_p->sdu_size);
#endif
      }

      data_sdu_p = (char*) &(sdu_in_buffer->data[sizeof (struct rlc_um_tx_sdu_management) + sdu_mngt_p->sdu_segmented_size]);

      if (sdu_mngt_p->sdu_remaining_size > pdu_remaining_size) {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Filling all remaining PDU with %d bytes\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              pdu_remaining_size);
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" pdu_mem_p %p pdu_p %p pdu_p->data %p data %p data_sdu_p %p pdu_remaining_size %d\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              pdu_mem_p,
              pdu_p,
              pdu_p->data,
              data,
              data_sdu_p,
              pdu_remaining_size);
#endif

        memcpy(data, data_sdu_p, pdu_remaining_size);
        sdu_mngt_p->sdu_remaining_size = sdu_mngt_p->sdu_remaining_size - pdu_remaining_size;
        sdu_mngt_p->sdu_segmented_size = sdu_mngt_p->sdu_segmented_size + pdu_remaining_size;
        fi_last_byte_pdu_is_last_byte_sdu = 0;
        // no LI
        rlc_pP->buffer_occupancy -= pdu_remaining_size;
        continue_fill_pdu_with_sdu = 0;
        pdu_remaining_size = 0;
      } else if (sdu_mngt_p->sdu_remaining_size == pdu_remaining_size) {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Exactly Filling remaining PDU with %d remaining bytes of SDU\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              pdu_remaining_size);
#endif
        memcpy(data, data_sdu_p, pdu_remaining_size);
        // free SDU
        rlc_pP->buffer_occupancy -= sdu_mngt_p->sdu_remaining_size;
        sdu_in_buffer = list_remove_head(&rlc_pP->input_sdus);
        free_mem_block (sdu_in_buffer, __func__);
        sdu_in_buffer = list_get_head(&rlc_pP->input_sdus);
        sdu_mngt_p    = NULL;

        fi_last_byte_pdu_is_last_byte_sdu = 1;
        // fi will indicate end of PDU is end of SDU, no need for LI
        continue_fill_pdu_with_sdu = 0;
        pdu_remaining_size = 0;
      } else if ((sdu_mngt_p->sdu_remaining_size + (li_length_in_bytes ^ 3)) < pdu_remaining_size ) {
#if TRACE_RLC_UM_SEGMENT
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Filling  PDU with %d all remaining bytes of SDU\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p->sdu_remaining_size);
#endif
        memcpy(data, data_sdu_p, sdu_mngt_p->sdu_remaining_size);
        data = &data[sdu_mngt_p->sdu_remaining_size];
        li_length_in_bytes = li_length_in_bytes ^ 3;
        fill_num_li += 1;

        if (li_length_in_bytes  == 2) {
          if (fill_num_li == test_num_li) {
            //e_li_p->e1  = 0;
            e_li_p->b1 = 0;
          } else {
            //e_li_p->e1  = 1;
            e_li_p->b1 =  0x80;
          }

          //e_li_p->li1 = sdu_mngt_p->sdu_remaining_size;
          e_li_p->b1 = e_li_p->b1 | (sdu_mngt_p->sdu_remaining_size >> 4);
          e_li_p->b2 = sdu_mngt_p->sdu_remaining_size << 4;
#if TRACE_RLC_UM_SEGMENT
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" set e_li_p->b1=%02X set e_li_p->b2=%02X fill_num_li=%d test_num_li=%d\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
                e_li_p->b1,
                e_li_p->b2,
                fill_num_li,
                test_num_li);
#endif
        } else {
          if (fill_num_li != test_num_li) {
            //e_li_p->e2  = 1;
            e_li_p->b2  = e_li_p->b2 | 0x08;
          }

          //e_li_p->li2 = sdu_mngt_p->sdu_remaining_size;
          e_li_p->b2 = e_li_p->b2 | (sdu_mngt_p->sdu_remaining_size >> 8);
          e_li_p->b3 = sdu_mngt_p->sdu_remaining_size & 0xFF;
#if TRACE_RLC_UM_SEGMENT
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" set e_li_p->b2=%02X set e_li_p->b3=%02X fill_num_li=%d test_num_li=%d\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
                e_li_p->b2,
                e_li_p->b3,
                fill_num_li,
                test_num_li);
#endif
          e_li_p++;
        }

        pdu_remaining_size = pdu_remaining_size - (sdu_mngt_p->sdu_remaining_size + li_length_in_bytes);

        // free SDU
        rlc_pP->buffer_occupancy -= sdu_mngt_p->sdu_remaining_size;
        sdu_in_buffer = list_remove_head(&rlc_pP->input_sdus);
        free_mem_block (sdu_in_buffer, __func__);
        sdu_in_buffer = list_get_head(&rlc_pP->input_sdus);
        sdu_mngt_p    = NULL;

      } else {
        LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Filling  PDU with %d all remaining bytes of SDU and reduce TB size by %d bytes\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sdu_mngt_p->sdu_remaining_size,
              pdu_remaining_size - sdu_mngt_p->sdu_remaining_size);
        //assert(1!=1);
        memcpy(data, data_sdu_p, sdu_mngt_p->sdu_remaining_size);
        // reduce the size of the PDU
        continue_fill_pdu_with_sdu = 0;
        fi_last_byte_pdu_is_last_byte_sdu = 1;
        pdu_remaining_size = pdu_remaining_size - sdu_mngt_p->sdu_remaining_size;
        // free SDU
        rlc_pP->buffer_occupancy -= sdu_mngt_p->sdu_remaining_size;
        sdu_in_buffer = list_remove_head(&rlc_pP->input_sdus);
        free_mem_block (sdu_in_buffer, __func__);
        sdu_in_buffer = list_get_head(&rlc_pP->input_sdus);
        sdu_mngt_p    = NULL;
      }
    }

    // set framing info
    if (fi_first_byte_pdu_is_first_byte_sdu) {
      fi = 0;
    } else {
      fi = 2;
    }

    if (!fi_last_byte_pdu_is_last_byte_sdu) {
      fi = fi + 1;
    }

    pdu_p->b1 =  (fi << 6); //pdu_p->b1 |

    // set fist e bit
    if (fill_num_li > 0) {
      pdu_p->b1 = pdu_p->b1 | 0x20;
    }

    pdu_p->b1 = pdu_p->b1 | (rlc_pP->vt_us & 0x1F);
    rlc_pP->vt_us = rlc_pP->vt_us+1;

    pdu_tb_req_p->data_ptr        = (unsigned char*)pdu_p;
    pdu_tb_req_p->tb_size         = data_pdu_size - pdu_remaining_size;
    list_add_tail_eurecom (pdu_mem_p, &rlc_pP->pdus_to_mac_layer);
#if TRACE_RLC_PAYLOAD
    rlc_util_print_hex_octets(RLC, (unsigned char*)pdu_mem_p->data, data_pdu_size);
#endif
    //AssertFatal( pdu_tb_req_p->tb_size > 0 , "SEGMENT5: FINAL RLC UM PDU LENGTH %d", pdu_tb_req_p->tb_size);
    if(pdu_tb_req_p->tb_size <= 0) {
      LOG_E(RLC, "SEGMENT5: FINAL RLC UM PDU LENGTH %d\n", pdu_tb_req_p->tb_size);
      break;
    }

    pdu_p = NULL;
    pdu_mem_p = NULL;

    //nb_bytes_to_transmit = nb_bytes_to_transmit - data_pdu_size;
    nb_bytes_to_transmit = 0; // 1 PDU only
  }

  RLC_UM_MUTEX_UNLOCK(&rlc_pP->lock_input_sdus);
}

