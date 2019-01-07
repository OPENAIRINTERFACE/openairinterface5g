/***** includes *****/
#include "lfds700_queue_internal.h"





/****************************************************************************/
int lfds700_queue_dequeue( struct lfds700_queue_state *qs,
                           struct lfds700_queue_element **qe,
                           struct lfds700_misc_prng_state *ps )
{
  char unsigned
    result = 0,
    unwanted_result;

  enum lfds700_queue_queue_state
    state = LFDS700_QUEUE_QUEUE_STATE_UNKNOWN;

  int
    rv = 1,
    finished_flag = LFDS700_MISC_FLAG_LOWERED;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  struct lfds700_queue_element LFDS700_PAL_ALIGN(LFDS700_PAL_ALIGN_DOUBLE_POINTER)
    *dequeue[PAC_SIZE],
    *enqueue[PAC_SIZE],
    *next[PAC_SIZE];

  void
    *key = NULL,
    *value = NULL;

  LFDS700_PAL_ASSERT( qs != NULL );
  LFDS700_PAL_ASSERT( qe != NULL );
  LFDS700_PAL_ASSERT( ps != NULL );

  LFDS700_MISC_BARRIER_LOAD;

  do
  {
    dequeue[COUNTER] = qs->dequeue[COUNTER];
    dequeue[POINTER] = qs->dequeue[POINTER];

    enqueue[COUNTER] = qs->enqueue[COUNTER];
    enqueue[POINTER] = qs->enqueue[POINTER];

    next[COUNTER] = qs->dequeue[POINTER]->next[COUNTER];
    next[POINTER] = qs->dequeue[POINTER]->next[POINTER];

    LFDS700_MISC_BARRIER_LOAD;

    if( dequeue[COUNTER] == qs->dequeue[COUNTER] and dequeue[POINTER] == qs->dequeue[POINTER] )
    {
      if( enqueue[POINTER] == dequeue[POINTER] and next[POINTER] == NULL )
        state = LFDS700_QUEUE_QUEUE_STATE_EMPTY;

      if( enqueue[POINTER] == dequeue[POINTER] and next[POINTER] != NULL )
        state = LFDS700_QUEUE_QUEUE_STATE_ENQUEUE_OUT_OF_PLACE;

      if( enqueue[POINTER] != dequeue[POINTER] )
        state = LFDS700_QUEUE_QUEUE_STATE_ATTEMPT_DEQUEUE;

      switch( state )
      {
        case LFDS700_QUEUE_QUEUE_STATE_UNKNOWN:
          // TRD : eliminates compiler warning
        break;

        case LFDS700_QUEUE_QUEUE_STATE_EMPTY:
          rv = 0;
          *qe = NULL;
          finished_flag = LFDS700_MISC_FLAG_RAISED;
        break;

        case LFDS700_QUEUE_QUEUE_STATE_ENQUEUE_OUT_OF_PLACE:
          next[COUNTER] = enqueue[COUNTER] + 1;
          LFDS700_MISC_BARRIER_STORE;
          LFDS700_PAL_ATOMIC_DWCAS( qs->enqueue, enqueue, next, LFDS700_MISC_CAS_STRENGTH_WEAK, unwanted_result );
        break;

        case LFDS700_QUEUE_QUEUE_STATE_ATTEMPT_DEQUEUE:
          key = next[POINTER]->key;
          value = next[POINTER]->value;

          next[COUNTER] = dequeue[COUNTER] + 1;
          LFDS700_MISC_BARRIER_STORE;
          LFDS700_PAL_ATOMIC_DWCAS_WITH_BACKOFF( qs->dequeue, dequeue, next, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );

          if( result == 1 )
            finished_flag = LFDS700_MISC_FLAG_RAISED;
        break;
      }
    }
  }
  while( finished_flag == LFDS700_MISC_FLAG_LOWERED );

  if( result == 1 )
  {
    *qe = dequeue[POINTER];
    (*qe)->key = key;
    (*qe)->value = value;
  }

  return( rv );
}

