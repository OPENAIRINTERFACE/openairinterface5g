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
#define RLC_AM_C 1
//-----------------------------------------------------------------------------
#include "platform_types.h"
#include "platform_constants.h"
//-----------------------------------------------------------------------------

#include "assertions.h"
#include "hashtable.h"
#include "rlc_am.h"
#include "rlc_am_segment.h"
#include "rlc_am_timer_poll_retransmit.h"
#include "mac_primitives.h"
#include "rlc_primitives.h"
#include "list.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
#include "LTE_UL-AM-RLC.h"
#include "LTE_DL-AM-RLC.h"

//-----------------------------------------------------------------------------
uint32_t
rlc_am_get_status_pdu_buffer_occupancy(
  rlc_am_entity_t *const      rlc_pP) {
  //Compute Max Status PDU size according to what has been received and not received in the window [vrR vrMS[
  // minimum header size in bits to be transmitted: D/C + CPT + ACK_SN + E1
  uint32_t                    nb_bits_to_transmit   = RLC_AM_PDU_D_C_BITS + RLC_AM_STATUS_PDU_CPT_LENGTH + RLC_AM_SN_BITS + RLC_AM_PDU_E_BITS;
  mem_block_t                  *cursor_p              = rlc_pP->receiver_buffer.head;
  rlc_am_pdu_info_t            *pdu_info_cursor_p     = NULL;
  int                           waited_so             = 0;
  rlc_sn_t sn_cursor = rlc_pP->vr_r;
  rlc_sn_t sn_prev = rlc_pP->vr_r;
  rlc_sn_t sn_end = rlc_pP->vr_ms;
  bool segment_loop_end    = false;

  if (sn_prev != sn_end) {
    while ((RLC_AM_DIFF_SN(sn_prev,rlc_pP->vr_r) < RLC_AM_DIFF_SN(sn_end,rlc_pP->vr_r)) && (cursor_p != NULL)) {
      pdu_info_cursor_p     = &((rlc_am_rx_pdu_management_t *)(cursor_p->data))->pdu_info;
      sn_cursor             = pdu_info_cursor_p->sn;

      // Add holes between sn_prev and sn_cursor
      while ((sn_prev != sn_cursor) && (sn_prev != sn_end)) {
        /* Add 1 NACK_SN + E1 + E2 */
        nb_bits_to_transmit += (RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1));
        sn_prev = RLC_AM_NEXT_SN(sn_prev);
      } //end while (sn_prev != sn_cursor)

      /* Handle case sn_cursor is partially received */
      /* Each gap will add NACK_SN + E1 + E2 + SOStart + SOEnd */
      if ((((rlc_am_rx_pdu_management_t *)(cursor_p->data))->all_segments_received == 0) && (RLC_AM_DIFF_SN(sn_cursor,rlc_pP->vr_r) < RLC_AM_DIFF_SN(sn_end,rlc_pP->vr_r))) {
        /* Check lsf */
        segment_loop_end = (pdu_info_cursor_p->lsf == 1);

        /* Fill for [0 SO[ if SO not null */
        if (pdu_info_cursor_p->so) {
          nb_bits_to_transmit += (RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1));
          waited_so = pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size;
        } else {
          waited_so = pdu_info_cursor_p->payload_size;
        }

        /* Go to next segment */
        cursor_p = cursor_p->next;

        if (cursor_p != NULL) {
          pdu_info_cursor_p     = &((rlc_am_rx_pdu_management_t *)(cursor_p->data))->pdu_info;
        }

        /* Fill following gaps if any */
        while (!segment_loop_end) {
          if ((cursor_p != NULL) && (pdu_info_cursor_p->sn == sn_cursor)) {
            /* Check lsf */
            segment_loop_end = (pdu_info_cursor_p->lsf == 1);

            if (waited_so < pdu_info_cursor_p->so) {
              nb_bits_to_transmit += (RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1));
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
          } else {
            /* Fill last gap assuming LSF is not received */
            nb_bits_to_transmit += (RLC_AM_SN_BITS + (RLC_AM_PDU_E_BITS << 1) + (RLC_AM_STATUS_PDU_SO_LENGTH << 1));
            segment_loop_end = true;
          }
        } // end while (!segment_loop_end)
      } // end if segments
      else {
        /* Go to next received PDU or PDU segment with different SN */
        do {
          cursor_p = cursor_p->next;
        } while ((cursor_p != NULL) && (((rlc_am_rx_pdu_management_t *)(cursor_p->data))->pdu_info.sn == sn_cursor));
      }

      sn_prev = RLC_AM_NEXT_SN(sn_cursor);
    }
  } // end if (sn_prev != sn_end)

  // round up to the greatest byte
  return ((nb_bits_to_transmit + 7) >> 3);
}

