/***** includes *****/
#include "lfds700_list_addonly_singlylinked_unordered_internal.h"





/****************************************************************************/
void lfds700_list_asu_insert_at_position( struct lfds700_list_asu_state *lasus,
                                          struct lfds700_list_asu_element *lasue,
                                          struct lfds700_list_asu_element *lasue_predecessor,
                                          enum lfds700_list_asu_position position,
                                          struct lfds700_misc_prng_state *ps )
{
  LFDS700_PAL_ASSERT( lasus != NULL );
  LFDS700_PAL_ASSERT( lasue != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->next % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->value % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->key % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  // TRD : lasue_predecessor asserted in the switch
  // TRD : position can be any value in its range
  LFDS700_PAL_ASSERT( ps != NULL );

  switch( position )
  {
    case LFDS700_LIST_ASU_POSITION_START:
      lfds700_list_asu_insert_at_start( lasus, lasue, ps );
    break;

    case LFDS700_LIST_ASU_POSITION_END:
      lfds700_list_asu_insert_at_end( lasus, lasue, ps );
    break;

    case LFDS700_LIST_ASU_POSITION_AFTER:
      lfds700_list_asu_insert_after_element( lasus, lasue, lasue_predecessor, ps );
    break;
  }

  return;
}





/****************************************************************************/
void lfds700_list_asu_insert_at_start( struct lfds700_list_asu_state *lasus,
                                       struct lfds700_list_asu_element *lasue,
                                       struct lfds700_misc_prng_state *ps )
{
  char unsigned 
    result;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  LFDS700_PAL_ASSERT( lasus != NULL );
  LFDS700_PAL_ASSERT( lasue != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->next % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->value % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->key % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( ps != NULL );

  LFDS700_MISC_BARRIER_LOAD;

  lasue->next = lasus->start->next;

  do
  {
    LFDS700_MISC_BARRIER_STORE;
    LFDS700_PAL_ATOMIC_CAS_WITH_BACKOFF( &lasus->start->next, &lasue->next, lasue, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );
  }
  while( result != 1 );

  return;
}





/****************************************************************************/
void lfds700_list_asu_insert_at_end( struct lfds700_list_asu_state *lasus,
                                     struct lfds700_list_asu_element *lasue,
                                     struct lfds700_misc_prng_state *ps )
{
  char unsigned 
    result;

  enum lfds700_misc_flag
    finished_flag = LFDS700_MISC_FLAG_LOWERED;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  struct lfds700_list_asu_element LFDS700_PAL_ALIGN(LFDS700_PAL_ALIGN_SINGLE_POINTER)
    *compare;

  struct lfds700_list_asu_element
    *volatile lasue_next,
    *volatile lasue_end;

  LFDS700_PAL_ASSERT( lasus != NULL );
  LFDS700_PAL_ASSERT( lasue != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->next % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->value % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->key % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( ps != NULL );

  /* TRD : begin by assuming end is correctly pointing to the final element
           try to link (comparing for next being NULL)
           if we fail, move down list till we find last element
           and retry
           when successful, update end to ourselves

           note there's a leading dummy element
           so lasus->end always points to an element
  */

  LFDS700_MISC_BARRIER_LOAD;

  lasue->next = NULL;
  lasue_end = lasus->end;

  while( finished_flag == LFDS700_MISC_FLAG_LOWERED )
  {
    compare = NULL;

    LFDS700_MISC_BARRIER_STORE;
    LFDS700_PAL_ATOMIC_CAS_WITH_BACKOFF( &lasue_end->next, &compare, lasue, LFDS700_MISC_CAS_STRENGTH_STRONG, result, backoff_iteration, ps );

    if( result == 1 )
      finished_flag = LFDS700_MISC_FLAG_RAISED;
    else
    {
      lasue_end = compare;
      lasue_next = LFDS700_LIST_ASU_GET_NEXT( *lasue_end );

      while( lasue_next != NULL )
      {
        lasue_end = lasue_next;
        lasue_next = LFDS700_LIST_ASU_GET_NEXT( *lasue_end );
      }
    }
  }

  lasus->end = lasue;

  return;
}





/****************************************************************************/

void lfds700_list_asu_insert_after_element( struct lfds700_list_asu_state *lasus,
                                            struct lfds700_list_asu_element *lasue,
                                            struct lfds700_list_asu_element *lasue_predecessor,
                                            struct lfds700_misc_prng_state *ps )
{
  char unsigned 
    result;

  lfds700_pal_uint_t
    backoff_iteration = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;

  LFDS700_PAL_ASSERT( lasus != NULL );
  LFDS700_PAL_ASSERT( lasue != NULL );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->next % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->value % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( (lfds700_pal_uint_t) &lasue->key % LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES == 0 );
  LFDS700_PAL_ASSERT( lasue_predecessor != NULL );
  LFDS700_PAL_ASSERT( ps != NULL );

  LFDS700_MISC_BARRIER_LOAD;

  lasue->next = lasue_predecessor->next;

  do
  {
    LFDS700_MISC_BARRIER_STORE;
    LFDS700_PAL_ATOMIC_CAS_WITH_BACKOFF( &lasue_predecessor->next, &lasue->next, lasue, LFDS700_MISC_CAS_STRENGTH_WEAK, result, backoff_iteration, ps );
  }
  while( result != 1 );

  return;
}


