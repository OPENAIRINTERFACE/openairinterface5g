/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_element
{
  lfds700_pal_uint_t
    thread_number,
    datum;
};

struct test_state
{
  lfds700_pal_uint_t
    thread_number,
    write_count;

  struct test_element
    te;

  struct lfds700_ringbuffer_state
    *rs;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_simple_writer( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_ringbuffer_writing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs[2] = { LFDS700_MISC_VALIDITY_VALID, LFDS700_MISC_VALIDITY_VALID };

  lfds700_pal_uint_t
    loop,
    number_elements_with_dummy_element,
    number_elements_without_dummy_element,
    number_logical_processors,
    *per_thread_counters;

  test_pal_thread_state_t
    *thread_handles;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_ringbuffer_element
    *re_array;

  struct lfds700_ringbuffer_state
    rs;

  struct lfds700_misc_validation_info
    vi;

  struct test_pal_logical_processor
    *lp;

  struct util_thread_starter_state
    *tts;

  struct test_element
    *te,
    *te_array;

  struct test_state
    *ts;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create a single ringbuffer
           with n elements
           we create n test elements
           which are thread_number/counter pairs
           init them to safe values
           and fully populate the ringbuffer

           we create one thread per CPU
           where each thread busy-works writing
           for ten seconds; each thread has one extra element
           which it uses for the first write and after that
           it uses the element it picks up from overwriting

           the user data in each written element is a combination
           of the thread number and the counter

           after the threads are complete, we validate by
           checking the user data counters increment on a per thread
           basis
  */

  internal_display_test_name( "Writing (%d seconds)", TEST_DURATION_IN_SECONDS );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements_with_dummy_element = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) + sizeof(struct lfds700_ringbuffer_element) );
  number_elements_without_dummy_element = number_elements_with_dummy_element - 1;

  vi.min_elements = number_elements_without_dummy_element;
  vi.max_elements = number_elements_without_dummy_element;

  re_array = util_aligned_malloc( sizeof(struct lfds700_ringbuffer_element) * number_elements_with_dummy_element, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_ringbuffer_init_valid_on_current_logical_core( &rs, re_array, number_elements_with_dummy_element, &ps, NULL );

  te_array = util_malloc_wrapper( sizeof(struct lfds700_ringbuffer_element) * number_elements_without_dummy_element );

  // TRD : init the test elements and write them into the ringbuffer
  for( loop = 0 ; loop < number_elements_without_dummy_element ; loop++ )
  {
    te_array[loop].thread_number = 0;
    te_array[loop].datum = 0;
    lfds700_ringbuffer_write( &rs, NULL, &te_array[loop], NULL, NULL, NULL, &ps );
  }

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->rs = &rs;
    (ts+loop)->thread_number = loop;
    (ts+loop)->write_count = 0;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_simple_writer, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  // TRD : now check results
  per_thread_counters = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    *(per_thread_counters+loop) = 0;

  lfds700_ringbuffer_query( &rs, LFDS700_RINGBUFFER_QUERY_SINGLETHREADED_VALIDATE, &vi, dvs );

  while( dvs[0] == LFDS700_MISC_VALIDITY_VALID and dvs[1] == LFDS700_MISC_VALIDITY_VALID and lfds700_ringbuffer_read(&rs, NULL, (void **) &te, &ps) )
  {
    if( te->thread_number >= number_logical_processors )
    {
      dvs[0] = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;
      break;
    }

    if( per_thread_counters[te->thread_number] == 0 )
      per_thread_counters[te->thread_number] = te->datum;

    if( te->datum < per_thread_counters[te->thread_number] )
      dvs[0] = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

    if( te->datum >= per_thread_counters[te->thread_number] )
      per_thread_counters[te->thread_number] = te->datum+1;
  }

  free( per_thread_counters );

  lfds700_ringbuffer_cleanup( &rs, NULL );

  free( ts );

  util_aligned_free( re_array );

  free( te_array );

  internal_display_test_result( 2, "queue", dvs[0], "freelist", dvs[1] );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_simple_writer( void *util_thread_starter_thread_state )
{
  enum lfds700_misc_flag
    overwrite_occurred_flag;

  lfds700_pal_uint_t
    time_loop = 0;

  struct lfds700_misc_prng_state
    ps;

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

  ts->te.thread_number = 0;
  ts->te.datum = 0;

  lfds700_ringbuffer_write( ts->rs, NULL, &ts->te, &overwrite_occurred_flag, NULL, (void **) &te, &ps );

  util_thread_starter_ready_and_wait( tsts );

  current_time = start_time = time( NULL );

  while( current_time < start_time + TEST_DURATION_IN_SECONDS )
  {
    te->thread_number = ts->thread_number;
    te->datum = ts->write_count++;

    lfds700_ringbuffer_write( ts->rs, NULL, te, &overwrite_occurred_flag, NULL, (void **) &te, &ps );

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

