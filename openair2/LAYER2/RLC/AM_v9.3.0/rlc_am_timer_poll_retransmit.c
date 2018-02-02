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
#define RLC_AM_TIMER_POLL_RETRANSMIT_C 1
//-----------------------------------------------------------------------------
//#include "rtos_header.h"
#include "platform_types.h"
#include "platform_constants.h"
//-----------------------------------------------------------------------------
#include "rlc_am.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "msc.h"
//-----------------------------------------------------------------------------
void
rlc_am_check_timer_poll_retransmit(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const      rlc_pP
)
{
  // 5.2.2.3 Expiry of t-PollRetransmit
  // Upon expiry of t-PollRetransmit, the transmitting side of an AM RLC entity shall:
  //     - if both the transmission buffer and the retransmission buffer are empty (excluding transmitted RLC data PDU
  //           awaiting for acknowledgements); or
  //     - if no new RLC data PDU can be transmitted (e.g. due to window stalling):
  //         - consider the AMD PDU with SN = VT(S) – 1 for retransmission; or
  //         - consider any AMD PDU which has not been positively acknowledged for retransmission;
  //     - include a poll in a RLC data PDU as described in section 5.2.2.1.

  if (rlc_pP->t_poll_retransmit.running) {
    if (
      // CASE 1:          start              time out
      //        +-----------+------------------+----------+
      //        |           |******************|          |
      //        +-----------+------------------+----------+
      //FRAME # 0                                     FRAME MAX
      ((rlc_pP->t_poll_retransmit.ms_start < rlc_pP->t_poll_retransmit.ms_time_out) &&
       ((PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) >= rlc_pP->t_poll_retransmit.ms_time_out) ||
        (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) < rlc_pP->t_poll_retransmit.ms_start)))                                   ||
      // CASE 2:        time out            start
      //        +-----------+------------------+----------+
      //        |***********|                  |**********|
      //        +-----------+------------------+----------+
      //FRAME # 0                                     FRAME MAX VALUE
      ((rlc_pP->t_poll_retransmit.ms_start > rlc_pP->t_poll_retransmit.ms_time_out) &&
       (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) < rlc_pP->t_poll_retransmit.ms_start) &&
       (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) >= rlc_pP->t_poll_retransmit.ms_time_out))
    ) {
      //if (rlc_pP->t_poll_retransmit.frame_time_out <= ctxt_pP->frame) {
      rlc_pP->t_poll_retransmit.running   = 0;
      rlc_pP->t_poll_retransmit.timed_out = 1;
      rlc_pP->stat_timer_poll_retransmit_timed_out += 1;
#if MESSAGE_CHART_GENERATOR_RLC_MAC
      MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                             "0 "PROTOCOL_RLC_AM_MSC_FMT" t_poll_retransmit timed-out",\
                             PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP));
