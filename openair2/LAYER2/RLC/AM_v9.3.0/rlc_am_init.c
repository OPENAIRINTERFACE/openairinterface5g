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
#define RLC_AM_INIT_C 1
#include <string.h>
//-----------------------------------------------------------------------------
#include "rlc_am.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
//-----------------------------------------------------------------------------
void
rlc_am_init(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const        rlc_pP)
{
  if (rlc_pP->initialized == TRUE) {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[AM INIT] INITIALIZATION ALREADY DONE, DOING NOTHING\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
  } else {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[AM INIT] INITIALIZATION: STATE VARIABLES, BUFFERS, LISTS\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
    memset(rlc_pP, 0, sizeof(rlc_am_entity_t));

    list2_init(&rlc_pP->receiver_buffer,      "RX BUFFER");
    list_init(&rlc_pP->pdus_to_mac_layer,     "PDUS TO MAC");
    list_init(&rlc_pP->control_pdu_list,      "CONTROL PDU LIST");
    list_init(&rlc_pP->segmentation_pdu_list, "SEGMENTATION PDU LIST");
    //LOG_D(RLC,"RLC_AM_SDU_CONTROL_BUFFER_SIZE %d sizeof(rlc_am_tx_sdu_management_t) %d \n",  RLC_AM_SDU_CONTROL_BUFFER_SIZE, sizeof(rlc_am_tx_sdu_management_t));

    pthread_mutex_init(&rlc_pP->lock_input_sdus, NULL);
    rlc_pP->input_sdus               = calloc(1, RLC_AM_SDU_CONTROL_BUFFER_SIZE*sizeof(rlc_am_tx_sdu_management_t));
//#warning "cast the rlc retrans buffer to uint32"
    //        rlc_pP->tx_data_pdu_buffer       = calloc(1, (uint16_t)((unsigned int)RLC_AM_PDU_RETRANSMISSION_BUFFER_SIZE*(unsigned int)sizeof(rlc_am_tx_data_pdu_management_t)));
    rlc_pP->tx_data_pdu_buffer       = calloc(1, (uint32_t)((unsigned int)RLC_AM_PDU_RETRANSMISSION_BUFFER_SIZE*(unsigned int)sizeof(
                                         rlc_am_tx_data_pdu_management_t)));
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[AM INIT] input_sdus[] = %p  element size=%zu\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->input_sdus,
          sizeof(rlc_am_tx_sdu_management_t));
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[AM INIT] tx_data_pdu_buffer[] = %p element size=%zu\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->tx_data_pdu_buffer,
          sizeof(rlc_am_tx_data_pdu_management_t));

    // TX state variables
    //rlc_pP->vt_a    = 0;
    rlc_pP->vt_ms   = rlc_pP->vt_a + RLC_AM_WINDOW_SIZE;
    //rlc_pP->vt_s    = 0;
    //rlc_pP->poll_sn = 0;
    // TX counters
    //rlc_pP->c_pdu_without_poll  = 0;
    //rlc_pP->c_byte_without_poll = 0;
    // RX state variables
    //rlc_pP->vr_r    = 0;
    rlc_pP->vr_mr   = rlc_pP->vr_r + RLC_AM_WINDOW_SIZE;
    rlc_pP->vr_x    = RLC_SN_UNDEFINED;
    //rlc_pP->vr_ms   = 0;
    //rlc_pP->vr_h    = 0;
    rlc_pP->sn_status_triggered_delayed = RLC_SN_UNDEFINED;

    rlc_pP->last_absolute_subframe_status_indication = 0xFFFFFFFF; // any value > 1

    rlc_pP->initialized                  = TRUE;
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_reestablish(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const        rlc_pP)
{
  /*
   * RLC re-establishment is performed upon request by RRC, and the function
   * is applicable for AM, UM and TM RLC entities.
   * When RRC indicates that an RLC entity should be re-established, the RLC entity shall:
   * - if it is an AM RLC entity:
   *    - when possible, reassemble RLC SDUs from any byte segments of AMD PDUs with SN < VR(MR) in the
   *       receiving side, remove RLC headers when doing so and deliver all reassembled RLC SDUs to upper layer in
   *        ascending order of the RLC SN, if not delivered before;
   *    - discard the remaining AMD PDUs and byte segments of AMD PDUs in the receiving side;
   *    - discard all RLC SDUs and AMD PDUs in the transmitting side;
   *    - discard all RLC control PDUs.
   *    - stop and reset all timers;
   *    - reset all state variables to their initial values.
   */
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[AM REESTABLISH] RE-INIT STATE VARIABLES, BUFFERS, LISTS\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

//#warning TODO when possible reassemble RLC SDUs from any byte segments of AMD PDUs with SN inf VR(MR)
  list2_free(&rlc_pP->receiver_buffer);

  list_free(&rlc_pP->pdus_to_mac_layer);
  list_free(&rlc_pP->control_pdu_list);
  list_free(&rlc_pP->segmentation_pdu_list);


  // TX state variables
  rlc_pP->vt_a    = 0;
  rlc_pP->vt_ms   = rlc_pP->vt_a + RLC_AM_WINDOW_SIZE;
  rlc_pP->vt_s    = 0;
  rlc_pP->poll_sn = 0;

  // TX counters
  rlc_pP->c_pdu_without_poll  = 0;
  rlc_pP->c_byte_without_poll = 0;

  // RX state variables
  rlc_pP->vr_r    = 0;
  rlc_pP->vr_mr   = rlc_pP->vr_r + RLC_AM_WINDOW_SIZE;
  rlc_pP->vr_x    = RLC_SN_UNDEFINED;
  rlc_pP->vr_ms   = 0;
  rlc_pP->vr_h    = 0;
  rlc_pP->sn_status_triggered_delayed = RLC_SN_UNDEFINED;
  rlc_pP->status_requested	= RLC_AM_STATUS_NOT_TRIGGERED;

  rlc_pP->last_absolute_subframe_status_indication = 0xFFFFFFFF; // any value > 1

  rlc_pP->initialized                  = TRUE;

}

