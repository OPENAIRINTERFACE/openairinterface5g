/***** includes *****/
#include "lfds700_freelist_internal.h"





/****************************************************************************/
void lfds700_freelist_push( struct lfds700_freelist_state *fs, struct lfds700_freelist_element *fe, struct lfds700_misc_prng_state *ps )
{
  char unsigned
    result;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  struct lfds700_freelist_element LFDS700_PAL_ALIGN(LFDS700_PAL_ALIGN_DOUBLE_POINTER)
    *new_top[PAC_SIZE],
    *volatile original_top[PAC_SIZE];

  LFDS700_PAL_ASSERT( fs != NULL );
  LFDS700_PAL_ASSERT( fe != NULL );
  LFDS700_PAL_ASSERT( ps != NULL );

  new_top[POINTER] = fe;

  original_top[COUNTER] = fs->top[COUNTER];
  original_top[POINTER] = fs->top[POINTER];

  do
  {
    new_top[COUNTER] = original_top[COUNTER] + 1;
    fe->next = original_top[POINTER];

    LFDS700_PAL_BARRIER_PROCESSOR_STORE;
    LFDS700_PAL_ATOMIC_DWCAS_WITH_BACKOFF( &fs->top, original_top, new_top, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );
  }
  while( result != 1 );

  return;
}