//-----------------------------------------------------------------------------
uint32_t
rlc_am_get_buffer_occupancy_in_bytes (
  const protocol_ctxt_t *const ctxt_pP,
  rlc_am_entity_t *const      rlc_pP) {
  // priority of control trafic
  rlc_pP->status_buffer_occupancy = 0;

  if ((rlc_pP->status_requested) && !(rlc_pP->status_requested & RLC_AM_STATUS_NO_TX_MASK)) {
    rlc_pP->status_buffer_occupancy = rlc_am_get_status_pdu_buffer_occupancy(rlc_pP);
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" BO : CONTROL PDU %d bytes \n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->status_buffer_occupancy);
  }

  if ( LOG_DEBUGFLAG(DEBUG_RLC)) {
    if ((rlc_pP->status_buffer_occupancy + rlc_pP->retrans_num_bytes_to_retransmit + rlc_pP->sdu_buffer_occupancy ) > 0) {
      LOG_UI(RLC, PROTOCOL_RLC_AM_CTXT_FMT" BO : STATUS  BUFFER %d bytes \n", PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP), rlc_pP->status_buffer_occupancy);
      LOG_UI(RLC, PROTOCOL_RLC_AM_CTXT_FMT" BO : RETRANS BUFFER %d bytes \n", PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP), rlc_pP->retrans_num_bytes_to_retransmit);
      LOG_UI(RLC, PROTOCOL_RLC_AM_CTXT_FMT" BO : SDU	BUFFER %d bytes + li_overhead %d bytes header_overhead %d bytes (nb sdu not segmented %d)\n",
             PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
             rlc_pP->sdu_buffer_occupancy,
             0,
             0,
             rlc_pP->nb_sdu_no_segmented);
    }
  }

  return rlc_pP->status_buffer_occupancy + rlc_pP->retrans_num_bytes_to_retransmit + rlc_pP->sdu_buffer_occupancy;
}
//-----------------------------------------------------------------------------
void
rlc_am_release (
  const protocol_ctxt_t *const ctxt_pP,
  rlc_am_entity_t *const      rlc_pP
) {
  // empty
}
//-----------------------------------------------------------------------------
void
config_req_rlc_am (
  const protocol_ctxt_t *const ctxt_pP,
  const srb_flag_t             srb_flagP,
  const rlc_am_info_t         *config_am_pP,
  const rb_id_t                rb_idP,
  const logical_chan_id_t      chan_idP
) {
  rlc_union_t       *rlc_union_p = NULL;
  rlc_am_entity_t *l_rlc_p         = NULL;
  hash_key_t       key           = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  hashtable_rc_t   h_rc;
  h_rc = hashtable_get(rlc_coll_p, key, (void **)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    l_rlc_p = &rlc_union_p->rlc.am;
    LOG_D(RLC,
          PROTOCOL_RLC_AM_CTXT_FMT" CONFIG_REQ (max_retx_threshold=%d poll_pdu=%d poll_byte=%d t_poll_retransmit=%d t_reord=%d t_status_prohibit=%d)\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p),
          config_am_pP->max_retx_threshold,
          config_am_pP->poll_pdu,
          config_am_pP->poll_byte,
          config_am_pP->t_poll_retransmit,
          config_am_pP->t_reordering,
          config_am_pP->t_status_prohibit);
    rlc_am_init(ctxt_pP, l_rlc_p);
    rlc_am_set_debug_infos(ctxt_pP, l_rlc_p, srb_flagP, rb_idP, chan_idP);
    rlc_am_configure(ctxt_pP, l_rlc_p,
                     config_am_pP->max_retx_threshold,
                     config_am_pP->poll_pdu,
                     config_am_pP->poll_byte,
                     config_am_pP->t_poll_retransmit,
                     config_am_pP->t_reordering,
                     config_am_pP->t_status_prohibit);
  } else {
    LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT" CONFIG_REQ RLC NOT FOUND\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p));
  }
}
uint16_t pollPDU_tab[LTE_PollPDU_pInfinity+1]= {4,8,16,32,64,128,256,RLC_AM_POLL_PDU_INFINITE}; //PollPDU_pInfinity is chosen to 0xFFFF for now
uint32_t maxRetxThreshold_tab[LTE_UL_AM_RLC__maxRetxThreshold_t32+1]= {1,2,3,4,6,8,16,32};
uint32_t pollByte_tab[LTE_PollByte_spare1]= {25000,50000,75000,100000,125000,250000,375000,500000,750000,1000000,1250000,1500000,2000000,3000000,RLC_AM_POLL_BYTE_INFINITE}; // PollByte_kBinfinity is chosen to 0xFFFFFFFF for now
uint32_t PollRetransmit_tab[LTE_T_PollRetransmit_spare5]= {5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,300,350,400,450,500,800,1000,2000,4000};
uint32_t am_t_Reordering_tab[32]= {0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,110,120,130,140,150,160,170,180,190,200,1600};
uint32_t t_StatusProhibit_tab[LTE_T_StatusProhibit_spare2]= {0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,300,350,400,450,500,800,1000,1200,1600,2000,2400};


//-----------------------------------------------------------------------------
void config_req_rlc_am_asn1 (
  const protocol_ctxt_t *const         ctxt_pP,
  const srb_flag_t                     srb_flagP,
  const struct LTE_RLC_Config__am   *const config_am_pP,
  const rb_id_t                        rb_idP,
  const logical_chan_id_t              chan_idP) {
  rlc_union_t     *rlc_union_p   = NULL;
  rlc_am_entity_t *l_rlc_p         = NULL;
  hash_key_t       key           = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  hashtable_rc_t   h_rc;
  h_rc = hashtable_get(rlc_coll_p, key, (void **)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    l_rlc_p = &rlc_union_p->rlc.am;

    if ((config_am_pP->ul_AM_RLC.maxRetxThreshold <= LTE_UL_AM_RLC__maxRetxThreshold_t32) &&
        (config_am_pP->ul_AM_RLC.pollPDU<=LTE_PollPDU_pInfinity) &&
        (config_am_pP->ul_AM_RLC.pollByte<LTE_PollByte_spare1) &&
        (config_am_pP->ul_AM_RLC.t_PollRetransmit<LTE_T_PollRetransmit_spare5) &&
        (config_am_pP->dl_AM_RLC.t_Reordering<32) &&
        (config_am_pP->dl_AM_RLC.t_StatusProhibit<LTE_T_StatusProhibit_spare2) ) {
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" CONFIG_REQ (max_retx_threshold=%d poll_pdu=%d poll_byte=%d t_poll_retransmit=%d t_reord=%d t_status_prohibit=%d)\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p),
            maxRetxThreshold_tab[config_am_pP->ul_AM_RLC.maxRetxThreshold],
            pollPDU_tab[config_am_pP->ul_AM_RLC.pollPDU],
            pollByte_tab[config_am_pP->ul_AM_RLC.pollByte],
            PollRetransmit_tab[config_am_pP->ul_AM_RLC.t_PollRetransmit],
            am_t_Reordering_tab[config_am_pP->dl_AM_RLC.t_Reordering],
            t_StatusProhibit_tab[config_am_pP->dl_AM_RLC.t_StatusProhibit]);
      rlc_am_init(ctxt_pP, l_rlc_p);
      rlc_am_set_debug_infos(ctxt_pP, l_rlc_p, srb_flagP, rb_idP, chan_idP);
      rlc_am_configure(ctxt_pP, l_rlc_p,
                       maxRetxThreshold_tab[config_am_pP->ul_AM_RLC.maxRetxThreshold],
                       pollPDU_tab[config_am_pP->ul_AM_RLC.pollPDU],
                       pollByte_tab[config_am_pP->ul_AM_RLC.pollByte],
                       PollRetransmit_tab[config_am_pP->ul_AM_RLC.t_PollRetransmit],
                       am_t_Reordering_tab[config_am_pP->dl_AM_RLC.t_Reordering],
                       t_StatusProhibit_tab[config_am_pP->dl_AM_RLC.t_StatusProhibit]);
    } else {
      LOG_D(RLC,
            PROTOCOL_RLC_AM_CTXT_FMT"ILLEGAL CONFIG_REQ (max_retx_threshold=%ld poll_pdu=%ld poll_byte=%ld t_poll_retransmit=%ld t_reord=%ld t_status_prohibit=%ld), RLC-AM NOT CONFIGURED\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p),
            config_am_pP->ul_AM_RLC.maxRetxThreshold,
            config_am_pP->ul_AM_RLC.pollPDU,
            config_am_pP->ul_AM_RLC.pollByte,
            config_am_pP->ul_AM_RLC.t_PollRetransmit,
            config_am_pP->dl_AM_RLC.t_Reordering,
            config_am_pP->dl_AM_RLC.t_StatusProhibit);
    }
  } else {
    LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT"CONFIG_REQ RLC NOT FOUND\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p));
  }
}

