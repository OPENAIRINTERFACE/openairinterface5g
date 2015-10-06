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
