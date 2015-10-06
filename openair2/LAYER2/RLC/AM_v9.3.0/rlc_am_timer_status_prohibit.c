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