//-----------------------------------------------------------------------------
void rlc_am_stat_req (
  const protocol_ctxt_t *const         ctxt_pP,
  rlc_am_entity_t *const              rlc_pP,
  unsigned int *stat_tx_pdcp_sdu,
  unsigned int *stat_tx_pdcp_bytes,
  unsigned int *stat_tx_pdcp_sdu_discarded,
  unsigned int *stat_tx_pdcp_bytes_discarded,
  unsigned int *stat_tx_data_pdu,
  unsigned int *stat_tx_data_bytes,
  unsigned int *stat_tx_retransmit_pdu_by_status,
  unsigned int *stat_tx_retransmit_bytes_by_status,
  unsigned int *stat_tx_retransmit_pdu,
  unsigned int *stat_tx_retransmit_bytes,
  unsigned int *stat_tx_control_pdu,
  unsigned int *stat_tx_control_bytes,
  unsigned int *stat_rx_pdcp_sdu,
  unsigned int *stat_rx_pdcp_bytes,
  unsigned int *stat_rx_data_pdus_duplicate,
  unsigned int *stat_rx_data_bytes_duplicate,
  unsigned int *stat_rx_data_pdu,
  unsigned int *stat_rx_data_bytes,
  unsigned int *stat_rx_data_pdu_dropped,
  unsigned int *stat_rx_data_bytes_dropped,
  unsigned int *stat_rx_data_pdu_out_of_window,
  unsigned int *stat_rx_data_bytes_out_of_window,
  unsigned int *stat_rx_control_pdu,
  unsigned int *stat_rx_control_bytes,
  unsigned int *stat_timer_reordering_timed_out,
  unsigned int *stat_timer_poll_retransmit_timed_out,
  unsigned int *stat_timer_status_prohibit_timed_out) {
  *stat_tx_pdcp_sdu                     = rlc_pP->stat_tx_pdcp_sdu;
  *stat_tx_pdcp_bytes                   = rlc_pP->stat_tx_pdcp_bytes;
  *stat_tx_pdcp_sdu_discarded           = rlc_pP->stat_tx_pdcp_sdu_discarded;
  *stat_tx_pdcp_bytes_discarded         = rlc_pP->stat_tx_pdcp_bytes_discarded;
  *stat_tx_data_pdu                     = rlc_pP->stat_tx_data_pdu;
  *stat_tx_data_bytes                   = rlc_pP->stat_tx_data_bytes;
  *stat_tx_retransmit_pdu_by_status     = rlc_pP->stat_tx_retransmit_pdu_by_status;
  *stat_tx_retransmit_bytes_by_status   = rlc_pP->stat_tx_retransmit_bytes_by_status;
  *stat_tx_retransmit_pdu               = rlc_pP->stat_tx_retransmit_pdu;
  *stat_tx_retransmit_bytes             = rlc_pP->stat_tx_retransmit_bytes;
  *stat_tx_control_pdu                  = rlc_pP->stat_tx_control_pdu;
  *stat_tx_control_bytes                = rlc_pP->stat_tx_control_bytes;
  *stat_rx_pdcp_sdu                     = rlc_pP->stat_rx_pdcp_sdu;
  *stat_rx_pdcp_bytes                   = rlc_pP->stat_rx_pdcp_bytes;
  *stat_rx_data_pdus_duplicate          = rlc_pP->stat_rx_data_pdus_duplicate;
  *stat_rx_data_bytes_duplicate         = rlc_pP->stat_rx_data_bytes_duplicate;
  *stat_rx_data_pdu                     = rlc_pP->stat_rx_data_pdu;
  *stat_rx_data_bytes                   = rlc_pP->stat_rx_data_bytes;
  *stat_rx_data_pdu_dropped             = rlc_pP->stat_rx_data_pdu_dropped;
  *stat_rx_data_bytes_dropped           = rlc_pP->stat_rx_data_bytes_dropped;
  *stat_rx_data_pdu_out_of_window       = rlc_pP->stat_rx_data_pdu_out_of_window;
  *stat_rx_data_bytes_out_of_window     = rlc_pP->stat_rx_data_bytes_out_of_window;
  *stat_rx_control_pdu                  = rlc_pP->stat_rx_control_pdu;
  *stat_rx_control_bytes                = rlc_pP->stat_rx_control_bytes;
  *stat_timer_reordering_timed_out      = rlc_pP->stat_timer_reordering_timed_out;
  *stat_timer_poll_retransmit_timed_out = rlc_pP->stat_timer_poll_retransmit_timed_out;
  *stat_timer_status_prohibit_timed_out = rlc_pP->stat_timer_status_prohibit_timed_out;
}
//-----------------------------------------------------------------------------
void
rlc_am_get_pdus (
  const protocol_ctxt_t *const ctxt_pP,
  rlc_am_entity_t *const      rlc_pP
) {
  //int display_flag = 0;
  // 5.1.3.1 Transmit operations
  // 5.1.3.1.1
  // General
  // The transmitting side of an AM RLC entity shall prioritize transmission of RLC control PDUs over RLC data PDUs.
  // The transmitting side of an AM RLC entity shall prioritize retransmission of RLC data PDUs over transmission of new
  // AMD PDUs.
  switch (rlc_pP->protocol_state) {
    case RLC_NULL_STATE:
      break;

    case RLC_DATA_TRANSFER_READY_STATE:

      // TRY TO SEND CONTROL PDU FIRST
      if ((rlc_pP->nb_bytes_requested_by_mac >= 2) &&
          ((rlc_pP->status_requested) && !(rlc_pP->status_requested & RLC_AM_STATUS_NO_TX_MASK))) {
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
        rlc_am_send_status_pdu(ctxt_pP, rlc_pP);
        mem_block_t *pdu = list_remove_head(&rlc_pP->control_pdu_list);

        if (pdu) {
          list_add_tail_eurecom (pdu, &rlc_pP->pdus_to_mac_layer);
          RLC_AM_CLEAR_ALL_STATUS(rlc_pP->status_requested);
          rlc_pP->status_buffer_occupancy = 0;
          rlc_am_start_timer_status_prohibit(ctxt_pP, rlc_pP);
          return;
        }
      } else {
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" DELAYED SENT STATUS PDU (Available MAC Data %u)(T-PROHIBIT %u) (DELAY FLAG %u)\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              rlc_pP->nb_bytes_requested_by_mac,rlc_pP->t_status_prohibit.ms_time_out,(rlc_pP->status_requested & RLC_AM_STATUS_TRIGGERED_DELAYED));
      }

      // THEN TRY TO SEND RETRANS PDU
      if ((rlc_pP->retrans_num_bytes_to_retransmit) && (rlc_pP->nb_bytes_requested_by_mac > 2)) {
        /* Get 1 AM data PDU or PDU segment to retransmit */
        mem_block_t *pdu_retx = rlc_am_get_pdu_to_retransmit(ctxt_pP, rlc_pP);

        if (pdu_retx != NULL) {
          list_add_tail_eurecom (pdu_retx, &rlc_pP->pdus_to_mac_layer);
          return;
        }
      }

      // THEN TRY TO SEND NEW DATA PDU
      if ((rlc_pP->nb_bytes_requested_by_mac > 2) && (rlc_pP->sdu_buffer_occupancy) && (rlc_pP->vt_s != rlc_pP->vt_ms)) {
        rlc_am_segment_10(ctxt_pP, rlc_pP);
        list_add_list (&rlc_pP->segmentation_pdu_list, &rlc_pP->pdus_to_mac_layer);

        if (rlc_pP->pdus_to_mac_layer.head != NULL) {
          rlc_pP->stat_tx_data_pdu                   += 1;
          rlc_pP->stat_tx_data_bytes                 += (((struct mac_tb_req *)(rlc_pP->pdus_to_mac_layer.head->data))->tb_size);
          return;
        }
      }

      break;

    default:
      LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT" MAC_DATA_REQ UNKNOWN PROTOCOL STATE 0x%02X\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            rlc_pP->protocol_state);
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_rx (
  const protocol_ctxt_t *const ctxt_pP,
  void *const                 arg_pP,
  struct mac_data_ind          data_indP
) {
  rlc_am_entity_t *rlc = (rlc_am_entity_t *) arg_pP;

  switch (rlc->protocol_state) {
    case RLC_NULL_STATE:
      LOG_I(RLC, PROTOCOL_RLC_AM_CTXT_FMT" ERROR MAC_DATA_IND IN RLC_NULL_STATE\n", PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP, rlc));
      list_free (&data_indP.data);
      break;

    case RLC_DATA_TRANSFER_READY_STATE:
      rlc_am_receive_routing (ctxt_pP, rlc, data_indP);
      break;

    default:
      LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT" TX UNKNOWN PROTOCOL STATE 0x%02X\n", PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP, rlc), rlc->protocol_state);
      list_free (&data_indP.data);
  }
}

