/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_state
{
  struct lfds700_stack_state
    *ss;
};

struct test_element
{
  struct lfds700_stack_element
    se;

  lfds700_pal_uint_t
    datum;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_rapid_popping_and_pushing( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_stack_rapid_popping_and_pushing( struct lfds700_list_asu_state *list_of_logical_processors )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_logical_processors;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_stack_state
    ss;

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
  // TRD : st can be any value in its range

  /* TRD : in these tests there is a fundamental antagonism between
           how much checking/memory clean up that we do and the
           likelyhood of collisions between threads in their lock-free
           operations

           the lock-free operations are very quick; if we do anything
           much at all between operations, we greatly reduce the chance
           of threads colliding

           so we have some tests which do enough checking/clean up that
           they can tell the stack is valid and don't leak memory
           and here, this test now is one of those which does minimal
           checking - in fact, the nature of the test is that you can't
           do any real checking - but goes very quickly

           what we do is create a small stack and then run one thread
           per CPU, where each thread simply pushes and then immediately
           pops

           the test runs for ten seconds

           after the test is done, the only check we do is to traverse
           the stack, checking for loops and ensuring the number of
           elements is correct
  */

  internal_display_test_name( "Rapid popping and pushing (%d seconds)", TEST_DURATION_IN_SECONDS );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  ts = util_malloc_wrapper( sizeof(struct test_state) * number_logical_processors );

  lfds700_stack_init_valid_on_current_logical_core( &ss, NULL );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    (ts+loop)->ss = &ss;

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * number_logical_processors );

  // TRD : we need one element per thread
  te_array = util_aligned_malloc( sizeof(struct test_element) * number_logical_processors, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    LFDS700_STACK_SET_VALUE_IN_ELEMENT( (te_array+loop)->se, te_array+loop );
    lfds700_stack_push( &ss, &(te_array+loop)->se, &ps );
  }

  util_thread_starter_new( &tts, number_logical_processors );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;
  lasue = NULL;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_rapid_popping_and_pushing, ts+loop );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  vi.min_elements = vi.max_elements = number_logical_processors;

  lfds700_stack_query( &ss, LFDS700_STACK_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  lfds700_stack_cleanup( &ss, NULL );

  util_aligned_free( te_array );

  free( ts );

  // TRD : print the test result
  internal_display_test_result( 1, "stack", dvs );

  return;
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_rapid_popping_and_pushing( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    time_loop = 0;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_stack_element
    *se;

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
    lfds700_stack_pop( ts->ss, &se, &ps );
    lfds700_stack_push( ts->ss, se, &ps );

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

