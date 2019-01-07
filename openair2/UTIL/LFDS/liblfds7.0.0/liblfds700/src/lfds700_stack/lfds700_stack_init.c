/***** includes *****/
#include "lfds700_stack_internal.h"





/****************************************************************************/
void lfds700_stack_init_valid_on_current_logical_core( struct lfds700_stack_state *ss, void *user_state )
{
  LFDS700_PAL_ASSERT( ss != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) ss->top % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &ss->user_state % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  // TRD : user_state can be NULL

  ss->top[POINTER] = NULL;
  ss->top[COUNTER] = 0;

  ss->user_state = user_state;

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}