//-----------------------------------------------------------------------------
struct mac_status_resp
rlc_am_mac_status_indication (
  const protocol_ctxt_t *const ctxt_pP,
  void *const                 rlc_pP,
  struct mac_status_ind        tx_statusP,
  const eNB_flag_t enb_flagP) {
  struct mac_status_resp  status_resp;
  uint16_t  sdu_size = 0;
  uint16_t  sdu_remaining_size = 0;
  int32_t diff_time=0;
  rlc_am_entity_t *rlc = (rlc_am_entity_t *) rlc_pP;
  status_resp.buffer_occupancy_in_bytes        = 0;
  status_resp.buffer_occupancy_in_pdus         = 0;
  status_resp.head_sdu_remaining_size_to_send  = 0;
  status_resp.head_sdu_creation_time           = 0;
  status_resp.head_sdu_is_segmented            = 0;
  status_resp.rlc_info.rlc_protocol_state = rlc->protocol_state;

  /* TODO: remove this hack. Problem is: there is a race.
   * UE comes. SRB2 is configured via message to RRC.
   * At some point the RLC AM is created but not configured yet.
   * At this moment (I think) MAC calls mac_rlc_status_ind
   * which calls this function. But the init was not finished yet
   * and we have a crash below when testing mem_block != NULL.
   */
  if (rlc->input_sdus == NULL) return status_resp;

  if (rlc->last_absolute_subframe_status_indication != (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP))) {
    rlc_am_check_timer_poll_retransmit(ctxt_pP, rlc);
    rlc_am_check_timer_reordering(ctxt_pP, rlc);
    rlc_am_check_timer_status_prohibit(ctxt_pP, rlc);
  }

  rlc->last_absolute_subframe_status_indication = PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP);

  status_resp.buffer_occupancy_in_bytes = rlc_am_get_buffer_occupancy_in_bytes(ctxt_pP, rlc);

  // For eNB scheduler : Add Max RLC header size for new PDU
  // For UE : do not add RLC header part to be compliant with BSR definition in 36.321
  if (enb_flagP == ENB_FLAG_YES) {
    uint32_t max_li_overhead = 0;
    uint32_t header_overhead = 0;

    if (rlc->nb_sdu_no_segmented > 1) {
      /* This computation assumes there is no SDU with size greater than 2047 bytes, otherwise a new PDU must be built except for LI15 configuration from Rel12*/
      uint32_t num_li = rlc->nb_sdu_no_segmented - 1;
      max_li_overhead = num_li + (num_li >> 1) + (num_li & 1);
    }

    if (rlc->sdu_buffer_occupancy > 0) {
      header_overhead = 2;
    }

    status_resp.buffer_occupancy_in_bytes += (header_overhead + max_li_overhead);
  }

  if ((rlc->input_sdus[rlc->current_sdu_index].mem_block != NULL) && (status_resp.buffer_occupancy_in_bytes)) {
    //status_resp.buffer_occupancy_in_bytes += ((rlc_am_entity_t *) rlc)->tx_header_min_length_in_bytes;
    status_resp.buffer_occupancy_in_pdus = rlc->nb_sdu;
    diff_time =   ctxt_pP->frame - ((rlc_am_tx_sdu_management_t *) (rlc->input_sdus[rlc->current_sdu_index].mem_block->data))->sdu_creation_time;
    status_resp.head_sdu_creation_time = (diff_time > 0 ) ? (uint32_t) diff_time :  (uint32_t)(0xffffffff - diff_time + ctxt_pP->frame) ;
    sdu_size            = ((rlc_am_tx_sdu_management_t *) (rlc->input_sdus[rlc->current_sdu_index].mem_block->data))->sdu_size;
    sdu_remaining_size  = ((rlc_am_tx_sdu_management_t *) (rlc->input_sdus[rlc->current_sdu_index].mem_block->data))->sdu_remaining_size;
    status_resp.head_sdu_remaining_size_to_send = sdu_remaining_size;

    if (sdu_size == sdu_remaining_size)  {
      status_resp.head_sdu_is_segmented = 0;
    } else {
      status_resp.head_sdu_is_segmented = 1;
    }
  } else {
    /* Not so many possibilities ... */
    /* either buffer_occupancy_in_bytes = 0 and that's it */
    /* or we have segmented all received SDUs and buffer occupancy is then made of retransmissions and/or status pdu pending */
    /* then consider only retransmission buffer for the specific BO values used by eNB scheduler (not used up to now...) */
    if (rlc->retrans_num_bytes_to_retransmit) {
      status_resp.buffer_occupancy_in_pdus = rlc->retrans_num_pdus;
      status_resp.head_sdu_remaining_size_to_send = rlc->retrans_num_bytes_to_retransmit;
      status_resp.head_sdu_is_segmented = 1;
    }
  }

  if (LOG_DEBUGFLAG(DEBUG_RLC)) {
    LOG_UI(RLC, PROTOCOL_RLC_AM_CTXT_FMT" MAC_STATUS_INDICATION (DATA) -> %d bytes\n",
           PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc),
           status_resp.buffer_occupancy_in_bytes);
  }

  return status_resp;
}

