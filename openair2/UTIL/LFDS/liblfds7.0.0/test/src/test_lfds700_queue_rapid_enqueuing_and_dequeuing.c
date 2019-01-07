/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  lfds700_pal_uint_t
    counter,
    thread_number;

  struct lfds700_queue_state
    *qs;
};

struct test_element
{
  struct lfds700_queue_element
    qe,
    *qe_use;

  lfds700_pal_uint_t
    counter,
    thread_number;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_rapid_enqueuer_and_dequeuer( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_queue_rapid_enqueuing_and_dequeuing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_elements_with_dummy_element,
    number_elements_without_dummy_element,
    number_logical_processors,
    *per_thread_counters;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_element
    *qe;

  struct lfds700_misc_validation_info
    vi;

  struct lfds700_queue_state
    qs;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_element
    *te_array,
    *te;

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create a single queue with 50,000 elements
           we don't want too many elements, so we ensure plenty of element re-use
           each thread simply loops dequeuing and enqueuing
           where the user data indicates thread number and an increment counter
           vertification is that the counter increments on a per-thread basis
  */

  internal_display_test_name( "Rapid enqueuing and dequeuing (%d seconds)", TEST_DURATION_IN_SECONDS );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements_with_dummy_element = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / sizeof(struct test_element);

  if( number_elements_with_dummy_element > (10000 * number_logical_processors) + 1 )
    number_elements_with_dummy_element = (10000 * number_logical_processors) + 1;

  number_elements_without_dummy_element = number_elements_with_dummy_element - 1;

  vi.min_elements = number_elements_without_dummy_element;
  vi.max_elements = number_elements_without_dummy_element;

  te_array = util_aligned_malloc( sizeof(struct test_element) * number_elements_with_dummy_element, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_queue_init_valid_on_current_logical_core( &qs, &(te_array+number_elements_without_dummy_element)->qe, &ps, NULL );

  // TRD : we assume the test will iterate at least once (or we'll have a false negative)
  for( loop = 0 ; loop < number_elements_without_dummy_element ; loop++ )
  {
    (te_array+loop)->thread_number = loop;
    (te_array+loop)->counter = 0;
    LFDS700_QUEUE_SET_VALUE_IN_ELEMENT( (te_array+loop)->qe, te_array+loop );
    lfds700_queue_enqueue( &qs, &(te_array+loop)->qe, &ps );
  }

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->qs = &qs;
    (ts+loop)->thread_number = loop;
    (ts+loop)->counter = 0;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_rapid_enqueuer_and_dequeuer, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  lfds700_queue_query( &qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  // TRD : now check results
  per_thread_counters = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    *(per_thread_counters+loop) = 0;

  while( dvs == LFDS700_MISC_VALIDITY_VALID and lfds700_queue_dequeue(&qs, &qe, &ps) )
  {
    te = LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( *qe );

    if( te->thread_number >= number_logical_processors )
    {
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
      break;
    }

    if( per_thread_counters[te->thread_number] == 0 )
      per_thread_counters[te->thread_number] = te->counter;

    if( te->counter > per_thread_counters[te->thread_number] )
      dvs = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;

    if( te->counter < per_thread_counters[te->thread_number] )
      dvs = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

    if( te->counter == per_thread_counters[te->thread_number] )
      per_thread_counters[te->thread_number]++;
  }

  free( per_thread_counters );

  lfds700_queue_cleanup( &qs, NULL );

  util_aligned_free( te_array );

  free( ts );

  internal_display_test_result( 1, "queue", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_rapid_enqueuer_and_dequeuer( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    time_loop = 0;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_element
    *qe;

  struct test_element
    *te;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  time_t
    current_time,
    start_time;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  util_thread_starter_ready_and_wait( tsts );

  current_time = start_time = time( NULL );

  while( current_time < start_time + TEST_DURATION_IN_SECONDS )
  {
    lfds700_queue_dequeue( ts->qs, &qe, &ps );
    te = LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( *qe );

    te->thread_number = ts->thread_number;
    te->counter = ts->counter++;

    LFDS700_QUEUE_SET_VALUE_IN_ELEMENT( *qe, te );
    lfds700_queue_enqueue( ts->qs, qe, &ps );

    if( time_loop++ == TIME_LOOP_COUNT )
    {
      time_loop = 0;
      time( &current_time );
    }
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

