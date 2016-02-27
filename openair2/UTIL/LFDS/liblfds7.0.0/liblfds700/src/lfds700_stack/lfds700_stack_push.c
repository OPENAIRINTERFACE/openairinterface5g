/***** includes *****/
#include "lfds700_stack_internal.h"





/****************************************************************************/
void lfds700_stack_push( struct lfds700_stack_state *ss, struct lfds700_stack_element *se, struct lfds700_misc_prng_state *ps )
{
  char unsigned
    result;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  struct lfds700_stack_element LFDS700_PAL_ALIGN(LFDS700_PAL_ALIGN_DOUBLE_POINTER)
    *new_top[PAC_SIZE],
    *volatile original_top[PAC_SIZE];

  LFDS700_PAL_ASSERT( ss != NULL );
  LFDS700_PAL_ASSERT( se != NULL );
  LFDS700_PAL_ASSERT( ps != NULL );

  new_top[POINTER] = se;

  original_top[COUNTER] = ss->top[COUNTER];
  original_top[POINTER] = ss->top[POINTER];

  do
  {
    new_top[COUNTER] = original_top[COUNTER] + 1;
    se->next = original_top[POINTER];

    LFDS700_PAL_BARRIER_PROCESSOR_STORE;
    LFDS700_PAL_ATOMIC_DWCAS_WITH_BACKOFF( &ss->top, original_top, new_top, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );
  }
  while( result != 1 );

  return;
}

