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
#define RLC_UM_DAR_C 1
#include "platform_types.h"
#include "assertions.h"
//-----------------------------------------------------------------------------
#include "msc.h"
#include "rlc.h"
#include "rlc_um.h"
#include "rlc_primitives.h"
#include "mac_primitives.h"
#include "list.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
//-----------------------------------------------------------------------------
signed int rlc_um_get_pdu_infos(
  const protocol_ctxt_t* const ctxt_pP,
  const rlc_um_entity_t* const rlc_pP,
  rlc_um_pdu_sn_10_t   * const header_pP,
  const sdu_size_t             total_sizeP,
  rlc_um_pdu_info_t    * const pdu_info_pP,
  const uint8_t                sn_lengthP)
{
  sdu_size_t         sum_li = 0;
  memset(pdu_info_pP, 0, sizeof (rlc_um_pdu_info_t));

  pdu_info_pP->num_li = 0;

  //AssertFatal( total_sizeP > 0 , "RLC UM PDU LENGTH %d", total_sizeP);
  if(total_sizeP <= 0) {
    LOG_E(RLC, "RLC UM PDU LENGTH %d\n", total_sizeP);
    return -1;
  }

  if (sn_lengthP == 10) {
    pdu_info_pP->fi           = (header_pP->b1 >> 3) & 0x03;
    pdu_info_pP->e            = (header_pP->b1 >> 2) & 0x01;
    pdu_info_pP->sn           = header_pP->b2 + (((uint16_t)(header_pP->b1 & 0x03)) << 8);
    pdu_info_pP->header_size  = 2;
    pdu_info_pP->payload      = &header_pP->data[0];
  } else if (sn_lengthP == 5) {
    pdu_info_pP->fi           = (header_pP->b1 >> 6) & 0x03;
    pdu_info_pP->e            = (header_pP->b1 >> 5) & 0x01;
    pdu_info_pP->sn           = header_pP->b1 & 0x1F;
    pdu_info_pP->header_size  = 1;
    pdu_info_pP->payload      = &header_pP->b2;
  } else {
    //AssertFatal( sn_lengthP == 5 || sn_lengthP == 10, "RLC UM SN LENGTH %d", sn_lengthP);
    if(!(sn_lengthP == 5 || sn_lengthP == 10)) {
      LOG_E(RLC, "RLC UM SN LENGTH %d\n", sn_lengthP);
      return -1;
    }
  }


  if (pdu_info_pP->e) {
    rlc_am_e_li_t      *e_li_p;
    unsigned int li_length_in_bytes  = 1;
    unsigned int li_to_read          = 1;

    e_li_p = (rlc_am_e_li_t*)(pdu_info_pP->payload);

    while (li_to_read)  {
      li_length_in_bytes = li_length_in_bytes ^ 3;

      if (li_length_in_bytes  == 2) {
        //AssertFatal( total_sizeP >= ((uint64_t)(&e_li_p->b2) - (uint64_t)header_pP),
        //             "DECODING PDU TOO FAR PDU size %d", total_sizeP);
        if(total_sizeP < ((uint64_t)(&e_li_p->b2) - (uint64_t)header_pP)) {
          LOG_E(RLC, "DECODING PDU TOO FAR PDU size %d\n", total_sizeP);
          return -1;
        }
        pdu_info_pP->li_list[pdu_info_pP->num_li] = ((uint16_t)(e_li_p->b1 << 4)) & 0x07F0;
        pdu_info_pP->li_list[pdu_info_pP->num_li] |= (((uint8_t)(e_li_p->b2 >> 4)) & 0x000F);
        li_to_read = e_li_p->b1 & 0x80;
        pdu_info_pP->header_size  += 2;
      } else {
        //AssertFatal( total_sizeP >= ((uint64_t)(&e_li_p->b3) - (uint64_t)header_pP),
        //             "DECODING PDU TOO FAR PDU size %d", total_sizeP);
        if(total_sizeP < ((uint64_t)(&e_li_p->b3) - (uint64_t)header_pP)) {
          LOG_E(RLC, "DECODING PDU TOO FAR PDU size %d\n", total_sizeP);
          return -1;
        }
        pdu_info_pP->li_list[pdu_info_pP->num_li] = ((uint16_t)(e_li_p->b2 << 8)) & 0x0700;
        pdu_info_pP->li_list[pdu_info_pP->num_li] |=  e_li_p->b3;
        li_to_read = e_li_p->b2 & 0x08;
        e_li_p++;
        pdu_info_pP->header_size  += 1;
      }

      //AssertFatal( pdu_info_pP->num_li <= RLC_UM_SEGMENT_NB_MAX_LI_PER_PDU,
      //             PROTOCOL_RLC_UM_CTXT_FMT"[GET PDU INFO]  SN %04d TOO MANY LIs ",
      //             PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
      //             pdu_info_pP->sn);
      if(pdu_info_pP->num_li >= RLC_UM_SEGMENT_NB_MAX_LI_PER_PDU) {
        LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT"[GET PDU INFO]  SN %04d TOO MANY LIs \n",
                   PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                   pdu_info_pP->sn);
        return -1;
      }

      sum_li += pdu_info_pP->li_list[pdu_info_pP->num_li];
      pdu_info_pP->num_li = pdu_info_pP->num_li + 1;

      if (pdu_info_pP->num_li > RLC_UM_SEGMENT_NB_MAX_LI_PER_PDU) {
        return -2;
      }
    }

    if (li_length_in_bytes  == 2) {
      pdu_info_pP->payload = &e_li_p->b3;
    } else {
      pdu_info_pP->payload = &e_li_p->b1;
    }
  }

  pdu_info_pP->payload_size = total_sizeP - pdu_info_pP->header_size;

  if (pdu_info_pP->payload_size > sum_li) {
    pdu_info_pP->hidden_size = pdu_info_pP->payload_size - sum_li;
  }

  return 0;
}
//-----------------------------------------------------------------------------
int rlc_um_read_length_indicators(unsigned char**data_ppP, rlc_um_e_li_t* e_liP, unsigned int* li_array_pP, unsigned int *num_li_pP, sdu_size_t *data_size_pP)
{
  int          continue_loop = 1;
  unsigned int e1  = 0;
  unsigned int li1 = 0;
  unsigned int e2  = 0;
  unsigned int li2 = 0;
  *num_li_pP = 0;

  while ((continue_loop)) {
    //msg("[RLC_UM] e_liP->b1 = %02X\n", e_liP->b1);
    //msg("[RLC_UM] e_liP->b2 = %02X\n", e_liP->b2);
    e1 = ((unsigned int)e_liP->b1 & 0x00000080) >> 7;
    li1 = (((unsigned int)e_liP->b1 & 0x0000007F) << 4) + (((unsigned int)e_liP->b2 & 0x000000F0) >> 4);
    li_array_pP[*num_li_pP] = li1;
    *data_size_pP = *data_size_pP - li1 - 2;
    *num_li_pP = *num_li_pP +1;

    if ((e1)) {
      e2 = ((unsigned int)e_liP->b2 & 0x00000008) >> 3;
      li2 = (((unsigned int)e_liP->b2 & 0x00000007) << 8) + ((unsigned int)e_liP->b3 & 0x000000FF);
      li_array_pP[*num_li_pP] = li2;
      *data_size_pP = *data_size_pP - li2 - 1;
      *num_li_pP = *num_li_pP +1;

      if (!(*data_size_pP >= 0)) LOG_E(RLC, "Invalid data_size=%d! (pdu_size=%d loop=%d e1=%d e2=%d li2=%d e_liP=%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x)\n",
          *data_size_pP, *data_size_pP, continue_loop, e1, e2, li2,
          (e_liP-(continue_loop-1)+0)->b1,
          (e_liP-(continue_loop-1)+0)->b2,
          (e_liP-(continue_loop-1)+0)->b3,
          (e_liP-(continue_loop-1)+1)->b1,
          (e_liP-(continue_loop-1)+1)->b2,
          (e_liP-(continue_loop-1)+1)->b3,
          (e_liP-(continue_loop-1)+2)->b1,
          (e_liP-(continue_loop-1)+2)->b2,
          (e_liP-(continue_loop-1)+2)->b3);
      // AssertFatal(*data_size_pP >= 0, "Invalid data_size!");

      if (e2 == 0) {
        continue_loop = 0;
      } else {
        e_liP++;
        continue_loop++;
      }
    } else {
      if (!(*data_size_pP >= 0)) LOG_E(RLC, "Invalid data_size=%d! (pdu_size=%d loop=%d e1=%d li1=%d e_liP=%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x)\n",
          *data_size_pP, *data_size_pP, continue_loop, e1, li1,
          (e_liP-(continue_loop-1)+0)->b1,
          (e_liP-(continue_loop-1)+0)->b2,
          (e_liP-(continue_loop-1)+0)->b3,
          (e_liP-(continue_loop-1)+1)->b1,
          (e_liP-(continue_loop-1)+1)->b2,
          (e_liP-(continue_loop-1)+1)->b3,
          (e_liP-(continue_loop-1)+2)->b1,
          (e_liP-(continue_loop-1)+2)->b2,
          (e_liP-(continue_loop-1)+2)->b3);
      continue_loop = 0;
      // AssertFatal(*data_size_pP >= 0, "Invalid data_size!");
    }

    if (*num_li_pP > RLC_UM_SEGMENT_NB_MAX_LI_PER_PDU) {
      return -1;
    }
  }

  *data_ppP = *data_ppP + (((*num_li_pP*3) +1) >> 1);
  if (*data_size_pP > 0) {
    return 0;
  } else if (*data_size_pP == 0) {
    LOG_W(RLC, "Last RLC SDU size is zero!\n");
    return -1;
  } else {
    LOG_W(RLC, "Last RLC SDU size is negative %d!\n", *data_size_pP);
    return -1;
  }
}
//-----------------------------------------------------------------------------
void
rlc_um_try_reassembly(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t *            rlc_pP,
  rlc_sn_t                     start_snP,
  rlc_sn_t                     end_snP)
{
  mem_block_t        *pdu_mem_p              = NULL;
  struct mac_tb_ind  *tb_ind_p               = NULL;
  rlc_um_e_li_t      *e_li_p                 = NULL;
  unsigned char      *data_p                 = NULL;
  int                 e                      = 0;
  int                 fi                     = 0;
  sdu_size_t          size                   = 0;
  rlc_sn_t            sn                     = 0;
  unsigned int        continue_reassembly    = 0;
  unsigned int        num_li                 = 0;
  unsigned int        li_array[RLC_UM_SEGMENT_NB_MAX_LI_PER_PDU];
  int                 i                      = 0;
  int                 reassembly_start_index = 0;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_TRY_REASSEMBLY,VCD_FUNCTION_IN);

  if (end_snP < 0)   {
    end_snP   = end_snP   + rlc_pP->rx_sn_modulo;
  }

  if (start_snP < 0) {
    start_snP = start_snP + rlc_pP->rx_sn_modulo;
  }

