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
#define RLC_AM_RECEIVER_C 1
#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "assertions.h"
#include "msc.h"
#include "rlc.h"
#include "rlc_am.h"
#include "list.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"


//-----------------------------------------------------------------------------
signed int
rlc_am_get_data_pdu_infos(
  const protocol_ctxt_t* const ctxt_pP,
  const rlc_am_entity_t* const rlc_pP,
  rlc_am_pdu_sn_10_t* header_pP,
  int16_t total_sizeP,
  rlc_am_pdu_info_t* pdu_info_pP)
{
    memset(pdu_info_pP, 0, sizeof (rlc_am_pdu_info_t));

    int16_t          sum_li = 0;
    pdu_info_pP->d_c = header_pP->b1 >> 7;
    pdu_info_pP->num_li = 0;

//Assertion(eNB)_PRAN_DesignDocument_annex No.766
  if(pdu_info_pP->d_c == 0)
  {
     LOG_E(RLC, "RLC AM Rx PDU Data D/C Header Error LcId=%d\n", rlc_pP->channel_id);
     return -2;
  }
/*
    AssertFatal (pdu_info_pP->d_c != 0, "RLC AM Rx PDU Data D/C Header Error LcId=%d\n", rlc_pP->channel_id);
*/
    pdu_info_pP->rf  = (header_pP->b1 >> 6) & 0x01;
    pdu_info_pP->p   = (header_pP->b1 >> 5) & 0x01;
    pdu_info_pP->fi  = (header_pP->b1 >> 3) & 0x03;
    pdu_info_pP->e   = (header_pP->b1 >> 2) & 0x01;
    pdu_info_pP->sn  = header_pP->b2 +  (((uint16_t)(header_pP->b1 & 0x03)) << 8);

    pdu_info_pP->header_size  = 2;

    if (pdu_info_pP->rf) {
      pdu_info_pP->lsf = (header_pP->data[0] >> 7) & 0x01;
      pdu_info_pP->so  = header_pP->data[1] +  (((uint16_t)(header_pP->data[0] & 0x7F)) << 8);
      pdu_info_pP->payload = &header_pP->data[2];
      pdu_info_pP->header_size  += 2;
    } else {
      pdu_info_pP->payload = &header_pP->data[0];
    }

    if (pdu_info_pP->e) {
      rlc_am_e_li_t      *e_li;
      unsigned int li_length_in_bytes  = 1;
      unsigned int li_to_read          = 1;

      if (pdu_info_pP->rf) {
        e_li = (rlc_am_e_li_t*)(&header_pP->data[2]);
      } else {
        e_li = (rlc_am_e_li_t*)(header_pP->data);
      }

      while (li_to_read)  {
        li_length_in_bytes = li_length_in_bytes ^ 3;

        if (li_length_in_bytes  == 2) {
          pdu_info_pP->li_list[pdu_info_pP->num_li] = ((uint16_t)(e_li->b1 << 4)) & 0x07F0;
          pdu_info_pP->li_list[pdu_info_pP->num_li] |= (((uint8_t)(e_li->b2 >> 4)) & 0x000F);
          li_to_read = e_li->b1 & 0x80;
          pdu_info_pP->header_size  += 2;
        } else {
          pdu_info_pP->li_list[pdu_info_pP->num_li] = ((uint16_t)(e_li->b2 << 8)) & 0x0700;
          pdu_info_pP->li_list[pdu_info_pP->num_li] |=  e_li->b3;
          li_to_read = e_li->b2 & 0x08;
          e_li++;
          pdu_info_pP->header_size  += 1;
        }

        sum_li += pdu_info_pP->li_list[pdu_info_pP->num_li];
        pdu_info_pP->num_li = pdu_info_pP->num_li + 1;

        if (pdu_info_pP->num_li > RLC_AM_MAX_SDU_IN_PDU) {
          LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[GET PDU INFO]  SN %04d TOO MANY LIs ",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                pdu_info_pP->sn);
          return -2;
        }
      }

      if (li_length_in_bytes  == 2) {
        pdu_info_pP->payload = &e_li->b3;
      } else {
        pdu_info_pP->payload = &e_li->b1;
      }
    }

    pdu_info_pP->payload_size = total_sizeP - pdu_info_pP->header_size;

    if (pdu_info_pP->payload_size > sum_li) {
      pdu_info_pP->hidden_size = pdu_info_pP->payload_size - sum_li;
    }

    return 0;
}
//-----------------------------------------------------------------------------
void
rlc_am_display_data_pdu_infos(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const rlc_pP,
  rlc_am_pdu_info_t* pdu_info_pP)
{
  int num_li;

  if (pdu_info_pP->d_c) {
    if (pdu_info_pP->rf) {
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[DISPLAY DATA PDU] RX DATA PDU SN %04d FI %1d SO %05d LSF %01d POLL %1d ",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            pdu_info_pP->sn,
            pdu_info_pP->fi,
            pdu_info_pP->so,
            pdu_info_pP->lsf, pdu_info_pP->p);
    } else {
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[DISPLAY DATA PDU] RX DATA PDU SN %04d FI %1d POLL %1d ",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            pdu_info_pP->sn,
            pdu_info_pP->fi,
            pdu_info_pP->p);
    }

    for (num_li = 0; num_li < pdu_info_pP->num_li; num_li++) {
      LOG_D(RLC, "LI %05d ",  pdu_info_pP->li_list[num_li]);
    }

    if (pdu_info_pP->hidden_size > 0) {
      LOG_D(RLC, "hidden size %05d ",  pdu_info_pP->hidden_size);
    }

    LOG_D(RLC, "\n");
  } else {
    LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[DISPLAY DATA PDU] ERROR RX CONTROL PDU\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
  }
}
// assumed the sn of the tb_p is equal to VR(MS)
//-----------------------------------------------------------------------------
void
rlc_am_rx_update_vr_ms(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const rlc_pP,
  mem_block_t* tb_pP)
{
  //rlc_am_pdu_info_t* pdu_info_p        = &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
  rlc_am_pdu_info_t* pdu_info_cursor_p = NULL;
  mem_block_t*       cursor_p;

  cursor_p = tb_pP;

  if (cursor_p) {
    do {
      pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

      if ((((rlc_am_rx_pdu_management_t*)(cursor_p->data))->all_segments_received == 0) ||
          (rlc_pP->vr_ms != pdu_info_cursor_p->sn)) {

#if TRACE_RLC_AM_RX
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[UPDATE VR(MS)] UPDATED VR(MS) %04d -> %04d\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              rlc_pP->vr_ms, pdu_info_cursor_p->sn);
#endif

        return;
      }

      rlc_pP->vr_ms = RLC_AM_NEXT_SN(pdu_info_cursor_p->sn);
      cursor_p = cursor_p->next;
    } while ((cursor_p != NULL) && (rlc_pP->vr_ms != rlc_pP->vr_h));

  }
}
// assumed the sn of the tb_p is equal to VR(R)
//-----------------------------------------------------------------------------
void
rlc_am_rx_update_vr_r(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const rlc_pP,
  mem_block_t* tb_pP)
{
  rlc_am_pdu_info_t* pdu_info_cursor_p = NULL;
  mem_block_t*       cursor_p;

  cursor_p = tb_pP;

  if (cursor_p) {
    do {
      pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

      if ((((rlc_am_rx_pdu_management_t*)(cursor_p->data))->all_segments_received == 0) ||
          (rlc_pP->vr_r != pdu_info_cursor_p->sn)) {
        return;
      }

#if TRACE_RLC_AM_RX
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[UPDATE VR(R)] UPDATED VR(R) %04d -> %04d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            rlc_pP->vr_r,
            (pdu_info_cursor_p->sn + 1) & RLC_AM_SN_MASK);
#endif

      if (((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.rf == 1) {
        if (((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.lsf == 1) {
          rlc_pP->vr_r = (rlc_pP->vr_r + 1) & RLC_AM_SN_MASK;
        }
      } else  {
        rlc_pP->vr_r = (rlc_pP->vr_r + 1) & RLC_AM_SN_MASK;
      }

      cursor_p = cursor_p->next;
    } while (cursor_p != NULL);

    //rlc_pP->vr_r = (pdu_info_cursor_p->sn + 1) & RLC_AM_SN_MASK;
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_receive_routing (
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const rlc_pP,
  struct mac_data_ind data_indP)
{
  mem_block_t           *tb_p             = NULL;
  uint8_t               *first_byte_p     = NULL;
  sdu_size_t             tb_size_in_bytes;
  RLC_AM_MUTEX_LOCK(&rlc_pP->lock_input_sdus, ctxt_pP, rlc_pP);

  while ((tb_p = list_remove_head (&data_indP.data))) {
    first_byte_p = ((struct mac_tb_ind *) (tb_p->data))->data_ptr;
    tb_size_in_bytes = ((struct mac_tb_ind *) (tb_p->data))->size;

    if (tb_size_in_bytes > 0) {
      if ((*first_byte_p & 0x80) == 0x80) {
        rlc_pP->stat_rx_data_bytes += tb_size_in_bytes;
        rlc_pP->stat_rx_data_pdu   += 1;
        rlc_am_receive_process_data_pdu (ctxt_pP, rlc_pP, tb_p, first_byte_p, tb_size_in_bytes);
      } else {
        rlc_pP->stat_rx_control_bytes += tb_size_in_bytes;
        rlc_pP->stat_rx_control_pdu += 1;
        rlc_am_receive_process_control_pdu (ctxt_pP, rlc_pP, tb_p, &first_byte_p, &tb_size_in_bytes);
        // Test if remaining bytes not processed (up to know, highest probability is bug in MAC)
//Assertion(eNB)_PRAN_DesignDocument_annex No.767
  if(tb_size_in_bytes != 0)
  {
     LOG_E(RLC, "Remaining %d bytes following a control PDU\n",
             tb_size_in_bytes);
  }
/*
        AssertFatal( tb_size_in_bytes == 0,
                     "Remaining %d bytes following a control PDU",
                     tb_size_in_bytes);
*/
      }

      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RX ROUTING] VR(R)=%03d VR(MR)=%03d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            rlc_pP->vr_r,
            rlc_pP->vr_mr);
    }
  } // end while
  RLC_AM_MUTEX_UNLOCK(&rlc_pP->lock_input_sdus);
}
//-----------------------------------------------------------------------------
void
rlc_am_receive_process_data_pdu (
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const rlc_pP,
  mem_block_t* tb_pP,
  uint8_t* first_byte_pP,
  uint16_t tb_size_in_bytesP)
{
  // 5.1.3.2 Receive operations
  // 5.1.3.2.1 General
  // The receiving side of an AM RLC entity shall maintain a receiving window according to state variables VR(R) and
  // VR(MR) as follows:
  //     - a SN falls within the receiving window if VR(R) <= SN < VR(MR);
  //     - a SN falls outside of the receiving window otherwise.
  //
  // When receiving a RLC data PDU from lower layer, the receiving side of an AM RLC entity shall:
  // - either discard the received RLC data PDU or place it in the reception buffer (see sub clause 5.1.3.2.2);
  // - if the received RLC data PDU was placed in the reception buffer:
  //     - update state variables, reassemble and deliver RLC SDUs to upper layer and start/stop t-Reordering as
  //       needed (see sub clause 5.1.3.2.3).
  // When t-Reordering expires, the receiving side of an AM RLC entity shall:
  //     - update state variables and start t-Reordering as needed (see sub clause 5.1.3.2.4).


  // 5.1.3.2.2 Actions when a RLC data PDU is received from lower layer
  // When a RLC data PDU is received from lower layer, where the RLC data PDU contains byte segment numbers y to z of
  // an AMD PDU with SN = x, the receiving side of an AM RLC entity shall:
  //     - if x falls outside of the receiving window; or
  //     - if byte segment numbers y to z of the AMD PDU with SN = x have been received before:
  //         - discard the received RLC data PDU;
  //     - else:
  //         - place the received RLC data PDU in the reception buffer;
  //         - if some byte segments of the AMD PDU contained in the RLC data PDU have been received before:
  //             - discard the duplicate byte segments.
  rlc_am_pdu_info_t*  pdu_info_p         = &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
  rlc_am_pdu_sn_10_t* rlc_am_pdu_sn_10_p = (rlc_am_pdu_sn_10_t*)first_byte_pP;
  rlc_am_rx_pdu_status_t pdu_status		= RLC_AM_DATA_PDU_STATUS_OK;
  boolean_t		reassemble = false;

  if (rlc_am_get_data_pdu_infos(ctxt_pP,rlc_pP, rlc_am_pdu_sn_10_p, tb_size_in_bytesP, pdu_info_p) >= 0) {

    ((rlc_am_rx_pdu_management_t*)(tb_pP->data))->all_segments_received = 0;

      if (RLC_AM_SN_IN_WINDOW(pdu_info_p->sn, rlc_pP->vr_r)) {

      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SN=%04d] VR(R) %04d VR(H) %04d VR(MR) %04d VR(MS) %04d VR(X) %04d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            pdu_info_p->sn,
            rlc_pP->vr_r,
            rlc_pP->vr_h,
            rlc_pP->vr_mr,
            rlc_pP->vr_ms,
            rlc_pP->vr_x);

      pdu_status = rlc_am_rx_list_check_duplicate_insert_pdu(ctxt_pP, rlc_pP,tb_pP);
      if (pdu_status != RLC_AM_DATA_PDU_STATUS_OK) {
        rlc_pP->stat_rx_data_pdu_dropped     += 1;
        rlc_pP->stat_rx_data_bytes_dropped   += tb_size_in_bytesP;
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU]  PDU DISCARDED CAUSE=%d SN=%d\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_status,pdu_info_p->sn);
#if RLC_STOP_ON_LOST_PDU
        AssertFatal( 0 == 1,
                     PROTOCOL_RLC_AM_CTXT_FMT" LOST PDU DETECTED\n",
                     PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
#endif
      } else {
        // 5.1.3.2.3
        // Actions when a RLC data PDU is placed in the reception buffer
        //
        // When a RLC data PDU with SN = x is placed in the reception buffer, the receiving side of an AM RLC entity shall:
        //     - if x >= VR(H)
        //         - update VR(H) to x+ 1;
        //
        //     - if all byte segments of the AMD PDU with SN = VR(MS) are received:
        //         - update VR(MS) to the SN of the first AMD PDU with SN > current VR(MS) for which not all byte segments
        //           have been received;
        //
        //     - if x = VR(R):
        //         - if all byte segments of the AMD PDU with SN = VR(R) are received:
        //             - update VR(R) to the SN of the first AMD PDU with SN > current VR(R) for which not all byte segments
        //               have been received;
        //             - update VR(MR) to the updated VR(R) + AM_Window_Size;
        //
        //         - reassemble RLC SDUs from any byte segments of AMD PDUs with SN that falls outside of the receiving
        //           window and in-sequence byte segments of the AMD PDU with SN = VR(R), remove RLC headers when
        //           doing so and deliver the reassembled RLC SDUs to upper layer in sequence if not delivered before;
        //
        //     - if t-Reordering is running:
        //         - if VR(X) = VR(R); or
        //         - if VR(X) falls outside of the receiving window and VR(X) is not equal to VR(MR):
        //             - stop and reset t-Reordering;
        //
        //     - if t-Reordering is not running (includes the case t-Reordering is stopped due to actions above):
        //         - if VR (H) > VR(R):
        //             - start t-Reordering;
        //             - set VR(X) to VR(H).


#if TRACE_RLC_AM_RX
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU]  RX LIST AFTER INSERTION:\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
        rlc_am_rx_list_display(rlc_pP, "rlc_am_receive_process_data_pdu AFTER INSERTION ");
#endif

        /* 1) Update vrH if sn >= vrH */
        if (RLC_AM_DIFF_SN(pdu_info_p->sn,rlc_pP->vr_r) >= RLC_AM_DIFF_SN(rlc_pP->vr_h,rlc_pP->vr_r))
        {
        	rlc_pP->vr_h = RLC_AM_NEXT_SN(pdu_info_p->sn);
        }

        rlc_am_rx_check_all_byte_segments(ctxt_pP, rlc_pP, tb_pP);

        /* 2) Reordering Window Processing: Update vr_ms if sn = vr_ms and all bytes received for sn */
        if ((pdu_info_p->sn == rlc_pP->vr_ms) && (((rlc_am_rx_pdu_management_t*)(tb_pP->data))->all_segments_received)) {
          rlc_am_rx_update_vr_ms(ctxt_pP, rlc_pP,  tb_pP);
        }

        if (pdu_info_p->sn == rlc_pP->vr_r) {
mem_block_t*       cursor_p                    = rlc_pP->receiver_buffer.head;
rlc_am_rx_pdu_management_t * pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);
if( (((rlc_am_rx_pdu_management_t*)(tb_pP->data))->all_segments_received) == (pdu_cursor_mgnt_p->all_segments_received)){
          if (((rlc_am_rx_pdu_management_t*)(tb_pP->data))->all_segments_received) {
            rlc_am_rx_update_vr_r(ctxt_pP, rlc_pP, tb_pP);
            rlc_pP->vr_mr = (rlc_pP->vr_r + RLC_AM_WINDOW_SIZE) & RLC_AM_SN_MASK;
          }
          reassemble = rlc_am_rx_check_vr_reassemble(ctxt_pP, rlc_pP);
          //TODO : optimization : check whether a reassembly is needed by looking at LI, FI, SO, etc...
}else{
  LOG_E(RLC, "BAD all_segments_received!!! discard buffer!!!\n");
  /* Discard received block if out of window, duplicate or header error */
  free_mem_block (tb_pP, __func__);
}
        }

        //FNA: fix check VrX out of receiving window
        if ((rlc_pP->t_reordering.running) || ((rlc_pP->t_reordering.ms_duration == 0) && (rlc_pP->vr_x != RLC_SN_UNDEFINED))) {
          if ((rlc_pP->vr_x == rlc_pP->vr_r) || (!(RLC_AM_SN_IN_WINDOW(rlc_pP->vr_x, rlc_pP->vr_r)) && (rlc_pP->vr_x != rlc_pP->vr_mr))) {
            rlc_am_stop_and_reset_timer_reordering(ctxt_pP, rlc_pP);
            rlc_pP->vr_x = RLC_SN_UNDEFINED;
          }
        }

        if (!(rlc_pP->t_reordering.running)) {
          if (rlc_pP->vr_h != rlc_pP->vr_r) { // - if VR (H) > VR(R) translated to - if VR (H) != VR(R)
            rlc_pP->vr_x = rlc_pP->vr_h;
            if (rlc_pP->t_reordering.ms_duration != 0) {
                rlc_am_start_timer_reordering(ctxt_pP, rlc_pP);
            }
            else {
            	/* specific case for no timer reordering configured */
            	/* reordering window directly advances with vrH */
            	rlc_pP->vr_ms = rlc_pP->vr_h;

				/* Trigger a Status and clear any existing Delay Flag */
				RLC_AM_SET_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_TRIGGERED_T_REORDERING);
				RLC_AM_CLEAR_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_TRIGGERED_DELAYED);
				rlc_pP->sn_status_triggered_delayed = RLC_SN_UNDEFINED;
            }
          }
        }
      }

      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SN=%04d] NEW VR(R) %04d VR(H) %04d  VR(MS) %04d  VR(MR) %04d VR(X) %04d reassemble=%d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            pdu_info_p->sn,
            rlc_pP->vr_r,
            rlc_pP->vr_h,
            rlc_pP->vr_ms,
            rlc_pP->vr_mr,
            rlc_pP->vr_x,
            reassemble);
    } else {
      rlc_pP->stat_rx_data_pdu_out_of_window     += 1;
      rlc_pP->stat_rx_data_bytes_out_of_window   += tb_size_in_bytesP;
      pdu_status = RLC_AM_DATA_PDU_STATUS_SN_OUTSIDE_WINDOW;
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU]  PDU OUT OF RX WINDOW, DISCARDED, SN=%d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_info_p->sn);
    }

      /* 3) Check for triggering a Tx Status PDU if a poll is received or if a pending status was delayed */
      if ((pdu_info_p->p) && (pdu_status < RLC_AM_DATA_PDU_STATUS_BUFFER_FULL)) {
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU]  POLL BIT SET, STATUS REQUESTED:\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

        /* Polling Info Saving for In and Out of Window PDU */
        /* avoid multi status trigger */
        if ((RLC_AM_GET_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_TRIGGERED_DELAYED)) ||
            !(RLC_AM_GET_STATUS(rlc_pP->status_requested,(RLC_AM_STATUS_TRIGGERED_POLL | RLC_AM_STATUS_TRIGGERED_T_REORDERING))))
        {
            RLC_AM_SET_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_TRIGGERED_POLL);

            if ((pdu_status != RLC_AM_DATA_PDU_STATUS_OK) || ((pdu_status == RLC_AM_DATA_PDU_STATUS_OK) &&
                                                           (!(RLC_AM_SN_IN_WINDOW(pdu_info_p->sn,rlc_pP->vr_r)) ||
                                                            (RLC_AM_DIFF_SN(pdu_info_p->sn,rlc_pP->vr_r) < RLC_AM_DIFF_SN(rlc_pP->vr_ms,rlc_pP->vr_r)))
                                                          )
               )
            {
                /* Conditions are met for sending a Status Report */
                /* Then clear Delay Flag and reset its corresponding sn */
                RLC_AM_CLEAR_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_TRIGGERED_DELAYED);
                rlc_pP->sn_status_triggered_delayed = RLC_SN_UNDEFINED;
            }
            else if (rlc_pP->sn_status_triggered_delayed == RLC_SN_UNDEFINED)
            {
                /* Delay status trigger if pdustatus OK and sn>= vr_ms */
                /* Note: vr_r and vr_ms have been updated */
                RLC_AM_SET_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_TRIGGERED_DELAYED);
                rlc_pP->sn_status_triggered_delayed = pdu_info_p->sn;
            }
        }
      }

      /* ReEnable a previously delayed Status Trigger if PDU discarded or */
      /* sn no more in RxWindow due to RxWindow advance or sn < vr_ms */
      if ((RLC_AM_GET_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_TRIGGERED_DELAYED)) &&
          (pdu_status == RLC_AM_DATA_PDU_STATUS_OK)  &&
          (!(RLC_AM_SN_IN_WINDOW(rlc_pP->sn_status_triggered_delayed,rlc_pP->vr_r)) ||
           (RLC_AM_DIFF_SN(rlc_pP->sn_status_triggered_delayed,rlc_pP->vr_r) < RLC_AM_DIFF_SN(rlc_pP->vr_ms,rlc_pP->vr_r)))
         )
      {
          RLC_AM_CLEAR_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_TRIGGERED_DELAYED);
          rlc_pP->sn_status_triggered_delayed = RLC_SN_UNDEFINED;
      }


  } else {
	  pdu_status = RLC_AM_DATA_PDU_STATUS_HEADER_ERROR;
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU]  PDU DISCARDED BAD HEADER FORMAT SN=%d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_info_p->sn);
  }

  if (pdu_status != RLC_AM_DATA_PDU_STATUS_OK) {
	  /* Discard received block if out of window, duplicate or header error */
      free_mem_block (tb_pP, __func__);
  }
  else if (reassemble) {
	  /* Reassemble SDUs */
	  rlc_am_rx_list_reassemble_rlc_sdus(ctxt_pP, rlc_pP);
  }
}
