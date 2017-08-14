/***** includes *****/
#include "lfds700_freelist_internal.h"





/****************************************************************************/
void lfds700_freelist_init_valid_on_current_logical_core( struct lfds700_freelist_state *fs, void *user_state )
{
  LFDS700_PAL_ASSERT( fs != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) fs->top % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &fs->user_state % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  // TRD : user_state can be NULL

  fs->top[POINTER] = NULL;
  fs->top[COUNTER] = 0;

  fs->user_state = user_state;

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}