#if TRACE_RLC_UM_DAR
  LOG_D(RLC,  PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY FROM PDU SN=%03d+1  TO  PDU SN=%03d   SN Length = %d bits (%s:%u)\n",
        PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
        rlc_pP->last_reassemblied_sn,
        end_snP,
        rlc_pP->rx_sn_length,
        __FILE__,
        __LINE__);
#endif

  // nothing to be reassemblied
  if (start_snP == end_snP) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_TRY_REASSEMBLY,VCD_FUNCTION_OUT);
    return;
  }

  continue_reassembly = 1;
  //sn = (rlc_pP->last_reassemblied_sn + 1) % rlc_pP->rx_sn_modulo;
  sn = start_snP;

  //check_mem_area();

  while (continue_reassembly) {
    if ((pdu_mem_p = rlc_pP->dar_buffer[sn])) {

      if ((rlc_pP->last_reassemblied_sn+1)%rlc_pP->rx_sn_modulo != sn) {
#if TRACE_RLC_UM_DAR
        LOG_W(RLC,
              PROTOCOL_RLC_UM_CTXT_FMT" FINDING a HOLE in RLC UM SN: CLEARING OUTPUT SDU BECAUSE NEW SN (%03d) TO REASSEMBLY NOT CONTIGUOUS WITH LAST REASSEMBLIED SN (%03d) (%s:%u)\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
              sn,
              rlc_pP->last_reassemblied_sn,
              __FILE__,
              __LINE__);
#endif
        rlc_um_clear_rx_sdu(ctxt_pP, rlc_pP);
      }

      rlc_pP->last_reassemblied_sn = sn;
      tb_ind_p = (struct mac_tb_ind *)(pdu_mem_p->data);

      if (rlc_pP->rx_sn_length == 10) {
#if TRACE_RLC_UM_DAR
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY 10 PDU SN=%03d\n (%s:%u)",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
              sn,
              __FILE__,
              __LINE__);
#endif
        e  = (((rlc_um_pdu_sn_10_t*)(tb_ind_p->data_ptr))->b1 & 0x04) >> 2;
        fi = (((rlc_um_pdu_sn_10_t*)(tb_ind_p->data_ptr))->b1 & 0x18) >> 3;
        e_li_p = (rlc_um_e_li_t*)((rlc_um_pdu_sn_10_t*)(tb_ind_p->data_ptr))->data;
        size   = tb_ind_p->size - 2;
        data_p = &tb_ind_p->data_ptr[2];
      } else {
#if TRACE_RLC_UM_DAR
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY 5 PDU SN=%03d Byte 0=%02X (%s:%u)\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
              sn,
              ((rlc_um_pdu_sn_5_t*)(tb_ind_p->data_ptr))->b1,
              __FILE__,
              __LINE__);
#endif
        e  = (((rlc_um_pdu_sn_5_t*)(tb_ind_p->data_ptr))->b1 & 0x00000020) >> 5;
        fi = (((rlc_um_pdu_sn_5_t*)(tb_ind_p->data_ptr))->b1 & 0x000000C0) >> 6;
        e_li_p = (rlc_um_e_li_t*)((rlc_um_pdu_sn_5_t*)(tb_ind_p->data_ptr))->data;
        size   = tb_ind_p->size - 1;
        data_p = &tb_ind_p->data_ptr[1];
#if TRACE_RLC_UM_DAR
        LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" e=%01X fi=%01X\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
              e,
              fi,
              __FILE__,
              __LINE__);
