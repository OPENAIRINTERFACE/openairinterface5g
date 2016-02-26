/***** includes *****/
#include "lfds700_queue_internal.h"

/***** private prototypes *****/
static void lfds700_queue_internal_validate( struct lfds700_queue_state *qs, struct lfds700_misc_validation_info *vi, enum lfds700_misc_validity *lfds700_queue_validity );





/****************************************************************************/
void lfds700_queue_query( struct lfds700_queue_state *qs, enum lfds700_queue_query query_type, void *query_input, void *query_output )
{
  struct lfds700_queue_element
    *qe;

  LFDS700_MISC_BARRIER_LOAD;

  LFDS700_PAL_ASSERT( qs != NULL );
  // TRD : query_type can be any value in its range

  switch( query_type )
  {
    case LFDS700_QUEUE_QUERY_SINGLETHREADED_GET_COUNT:
      LFDS700_PAL_ASSERT( query_input == NULL );
      LFDS700_PAL_ASSERT( query_output != NULL );

      *(lfds700_pal_uint_t *) query_output = 0;

      qe = (struct lfds700_queue_element *) qs->dequeue[POINTER];

      while( qe != NULL )
      {
        ( *(lfds700_pal_uint_t *) query_output )++;
        qe = (struct lfds700_queue_element *) qe->next[POINTER];
      }

      // TRD : remember there is a dummy element in the queue
      ( *(lfds700_pal_uint_t *) query_output )--;
    break;

    case LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE:
      // TRD : query_input can be NULL
      LFDS700_PAL_ASSERT( query_output != NULL );

      lfds700_queue_internal_validate( qs, (struct lfds700_misc_validation_info *) query_input, (enum lfds700_misc_validity *) query_output );
    break;
  }

  return;
}





/****************************************************************************/
static void lfds700_queue_internal_validate( struct lfds700_queue_state *qs, struct lfds700_misc_validation_info *vi, enum lfds700_misc_validity *lfds700_queue_validity )
{
  lfds700_pal_uint_t
    number_elements = 0;

  struct lfds700_queue_element
    *qe_fast,
    *qe_slow;

  LFDS700_PAL_ASSERT( qs != NULL );
  // TRD : vi can be NULL
  LFDS700_PAL_ASSERT( lfds700_queue_validity != NULL );

  *lfds700_queue_validity = LFDS700_MISC_VALIDITY_VALID;

  qe_slow = qe_fast = (struct lfds700_queue_element *) qs->dequeue[POINTER];

  /* TRD : first, check for a loop
           we have two pointers
           both of which start at the dequeue end of the queue
           we enter a loop
           and on each iteration
           we advance one pointer by one element
           and the other by two

           we exit the loop when both pointers are NULL
           (have reached the end of the queue)

           or

           if we fast pointer 'sees' the slow pointer
           which means we have a loop
  */

  if( qe_slow != NULL )
    do
    {
      qe_slow = qe_slow->next[POINTER];

      if( qe_fast != NULL )
        qe_fast = qe_fast->next[POINTER];

      if( qe_fast != NULL )
        qe_fast = qe_fast->next[POINTER];
    }
    while( qe_slow != NULL and qe_fast != qe_slow );

  if( qe_fast != NULL and qe_slow != NULL and qe_fast == qe_slow )
    *lfds700_queue_validity = LFDS700_MISC_VALIDITY_INVALID_LOOP;

  /* TRD : now check for expected number of elements
           vi can be NULL, in which case we do not check
           we know we don't have a loop from our earlier check
  */

  if( *lfds700_queue_validity == LFDS700_MISC_VALIDITY_VALID and vi != NULL )
  {
    lfds700_queue_query( qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_GET_COUNT, NULL, (void *) &number_elements );

    if( number_elements < vi->min_elements )
      *lfds700_queue_validity = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( number_elements > vi->max_elements )
      *lfds700_queue_validity = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;
  }

  return;
}

