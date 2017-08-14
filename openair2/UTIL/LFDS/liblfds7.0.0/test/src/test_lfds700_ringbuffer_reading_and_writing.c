/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  enum flag
    error_flag;

  lfds700_pal_uint_t
    counter,
    number_logical_processors,
    *per_thread_counters,
    thread_number;

  struct lfds700_ringbuffer_state
    *rs;
};

struct test_element
{
  lfds700_pal_uint_t
    datum,
    thread_number;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_reader_writer( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_ringbuffer_reading_and_writing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs[2] = { LFDS700_MISC_VALIDITY_VALID, LFDS700_MISC_VALIDITY_VALID };

  lfds700_pal_uint_t
    loop,
    number_elements_with_dummy_element,
    number_elements_without_dummy_element,
    number_logical_processors,
    subloop;

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
    *te_array;

  struct test_state
    *ts;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create a single ringbuffer
           with 100,000 elements
           the ringbuffers starts empty

           we create one thread per CPU
           where each thread busy-works writing
           and then immediately reading
           for ten seconds

           the user data in each written element is a combination
           of the thread number and the counter

           while a thread runs, it keeps track of the
           counters for the other threads and throws an error
           if it sees the number stay the same or decrease
  */

  internal_display_test_name( "Reading and writing (%d seconds)", TEST_DURATION_IN_SECONDS );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements_with_dummy_element = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) + sizeof(struct lfds700_ringbuffer_element) );
  number_elements_without_dummy_element = number_elements_with_dummy_element - 1;

  vi.min_elements = 0;
  vi.max_elements = number_elements_without_dummy_element;

  re_array = util_aligned_malloc( sizeof(struct lfds700_ringbuffer_element) * number_elements_with_dummy_element, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_ringbuffer_init_valid_on_current_logical_core( &rs, re_array, number_elements_with_dummy_element, &ps, NULL );

  te_array = util_malloc_wrapper( sizeof(struct test_element) * number_elements_without_dummy_element );

  // TRD : populate the ringbuffer
  for( loop = 0 ; loop < number_elements_without_dummy_element ; loop++ )
  {
    te_array[loop].thread_number = 0;
    te_array[loop].datum = (lfds700_pal_uint_t) -1 ;
    lfds700_ringbuffer_write( &rs, NULL, &te_array[loop], NULL, NULL, NULL, &ps );
  }

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->rs = &rs;
    (ts+loop)->thread_number = loop;
    (ts+loop)->counter = 0;
    (ts+loop)->number_logical_processors = number_logical_processors;
    (ts+loop)->error_flag = LOWERED;
    (ts+loop)->per_thread_counters = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_logical_processors );

    for( subloop = 0 ; subloop < number_logical_processors ; subloop++ )
      *((ts+loop)->per_thread_counters+subloop) = 0;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_reader_writer, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  lfds700_ringbuffer_query( &rs, LFDS700_RINGBUFFER_QUERY_SINGLETHREADED_VALIDATE, (void *) &vi, (void *) dvs );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    if( (ts+loop)->error_flag == RAISED )
      dvs[0] = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    free( (ts+loop)->per_thread_counters );

  free( ts );

  lfds700_ringbuffer_cleanup( &rs, NULL );

  util_aligned_free( re_array );

  free( te_array );

  internal_display_test_result( 2, "queue", dvs[0], "freelist", dvs[1] );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_reader_writer( void *util_thread_starter_thread_state )
{
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

  util_thread_starter_ready_and_wait( tsts );

  current_time = start_time = time( NULL );

  while( current_time < start_time + TEST_DURATION_IN_SECONDS )
  {
    lfds700_ringbuffer_read( ts->rs, NULL, (void **) &te, &ps );

    if( te->thread_number >= ts->number_logical_processors )
      ts->error_flag = RAISED;
    else
    {
      if( te->datum < ts->per_thread_counters[te->thread_number] )
        ts->error_flag = RAISED;

      if( te->datum >= ts->per_thread_counters[te->thread_number] )
        ts->per_thread_counters[te->thread_number] = te->datum+1;
    }

    te->thread_number = ts->thread_number;
    te->datum = ts->counter++;

    lfds700_ringbuffer_write( ts->rs, NULL, te, NULL, NULL, NULL, &ps );

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