#endif
      }
      //AssertFatal(size >= 0, "invalid size!");
      //AssertFatal((e==0) || (e==1), "invalid e!");
      //AssertFatal((fi >= 0) && (fi <= 3), "invalid fi!");
      if((size < 0) || ((e!=0) && (e!=1)) || ((fi < 0) || (fi > 3))){
        LOG_E(RLC, "invalid size %d, e %d, fi %d\n", size, e, fi);
        sn = (sn + 1) % rlc_pP->rx_sn_modulo;
        if ((sn == rlc_pP->vr_uh) || (sn == end_snP)) {
          continue_reassembly = 0;
        }
        continue;
      }

      if (e == RLC_E_FIXED_PART_DATA_FIELD_FOLLOW) {
        switch (fi) {
        case RLC_FI_1ST_BYTE_DATA_IS_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_LAST_BYTE_SDU:
#if TRACE_RLC_UM_DAR
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU NO E_LI FI=11 (00) (%s:%u)\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                __FILE__,
                __LINE__);
#endif
          // one complete SDU
          //LGrlc_um_send_sdu(rlc_pP,ctxt_pP->frame,ctxt_pP->enb_flag); // may be not necessary
          rlc_um_clear_rx_sdu(ctxt_pP, rlc_pP);
          rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, size);
          rlc_um_send_sdu(ctxt_pP, rlc_pP);
          rlc_pP->reassembly_missing_sn_detected = 0;

          break;

        case RLC_FI_1ST_BYTE_DATA_IS_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_NOT_LAST_BYTE_SDU:
#if TRACE_RLC_UM_DAR
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU NO E_LI FI=10 (01) (%s:%u)\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                __FILE__,
                __LINE__);
#endif
          // one beginning segment of SDU in PDU
          //LG rlc_um_send_sdu(rlc_pP,ctxt_pP->frame,ctxt_pP->enb_flag); // may be not necessary
          rlc_um_clear_rx_sdu(ctxt_pP, rlc_pP);
          rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, size);
          rlc_pP->reassembly_missing_sn_detected = 0;
          break;

        case RLC_FI_1ST_BYTE_DATA_IS_NOT_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_LAST_BYTE_SDU:
#if TRACE_RLC_UM_DAR
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU NO E_LI FI=01 (10) (%s:%u)\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                __FILE__,
                __LINE__);
#endif

          // one last segment of SDU
          if (rlc_pP->reassembly_missing_sn_detected == 0) {
            rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, size);
            rlc_um_send_sdu(ctxt_pP, rlc_pP);
          } else {
            //clear sdu already done
            rlc_pP->stat_rx_data_pdu_dropped += 1;
            rlc_pP->stat_rx_data_bytes_dropped += tb_ind_p->size;
          }

          rlc_pP->reassembly_missing_sn_detected = 0;
          break;

        case RLC_FI_1ST_BYTE_DATA_IS_NOT_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_NOT_LAST_BYTE_SDU:
#if TRACE_RLC_UM_DAR
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU NO E_LI FI=00 (11) (%s:%u)\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                __FILE__,
                __LINE__);
