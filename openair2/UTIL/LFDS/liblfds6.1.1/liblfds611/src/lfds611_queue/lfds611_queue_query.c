#include "lfds611_queue_internal.h"





/****************************************************************************/
//#pragma warning( disable : 4100 )

void lfds611_queue_query( struct lfds611_queue_state *qs, enum lfds611_queue_query_type query_type, void *query_input, void *query_output )
{
  assert( qs != NULL );
  // TRD : query_type can be any value in its range
  // TRD : query_input can be NULL
  assert( query_output != NULL );

  switch( query_type ) {
  case LFDS611_QUEUE_QUERY_ELEMENT_COUNT:
    assert( query_input == NULL );

    lfds611_freelist_query( qs->fs, LFDS611_FREELIST_QUERY_ELEMENT_COUNT, NULL, query_output );
    break;

  case LFDS611_QUEUE_QUERY_VALIDATE:
    // TRD : query_input can be NULL

    lfds611_queue_internal_validate( qs, (struct lfds611_validation_info *) query_input, (enum lfds611_data_structure_validity *) query_output, ((enum lfds611_data_structure_validity *) query_output)+1 );
    break;
  }

  return;
}

//#pragma warning( default : 4100 )





/****************************************************************************/
void lfds611_queue_internal_validate( struct lfds611_queue_state *qs, struct lfds611_validation_info *vi, enum lfds611_data_structure_validity *lfds611_queue_validity,
                                      enum lfds611_data_structure_validity *lfds611_freelist_validity )
{
  struct lfds611_queue_element
      *qe,
      *qe_slow,
      *qe_fast;

  lfds611_atom_t
  element_count = 0,
  total_elements;

  struct lfds611_validation_info
      lfds611_freelist_vi;

  assert( qs != NULL );
  // TRD : vi can be NULL
  assert( lfds611_queue_validity != NULL );
  assert( lfds611_freelist_validity != NULL );

  *lfds611_queue_validity = LFDS611_VALIDITY_VALID;

  LFDS611_BARRIER_LOAD;

  qe_slow = qe_fast = (struct lfds611_queue_element *) qs->dequeue[LFDS611_QUEUE_POINTER];

  /* TRD : first, check for a loop
           we have two pointers
           both of which start at the dequeue end of the lfds611_queue
           we enter a loop
           and on each iteration
           we advance one pointer by one element
           and the other by two

           we exit the loop when both pointers are NULL
           (have reached the end of the lfds611_queue)

           or

           if we fast pointer 'sees' the slow pointer
           which means we have a loop
  */

  if( qe_slow != NULL )
    do {
      qe_slow = qe_slow->next[LFDS611_QUEUE_POINTER];

      if( qe_fast != NULL )
        qe_fast = qe_fast->next[LFDS611_QUEUE_POINTER];

      if( qe_fast != NULL )
        qe_fast = qe_fast->next[LFDS611_QUEUE_POINTER];
    } while( qe_slow != NULL and qe_fast != qe_slow );

  if( qe_fast != NULL and qe_slow != NULL and qe_fast == qe_slow )
    *lfds611_queue_validity = LFDS611_VALIDITY_INVALID_LOOP;

  /* TRD : now check for expected number of elements
           vi can be NULL, in which case we do not check
           we know we don't have a loop from our earlier check
  */

  if( *lfds611_queue_validity == LFDS611_VALIDITY_VALID and vi != NULL ) {
    qe = (struct lfds611_queue_element *) qs->dequeue[LFDS611_QUEUE_POINTER];

    while( qe != NULL ) {
      element_count++;
      qe = (struct lfds611_queue_element *) qe->next[LFDS611_QUEUE_POINTER];
    }

    /* TRD : remember there is a dummy element in the lfds611_queue */
    element_count--;

    if( element_count < vi->min_elements )
      *lfds611_queue_validity = LFDS611_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( element_count > vi->max_elements )
      *lfds611_queue_validity = LFDS611_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;
  }

  /* TRD : now we validate the lfds611_freelist

           we may be able to check for the expected number of
           elements in the lfds611_freelist

           if the caller has given us an expected min and max
           number of elements in the lfds611_queue, then the total number
           of elements in the lfds611_freelist, minus that min and max,
           gives us the expected number of elements in the
           lfds611_freelist
  */

  if( vi != NULL ) {
    lfds611_freelist_query( qs->fs, LFDS611_FREELIST_QUERY_ELEMENT_COUNT, NULL, (void *) &total_elements );

    /* TRD : remember there is a dummy element in the lfds611_queue */
    total_elements--;

    lfds611_freelist_vi.min_elements = total_elements - vi->max_elements;
    lfds611_freelist_vi.max_elements = total_elements - vi->min_elements;

    lfds611_freelist_query( qs->fs, LFDS611_FREELIST_QUERY_VALIDATE, (void *) &lfds611_freelist_vi, (void *) lfds611_freelist_validity );
  }

  if( vi == NULL )
    lfds611_freelist_query( qs->fs, LFDS611_FREELIST_QUERY_VALIDATE, NULL, (void *) lfds611_freelist_validity );

  return;
}

