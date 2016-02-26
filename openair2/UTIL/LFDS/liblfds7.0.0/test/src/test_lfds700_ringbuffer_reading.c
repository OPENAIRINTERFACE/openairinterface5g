/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  enum flag
    error_flag;

  lfds700_pal_uint_t
    read_count;

  struct lfds700_ringbuffer_state
    *rs;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_simple_reader( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_ringbuffer_reading( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs[2] = { LFDS700_MISC_VALIDITY_VALID, LFDS700_MISC_VALIDITY_VALID };

  lfds700_pal_uint_t
    loop,
    number_elements_with_dummy_element,
    number_elements_without_dummy_element,
    number_logical_processors,
    total_read = 0;

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

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we create a single ringbuffer
           with 1,000,000 elements
           we populate the ringbuffer, where the
           user data is an incrementing counter

           we create one thread per CPU
           where each thread busy-works,
           reading until the ringbuffer is empty

           each thread keep track of the number of reads it manages
           and that each user data it reads is greater than the
           previous user data that was read
  */

  internal_display_test_name( "Reading" );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements_with_dummy_element = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / sizeof(struct lfds700_ringbuffer_element);
  number_elements_without_dummy_element = number_elements_with_dummy_element - 1;

  vi.min_elements = 0;
  vi.max_elements = number_elements_without_dummy_element;

  re_array = util_aligned_malloc( sizeof(struct lfds700_ringbuffer_element) * number_elements_with_dummy_element, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_ringbuffer_init_valid_on_current_logical_core( &rs, re_array, number_elements_with_dummy_element, &ps, NULL );

  // TRD : init the ringbuffer contents for the test
  for( loop = 0 ; loop < number_elements_without_dummy_element ; loop++ )
    lfds700_ringbuffer_write( &rs, NULL, (void *) (size_t) loop, NULL, NULL, NULL, &ps );

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    (ts+loop)->rs = &rs;
    (ts+loop)->read_count = 0;
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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_simple_reader, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  lfds700_ringbuffer_query( &rs, LFDS700_RINGBUFFER_QUERY_SINGLETHREADED_VALIDATE, (void *) &vi, (void *) dvs );

  // TRD : check for raised error flags
  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    if( (ts+loop)->error_flag == RAISED )
      dvs[0] = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  // TRD : check thread reads total to 1,000,000
  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    total_read += (ts+loop)->read_count;

  if( total_read < number_elements_without_dummy_element )
    dvs[0] = LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS;

  if( total_read > number_elements_without_dummy_element )
    dvs[0] = LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS;

  free( ts );

  lfds700_ringbuffer_cleanup( &rs, NULL );

  util_aligned_free( re_array );

  internal_display_test_result( 2, "queue", dvs[0], "freelist", dvs[1] );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_simple_reader( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    *prev_value,
    *value;

  struct lfds700_misc_prng_state
    ps;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  lfds700_ringbuffer_read( ts->rs, NULL, (void **) &prev_value, &ps );
  ts->read_count++;

  util_thread_starter_ready_and_wait( tsts );

  while( lfds700_ringbuffer_read(ts->rs, NULL, (void **) &value, &ps) )
  {
    if( value <= prev_value )
      ts->error_flag = RAISED;

    prev_value = value;

    ts->read_count++;
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