#endif

          if (rlc_pP->reassembly_missing_sn_detected == 0) {
            // one whole segment of SDU in PDU
            rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, size);
          } else {
#if TRACE_RLC_UM_DAR
            LOG_W(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU NO E_LI FI=00 (11) MISSING SN DETECTED (%s:%u)\n",
                  PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                  __FILE__,
                  __LINE__);
#endif
            //LOG_D(RLC, "[MSC_NBOX][FRAME %05u][%s][RLC_UM][MOD %u/%u][RB %u][Missing SN detected][RLC_UM][MOD %u/%u][RB %u]\n",
            //      ctxt_pP->frame, rlc_pP->module_id,rlc_pP->rb_id, rlc_pP->module_id,rlc_pP->rb_id);
            rlc_pP->reassembly_missing_sn_detected = 1; // not necessary but for readability of the code
            rlc_pP->stat_rx_data_pdu_dropped += 1;
            rlc_pP->stat_rx_data_bytes_dropped += tb_ind_p->size;
#if RLC_STOP_ON_LOST_PDU
            AssertFatal( rlc_pP->reassembly_missing_sn_detected == 1,
                         PROTOCOL_RLC_UM_CTXT_FMT" MISSING PDU DETECTED (%s:%u)\n",
                         PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                         __FILE__,
                         __LINE__);
#endif
          }

          break;

        default:
          //AssertFatal( 0 , PROTOCOL_RLC_UM_CTXT_FMT" fi=%d! TRY REASSEMBLY SHOULD NOT GO HERE (%s:%u)\n",
          //             PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          //             fi,
          //             __FILE__,
          //             __LINE__);
          LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" fi=%d! TRY REASSEMBLY SHOULD NOT GO HERE (%s:%u)\n",
                     PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP), fi, __FILE__, __LINE__);
          
        }
      } else {
        if (rlc_um_read_length_indicators(&data_p, e_li_p, li_array, &num_li, &size ) >= 0) {
          switch (fi) {
          case RLC_FI_1ST_BYTE_DATA_IS_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_LAST_BYTE_SDU:
#if TRACE_RLC_UM_DAR
            LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU FI=11 (00) Li=",
                  PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));

            for (i=0; i < num_li; i++) {
              LOG_D(RLC, "%d ",li_array[i]);
            }

            LOG_D(RLC, " remaining size %d\n",size);
#endif
            // N complete SDUs
            //LGrlc_um_send_sdu(rlc_pP,ctxt_pP->frame,ctxt_pP->enb_flag);
            rlc_um_clear_rx_sdu(ctxt_pP, rlc_pP);

            for (i = 0; i < num_li; i++) {
              rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, li_array[i]);
              rlc_um_send_sdu(ctxt_pP, rlc_pP);
              data_p = &data_p[li_array[i]];
            }

            if (size > 0) { // normally should always be > 0 but just for help debug
              // data_p is already ok, done by last loop above
              rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, size);
              rlc_um_send_sdu(ctxt_pP, rlc_pP);
            }

            rlc_pP->reassembly_missing_sn_detected = 0;
            break;

          case RLC_FI_1ST_BYTE_DATA_IS_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_NOT_LAST_BYTE_SDU:
#if TRACE_RLC_UM_DAR
            LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU FI=10 (01) Li=",
                  PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));

            for (i=0; i < num_li; i++) {
              LOG_D(RLC, "%d ",li_array[i]);
            }

            LOG_D(RLC, " remaining size %d\n",size);
#endif
            // N complete SDUs + one segment of SDU in PDU
            //LG rlc_um_send_sdu(rlc_pP,ctxt_pP->frame,ctxt_pP->enb_flag);
            rlc_um_clear_rx_sdu(ctxt_pP, rlc_pP);

            for (i = 0; i < num_li; i++) {
              rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, li_array[i]);
              rlc_um_send_sdu(ctxt_pP, rlc_pP);
              data_p = &data_p[li_array[i]];
            }

            if (size > 0) { // normally should always be > 0 but just for help debug
              // data_p is already ok, done by last loop above
              rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, size);
            }

            rlc_pP->reassembly_missing_sn_detected = 0;
            break;

          case RLC_FI_1ST_BYTE_DATA_IS_NOT_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_LAST_BYTE_SDU:
#if TRACE_RLC_UM_DAR
            LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU FI=01 (10) Li=",
                  PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));

            for (i=0; i < num_li; i++) {
              LOG_D(RLC, "%d ",li_array[i]);
            }

            LOG_D(RLC, " remaining size %d\n",size);
#endif

            if (rlc_pP->reassembly_missing_sn_detected) {
              reassembly_start_index = 1;
              data_p = &data_p[li_array[0]];
              //rlc_pP->stat_rx_data_pdu_dropped += 1;
              rlc_pP->stat_rx_data_bytes_dropped += li_array[0];
            } else {
              reassembly_start_index = 0;
            }

            // one last segment of SDU + N complete SDUs in PDU
            for (i = reassembly_start_index; i < num_li; i++) {
              rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, li_array[i]);
              rlc_um_send_sdu(ctxt_pP, rlc_pP);
              data_p = &data_p[li_array[i]];
            }

            if (size > 0) { // normally should always be > 0 but just for help debug
              // data_p is already ok, done by last loop above
              rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, size);
              rlc_um_send_sdu(ctxt_pP, rlc_pP);
            }

            rlc_pP->reassembly_missing_sn_detected = 0;
            break;

          case RLC_FI_1ST_BYTE_DATA_IS_NOT_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_NOT_LAST_BYTE_SDU:
#if TRACE_RLC_UM_DAR
            LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRY REASSEMBLY PDU FI=00 (11) Li=",
                  PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));

            for (i=0; i < num_li; i++) {
              LOG_D(RLC, "%d ",li_array[i]);
            }

            LOG_D(RLC, " remaining size %d\n",size);
#endif

            if (rlc_pP->reassembly_missing_sn_detected) {
#if TRACE_RLC_UM_DAR
              LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" DISCARD FIRST LI %d (%s:%u)",
                    PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                    li_array[0],
                    __FILE__,
                    __LINE__);
#endif
              reassembly_start_index = 1;
              data_p = &data_p[li_array[0]];
              //rlc_pP->stat_rx_data_pdu_dropped += 1;
              rlc_pP->stat_rx_data_bytes_dropped += li_array[0];
            } else {
              reassembly_start_index = 0;
            }

            for (i = reassembly_start_index; i < num_li; i++) {
              rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, li_array[i]);
              rlc_um_send_sdu(ctxt_pP, rlc_pP);
              data_p = &data_p[li_array[i]];
            }

            if (size > 0) { // normally should always be > 0 but just for help debug
              // data_p is already ok, done by last loop above
              rlc_um_reassembly (ctxt_pP, rlc_pP, data_p, size);
            } else {
              //AssertFatal( 0 !=0, PROTOCOL_RLC_UM_CTXT_FMT" size=%d! SHOULD NOT GO HERE (%s:%u)\n",
              //             PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
              //             size,
              //             __FILE__,
              //             __LINE__);
              LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" size=%d! SHOULD NOT GO HERE (%s:%u)\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP), size, __FILE__, __LINE__);
              
              //rlc_pP->stat_rx_data_pdu_dropped += 1;
              rlc_pP->stat_rx_data_bytes_dropped += size;
            }

            rlc_pP->reassembly_missing_sn_detected = 0;
            break;

          default:
#if TRACE_RLC_UM_DAR
            LOG_W(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Missing SN detected (%s:%u)\n",
                  PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                  __FILE__,
                  __LINE__);
#endif
            rlc_pP->stat_rx_data_pdu_dropped += 1;
            rlc_pP->stat_rx_data_bytes_dropped += tb_ind_p->size;

            rlc_pP->reassembly_missing_sn_detected = 1;
