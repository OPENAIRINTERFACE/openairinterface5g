/***** includes *****/
#include "lfds700_queue_internal.h"





/****************************************************************************/
void lfds700_queue_enqueue( struct lfds700_queue_state *qs,
                            struct lfds700_queue_element *qe,
                            struct lfds700_misc_prng_state *ps )
{
  char unsigned
    result = 0,
    unwanted_result;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  struct lfds700_queue_element LFDS700_PAL_ALIGN(LFDS700_PAL_ALIGN_DOUBLE_POINTER)
    *volatile enqueue[PAC_SIZE],
    *new_enqueue[PAC_SIZE],
    *volatile next[PAC_SIZE];

  LFDS700_PAL_ASSERT( qs != NULL );
  LFDS700_PAL_ASSERT( qe != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) qe->next % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &qe->key % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( ps != NULL );

  qe->next[COUNTER] = (struct lfds700_queue_element *) LFDS700_MISC_PRNG_GENERATE( ps );
  qe->next[POINTER] = NULL;

  new_enqueue[POINTER] = qe;

  LFDS700_MISC_BARRIER_LOAD;

  do
  {
    enqueue[COUNTER] = qs->enqueue[COUNTER];
    enqueue[POINTER] = qs->enqueue[POINTER];

    next[COUNTER] = qs->enqueue[POINTER]->next[COUNTER];
    next[POINTER] = qs->enqueue[POINTER]->next[POINTER];

    LFDS700_MISC_BARRIER_LOAD;

    if( qs->enqueue[COUNTER] == enqueue[COUNTER] and qs->enqueue[POINTER] == enqueue[POINTER] )
    {
      if( next[POINTER] == NULL )
      {
        new_enqueue[COUNTER] = next[COUNTER] + 1;
        LFDS700_MISC_BARRIER_STORE;
        LFDS700_PAL_ATOMIC_DWCAS_WITH_BACKOFF( enqueue[POINTER]->next, next, new_enqueue, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );
      }
      else
      {
        next[COUNTER] = enqueue[COUNTER] + 1;
        LFDS700_MISC_BARRIER_STORE;
        // TRD : strictly, this is a weak CAS, but we do an extra iteration of the main loop on a fake failure, so we set it to be strong
        LFDS700_PAL_ATOMIC_DWCAS( qs->enqueue, enqueue, next, LFDS700_MISC_CAS_STRENGTH_STRONG, unwanted_result );
      }
    }
  }
  while( result != 1 );

  new_enqueue[COUNTER] = enqueue[COUNTER] + 1;
  LFDS700_MISC_BARRIER_STORE;
  // TRD : move enqueue along; only a weak CAS as the dequeue will solve this if its out of place
  LFDS700_PAL_ATOMIC_DWCAS( qs->enqueue, enqueue, new_enqueue, LFDS700_MISC_CAS_STRENGTH_WEAK, unwanted_result );

  return;
}

