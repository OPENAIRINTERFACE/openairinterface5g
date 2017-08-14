/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_element
{
  struct lfds700_btree_au_element
    baue;

  lfds700_pal_uint_t
    key;
};

struct test_state
{
  lfds700_pal_uint_t
    insert_fail_count,
    number_elements;

  struct lfds700_btree_au_state
    *baus;

  struct test_element
    *element_array;
};

/***** private prototypes *****/
static int key_compare_function( void const *new_value, void const *value_in_tree );
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_adding( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_btree_au_random_adds_fail_on_existing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    actual_sum_insert_failure_count,
    expected_sum_insert_failure_count,
    index = 0,
    *key_count_array,
    loop,
    number_elements,
    number_logical_processors,
    random_value,
    subloop;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_btree_au_element
    *baue = NULL;

  struct lfds700_btree_au_state
    baus;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_misc_validation_info
    vi;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  void
    *key;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create a single btree_au
           we generate 10k elements per thread (one per logical processor) in an array
           we set a random number in each element, which is the key
           random numbers are generated are from 0 to 5000, so we must have some duplicates
           (we don't use value, so we always pass in a NULL for that when we insert)

           each thread loops, adds those elements into the btree, and counts the total number of insert fails
           (we don't count on a per value basis because of the performance hit - we'll be TLBing all the time)
           this test has the btree_au set to fail on add, so duplicates should be eliminated

           we then merge the per-thread arrays

           we should find in the tree one of every value, and the sum of the counts of each value (beyond the
           first value, which was inserted) in the merged arrays should equal the sum of the insert fails from
           each thread

           we check the count of unique values in the merged array and use that when calling the btree_au validation function

           we in-order walk and check that what we have in the tree matches what we have in the merged array
           and then check the fail counts
  */

  internal_display_test_name( "Random adds and walking (fail on existing key)" );

  lfds700_misc_prng_init( &ps );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_btree_au_init_valid_on_current_logical_core( &baus, key_compare_function, LFDS700_BTREE_AU_EXISTING_KEY_FAIL, NULL );

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );
  number_elements = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) * number_logical_processors );
  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->baus = &baus;
    (ts+loop)->element_array = util_aligned_malloc( sizeof(struct test_element) * number_elements, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );
    (ts+loop)->number_elements = number_elements;
    (ts+loop)->insert_fail_count = 0;

    for( subloop = 0 ; subloop < number_elements ; subloop++ )
    {
      random_value = LFDS700_MISC_PRNG_GENERATE( &ps );
      ((ts+loop)->element_array+subloop)->key = (lfds700_pal_uint_t) floor( (number_elements/2) * ((double) random_value / (double) LFDS700_MISC_PRNG_MAX) );
    }
  }

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * number_logical_processors );

  util_thread_starter_new( &tts, number_logical_processors );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;
  lasue = NULL;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_adding, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  /* TRD : now for validation
           make an array equal to number_elements, set all to 0
           iterate over every per-thread array, counting the number of each value into this array
           so we can know how many elements ought to have failed to be inserted
           as well as being able to work out the actual number of elements which should be present in the btree, for the btree validation call
  */

  key_count_array = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_elements );
  for( loop = 0 ; loop < number_elements ; loop++ )
    *(key_count_array+loop) = 0;

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    for( subloop = 0 ; subloop < number_elements ; subloop++ )
      ( *(key_count_array+( (ts+loop)->element_array+subloop)->key) )++;

  // TRD : first, btree validation function
  vi.min_elements = number_elements;

  for( loop = 0 ; loop < number_elements ; loop++ )
    if( *(key_count_array+loop) == 0 )
      vi.min_elements--;

  vi.max_elements = vi.min_elements;

  lfds700_btree_au_query( &baus, LFDS700_BTREE_AU_QUERY_SINGLETHREADED_VALIDATE, (void *) &vi, (void *) &dvs );

  /* TRD : now check the sum of per-thread insert failures
           is what it should be, which is the sum of key_count_array,
           but with every count minus one (for the single succesful insert)
           and where elements of 0 are ignored (i.e. do not have -1 applied)
  */

  expected_sum_insert_failure_count = 0;

  for( loop = 0 ; loop < number_elements ; loop++ )
    if( *(key_count_array+loop) != 0 )
      expected_sum_insert_failure_count += *(key_count_array+loop) - 1;

  actual_sum_insert_failure_count = 0;

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    actual_sum_insert_failure_count += (ts+loop)->insert_fail_count;

  if( expected_sum_insert_failure_count != actual_sum_insert_failure_count )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  /* TRD : now compared the combined array and an in-order walk of the tree
           ignoring array elements with the value 0, we should find an exact match
  */

  if( dvs == LFDS700_MISC_VALIDITY_VALID )
  {
    // TRD : in-order walk over btree_au and check key_count_array matches
    while( dvs == LFDS700_MISC_VALIDITY_VALID and lfds700_btree_au_get_by_absolute_position_and_then_by_relative_position(&baus, &baue, LFDS700_BTREE_AU_ABSOLUTE_POSITION_SMALLEST_IN_TREE, LFDS700_BTREE_AU_RELATIVE_POSITION_NEXT_LARGER_ELEMENT_IN_ENTIRE_TREE) )
    {
      key = LFDS700_BTREE_AU_GET_KEY_FROM_ELEMENT( *baue );

      while( *(key_count_array+index) == 0 )
        index++;

      if( index++ != (lfds700_pal_uint_t) key )
        dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
    }
  }

  // TRD : cleanup
  free( key_count_array );

  lfds700_btree_au_cleanup( &baus, NULL );

  // TRD : cleanup
  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    util_aligned_free( (ts+loop)->element_array );

  free( ts );

  // TRD : print the test result
  internal_display_test_result( 1, "btree_au", dvs );

  return;
}





/****************************************************************************/
#pragma warning( disable : 4100 )

static int key_compare_function( void const *new_key, void const *key_in_tree )
{
  int
    cr = 0;

  // TRD : key_new can be any value in its range
  // TRD : key_in_tree can be any value in its range

  if( (lfds700_pal_uint_t) new_key < (lfds700_pal_uint_t) key_in_tree )
    cr = -1;

  if( (lfds700_pal_uint_t) new_key > (lfds700_pal_uint_t) key_in_tree )
    cr = 1;

  return( cr );
}

#pragma warning( default : 4100 )





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_adding( void *util_thread_starter_thread_state )
{
  enum lfds700_btree_au_insert_result
    alr;

  lfds700_pal_uint_t
    index = 0;

  struct test_state
    *ts;

  struct lfds700_misc_prng_state
    ps;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  util_thread_starter_ready_and_wait( tsts );

  while( index < ts->number_elements )
  {
    LFDS700_BTREE_AU_SET_KEY_IN_ELEMENT( (ts->element_array+index)->baue, (ts->element_array+index)->key );
    LFDS700_BTREE_AU_SET_VALUE_IN_ELEMENT( (ts->element_array+index)->baue, 0 );
    alr = lfds700_btree_au_insert( ts->baus, &(ts->element_array+index)->baue, NULL, &ps );

    if( alr == LFDS700_BTREE_AU_INSERT_RESULT_FAILURE_EXISTING_KEY )
      ts->insert_fail_count++;

    index++;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