#if RLC_STOP_ON_LOST_PDU
            AssertFatal( rlc_pP->reassembly_missing_sn_detected == 1,
                         PROTOCOL_RLC_UM_CTXT_FMT" MISSING PDU DETECTED (%s:%u)\n",
                         PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                         __FILE__,
                         __LINE__);
#endif
          }
        } else {
          rlc_pP->stat_rx_data_pdu_dropped += 1;
          rlc_pP->stat_rx_data_bytes_dropped += tb_ind_p->size;
          rlc_pP->reassembly_missing_sn_detected = 1;

          LOG_W(RLC, "[SN %d] Bad RLC header! Discard this RLC PDU (size=%d)\n", sn, size);
        }
      }

#if TRACE_RLC_UM_DAR
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" REMOVE PDU FROM DAR BUFFER  SN=%03d (%s:%u)\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            sn,
            __FILE__,
            __LINE__);
#endif
      free_mem_block(rlc_pP->dar_buffer[sn], __func__);
      rlc_pP->dar_buffer[sn] = NULL;
    } else {
      rlc_pP->last_reassemblied_missing_sn = sn;
#if TRACE_RLC_UM_DAR
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" Missing SN %04d detected, clearing RX SDU (%s:%u)\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            sn,
            __FILE__,
            __LINE__);
#endif
      rlc_pP->reassembly_missing_sn_detected = 1;
      rlc_um_clear_rx_sdu(ctxt_pP, rlc_pP);
#if RLC_STOP_ON_LOST_PDU
      AssertFatal( rlc_pP->reassembly_missing_sn_detected == 1,
                   PROTOCOL_RLC_UM_CTXT_FMT" MISSING PDU DETECTED (%s:%u)\n",
                   PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                   __FILE__,
                   __LINE__);
#endif
    }

    sn = (sn + 1) % rlc_pP->rx_sn_modulo;

    if ((sn == rlc_pP->vr_uh) || (sn == end_snP)) {
      continue_reassembly = 0;
    }
  }

#if TRACE_RLC_UM_DAR
  LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TRIED REASSEMBLY VR(UR)=%03d VR(UX)=%03d VR(UH)=%03d (%s:%u)\n",
        PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
        rlc_pP->vr_ur,
        rlc_pP->vr_ux,
        rlc_pP->vr_uh,
        __FILE__,
        __LINE__);
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_TRY_REASSEMBLY,VCD_FUNCTION_OUT);
}
//-----------------------------------------------------------------------------
void
rlc_um_stop_and_reset_timer_reordering(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t *            rlc_pP)
{
#if TRACE_RLC_UM_DAR
  LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" [T-REORDERING] STOPPED AND RESET\n",
        PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
#endif
  rlc_pP->t_reordering.running         = 0;
  rlc_pP->t_reordering.ms_time_out     = 0;
  rlc_pP->t_reordering.ms_start        = 0;
  rlc_pP->t_reordering.timed_out       = 0;
}
//-----------------------------------------------------------------------------
void
rlc_um_start_timer_reordering(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t *            rlc_pP)
{
  rlc_pP->t_reordering.timed_out       = 0;

  if (rlc_pP->t_reordering.ms_duration > 0) {
  rlc_pP->t_reordering.running         = 1;
    rlc_pP->t_reordering.ms_time_out      = PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) + rlc_pP->t_reordering.ms_duration;
    rlc_pP->t_reordering.ms_start        = PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP);
#if TRACE_RLC_UM_DAR
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" [T-REORDERING] STARTED (TIME-OUT = FRAME %05u)\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
        rlc_pP->t_reordering.ms_time_out);
#endif
  } else {
    LOG_T(RLC, PROTOCOL_RLC_UM_CTXT_FMT"[T-REORDERING] NOT STARTED, CAUSE CONFIGURED 0 ms\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
  }
}
//-----------------------------------------------------------------------------
void
rlc_um_init_timer_reordering(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const rlc_pP,
  const uint32_t  ms_durationP)
{
  rlc_pP->t_reordering.running         = 0;
  rlc_pP->t_reordering.ms_time_out     = 0;
  rlc_pP->t_reordering.ms_start        = 0;
  rlc_pP->t_reordering.ms_duration     = ms_durationP;
  rlc_pP->t_reordering.timed_out       = 0;
}
//-----------------------------------------------------------------------------
void rlc_um_check_timer_dar_time_out(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const rlc_pP)
{
  signed int     in_window;
  rlc_usn_t      old_vr_ur;


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_CHECK_TIMER_DAR_TIME_OUT,VCD_FUNCTION_IN);

  if ((rlc_pP->t_reordering.running)) {
    if (
      // CASE 1:          start              time out
      //        +-----------+------------------+----------+
      //        |           |******************|          |
      //        +-----------+------------------+----------+
      //FRAME # 0                                     FRAME MAX
      ((rlc_pP->t_reordering.ms_start < rlc_pP->t_reordering.ms_time_out) &&
       ((PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) >= rlc_pP->t_reordering.ms_time_out) ||
        (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) < rlc_pP->t_reordering.ms_start)))                                   ||
      // CASE 2:        time out            start
      //        +-----------+------------------+----------+
      //        |***********|                  |**********|
      //        +-----------+------------------+----------+
      //FRAME # 0                                     FRAME MAX VALUE
      ((rlc_pP->t_reordering.ms_start > rlc_pP->t_reordering.ms_time_out) &&
       (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) < rlc_pP->t_reordering.ms_start) &&
       (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) >= rlc_pP->t_reordering.ms_time_out))
    ) {

      //if ((uint32_t)((uint32_t)rlc_pP->timer_reordering  + (uint32_t)rlc_pP->timer_reordering_init)   <= ctxt_pP->frame) {
      // 5.1.2.2.4   Actions when t-Reordering expires
      //  When t-Reordering expires, the receiving UM RLC entity shall:
      //  -update VR(UR) to the SN of the first UMD PDU with SN >= VR(UX) that has not been received;
      //  -reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR), remove RLC headers when doing so and deliver the reassembled RLC SDUs to upper layer in ascending order of the RLC SN if not delivered before;
      //  -if VR(UH) > VR(UR):
      //      -start t-Reordering;
      //      -set VR(UX) to VR(UH).
      rlc_pP->stat_timer_reordering_timed_out += 1;
#if TRACE_RLC_UM_DAR
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT"*****************************************************\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT"*    T I M E  -  O U T                              *\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT"*****************************************************\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" TIMER t-Reordering expiration\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP));
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" timer_reordering=%d frame=%d expire ms %d\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            rlc_pP->t_reordering.ms_duration,
            ctxt_pP->frame,
            rlc_pP->t_reordering.ms_time_out);
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" set VR(UR)=%03d to",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            rlc_pP->vr_ur);
#endif

      if (pthread_mutex_trylock(&rlc_pP->lock_dar_buffer) == 0) {
        old_vr_ur   = rlc_pP->vr_ur;

        rlc_pP->vr_ur = rlc_pP->vr_ux;

        while (rlc_um_get_pdu_from_dar_buffer(ctxt_pP, rlc_pP, rlc_pP->vr_ur)) {
          rlc_pP->vr_ur = (rlc_pP->vr_ur+1)%rlc_pP->rx_sn_modulo;
        }

#if TRACE_RLC_UM_DAR
        LOG_D(RLC, " %d", rlc_pP->vr_ur);
        LOG_D(RLC, "\n");
#endif
        rlc_um_try_reassembly(ctxt_pP, rlc_pP ,old_vr_ur, rlc_pP->vr_ur);

        in_window = rlc_um_in_window(ctxt_pP, rlc_pP, rlc_pP->vr_ur,  rlc_pP->vr_uh,  rlc_pP->vr_uh);

        if (in_window == 2) {
          rlc_um_start_timer_reordering(ctxt_pP, rlc_pP);
          rlc_pP->vr_ux = rlc_pP->vr_uh;
#if TRACE_RLC_UM_DAR
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" restarting t-Reordering set VR(UX) to %d (VR(UH)>VR(UR))\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                rlc_pP->vr_ux);
#endif
        } else {
#if TRACE_RLC_UM_DAR
          LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" STOP t-Reordering VR(UX) = %03d\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
                rlc_pP->vr_ux);
#endif
          rlc_um_stop_and_reset_timer_reordering(ctxt_pP, rlc_pP);
        }

        RLC_UM_MUTEX_UNLOCK(&rlc_pP->lock_dar_buffer);
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_CHECK_TIMER_DAR_TIME_OUT,VCD_FUNCTION_OUT);
}
//-----------------------------------------------------------------------------
mem_block_t*
rlc_um_remove_pdu_from_dar_buffer(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const rlc_pP,
  rlc_usn_t snP)
{
  mem_block_t * pdu_p     = rlc_pP->dar_buffer[snP];
#if TRACE_RLC_UM_DAR
  LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" REMOVE PDU FROM DAR BUFFER  SN=%03d\n",
        PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
        snP);
