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
                          umts_timer_proto_extern.h  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr


 ***************************************************************************/
#ifndef __UMTS_TIMER_PROTO_EXTERN_H__
#    define __UMTS_TIMER_PROTO_EXTERN_H__

#    include "platform_types.h"
#    include "list.h"
#    include "mem_block.h"

extern void     umts_timer_check_time_out (list2_t * atimer_listP, uint32_t current_frame_tick_millisecondsP);
extern void     umts_timer_delete_timer (list2_t * atimer_listP, void *timer_idP);
extern mem_block_t *umts_add_timer_list_up (list2_t * atimer_listP, void (*procP) (void *, void *), void *protocolP, void *timer_idP, uint32_t frame_time_outP,
    uint32_t current_frame_tick_millisecondsP);
extern void     umts_stop_all_timers (list2_t * atimer_listP);
extern void     umts_stop_all_timers_except (list2_t * atimer_listP, void (*procP) (void *, void *));
#endif
