/***** includes *****/
#include "lfds700_list_addonly_ordered_singlylinked_internal.h"





/****************************************************************************/
enum lfds700_list_aos_insert_result lfds700_list_aos_insert( struct lfds700_list_aos_state *laoss,
                                                         struct lfds700_list_aos_element *laose,
                                                         struct lfds700_list_aos_element **existing_laose,
                                                         struct lfds700_misc_prng_state *ps )
{
  char unsigned 
    result;

  enum lfds700_misc_flag
    finished_flag = LFDS700_MISC_FLAG_LOWERED;

  int
    compare_result = 0;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  struct lfds700_list_aos_element
    *volatile laose_temp = NULL,
    *volatile laose_trailing;

  LFDS700_PAL_ASSERT( laoss != NULL );
  LFDS700_PAL_ASSERT( laose != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &laose->next % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &laose->value % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &laose->key % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  // TRD : existing_laose can be NULL
  LFDS700_PAL_ASSERT( ps != NULL );

  /* TRD : imagine a list, sorted small to large

           we arrive at an element
           we obtain its next pointer
           we check we are greater than the current element and smaller than the next element
           this means we have found the correct location to insert
           we try to CAS ourselves in; in the meantime,
           someone else has *aready* swapped in an element which is smaller than we are

           e.g.

           the list is { 1, 10 } and we are the value 5

           we arrive at 1; we check the next element and see it is 10
           so we are larger than the current element and smaller than the next
           we are in the correct location to insert and we go to insert...

           in the meantime, someone else with the value 3 comes along
           he too finds this is the correct location and inserts before we do
           the list is now { 1, 3, 10 } and we are trying to insert now after
           1 and before 3!

           our insert CAS fails, because the next pointer of 1 has changed aready;
           but we see we are in the wrong location - we need to move forward an
           element
  */

  LFDS700_MISC_BARRIER_LOAD;

  /* TRD : we need to begin with the leading dummy element
           as the element to be inserted
           may be smaller than all elements in the list
  */

  laose_trailing = laoss->start;
  laose_temp = laoss->start->next;

  while( finished_flag == LFDS700_MISC_FLAG_LOWERED )
  {
    if( laose_temp == NULL )
      compare_result = -1;

    if( laose_temp != NULL )
    {
      LFDS700_MISC_BARRIER_LOAD;
      compare_result = laoss->key_compare_function( laose->key, laose_temp->key );
    }

    if( compare_result == 0 )
    {
      if( existing_laose != NULL )
        *existing_laose = laose_temp;

      switch( laoss->existing_key )
      {
        case LFDS700_LIST_AOS_EXISTING_KEY_OVERWRITE:
          LFDS700_LIST_AOS_SET_VALUE_IN_ELEMENT( *laose_temp, laose->value );
          return( LFDS700_LIST_AOS_INSERT_RESULT_SUCCESS_OVERWRITE );
        break;

        case LFDS700_LIST_AOS_EXISTING_KEY_FAIL:
          return( LFDS700_LIST_AOS_INSERT_RESULT_FAILURE_EXISTING_KEY );
        break;
      }

      finished_flag = LFDS700_MISC_FLAG_RAISED;
    }

    if( compare_result < 0 )
    {
      laose->next = laose_temp;
      LFDS700_MISC_BARRIER_STORE;
      LFDS700_PAL_ATOMIC_CAS_WITH_BACKOFF( &laose_trailing->next, &laose->next, laose, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );

      if( result == 1 )
        finished_flag = LFDS700_MISC_FLAG_RAISED;
      else
        // TRD : if we fail to link, someone else has linked and so we need to redetermine our position is correct
        laose_temp = laose_trailing->next;
    }

    if( compare_result > 0 )
    {
      // TRD : move trailing along by one element
      laose_trailing = laose_trailing->next;

      /* TRD : set temp as the element after trailing
               if the new element we're linking is larger than all elements in the list,
               laose_temp will now go to NULL and we'll link at the end
      */
      laose_temp = laose_trailing->next;
    }
  }

  return( LFDS700_LIST_AOS_INSERT_RESULT_SUCCESS );
}