#endif
  rlc_pP->dar_buffer[snP] = NULL;
  return pdu_p;
}
//-----------------------------------------------------------------------------
mem_block_t*
rlc_um_get_pdu_from_dar_buffer(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, rlc_usn_t snP)
{
  return rlc_pP->dar_buffer[snP];
}
//-----------------------------------------------------------------------------
void
rlc_um_store_pdu_in_dar_buffer(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const rlc_pP,
  mem_block_t *pdu_pP,
  rlc_usn_t snP)
{
#if TRACE_RLC_UM_DAR
  LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" STORE PDU IN DAR BUFFER  SN=%03d  VR(UR)=%03d VR(UX)=%03d VR(UH)=%03d\n",
        PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
        snP,
        rlc_pP->vr_ur,
        rlc_pP->vr_ux,
        rlc_pP->vr_uh);
#endif
  rlc_pP->dar_buffer[snP] = pdu_pP;
}
//-----------------------------------------------------------------------------
// returns -2 if lower_bound  > sn
// returns -1 if higher_bound < sn
// returns  0 if lower_bound  < sn < higher_bound
// returns  1 if lower_bound  == sn
// returns  2 if higher_bound == sn
// returns  3 if higher_bound == sn == lower_bound
signed int
rlc_um_in_window(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const rlc_pP,
  rlc_sn_t lower_boundP,
  rlc_sn_t snP,
  rlc_sn_t higher_boundP)
{
  rlc_sn_t modulus = (rlc_sn_t)rlc_pP->vr_uh - rlc_pP->rx_um_window_size;
#if TRACE_RLC_UM_RX
  rlc_sn_t     lower_bound  = lower_boundP;
  rlc_sn_t     higher_bound = higher_boundP;
  rlc_sn_t     sn           = snP;
#endif
  lower_boundP  = (lower_boundP  - modulus) % rlc_pP->rx_sn_modulo;
  higher_boundP = (higher_boundP - modulus) % rlc_pP->rx_sn_modulo;
  snP           = (snP           - modulus) % rlc_pP->rx_sn_modulo;

  if ( lower_boundP > snP) {
#if TRACE_RLC_UM_RX
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" %d not in WINDOW[%03d:%03d] (SN<LOWER BOUND)\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn,
          lower_bound,
          higher_bound);
#endif
    return -2;
  }

  if ( higher_boundP < snP) {
#if TRACE_RLC_UM_RX
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" %d not in WINDOW[%03d:%03d] (SN>HIGHER BOUND) <=> %d not in WINDOW[%03d:%03d]\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn,
          lower_bound,
          higher_bound,
          snP,
          lower_boundP,
          higher_boundP);
#endif
    return -1;
  }

  if ( lower_boundP == snP) {
    if ( higher_boundP == snP) {
#if TRACE_RLC_UM_RX
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" %d  in WINDOW[%03d:%03d] (SN=HIGHER BOUND=LOWER BOUND)\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            sn,
            lower_bound,
            higher_bound);
#endif
      return 3;
    }

#if TRACE_RLC_UM_RX
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" %d  in WINDOW[%03d:%03d] (SN=LOWER BOUND)\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn,
          lower_bound,
          higher_bound);
#endif
    return 1;
  }

  if ( higher_boundP == snP) {
#if TRACE_RLC_UM_RX
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" %d  in WINDOW[%03d:%03d] (SN=HIGHER BOUND)\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn,
          lower_bound,
          higher_bound);
