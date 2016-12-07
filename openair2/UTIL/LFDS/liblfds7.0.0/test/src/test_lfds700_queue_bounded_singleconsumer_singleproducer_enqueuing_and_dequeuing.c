/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  enum flag
    error_flag;

  struct lfds700_queue_bss_state
    *qs;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_enqueuer( void *util_thread_starter_thread_state );
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_dequeuer( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_queue_bss_enqueuing_and_dequeuing( struct lfds700_list_asu_state *list_of_logical_processors )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_logical_processors,
    subloop;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_queue_bss_element
    element_array[4];

  struct lfds700_queue_bss_state
    qs;

  struct test_pal_logical_processor
    *lp,
    *lp_first;

  struct util_thread_starter_state
    *tts;

  struct test_state
    *ts;

  test_pal_thread_state_t
    *thread_handles;

  assert( list_of_logical_processors != NULL );

  /* TRD : so, this is the real test
           problem is, because we use memory barriers only
           and we only support one producer and one consumer
           we need to ensure these threads are on different physical cores
           if they're on the same core, the code would work even without memory barriers

           problem is, in the test application, we only know the *number* of logical cores
           obtaining topology information adds a great deal of complexity to the test app
           and makes porting much harder

           so, we know how many logical cores there are; my thought is to partially
           permutate over them - we always run the producer on core 0, but we iterate
           over the other logical cores, running the test once each time, with the
           consumer being run on core 0, then core 1, then core 2, etc

           (we run on core 0 for the single-cpu case; it's redundent, since a single
            logical core running both producer and consumer will work, but otherwise
            we have to skip the test, which is confusing for the user)

           the test is one thread enqueuing and one thread dequeuing for two seconds
  */

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  internal_display_test_name( "Enqueuing and dequeuing (%d seconds)", number_logical_processors * 2 );

  ts = util_malloc_wrapper( sizeof(struct test_state) * 2 );

  for( loop = 0 ; loop < 2 ; loop++ )
  {
    (ts+loop)->qs = &qs;
    (ts+loop)->error_flag = LOWERED;
  }

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * 2 );

  /* TRD : producer always on core 0
           iterate over the other cores with consumer
  */
  
  lasue = LFDS700_LIST_ASU_GET_START( *list_of_logical_processors );
  lp_first = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );

  while( lasue != NULL )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );

    lfds700_queue_bss_init_valid_on_current_logical_core( &qs, element_array, 4, NULL );

    util_thread_starter_new( &tts, 2 );

    LFDS700_MISC_BARRIER_STORE;

    lfds700_misc_force_store();

    util_thread_starter_start( tts, &thread_handles[0], 0, lp_first, thread_enqueuer, ts );
    util_thread_starter_start( tts, &thread_handles[1], 1, lp, thread_dequeuer, ts+1 );

    util_thread_starter_run( tts );

    for( subloop = 0 ; subloop < 2 ; subloop++ )
      test_pal_thread_wait( thread_handles[subloop] );

    util_thread_starter_delete( tts );

    LFDS700_MISC_BARRIER_LOAD;

    lfds700_queue_bss_cleanup( &qs, NULL );

    lasue = LFDS700_LIST_ASU_GET_NEXT( *lasue );
  }

  if( (ts+1)->error_flag == RAISED )
    dvs = LFDS700_MISC_VALIDITY_INVALID_TEST_DATA;

  free( thread_handles );

  free( ts );

  internal_display_test_result( 1, "queue_bss", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_enqueuer( void *util_thread_starter_thread_state )
{
  int
    rv;

  lfds700_pal_uint_t
    datum = 0,
    time_loop = 0;

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

  util_thread_starter_ready_and_wait( tsts );

  current_time = start_time = time( NULL );

  while( current_time < start_time + 2 )
  {
    rv = lfds700_queue_bss_enqueue( ts->qs, NULL, (void *) datum );

    if( rv == 1 )
      if( ++datum == 4 )
        datum = 0;

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





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_dequeuer( void *util_thread_starter_thread_state )
{
  int
    rv;

  lfds700_pal_uint_t
    datum,
    expected_datum = 0,
    time_loop = 0;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  time_t
    current_time,
    start_time;

  LFDS700_MISC_BARRIER_LOAD;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  util_thread_starter_ready_and_wait( tsts );

  current_time = start_time = time( NULL );

  while( current_time < start_time + 2 )
  {
    rv = lfds700_queue_bss_dequeue( ts->qs, NULL, (void *) &datum );

    if( rv == 1 )
    {
      if( datum != expected_datum )
        ts->error_flag = RAISED;

      if( ++expected_datum == 4 )
        expected_datum = 0;
    }

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

