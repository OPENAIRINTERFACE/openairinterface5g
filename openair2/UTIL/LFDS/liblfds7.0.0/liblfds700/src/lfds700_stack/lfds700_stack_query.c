/***** includes *****/
#include "lfds700_stack_internal.h"

/***** private prototypes *****/
static void lfds700_stack_internal_stack_validate( struct lfds700_stack_state *ss, struct lfds700_misc_validation_info *vi, enum lfds700_misc_validity *lfds700_stack_validity );





/****************************************************************************/
void lfds700_stack_query( struct lfds700_stack_state *ss, enum lfds700_stack_query query_type, void *query_input, void *query_output )
{
  struct lfds700_stack_element
    *se;

  LFDS700_MISC_BARRIER_LOAD;

  LFDS700_PAL_ASSERT( ss != NULL );
  // TRD : query_type can be any value in its range

  switch( query_type )
  {
    case LFDS700_STACK_QUERY_SINGLETHREADED_GET_COUNT:
      LFDS700_PAL_ASSERT( query_input == NULL );
      LFDS700_PAL_ASSERT( query_output != NULL );

      *(lfds700_pal_uint_t *) query_output = 0;

      se = (struct lfds700_stack_element *) ss->top[POINTER];

      while( se != NULL )
      {
        ( *(lfds700_pal_uint_t *) query_output )++;
        se = (struct lfds700_stack_element *) se->next;
      }
    break;

    case LFDS700_STACK_QUERY_SINGLETHREADED_VALIDATE:
      // TRD : query_input can be NULL
      LFDS700_PAL_ASSERT( query_output != NULL );

      lfds700_stack_internal_stack_validate( ss, (struct lfds700_misc_validation_info *) query_input, (enum lfds700_misc_validity *) query_output );
    break;
  }

  return;
}





/****************************************************************************/
static void lfds700_stack_internal_stack_validate( struct lfds700_stack_state *ss, struct lfds700_misc_validation_info *vi, enum lfds700_misc_validity *lfds700_stack_validity )
{
  lfds700_pal_uint_t
    number_elements = 0;

  struct lfds700_stack_element
    *se_fast,
    *se_slow;

  LFDS700_PAL_ASSERT( ss != NULL );
  // TRD : vi can be NULL
  LFDS700_PAL_ASSERT( lfds700_stack_validity != NULL );

  *lfds700_stack_validity = LFDS700_MISC_VALIDITY_VALID;

  se_slow = se_fast = (struct lfds700_stack_element *) ss->top[POINTER];

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
    do
    {
      se_slow = se_slow->next;

      if( se_fast != NULL )
        se_fast = se_fast->next;

      if( se_fast != NULL )
        se_fast = se_fast->next;
    }
    while( se_slow != NULL and se_fast != se_slow );

  if( se_fast != NULL and se_slow != NULL and se_fast == se_slow )
    *lfds700_stack_validity = LFDS700_MISC_VALIDITY_INVALID_LOOP;

  /* TRD : now check for expected number of elements
           vi can be NULL, in which case we do not check
           we know we don't have a loop from our earlier check
  */

  if( *lfds700_stack_validity == LFDS700_MISC_VALIDITY_VALID and vi != NULL )
  {
    lfds700_stack_query( ss, LFDS700_STACK_QUERY_SINGLETHREADED_GET_COUNT, NULL, (void *) &number_elements );

    if( number_elements < vi->min_elements )
      *lfds700_stack_validity = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( number_elements > vi->max_elements )
      *lfds700_stack_validity = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;
  }

  return;
}

