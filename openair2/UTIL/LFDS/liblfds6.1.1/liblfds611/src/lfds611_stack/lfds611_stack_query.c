#include "lfds611_stack_internal.h"





/****************************************************************************/
void lfds611_stack_query( struct lfds611_stack_state *ss, enum lfds611_stack_query_type query_type, void *query_input, void *query_output )
{
  assert( ss != NULL );
  // TRD : query_type can be any value in its range
  // TRD : query_iput can be NULL
  assert( query_output != NULL );

  LFDS611_BARRIER_LOAD;

  switch( query_type ) {
  case LFDS611_STACK_QUERY_ELEMENT_COUNT:
    assert( query_input == NULL );

    lfds611_freelist_query( ss->fs, LFDS611_FREELIST_QUERY_ELEMENT_COUNT, NULL, query_output );
    break;

  case LFDS611_STACK_QUERY_VALIDATE:
    // TRD : query_input can be NULL

    /* TRD : the validation info passed in is for the stack
             it indicates the minimum and maximum number of elements
             which should be present

             we need to validate the freelist
             and validate the stack

             we cannot know the min/max for the freelist, given only
             the min/max for the stack
    */

    lfds611_freelist_query( ss->fs, LFDS611_FREELIST_QUERY_VALIDATE, NULL, (enum lfds611_data_structure_validity *) query_output );

    if( *(enum lfds611_data_structure_validity *) query_output == LFDS611_VALIDITY_VALID )
      lfds611_stack_internal_validate( ss, (struct lfds611_validation_info *) query_input, (enum lfds611_data_structure_validity *) query_output, ((enum lfds611_data_structure_validity *) query_output)+1 );

    break;
  }

  return;
}





/****************************************************************************/
void lfds611_stack_internal_validate( struct lfds611_stack_state *ss, struct lfds611_validation_info *vi, enum lfds611_data_structure_validity *stack_validity,
                                      enum lfds611_data_structure_validity *freelist_validity )
{
  struct lfds611_stack_element
      *se,
      *se_slow,
      *se_fast;

  lfds611_atom_t
  element_count = 0,
  total_elements;

  struct lfds611_validation_info
      freelist_vi;

  assert( ss != NULL );
  // TRD : vi can be NULL
  assert( stack_validity != NULL );

  *stack_validity = LFDS611_VALIDITY_VALID;

  se_slow = se_fast = (struct lfds611_stack_element *) ss->top[LFDS611_STACK_POINTER];

  /* TRD : first, check for a loop
           we have two pointers
           both of which start at the top of the stack
           we enter a loop
           and on each iteration
           we advance one pointer by one element
           and the other by two

           we exit the loop when both pointers are NULL
           (have reached the end of the stack)

           or

           if we fast pointer 'sees' the slow pointer
           which means we have a loop
  */

  if( se_slow != NULL )
    do {
      se_slow = se_slow->next[LFDS611_STACK_POINTER];

      if( se_fast != NULL )
        se_fast = se_fast->next[LFDS611_STACK_POINTER];

      if( se_fast != NULL )
        se_fast = se_fast->next[LFDS611_STACK_POINTER];
    } while( se_slow != NULL and se_fast != se_slow );

  if( se_fast != NULL and se_slow != NULL and se_fast == se_slow )
    *stack_validity = LFDS611_VALIDITY_INVALID_LOOP;

  /* TRD : now check for expected number of elements
           vi can be NULL, in which case we do not check
           we know we don't have a loop from our earlier check
  */

  if( *stack_validity == LFDS611_VALIDITY_VALID and vi != NULL ) {
    se = (struct lfds611_stack_element *) ss->top[LFDS611_STACK_POINTER];

    while( se != NULL ) {
      element_count++;
      se = (struct lfds611_stack_element *) se->next[LFDS611_STACK_POINTER];
    }

    if( element_count < vi->min_elements )
      *stack_validity = LFDS611_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( element_count > vi->max_elements )
      *stack_validity = LFDS611_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;
  }

  /* TRD : now we validate the freelist

           we may be able to check for the expected number of
           elements in the freelist

           if the caller has given us an expected min and max
           number of elements in the stack, then the total number
           of elements in the freelist, minus that min and max,
           gives us the expected number of elements in the
           freelist
  */

  if( vi != NULL ) {
    lfds611_freelist_query( ss->fs, LFDS611_FREELIST_QUERY_ELEMENT_COUNT, NULL, (void *) &total_elements );

    freelist_vi.min_elements = total_elements - vi->max_elements;
    freelist_vi.max_elements = total_elements - vi->min_elements;

    lfds611_freelist_query( ss->fs, LFDS611_FREELIST_QUERY_VALIDATE, (void *) &freelist_vi, (void *) freelist_validity );
  }

  if( vi == NULL )
    lfds611_freelist_query( ss->fs, LFDS611_FREELIST_QUERY_VALIDATE, NULL, (void *) freelist_validity );

  return;
}

