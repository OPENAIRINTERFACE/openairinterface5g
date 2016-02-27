/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  enum flag
    error_flag;

  struct lfds700_queue_state
    *qs;
};

struct test_element
{
  struct lfds700_queue_element
    qe;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_simple_dequeuer( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_queue_dequeuing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_elements_with_dummy_element,
    number_elements_without_dummy_element,
    number_logical_processors;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_state
    qs;

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

  /* TRD : create a queue, add 1,000,000 elements

           use a single thread to enqueue every element
           each elements user data is an incrementing counter

           then run one thread per CPU
           where each busy-works dequeuing

           when an element is dequeued, we check (on a per-thread basis) the
           value dequeued is greater than the element previously dequeued

           note we have no variation in the test for CAS+GC vs DWCAS
           this is because all we do is dequeue
           what we actually want to stress test is the queue
           not CAS
           so it's better to let the dequeue run as fast as possible
  */

  internal_display_test_name( "Dequeuing" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements_with_dummy_element = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / sizeof(struct test_element);
  number_elements_without_dummy_element = number_elements_with_dummy_element - 1;

  te_array = util_aligned_malloc( sizeof(struct test_element) * number_elements_with_dummy_element, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_queue_init_valid_on_current_logical_core( &qs, &(te_array + number_elements_without_dummy_element)->qe, &ps, NULL );

  for( loop = 0 ; loop < number_elements_without_dummy_element ; loop++ )
  {
    LFDS700_QUEUE_SET_VALUE_IN_ELEMENT( (te_array+loop)->qe, loop );
    lfds700_queue_enqueue( &qs, &(te_array+loop)->qe, &ps );
  }

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->qs = &qs;
    (ts+loop)->error_flag = LOWERED;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_simple_dequeuer, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  // TRD : check queue is empty
  lfds700_queue_query( &qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  // TRD : check for raised error flags
  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    if( (ts+loop)->error_flag == RAISED )
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  free( ts );

  util_aligned_free( te_array );

  lfds700_queue_cleanup( &qs, NULL );

  internal_display_test_result( 1, "queue", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_simple_dequeuer( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    *prev_value,
    *value;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_element
    *qe;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  lfds700_queue_dequeue( ts->qs, &qe, &ps );
  prev_value = LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( *qe );

  util_thread_starter_ready_and_wait( tsts );

  while( lfds700_queue_dequeue(ts->qs, &qe, &ps) )
  {
    value = LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( *qe );

    if( value <= prev_value )
      ts->error_flag = RAISED;

    prev_value = value;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

