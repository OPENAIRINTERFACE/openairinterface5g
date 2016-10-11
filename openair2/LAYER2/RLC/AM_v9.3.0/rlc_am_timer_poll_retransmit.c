/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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


      rlc_pP->force_poll= TRUE;
      //#warning         TO DO rlc_am_check_timer_poll_retransmit
      rlc_pP->t_poll_retransmit.ms_time_out = PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) + rlc_pP->t_poll_retransmit.ms_duration;
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
  rlc_pP->t_poll_retransmit.timed_out       = 0;

  if (rlc_pP->t_poll_retransmit.running == 0) {
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
  } else {
#if MESSAGE_CHART_GENERATOR_RLC_MAC
      MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                             "0 "PROTOCOL_RLC_AM_MSC_FMT" t_poll_retransmit not restarted (TO %u ms)",\
                             PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP), rlc_pP->t_poll_retransmit.ms_time_out);
#endif

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
