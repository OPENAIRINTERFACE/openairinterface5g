/***** includes *****/
#include "lfds700_ringbuffer_internal.h"





/****************************************************************************/
void lfds700_ringbuffer_init_valid_on_current_logical_core( struct lfds700_ringbuffer_state *rs,
                                                            struct lfds700_ringbuffer_element *re_array_inc_dummy,
                                                            lfds700_pal_uint_t number_elements,
                                                            struct lfds700_misc_prng_state *ps,
                                                            void *user_state )
{
  lfds700_pal_uint_t
    loop;

  LFDS700_PAL_ASSERT( rs != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &rs->fs % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &rs->qs % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( re_array_inc_dummy != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &re_array_inc_dummy[0].fe % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &re_array_inc_dummy[0].qe % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( number_elements >= 2 );
  LFDS700_PAL_ASSERT( ps != NULL );
  // TRD : user_state can be NULL

  rs->user_state = user_state;

  re_array_inc_dummy[0].qe_use = &re_array_inc_dummy[0].qe;

  lfds700_freelist_init_valid_on_current_logical_core( &rs->fs, rs );
  lfds700_queue_init_valid_on_current_logical_core( &rs->qs, &re_array_inc_dummy[0].qe, ps, rs );

  for( loop = 1 ; loop < number_elements ; loop++ )
  {
    LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &re_array_inc_dummy[loop].fe % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
    LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &re_array_inc_dummy[loop].qe % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );

    re_array_inc_dummy[loop].qe_use = &re_array_inc_dummy[loop].qe;
    LFDS700_FREELIST_SET_VALUE_IN_ELEMENT( re_array_inc_dummy[loop].fe, &re_array_inc_dummy[loop] );
    lfds700_freelist_push( &rs->fs, &re_array_inc_dummy[loop].fe, ps );
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}