#endif
    return 2;
  }

  return 0;

}
//-----------------------------------------------------------------------------
signed int
rlc_um_in_reordering_window(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const rlc_pP,
  const rlc_sn_t snP)
{
  rlc_sn_t   modulus = (signed int)rlc_pP->vr_uh - rlc_pP->rx_um_window_size;
  rlc_sn_t   sn_mod = (snP - modulus) % rlc_pP->rx_sn_modulo;

  if ( 0 <= sn_mod) {
    if (sn_mod < rlc_pP->rx_um_window_size) {
#if TRACE_RLC_UM_DAR
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" %d IN REORDERING WINDOW[%03d:%03d[ SN %d IN [%03d:%03d[ VR(UR)=%03d VR(UH)=%03d\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            sn_mod,
            0,
            rlc_pP->rx_um_window_size,
            snP,
            (signed int)rlc_pP->vr_uh - rlc_pP->rx_um_window_size,
            rlc_pP->vr_uh,
            rlc_pP->vr_ur,
            rlc_pP->vr_uh);
#endif
      return 0;
    }
  }

#if TRACE_RLC_UM_DAR

  if (modulus < 0) {
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" %d NOT IN REORDERING WINDOW[%03d:%03d[ SN %d NOT IN [%03d:%03d[ VR(UR)=%03d VR(UH)=%03d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn_mod,
          modulus + 1024,
          rlc_pP->rx_um_window_size,
          snP,
          modulus + 1024 ,
          rlc_pP->vr_uh,
          rlc_pP->vr_ur,
          rlc_pP->vr_uh);
  } else {
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" %d NOT IN REORDERING WINDOW[%03d:%03d[ SN %d NOT IN [%03d:%03d[ VR(UR)=%03d VR(UH)=%03d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn_mod,
          modulus,
          rlc_pP->rx_um_window_size,
          snP,
          modulus ,
          rlc_pP->vr_uh,
          rlc_pP->vr_ur,
          rlc_pP->vr_uh);
  }

#endif
  return -1;
}
//-----------------------------------------------------------------------------
void
rlc_um_receive_process_dar (
  const protocol_ctxt_t* const ctxt_pP,
  rlc_um_entity_t * const      rlc_pP,
  mem_block_t *                pdu_mem_pP,
  rlc_um_pdu_sn_10_t * const   pdu_pP,
  const sdu_size_t             tb_sizeP)
{
  // 36.322v9.3.0 section 5.1.2.2.1:
  // The receiving UM RLC entity shall maintain a reordering window according to state variable VR(UH) as follows:
  //      -a SN falls within the reordering window if (VR(UH) – UM_Window_Size) <= SN < VR(UH);
  //      -a SN falls outside of the reordering window otherwise.
  // When receiving an UMD PDU from lower layer, the receiving UM RLC entity shall:
  //      -either discard the received UMD PDU or place it in the reception buffer (see sub clause 5.1.2.2.2);
  //      -if the received UMD PDU was placed in the reception buffer:
  //          -update state variables, reassemble and deliver RLC SDUs to upper layer and start/stop t-Reordering as needed (see sub clause 5.1.2.2.3);
  // When t-Reordering expires, the receiving UM RLC entity shall:
  // -   update state variables, reassemble and deliver RLC SDUs to upper layer and start t-Reordering as needed (see sub clause 5.1.2.2.4).



  // When an UMD PDU with SN = x is received from lower layer, the receiving UM RLC entity shall:
  // -if VR(UR) < x < VR(UH) and the UMD PDU with SN = x has been received before; or
  // -if (VR(UH) – UM_Window_Size) <= x < VR(UR):
  //      -discard the received UMD PDU;
  // -else:
  //      -place the received UMD PDU in the reception buffer.

  rlc_sn_t sn = -1;
  signed int in_window;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_RECEIVE_PROCESS_DAR, VCD_FUNCTION_IN);

  if (rlc_pP->rx_sn_length == 10) {
    sn = ((pdu_pP->b1 & 0x00000003) << 8) + pdu_pP->b2;
  } else if (rlc_pP->rx_sn_length == 5) {
    sn = pdu_pP->b1 & 0x1F;
  } else {
    free_mem_block(pdu_mem_pP, __func__);
    pdu_mem_pP = NULL;
    return;
  }

  RLC_UM_MUTEX_LOCK(&rlc_pP->lock_dar_buffer, ctxt_pP, rlc_pP);

  in_window = rlc_um_in_window(ctxt_pP, rlc_pP, rlc_pP->vr_uh - rlc_pP->rx_um_window_size, sn, rlc_pP->vr_ur);

#if TRACE_RLC_PAYLOAD
  rlc_util_print_hex_octets(RLC, &pdu_pP->b1, tb_sizeP);
#endif

  // rlc_um_in_window() returns -2 if lower_bound  > sn
  // rlc_um_in_window() returns -1 if higher_bound < sn
  // rlc_um_in_window() returns  0 if lower_bound  < sn < higher_bound
  // rlc_um_in_window() returns  1 if lower_bound  == sn
  // rlc_um_in_window() returns  2 if higher_bound == sn
  // rlc_um_in_window() returns  3 if higher_bound == sn == lower_bound
  if ((in_window == 1) || (in_window == 0)) {
#if TRACE_RLC_UM_DAR
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" RX PDU  VR(UH) – UM_Window_Size) <= SN %d < VR(UR) -> GARBAGE\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn);
#endif
    rlc_pP->stat_rx_data_pdu_out_of_window   += 1;
    rlc_pP->stat_rx_data_bytes_out_of_window += tb_sizeP;
    free_mem_block(pdu_mem_pP, __func__);
    pdu_mem_pP = NULL;
    RLC_UM_MUTEX_UNLOCK(&rlc_pP->lock_dar_buffer);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_RECEIVE_PROCESS_DAR, VCD_FUNCTION_OUT);
    return;
  }

  if ((rlc_um_get_pdu_from_dar_buffer(ctxt_pP, rlc_pP, sn))) {
    in_window = rlc_um_in_window(ctxt_pP, rlc_pP, rlc_pP->vr_ur, sn, rlc_pP->vr_uh);

    if (in_window == 0) {
#if TRACE_RLC_UM_DAR
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" RX PDU  VR(UR) < SN %d < VR(UH) and RECEIVED BEFORE-> GARBAGE\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            sn);
#endif
      //discard the PDU
      rlc_pP->stat_rx_data_pdus_duplicate  += 1;
      rlc_pP->stat_rx_data_bytes_duplicate += tb_sizeP;
      free_mem_block(pdu_mem_pP, __func__);
      pdu_mem_pP = NULL;
      RLC_UM_MUTEX_UNLOCK(&rlc_pP->lock_dar_buffer);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_RECEIVE_PROCESS_DAR, VCD_FUNCTION_OUT);
      return;
    }

    // 2 lines to avoid memory leaks
    rlc_pP->stat_rx_data_pdus_duplicate  += 1;
    rlc_pP->stat_rx_data_bytes_duplicate += tb_sizeP;