#endif
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T_POLL_RETRANSMIT] TIME-OUT\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

      /* Check for any retransmittable PDU if Buffer Occupancy empty or window stall */
	  if (((rlc_pP->sdu_buffer_occupancy == 0) && (rlc_pP->retrans_num_bytes_to_retransmit == 0)) ||
			    (rlc_pP->vt_s == rlc_pP->vt_ms)) {
		  // force BO to be > 0
		  rlc_sn_t             sn           = RLC_AM_PREV_SN(rlc_pP->vt_s);
		  rlc_sn_t             sn_end       = RLC_AM_PREV_SN(rlc_pP->vt_a);
		  rlc_am_tx_data_pdu_management_t *tx_data_pdu_buffer_p;

          /* Look for the first retransmittable PDU starting from vtS - 1 */
		  while (sn != sn_end) {
			tx_data_pdu_buffer_p = &rlc_pP->tx_data_pdu_buffer[sn % RLC_AM_WINDOW_SIZE];
			AssertFatal (tx_data_pdu_buffer_p->mem_block != NULL, "RLC AM Tpoll Retx expiry sn=%d ack=%d is empty vtA=%d vtS=%d LcId=%d\n",
					sn, tx_data_pdu_buffer_p->flags.ack,rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);
		    if ((tx_data_pdu_buffer_p->flags.ack == 0) && (tx_data_pdu_buffer_p->flags.max_retransmit == 0)) {
		    	tx_data_pdu_buffer_p->flags.retransmit = 1;
		    	tx_data_pdu_buffer_p->retx_payload_size = tx_data_pdu_buffer_p->payload_size;
		    	if (tx_data_pdu_buffer_p->retx_count == tx_data_pdu_buffer_p->retx_count_next) {
		    		tx_data_pdu_buffer_p->retx_count_next ++;
		    	}
		    	rlc_pP->retrans_num_pdus += 1;
		    	rlc_pP->retrans_num_bytes_to_retransmit += tx_data_pdu_buffer_p->payload_size;
		        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T_POLL_RETRANSMIT] TIME-OUT PUT SN=%d in ReTx Buffer\n",
		              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),tx_data_pdu_buffer_p->sn);
		    	break;
		    }
		    else
		    {
		    	sn = RLC_AM_PREV_SN(sn);
		    }
		  }
	  }


      rlc_pP->force_poll= TRUE;
      //BugFix : new ms_time_out is computed when next poll is transmitter
    }
  }
}
//-----------------------------------------------------------------------------
int
rlc_am_is_timer_poll_retransmit_timed_out(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const      rlc_pP
)
{
  return rlc_pP->t_poll_retransmit.timed_out;
}
//-----------------------------------------------------------------------------
void
rlc_am_stop_and_reset_timer_poll_retransmit(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const      rlc_pP
)
{
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T_POLL_RETRANSMIT] STOPPED AND RESET\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
  rlc_pP->t_poll_retransmit.running         = 0;
  rlc_pP->t_poll_retransmit.ms_time_out     = 0;
  rlc_pP->t_poll_retransmit.ms_start        = 0;
  rlc_pP->t_poll_retransmit.timed_out       = 0;
#if MESSAGE_CHART_GENERATOR_RLC_MAC
    MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                  "0 "PROTOCOL_RLC_AM_MSC_FMT" t_poll_retransmit stopped & reseted",\
                  PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP));
#endif
}
//-----------------------------------------------------------------------------
void
rlc_am_start_timer_poll_retransmit(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const      rlc_pP
)
{
  /* Stop timer if it was previously running */
  rlc_am_stop_and_reset_timer_poll_retransmit(ctxt_pP,rlc_pP);

    if (rlc_pP->t_poll_retransmit.ms_duration > 0) {
      rlc_pP->t_poll_retransmit.running         = 1;
      rlc_pP->t_poll_retransmit.ms_time_out     = PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) + rlc_pP->t_poll_retransmit.ms_duration;
      rlc_pP->t_poll_retransmit.ms_start        = PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP);
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T_POLL_RETRANSMIT] STARTED (TIME-OUT = FRAME %05d)\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->t_poll_retransmit.ms_time_out);
#if MESSAGE_CHART_GENERATOR_RLC_MAC
      MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                             "0 "PROTOCOL_RLC_AM_MSC_FMT" t_poll_retransmit started (TO %u ms)",\
                             PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP), rlc_pP->t_poll_retransmit.ms_time_out);
#endif
    } else {
    LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T_POLL_RETRANSMIT] NOT STARTED, CAUSE CONFIGURED 0 ms\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
    }

}
//-----------------------------------------------------------------------------
void
rlc_am_init_timer_poll_retransmit(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const      rlc_pP,
  const uint32_t ms_durationP)
{
  rlc_pP->t_poll_retransmit.running         = 0;
  rlc_pP->t_poll_retransmit.ms_time_out     = 0;
  rlc_pP->t_poll_retransmit.ms_start        = 0;
  rlc_pP->t_poll_retransmit.ms_duration     = ms_durationP;
  rlc_pP->t_poll_retransmit.timed_out       = 0;
}
