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

/***************************************************************************
                          umts_timer.h  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr


 ***************************************************************************/
#ifndef __UMTS_TIMER_H__
#    define __UMTS_TIMER_H__


#    include "platform_types.h"
#    include "lists_proto_extern.h"
#    include "mem_mngt_proto_extern.h"

#    define UMTS_TIMER_NOT_STARTED  0x00
#    define UMTS_TIMER_STARTED      0x01
#    define UMTS_TIMER_TIMED_OUT    0x02


void            umts_timer_check_time_out (list2_t * atimer_listP, uint32_t current_frame_tick_millisecondsP);
mem_block      *umts_add_timer_list_up (list2_t * atimer_listP, void (*procP) (void *, void *), void *protocolP, void *timer_idP, uint32_t frame_time_outP, uint32_t current_frame_tick_millisecondsP);

struct timer_unit {

  void            (*proc) (void *, void *);     // proc executed when time_out
  void           *protocol;     // arg should be a pointer on a allocated protocol entity private struct including its variables
  void           *timer_id;     // arg should be a value or a pointer identifying the timer
  // Example: rlc_am_sdu_discard_time_out(rlc_am, sdu)
  uint32_t             frame_time_out;
  uint32_t             frame_tick_start;
};
#endif