#if TRACE_RLC_UM_DAR
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" RX PDU SN %03d REMOVE OLD PDU BEFORE STORING NEW PDU\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn);
#endif
    mem_block_t *pdu = rlc_um_remove_pdu_from_dar_buffer(ctxt_pP, rlc_pP, sn);
    free_mem_block(pdu, __func__);
  }

  rlc_um_store_pdu_in_dar_buffer(ctxt_pP, rlc_pP, pdu_mem_pP, sn);


  // -if x falls outside of the reordering window:
  //      -update VR(UH) to x + 1;
  //      -reassemble RLC SDUs from any UMD PDUs with SN that falls outside of
  //       the reordering window, remove RLC headers when doing so and deliver
  //       the reassembled RLC SDUs to upper layer in ascending order of the
  //       RLC SN if not delivered before;
  //
  //      -if VR(UR) falls outside of the reordering window:
  //          -set VR(UR) to (VR(UH) – UM_Window_Size);
  if (rlc_um_in_reordering_window(ctxt_pP, rlc_pP, sn) < 0) {
#if TRACE_RLC_UM_DAR
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" RX PDU  SN %d OUTSIDE REORDERING WINDOW VR(UH)=%d UM_Window_Size=%d\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
          sn,
          rlc_pP->vr_uh,
          rlc_pP->rx_um_window_size);
#endif
    rlc_pP->vr_uh = (sn + 1) % rlc_pP->rx_sn_modulo;

    if (rlc_um_in_reordering_window(ctxt_pP, rlc_pP, rlc_pP->vr_ur) != 0) {
      in_window = rlc_pP->vr_uh - rlc_pP->rx_um_window_size;

      if (in_window < 0) {
        in_window = in_window + rlc_pP->rx_sn_modulo;
      }

      rlc_um_try_reassembly(ctxt_pP, rlc_pP, rlc_pP->vr_ur, in_window);
    }


    if (rlc_um_in_reordering_window(ctxt_pP, rlc_pP, rlc_pP->vr_ur) < 0) {
#if TRACE_RLC_UM_DAR
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" VR(UR) %d OUTSIDE REORDERING WINDOW SET TO VR(UH) – UM_Window_Size = %d\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            rlc_pP->vr_ur,
            in_window);
#endif
      rlc_pP->vr_ur = in_window;
    }
  }

  // -if the reception buffer contains an UMD PDU with SN = VR(UR):
  //      -update VR(UR) to the SN of the first UMD PDU with SN > current
  //          VR(UR) that has not been received;
  //      -reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR),
  //          remove RLC headers when doing so and deliver the reassembled RLC
  //          SDUs to upper layer in ascending order of the RLC SN if not
  //          delivered before;
  if ((sn == rlc_pP->vr_ur) && rlc_um_get_pdu_from_dar_buffer(ctxt_pP, rlc_pP, rlc_pP->vr_ur)) {
    //sn_tmp = rlc_pP->vr_ur;
    do {
      rlc_pP->vr_ur = (rlc_pP->vr_ur+1) % rlc_pP->rx_sn_modulo;
    } while (rlc_um_get_pdu_from_dar_buffer(ctxt_pP, rlc_pP, rlc_pP->vr_ur) && (rlc_pP->vr_ur != rlc_pP->vr_uh));

    rlc_um_try_reassembly(ctxt_pP, rlc_pP, sn, rlc_pP->vr_ur);
  }

  // -if t-Reordering is running:
  //      -if VR(UX) <= VR(UR); or
  //      -if VR(UX) falls outside of the reordering window and VR(UX) is not
  //          equal to VR(UH)::
  //          -stop and reset t-Reordering;
  if (rlc_pP->t_reordering.running) {
    if (rlc_pP->vr_uh != rlc_pP->vr_ux) {
      in_window = rlc_um_in_reordering_window(ctxt_pP, rlc_pP, rlc_pP->vr_ux);

      if (in_window < 0) {
#if TRACE_RLC_UM_DAR
        LOG_D(RLC,
              PROTOCOL_RLC_UM_CTXT_FMT" STOP and RESET t-Reordering because VR(UX) falls outside of the reordering window and VR(UX)=%d is not equal to VR(UH)=%d -or- VR(UX) <= VR(UR)\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
              rlc_pP->vr_ux,
              rlc_pP->vr_uh);
#endif
        rlc_um_stop_and_reset_timer_reordering(ctxt_pP, rlc_pP);
      }
    }
  }

  if (rlc_pP->t_reordering.running) {
    in_window = rlc_um_in_window(ctxt_pP, rlc_pP, rlc_pP->vr_ur,  rlc_pP->vr_ux,  rlc_pP->vr_uh);

    if ((in_window == -2) || (in_window == 1)) {
#if TRACE_RLC_UM_DAR
      LOG_D(RLC,
            PROTOCOL_RLC_UM_CTXT_FMT" STOP and RESET t-Reordering because VR(UX) falls outside of the reordering window and VR(UX)=%d is not equal to VR(UH)=%d\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            rlc_pP->vr_ux,
            rlc_pP->vr_uh);
#endif
      rlc_um_stop_and_reset_timer_reordering(ctxt_pP, rlc_pP);
    }
  }

  // -if t-Reordering is not running (includes the case when t-Reordering is
  //      stopped due to actions above):
  //      -if VR(UH) > VR(UR):
  //          -start t-Reordering;
  //          -set VR(UX) to VR(UH).
  if (rlc_pP->t_reordering.running == 0) {
    in_window = rlc_um_in_window(ctxt_pP, rlc_pP, rlc_pP->vr_ur,  rlc_pP->vr_uh,  rlc_pP->vr_uh);

    if (in_window == 2) {
      rlc_um_start_timer_reordering(ctxt_pP, rlc_pP);
      rlc_pP->vr_ux = rlc_pP->vr_uh;
#if TRACE_RLC_UM_DAR
      LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" RESTART t-Reordering set VR(UX) to VR(UH) =%d\n",
            PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP, rlc_pP),
            rlc_pP->vr_ux);
#endif
    }
  }

  RLC_UM_MUTEX_UNLOCK(&rlc_pP->lock_dar_buffer);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_UM_RECEIVE_PROCESS_DAR, VCD_FUNCTION_OUT);
}
