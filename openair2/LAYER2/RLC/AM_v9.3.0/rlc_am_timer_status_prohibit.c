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
#define RLC_AM_TIMER_STATUS_PROHIBIT_C 1
//-----------------------------------------------------------------------------
#include "platform_types.h"
#include "platform_constants.h"
//-----------------------------------------------------------------------------
#include "rlc_am.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "msc.h"
//-----------------------------------------------------------------------------
void
rlc_am_check_timer_status_prohibit(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t * const rlc_pP)
{
  if (rlc_pP->t_status_prohibit.ms_duration > 0) {
    if (rlc_pP->t_status_prohibit.running) {
      if (
        // CASE 1:          start              time out
        //        +-----------+------------------+----------+
        //        |           |******************|          |
        //        +-----------+------------------+----------+
        //FRAME # 0                                     FRAME MAX
        ((rlc_pP->t_status_prohibit.ms_start < rlc_pP->t_status_prohibit.ms_time_out) &&
         ((PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) >= rlc_pP->t_status_prohibit.ms_time_out) ||
          (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) < rlc_pP->t_status_prohibit.ms_start)))                                   ||
        // CASE 2:        time out            start
        //        +-----------+------------------+----------+
        //        |***********|                  |**********|
        //        +-----------+------------------+----------+
        //FRAME # 0                                     FRAME MAX VALUE
        ((rlc_pP->t_status_prohibit.ms_start > rlc_pP->t_status_prohibit.ms_time_out) &&
         (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) < rlc_pP->t_status_prohibit.ms_start) &&
         (PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP) >= rlc_pP->t_status_prohibit.ms_time_out))
      ) {

        //if ((rlc_pP->t_status_prohibit.frame_time_out <= ctxt_pP->frame) && (rlc_pP->t_status_prohibit.frame_start)) {
        rlc_pP->t_status_prohibit.running   = 0;
        rlc_pP->t_status_prohibit.timed_out = 1;
        rlc_pP->stat_timer_status_prohibit_timed_out += 1;

#if MESSAGE_CHART_GENERATOR_RLC_MAC
      MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                             "0 "PROTOCOL_RLC_AM_MSC_FMT" t_status_prohibit timed out",\
                             PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP));
#endif
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T-STATUS-PROHIBIT] TIME-OUT\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
        //#warning         TO DO rlc_am_check_timer_status_prohibit
        rlc_am_stop_and_reset_timer_status_prohibit(ctxt_pP, rlc_pP);
        /* Clear StatusProhibit flag */
        RLC_AM_CLEAR_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_PROHIBIT);
        //rlc_pP->t_status_prohibit.frame_time_out = ctxt_pP->frame + rlc_pP->t_status_prohibit.time_out;
      }
    }
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_stop_and_reset_timer_status_prohibit(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t *const rlc_pP
)
{
  if (rlc_pP->t_status_prohibit.ms_duration > 0) {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T-STATUS-PROHIBIT] STOPPED AND RESET\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
    rlc_pP->t_status_prohibit.running        = 0;
    rlc_pP->t_status_prohibit.ms_time_out    = 0;
    rlc_pP->t_status_prohibit.ms_start       = 0;
    rlc_pP->t_status_prohibit.timed_out      = 0;
#if MESSAGE_CHART_GENERATOR_RLC_MAC
    MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                  "0 "PROTOCOL_RLC_AM_MSC_FMT" t_status_prohibit stopped & reseted",\
                  PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP));
#endif
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_start_timer_status_prohibit(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t *const rlc_pP
)
{
  rlc_pP->t_status_prohibit.timed_out      = 0;

  if (rlc_pP->t_status_prohibit.running == 0){
    if (rlc_pP->t_status_prohibit.ms_duration > 0) {
      rlc_pP->t_status_prohibit.running        = 1;
      rlc_pP->t_status_prohibit.ms_time_out    = rlc_pP->t_status_prohibit.ms_duration + PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP);
      rlc_pP->t_status_prohibit.ms_start       = PROTOCOL_CTXT_TIME_MILLI_SECONDS(ctxt_pP);
      RLC_AM_SET_STATUS(rlc_pP->status_requested,RLC_AM_STATUS_PROHIBIT);
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T-STATUS-PROHIBIT] STARTED (TIME-OUT = %u ms)\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->t_status_prohibit.ms_time_out);
      LOG_D(RLC, "TIME-OUT = FRAME %u\n",  rlc_pP->t_status_prohibit.ms_time_out);
#if MESSAGE_CHART_GENERATOR_RLC_MAC
      MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                             "0 "PROTOCOL_RLC_AM_MSC_FMT" t_status_prohibit started (TO %u ms)",\
                             PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP), rlc_pP->t_status_prohibit.ms_time_out);
#endif
    } else {
    LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[T-STATUS-PROHIBIT] NOT STARTED, CAUSE CONFIGURED 0 ms\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
    }
  }
}
//-----------------------------------------------------------------------------
void rlc_am_init_timer_status_prohibit(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  const uint32_t ms_durationP)
{
  rlc_pP->t_status_prohibit.running        = 0;
  rlc_pP->t_status_prohibit.ms_time_out    = 0;
  rlc_pP->t_status_prohibit.ms_start       = 0;
  rlc_pP->t_status_prohibit.ms_duration    = ms_durationP;
  rlc_pP->t_status_prohibit.timed_out      = 0;
}