//-----------------------------------------------------------------------------
void
rlc_am_cleanup(
  rlc_am_entity_t* const        rlc_pP
)
{
  list2_free(&rlc_pP->receiver_buffer);
  list_free(&rlc_pP->pdus_to_mac_layer);
  list_free(&rlc_pP->control_pdu_list);
  list_free(&rlc_pP->segmentation_pdu_list);


  if (rlc_pP->output_sdu_in_construction != NULL) {
    free_mem_block(rlc_pP->output_sdu_in_construction, __func__);
    rlc_pP->output_sdu_in_construction = NULL;
  }

  unsigned int i;

  if (rlc_pP->input_sdus != NULL) {
    for (i=0; i < RLC_AM_SDU_CONTROL_BUFFER_SIZE; i++) {
      if (rlc_pP->input_sdus[i].mem_block != NULL) {
        free_mem_block(rlc_pP->input_sdus[i].mem_block, __func__);
        rlc_pP->input_sdus[i].mem_block = NULL;
      }
    }

    free(rlc_pP->input_sdus);
    rlc_pP->input_sdus       = NULL;
  }

  pthread_mutex_destroy(&rlc_pP->lock_input_sdus);

  if (rlc_pP->tx_data_pdu_buffer != NULL) {
    for (i=0; i < RLC_AM_PDU_RETRANSMISSION_BUFFER_SIZE; i++) {
      if (rlc_pP->tx_data_pdu_buffer[i % RLC_AM_WINDOW_SIZE].mem_block != NULL) {
        free_mem_block(rlc_pP->tx_data_pdu_buffer[i % RLC_AM_WINDOW_SIZE].mem_block, __func__);
        rlc_pP->tx_data_pdu_buffer[i % RLC_AM_WINDOW_SIZE].mem_block = NULL;
      }
    }

    free(rlc_pP->tx_data_pdu_buffer);
    rlc_pP->tx_data_pdu_buffer       = NULL;
  }

  memset(rlc_pP, 0, sizeof(rlc_am_entity_t));
}
//-----------------------------------------------------------------------------
void
rlc_am_configure(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const        rlc_pP,
  const uint16_t                max_retx_thresholdP,
  const uint16_t                poll_pduP,
  const uint16_t                poll_byteP,
  const uint32_t                t_poll_retransmitP,
  const uint32_t                t_reorderingP,
  const uint32_t                t_status_prohibitP)
{
  if (rlc_pP->configured == TRUE) {
    LOG_I(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RECONFIGURE] max_retx_threshold %d poll_pdu %d poll_byte %d t_poll_retransmit %d t_reordering %d t_status_prohibit %d\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          max_retx_thresholdP,
          poll_pduP,
          poll_byteP,
          t_poll_retransmitP,
          t_reorderingP,
          t_status_prohibitP);

    rlc_pP->max_retx_threshold = max_retx_thresholdP;
    rlc_pP->poll_pdu           = poll_pduP;
    rlc_pP->poll_byte          = poll_byteP;
    rlc_pP->protocol_state     = RLC_DATA_TRANSFER_READY_STATE;
    rlc_pP->t_poll_retransmit.ms_duration   = t_poll_retransmitP;
    rlc_pP->t_reordering.ms_duration        = t_reorderingP;
    rlc_pP->t_status_prohibit.ms_duration   = t_status_prohibitP;

  } else {
    LOG_I(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[CONFIGURE] max_retx_threshold %d poll_pdu %d poll_byte %d t_poll_retransmit %d t_reordering %d t_status_prohibit %d\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          max_retx_thresholdP,
          poll_pduP,
          poll_byteP,
          t_poll_retransmitP,
          t_reorderingP,
          t_status_prohibitP);

    rlc_pP->max_retx_threshold = max_retx_thresholdP;
    rlc_pP->poll_pdu           = poll_pduP;
    rlc_pP->poll_byte          = poll_byteP;
    rlc_pP->protocol_state     = RLC_DATA_TRANSFER_READY_STATE;


    rlc_am_init_timer_poll_retransmit(ctxt_pP, rlc_pP, t_poll_retransmitP);
    rlc_am_init_timer_reordering     (ctxt_pP, rlc_pP, t_reorderingP);
    rlc_am_init_timer_status_prohibit(ctxt_pP, rlc_pP, t_status_prohibitP);

    rlc_pP->configured = TRUE;
  }

}
//-----------------------------------------------------------------------------
void
rlc_am_set_debug_infos(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const        rlc_pP,
  const srb_flag_t              srb_flagP,
  const rb_id_t                 rb_idP,
  const logical_chan_id_t       chan_idP) 
{
  rlc_pP->rb_id         = rb_idP;
  rlc_pP->channel_id    = chan_idP;

  if (srb_flagP) {
    rlc_pP->is_data_plane = 0;
  } else {
    rlc_pP->is_data_plane = 1;
  }

  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SET DEBUG INFOS]\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

}
