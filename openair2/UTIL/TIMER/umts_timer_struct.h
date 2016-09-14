/***************************************************************************
                          umts_timer_struct.h  -  description
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr



 ***************************************************************************/
#ifndef __UMTS_TIMER_STRUCT_H__
#    define __UMTS_TIMER_STRUCT_H__

#    include "platform_types.h"

struct timer_unit {

  void            (*proc) (void *, void *);     // proc executed when time_out
  void           *protocol;     // arg should be a pointer on a allocated protocol entity private struct including its variables
  void           *timer_id;     // arg should be a value or a pointer identifying the timer
  // Example: rlc_am_sdu_discard_time_out(rlc_am, sdu)
  uint32_t             frame_time_out;
  uint32_t             frame_tick_start;
};
#endif
