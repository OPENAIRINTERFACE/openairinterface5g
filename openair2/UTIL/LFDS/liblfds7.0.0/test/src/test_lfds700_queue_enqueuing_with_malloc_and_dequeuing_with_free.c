/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  struct lfds700_queue_state
    *qs;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_enqueuer_with_malloc_and_dequeuer_with_free( void *util_thread_starter_thread_state );
static void queue_element_cleanup_callback( struct lfds700_queue_state *qs, struct lfds700_queue_element *qe, enum lfds700_misc_flag dummy_element_flag );





/****************************************************************************/
void test_lfds700_queue_enqueuing_with_malloc_and_dequeuing_with_free( struct lfds700_list_asu_state *list_of_logical_processors )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_logical_processors;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_element
    *qe;

  struct lfds700_queue_state
    qs;

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
  // TRD : qt can be any value in its range

  /* TRD : one thread per logical core
           each thread loops for ten seconds
           mallocs and enqueues 1k elements, then dequeues and frees 1k elements
  */

  internal_display_test_name( "Enqueuing with malloc dequeuing with free (%d seconds)", TEST_DURATION_IN_SECONDS );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  qe = util_aligned_malloc( sizeof(struct lfds700_queue_element), LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  lfds700_queue_init_valid_on_current_logical_core( &qs, qe, &ps, NULL );

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    (ts+loop)->qs = &qs;

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * number_logical_processors );

  util_thread_starter_new( &tts, number_logical_processors );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;
  lasue = NULL;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_enqueuer_with_malloc_and_dequeuer_with_free, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  vi.min_elements = vi.max_elements = 0;

  lfds700_queue_query( &qs, LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  free( ts );

  lfds700_queue_cleanup( &qs, queue_element_cleanup_callback );

  internal_display_test_result( 1, "queue", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_enqueuer_with_malloc_and_dequeuer_with_free( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    loop,
    time_loop = 0;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_queue_element
    *qe;

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
    for( loop = 0 ; loop < 1000 ; loop++ )
    {
      qe = util_aligned_malloc( sizeof(struct lfds700_queue_element), LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );
      lfds700_queue_enqueue( ts->qs, qe, &ps );
    }

    for( loop = 0 ; loop < 1000 ; loop++ )
    {
      lfds700_queue_dequeue( ts->qs, &qe, &ps );
      util_aligned_free( qe );
    }

    if( time_loop++ == REDUCED_TIME_LOOP_COUNT )
    {
      time_loop = 0;
      time( &current_time );
    }
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
#pragma warning( disable : 4100 )

static void queue_element_cleanup_callback( struct lfds700_queue_state *qs, struct lfds700_queue_element *qe, enum lfds700_misc_flag dummy_element_flag )
{
  assert( qs != NULL );
  assert( qe != NULL );
  // TRD : dummy_element_flag can be any value in its range

  util_aligned_free( qe );

  return;
}

#pragma warning( default : 4100 )

