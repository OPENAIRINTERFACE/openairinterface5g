/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  struct lfds700_freelist_state
    *fs;
};

struct test_element
{
  struct lfds700_freelist_element
    fe;

  enum flag
    popped_flag;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_popping( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_freelist_popping( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_elements,
    number_logical_processors;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_freelist_state
    fs;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_misc_validation_info
    vi = { 0, 0 };

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_element
    *te_array;

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create a freelist with 1,000,000 elements

           the creation function runs in a single thread and creates
           and pushes those elements onto the freelist

           each element contains a void pointer to the container test element

           we then run one thread per CPU
           where each thread loops, popping as quickly as possible
           each test element has a flag which indicates it has been popped

           the threads run till the source freelist is empty

           we then check the test elements
           every element should have been popped

           then tidy up

           we have no extra code for CAS/GC as we're only popping
  */

  internal_display_test_name( "Popping" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  lfds700_freelist_init_valid_on_current_logical_core( &fs, NULL );

  number_elements = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / sizeof(struct test_element);

  te_array = util_aligned_malloc( sizeof(struct test_element) * number_elements, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < number_elements ; loop++ )
  {
    (te_array+loop)->popped_flag = LOWERED;
    LFDS700_FREELIST_SET_VALUE_IN_ELEMENT( (te_array+loop)->fe, te_array+loop );
    lfds700_freelist_push( &fs, &(te_array+loop)->fe, &ps );
  }

  ts = util_aligned_malloc( sizeof(struct test_state) * number_logical_processors, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    (ts+loop)->fs = &fs;

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * number_logical_processors );

  util_thread_starter_new( &tts, number_logical_processors );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;
  lasue = NULL;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_popping, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  lfds700_freelist_query( &fs, LFDS700_FREELIST_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  // TRD : now we check each element has popped_flag set to RAISED
  for( loop = 0 ; loop < number_elements ; loop++ )
    if( (te_array+loop)->popped_flag == LOWERED )
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  // TRD : cleanup
  lfds700_freelist_cleanup( &fs, NULL );
  util_aligned_free( ts );
  util_aligned_free( te_array );

  // TRD : print the test result
  internal_display_test_result( 1, "freelist", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_popping( void *util_thread_starter_thread_state )
{
  struct lfds700_freelist_element
    *fe;

  struct lfds700_misc_prng_state
    ps;

  struct test_element
    *te;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  util_thread_starter_ready_and_wait( tsts );

  while( lfds700_freelist_pop(ts->fs, &fe, &ps) )
  {
    te = LFDS700_FREELIST_GET_VALUE_FROM_ELEMENT( *fe );
    te->popped_flag = RAISED;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

