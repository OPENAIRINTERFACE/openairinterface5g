/***** includes *****/
#include "lfds700_stack_internal.h"





/****************************************************************************/
int lfds700_stack_pop( struct lfds700_stack_state *ss, struct lfds700_stack_element **se, struct lfds700_misc_prng_state *ps )
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

  LFDS700_PAL_BARRIER_PROCESSOR_LOAD;

  original_top[COUNTER] = ss->top[COUNTER];
  original_top[POINTER] = ss->top[POINTER];

  do
  {
    if( original_top[POINTER] == NULL )
    {
      *se = NULL;
      return( 0 );
    }

    new_top[COUNTER] = original_top[COUNTER] + 1;
    new_top[POINTER] = original_top[POINTER]->next;

    LFDS700_PAL_ATOMIC_DWCAS_WITH_BACKOFF( &ss->top, original_top, new_top, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );

    if( result != 1 )
      LFDS700_PAL_BARRIER_PROCESSOR_LOAD;
  }
  while( result != 1 );

  *se = original_top[POINTER];

  return( 1 );
}

