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