//-----------------------------------------------------------------------------
struct mac_data_req
rlc_am_mac_data_request (
  const protocol_ctxt_t *const ctxt_pP,
  void *const                 rlc_pP,
  const eNB_flag_t        enb_flagP
) {
  struct mac_data_req data_req;
  rlc_am_entity_t *l_rlc_p = (rlc_am_entity_t *) rlc_pP;
  unsigned int nb_bytes_requested_by_mac = ((rlc_am_entity_t *) rlc_pP)->nb_bytes_requested_by_mac;
  rlc_am_pdu_info_t   pdu_info;
  rlc_am_pdu_sn_10_t *rlc_am_pdu_sn_10_p;
  mem_block_t        *tb_p;
  tb_size_t           tb_size_in_bytes;
  int                 num_nack;
  char                message_string[9000];
  size_t              message_string_size = 0;
  int                 octet_index, index;
  /* for no gcc warnings */
  (void)num_nack;
  (void)message_string;
  (void)message_string_size;
  (void)octet_index;
  (void)index;
  list_init (&data_req.data, NULL);
  rlc_am_get_pdus (ctxt_pP, l_rlc_p);
  list_add_list (&l_rlc_p->pdus_to_mac_layer, &data_req.data);

  //((rlc_am_entity_t *) rlc_pP)->tx_pdus += data_req.data.nb_elements;
  if ((nb_bytes_requested_by_mac + data_req.data.nb_elements) > 0) {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" MAC_DATA_REQUEST %05d BYTES REQUESTED -> %d TBs\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p),
          nb_bytes_requested_by_mac,
          data_req.data.nb_elements);
  }

  if (enb_flagP) {
    // redundant in UE MAC Tx processing and not used in eNB ...
    data_req.buffer_occupancy_in_bytes   = rlc_am_get_buffer_occupancy_in_bytes(ctxt_pP, l_rlc_p);
  }

  data_req.rlc_info.rlc_protocol_state = l_rlc_p->protocol_state;

  if ( (LOG_DEBUGFLAG(DEBUG_RLC))&& data_req.data.nb_elements > 0) {
    tb_p = data_req.data.head;

    while (tb_p != NULL) {
      rlc_am_pdu_sn_10_p = (rlc_am_pdu_sn_10_t *)((struct mac_tb_req *) (tb_p->data))->data_ptr;
      tb_size_in_bytes   = ((struct mac_tb_req *) (tb_p->data))->tb_size;

      if ((((struct mac_tb_req *) (tb_p->data))->data_ptr[0] & RLC_DC_MASK) == RLC_DC_DATA_PDU ) {
        if (rlc_am_get_data_pdu_infos(ctxt_pP,l_rlc_p,rlc_am_pdu_sn_10_p, tb_size_in_bytes, &pdu_info) >= 0) {
          if ( LOG_DEBUGFLAG(DEBUG_RLC)) {
            message_string_size = 0;
            message_string_size += sprintf(&message_string[message_string_size], "Bearer      : %ld\n", l_rlc_p->rb_id);
            message_string_size += sprintf(&message_string[message_string_size], "PDU size    : %u\n", tb_size_in_bytes);
            message_string_size += sprintf(&message_string[message_string_size], "Header size : %u\n", pdu_info.header_size);
            message_string_size += sprintf(&message_string[message_string_size], "Payload size: %u\n", pdu_info.payload_size);

            if (pdu_info.rf) {
              message_string_size += sprintf(&message_string[message_string_size], "PDU type    : RLC AM DATA REQ: AMD PDU segment\n\n");
            } else {
              message_string_size += sprintf(&message_string[message_string_size], "PDU type    : RLC AM DATA REQ: AMD PDU\n\n");
            }

            message_string_size += sprintf(&message_string[message_string_size], "Header      :\n");
            message_string_size += sprintf(&message_string[message_string_size], "  D/C       : %u\n", pdu_info.d_c);
            message_string_size += sprintf(&message_string[message_string_size], "  RF        : %u\n", pdu_info.rf);
            message_string_size += sprintf(&message_string[message_string_size], "  P         : %u\n", pdu_info.p);
            message_string_size += sprintf(&message_string[message_string_size], "  FI        : %u\n", pdu_info.fi);
            message_string_size += sprintf(&message_string[message_string_size], "  E         : %u\n", pdu_info.e);
            message_string_size += sprintf(&message_string[message_string_size], "  SN        : %u\n", pdu_info.sn);

            if (pdu_info.rf) {
              message_string_size += sprintf(&message_string[message_string_size], "  LSF       : %u\n", pdu_info.lsf);
              message_string_size += sprintf(&message_string[message_string_size], "  SO        : %u\n", pdu_info.so);
            }

            if (pdu_info.e) {
              message_string_size += sprintf(&message_string[message_string_size], "\nHeader extension  : \n");

              for (index=0; index < pdu_info.num_li; index++) {
                message_string_size += sprintf(&message_string[message_string_size], "  LI        : %u\n", pdu_info.li_list[index]);
              }
            }

            message_string_size += sprintf(&message_string[message_string_size], "\nPayload  : \n");
            message_string_size += sprintf(&message_string[message_string_size], "------+-------------------------------------------------|\n");
            message_string_size += sprintf(&message_string[message_string_size], "      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
            message_string_size += sprintf(&message_string[message_string_size], "------+-------------------------------------------------|\n");

            for (octet_index = 0; octet_index < pdu_info.payload_size; octet_index++) {
              if ((octet_index % 16) == 0) {
                if (octet_index != 0) {
                  message_string_size += sprintf(&message_string[message_string_size], " |\n");
                }

                message_string_size += sprintf(&message_string[message_string_size], " %04d |", octet_index);
              }

              /*
               * Print every single octet in hexadecimal form
               */
              message_string_size += sprintf(&message_string[message_string_size], " %02x", pdu_info.payload[octet_index]);
              /*
               * Align newline and pipes according to the octets in groups of 2
               */
            }

            /*
             * Append enough spaces and put final pipe
             */
            for (index = octet_index; index < 16; ++index) {
              message_string_size += sprintf(&message_string[message_string_size], "   ");
            }

            LOG_UI(RLC,"%s\n",message_string);
          } /* LOG_DEBUGFLAG(DEBUG_RLC) */
        }
      } else {
        if (rlc_am_get_control_pdu_infos(rlc_am_pdu_sn_10_p, &tb_size_in_bytes, &l_rlc_p->control_pdu_info) >= 0) {
          tb_size_in_bytes   = ((struct mac_tb_req *) (tb_p->data))->tb_size; //tb_size_in_bytes modified by rlc_am_get_control_pdu_infos!

          if ( LOG_DEBUGFLAG(DEBUG_RLC)) {
            message_string_size = 0;
            message_string_size += sprintf(&message_string[message_string_size], "Bearer      : %ld\n", l_rlc_p->rb_id);
            message_string_size += sprintf(&message_string[message_string_size], "PDU size    : %u\n", tb_size_in_bytes);
            message_string_size += sprintf(&message_string[message_string_size], "PDU type    : RLC AM DATA REQ: STATUS PDU\n\n");
            message_string_size += sprintf(&message_string[message_string_size], "Header      :\n");
            message_string_size += sprintf(&message_string[message_string_size], "  D/C       : %u\n", l_rlc_p->control_pdu_info.d_c);
            message_string_size += sprintf(&message_string[message_string_size], "  CPT       : %u\n", l_rlc_p->control_pdu_info.cpt);
            message_string_size += sprintf(&message_string[message_string_size], "  ACK_SN    : %u\n", l_rlc_p->control_pdu_info.ack_sn);
            message_string_size += sprintf(&message_string[message_string_size], "  E1        : %u\n", l_rlc_p->control_pdu_info.e1);

            for (num_nack = 0; num_nack < l_rlc_p->control_pdu_info.num_nack; num_nack++) {
              if (l_rlc_p->control_pdu_info.nack_list[num_nack].e2) {
                message_string_size += sprintf(&message_string[message_string_size], "  NACK SN %04d SO START %05d SO END %05d",
                                               l_rlc_p->control_pdu_info.nack_list[num_nack].nack_sn,
                                               l_rlc_p->control_pdu_info.nack_list[num_nack].so_start,
                                               l_rlc_p->control_pdu_info.nack_list[num_nack].so_end);
              } else {
                message_string_size += sprintf(&message_string[message_string_size], "  NACK SN %04d",  l_rlc_p->control_pdu_info.nack_list[num_nack].nack_sn);
              }
            }

            LOG_UI(RLC,"%s\n",message_string);
          } /* LOG_DEBUGFLAG(DEBUG_RLC) */
        }
      }

      tb_p = tb_p->next;
    } /* while */
  } /* data_req.data.nb_elements > 0 */

  data_req.buffer_occupancy_in_pdus = 0;
  return data_req;
}
//-----------------------------------------------------------------------------
void
rlc_am_mac_data_indication (
  const protocol_ctxt_t *const ctxt_pP,
  void *const                 rlc_pP,
  struct mac_data_ind          data_indP
) {
  rlc_am_entity_t           *l_rlc_p = (rlc_am_entity_t *) rlc_pP;
  /*rlc_am_control_pdu_info_t control_pdu_info;
  int                       num_li;
  int16_t                     tb_size;*/
  rlc_am_pdu_info_t   pdu_info;
  rlc_am_pdu_sn_10_t *rlc_am_pdu_sn_10_p;
  mem_block_t        *tb_p;
  sdu_size_t          tb_size_in_bytes;
  int                 num_nack;
  char                message_string[7000];
  size_t              message_string_size = 0;
  int                 octet_index, index;
  /* for no gcc warnings */
  (void)num_nack;
  (void)message_string;
  (void)message_string_size;
  (void)octet_index;
  (void)index;
  (void)l_rlc_p; /* avoid gcc warning "unused variable" */

  if ( LOG_DEBUGFLAG(DEBUG_RLC)) {
    if (data_indP.data.nb_elements > 0) {
      tb_p = data_indP.data.head;

      while (tb_p != NULL) {
        rlc_am_pdu_sn_10_p = (rlc_am_pdu_sn_10_t *)((struct mac_tb_ind *) (tb_p->data))->data_ptr;
        tb_size_in_bytes   = ((struct mac_tb_ind *) (tb_p->data))->size;

        if ((((struct mac_tb_ind *) (tb_p->data))->data_ptr[0] & RLC_DC_MASK) == RLC_DC_DATA_PDU ) {
          if (rlc_am_get_data_pdu_infos(ctxt_pP,l_rlc_p,rlc_am_pdu_sn_10_p, tb_size_in_bytes, &pdu_info) >= 0) {
            if ( LOG_DEBUGFLAG(DEBUG_RLC)) {
              message_string_size += sprintf(&message_string[message_string_size], "Bearer	: %ld\n", l_rlc_p->rb_id);
              message_string_size += sprintf(&message_string[message_string_size], "PDU size	: %u\n", tb_size_in_bytes);
              message_string_size += sprintf(&message_string[message_string_size], "Header size : %u\n", pdu_info.header_size);
              message_string_size += sprintf(&message_string[message_string_size], "Payload size: %u\n", pdu_info.payload_size);

              if (pdu_info.rf) {
                message_string_size += sprintf(&message_string[message_string_size], "PDU type    : RLC AM DATA IND: AMD PDU segment\n\n");
              } else {
                message_string_size += sprintf(&message_string[message_string_size], "PDU type    : RLC AM DATA IND: AMD PDU\n\n");
              }

              message_string_size += sprintf(&message_string[message_string_size], "Header      :\n");
              message_string_size += sprintf(&message_string[message_string_size], "  D/C       : %u\n", pdu_info.d_c);
              message_string_size += sprintf(&message_string[message_string_size], "  RF        : %u\n", pdu_info.rf);
              message_string_size += sprintf(&message_string[message_string_size], "  P         : %u\n", pdu_info.p);
              message_string_size += sprintf(&message_string[message_string_size], "  FI        : %u\n", pdu_info.fi);
              message_string_size += sprintf(&message_string[message_string_size], "  E         : %u\n", pdu_info.e);
              message_string_size += sprintf(&message_string[message_string_size], "  SN        : %u\n", pdu_info.sn);

              if (pdu_info.rf) {
                message_string_size += sprintf(&message_string[message_string_size], "  LSF       : %u\n", pdu_info.lsf);
                message_string_size += sprintf(&message_string[message_string_size], "  SO        : %u\n", pdu_info.so);
              }

              if (pdu_info.e) {
                message_string_size += sprintf(&message_string[message_string_size], "\nHeader extension  : \n");

                for (index=0; index < pdu_info.num_li; index++) {
                  message_string_size += sprintf(&message_string[message_string_size], "  LI        : %u\n", pdu_info.li_list[index]);
                }
              }

              message_string_size += sprintf(&message_string[message_string_size], "\nPayload  : \n");
              message_string_size += sprintf(&message_string[message_string_size], "------+-------------------------------------------------|\n");
              message_string_size += sprintf(&message_string[message_string_size], "      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
              message_string_size += sprintf(&message_string[message_string_size], "------+-------------------------------------------------|\n");

              for (octet_index = 0; octet_index < pdu_info.payload_size; octet_index++) {
                if ((octet_index % 16) == 0) {
                  if (octet_index != 0) {
                    message_string_size += sprintf(&message_string[message_string_size], " |\n");
                  }

                  message_string_size += sprintf(&message_string[message_string_size], " %04d |", octet_index);
                }

                /*
                 * Print every single octet in hexadecimal form
                 */
                message_string_size += sprintf(&message_string[message_string_size], " %02x", pdu_info.payload[octet_index]);
                /*
                 * Align newline and pipes according to the octets in groups of 2
                 */
              }

              /*
               * Append enough spaces and put final pipe
               */
              for (index = octet_index; index < 16; ++index) {
                message_string_size += sprintf(&message_string[message_string_size], "   ");
              }

              LOG_UI(RLC,"%s\n",message_string);
            } /* LOG_DEBUGFLAG */
          }
        } else {
          if (rlc_am_get_control_pdu_infos(rlc_am_pdu_sn_10_p, &tb_size_in_bytes, &l_rlc_p->control_pdu_info) >= 0) {
            if ( LOG_DEBUGFLAG(DEBUG_RLC)) {
              message_string_size = 0;
              message_string_size += sprintf(&message_string[message_string_size], "Bearer      : %ld\n", l_rlc_p->rb_id);
              message_string_size += sprintf(&message_string[message_string_size], "PDU size    : %u\n", ((struct mac_tb_ind *) (tb_p->data))->size);
              message_string_size += sprintf(&message_string[message_string_size], "PDU type    : RLC AM DATA IND: STATUS PDU\n\n");
              message_string_size += sprintf(&message_string[message_string_size], "Header      :\n");
              message_string_size += sprintf(&message_string[message_string_size], "  D/C       : %u\n", l_rlc_p->control_pdu_info.d_c);
              message_string_size += sprintf(&message_string[message_string_size], "  CPT       : %u\n", l_rlc_p->control_pdu_info.cpt);
              message_string_size += sprintf(&message_string[message_string_size], "  ACK_SN    : %u\n", l_rlc_p->control_pdu_info.ack_sn);
              message_string_size += sprintf(&message_string[message_string_size], "  E1        : %u\n", l_rlc_p->control_pdu_info.e1);

              for (num_nack = 0; num_nack < l_rlc_p->control_pdu_info.num_nack; num_nack++) {
                if (l_rlc_p->control_pdu_info.nack_list[num_nack].e2) {
                  message_string_size += sprintf(&message_string[message_string_size], "  NACK SN %04d SO START %05d SO END %05d",
                                                 l_rlc_p->control_pdu_info.nack_list[num_nack].nack_sn,
                                                 l_rlc_p->control_pdu_info.nack_list[num_nack].so_start,
                                                 l_rlc_p->control_pdu_info.nack_list[num_nack].so_end);
                } else {
                  message_string_size += sprintf(&message_string[message_string_size], "  NACK SN %04d",  l_rlc_p->control_pdu_info.nack_list[num_nack].nack_sn);
                }
              }

              LOG_UI(RLC, "%s\n",message_string);
            }
          }
        }

        tb_p = tb_p->next;
      }
    }
  } /* LOG_DEBUGFLAG(RLC) || MESSAGE_TRACE_GENERATOR) */

  rlc_am_rx (ctxt_pP, rlc_pP, data_indP);
}

//-----------------------------------------------------------------------------
void
rlc_am_data_req (
  const protocol_ctxt_t *const ctxt_pP,
  void *const                rlc_pP,
  mem_block_t *const         sdu_pP) {
  rlc_am_entity_t     *l_rlc_p = (rlc_am_entity_t *) rlc_pP;
  uint32_t             mui;
  uint16_t             data_offset;
  uint16_t             data_size;
  char                 message_string[7000];
  size_t               message_string_size = 0;
  int                  octet_index, index;
  RLC_AM_MUTEX_LOCK(&l_rlc_p->lock_input_sdus, ctxt_pP, l_rlc_p);

  if ((l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].mem_block == NULL) &&
      (l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].flags.segmented == 0) &&
      (((l_rlc_p->next_sdu_index + 1) % RLC_AM_SDU_CONTROL_BUFFER_SIZE) != l_rlc_p->current_sdu_index)) {
    memset(&l_rlc_p->input_sdus[l_rlc_p->next_sdu_index], 0, sizeof(rlc_am_tx_sdu_management_t));
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].mem_block = sdu_pP;
    mui         = ((struct rlc_am_data_req *) (sdu_pP->data))->mui;
    data_offset = ((struct rlc_am_data_req *) (sdu_pP->data))->data_offset;
    data_size   = ((struct rlc_am_data_req *) (sdu_pP->data))->data_size;

    if (LOG_DEBUGFLAG(DEBUG_RLC)) {
      message_string_size += sprintf(&message_string[message_string_size], "Bearer      : %ld\n", l_rlc_p->rb_id);
      message_string_size += sprintf(&message_string[message_string_size], "SDU size    : %u\n", data_size);
      message_string_size += sprintf(&message_string[message_string_size], "MUI         : %u\n", mui);
      message_string_size += sprintf(&message_string[message_string_size], "CONF        : %u\n", ((struct rlc_am_data_req *) (sdu_pP->data))->conf);
      message_string_size += sprintf(&message_string[message_string_size], "\nPayload  : \n");
      message_string_size += sprintf(&message_string[message_string_size], "------+-------------------------------------------------|\n");
      message_string_size += sprintf(&message_string[message_string_size], "      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
      message_string_size += sprintf(&message_string[message_string_size], "------+-------------------------------------------------|\n");

      for (octet_index = 0; octet_index < data_size; octet_index++) {
        if ((octet_index % 16) == 0) {
          if (octet_index != 0) {
            message_string_size += sprintf(&message_string[message_string_size], " |\n");
          }

          message_string_size += sprintf(&message_string[message_string_size], " %04d |", octet_index);
        }

        /*
         * Print every single octet in hexadecimal form
         */
        message_string_size += sprintf(&message_string[message_string_size], " %02x", ((uint8_t *)(&sdu_pP->data[data_offset]))[octet_index]);
        /*
         * Align newline and pipes according to the octets in groups of 2
         */
      }

      /*
       * Append enough spaces and put final pipe
       */
      for (index = octet_index; index < 16; ++index) {
        message_string_size += sprintf(&message_string[message_string_size], "   ");
      }

      message_string_size += sprintf(&message_string[message_string_size], " |\n");
      LOG_UI(RLC, "%s\n", message_string);
    } /* LOG_DEBUGFLAG(RLC) */

    l_rlc_p->stat_tx_pdcp_sdu   += 1;
    l_rlc_p->stat_tx_pdcp_bytes += data_size;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].mui      = mui;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].sdu_size = data_size;
    l_rlc_p->sdu_buffer_occupancy += data_size;
    l_rlc_p->nb_sdu += 1;
    l_rlc_p->nb_sdu_no_segmented += 1;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].first_byte = (uint8_t *)(&sdu_pP->data[data_offset]);
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].sdu_remaining_size = l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].sdu_size;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].sdu_segmented_size = 0;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].sdu_creation_time  = ctxt_pP->frame;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].nb_pdus            = 0;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].nb_pdus_ack        = 0;
    //l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].nb_pdus_time = 0;
    //l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].nb_pdus_internal_use = 0;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].flags.discarded    = 0;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].flags.segmented    = 0;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].flags.segmentation_in_progress = 0;
    l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].flags.no_new_sdu_segmented_in_last_pdu = 0;
    //l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].li_index_for_discard = -1;
    l_rlc_p->next_sdu_index = (l_rlc_p->next_sdu_index + 1) % RLC_AM_SDU_CONTROL_BUFFER_SIZE;

    if (l_rlc_p->channel_id <3) {
      LOG_I(RLC, PROTOCOL_RLC_AM_CTXT_FMT" RLC_AM_DATA_REQ size %d Bytes,  NB SDU %d current_sdu_index=%d next_sdu_index=%d conf %d mui %d vtA %d vtS %d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p),
            data_size,
            l_rlc_p->nb_sdu,
            l_rlc_p->current_sdu_index,
            l_rlc_p->next_sdu_index,
            ((struct rlc_am_data_req *) (sdu_pP->data))->conf,
            mui,
            l_rlc_p->vt_a,
            l_rlc_p->vt_s);
    }
  } else {
    LOG_W(RLC, PROTOCOL_RLC_AM_CTXT_FMT" RLC_AM_DATA_REQ BUFFER FULL, NB SDU %d current_sdu_index=%d next_sdu_index=%d size_input_sdus_buffer=%d vtA=%d vtS=%d\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p),
          l_rlc_p->nb_sdu,
          l_rlc_p->current_sdu_index,
          l_rlc_p->next_sdu_index,
          RLC_AM_SDU_CONTROL_BUFFER_SIZE,
          l_rlc_p->vt_a,
          l_rlc_p->vt_s);
    LOG_W(RLC, "                                        input_sdus[].mem_block=%p next input_sdus[].flags.segmented=%d\n",
          l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].mem_block, l_rlc_p->input_sdus[l_rlc_p->next_sdu_index].flags.segmented);
    l_rlc_p->stat_tx_pdcp_sdu_discarded   += 1;
    l_rlc_p->stat_tx_pdcp_bytes_discarded += ((struct rlc_am_data_req *) (sdu_pP->data))->data_size;
    free_mem_block (sdu_pP, __func__);
#if STOP_ON_IP_TRAFFIC_OVERLOAD
    AssertFatal(0, PROTOCOL_RLC_AM_CTXT_FMT" RLC_AM_DATA_REQ size %d Bytes, SDU DROPPED, INPUT BUFFER OVERFLOW NB SDU %d current_sdu_index=%d next_sdu_index=%d \n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,l_rlc_p),
                data_size,
                l_rlc_p->nb_sdu,
                l_rlc_p->current_sdu_index,
                l_rlc_p->next_sdu_index);
#endif
  }

  RLC_AM_MUTEX_UNLOCK(&l_rlc_p->lock_input_sdus);
}
