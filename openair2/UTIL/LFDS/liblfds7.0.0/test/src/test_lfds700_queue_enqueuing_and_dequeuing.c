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
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_enqueuer_and_dequeuer( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_queue_enqueuing_and_dequeuing( struct lfds700_list_asu_state *list_of_logical_processors )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_logical_processors,
    subloop;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_state
    qs;

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

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );
  // TRD : qt can be any value in its range

  /* TRD : create a queue with one element per thread
           each thread constly dequeues and enqueues from that one queue
           where when enqueuing sets in the element
           its thread number and counter
           and when dequeuing, checks the thread number and counter
           against previously seen counter for that thread
           where it should always see a higher number
  */

  internal_display_test_name( "Enqueuing and dequeuing (%d seconds)", TEST_DURATION_IN_SECONDS );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  te_array = util_aligned_malloc( sizeof(struct test_element) * (number_logical_processors+1), LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_queue_init_valid_on_current_logical_core( &qs, &(te_array+number_logical_processors)->qe, &ps, NULL );

  // TRD : we assume the test will iterate at least once (or we'll have a false negative)
  for( loop = 0 ; loop < number_logical_processors ; loop++ )
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
    (ts+loop)->error_flag = LOWERED;
    (ts+loop)->per_thread_counters = util_malloc_wrapper( sizeof(lfds700_pal_uint_t) * number_logical_processors );
    (ts+loop)->number_logical_processors = number_logical_processors;

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
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_enqueuer_and_dequeuer, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  vi.min_elements = vi.max_elements = number_logical_processors;

  lfds700_queue_query( &qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    if( (ts+loop)->error_flag == RAISED )
      dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    free( (ts+loop)->per_thread_counters );

  util_aligned_free( te_array );

  free( ts );

  lfds700_queue_cleanup( &qs, NULL );

  internal_display_test_result( 1, "queue", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_enqueuer_and_dequeuer( void *util_thread_starter_thread_state )
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

    if( te->thread_number >= ts->number_logical_processors )
      ts->error_flag = RAISED;
    else
    {
      if( te->counter < ts->per_thread_counters[te->thread_number] )
        ts->error_flag = RAISED;

      if( te->counter >= ts->per_thread_counters[te->thread_number] )
        ts->per_thread_counters[te->thread_number] = te->counter+1;
    }

    te->thread_number = ts->thread_number;
    te->counter = ++ts->counter;

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

