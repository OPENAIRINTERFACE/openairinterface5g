/***** includes *****/
#include "internal.h"

/***** structs *****/
struct test_element;

struct test_state
{
  lfds700_pal_uint_t
    number_elements;

  struct lfds700_stack_state
    *ss,
    ss_thread_local;

  struct test_element
    *ss_thread_local_te_array;
};

struct test_element
{
  struct lfds700_stack_element
    se,
    thread_local_se;

  lfds700_pal_uint_t
    datum;
};

/***** private prototypes *****/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_popping_and_pushing_start_popping( void *util_thread_starter_thread_state );
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_popping_and_pushing_start_pushing( void *util_thread_starter_thread_state );





/****************************************************************************/
void test_lfds700_stack_popping_and_pushing( struct lfds700_list_asu_state *list_of_logical_processors, lfds700_pal_uint_t memory_in_megabytes )
{
  enum lfds700_misc_validity
    dvs = LFDS700_MISC_VALIDITY_VALID;

  lfds700_pal_uint_t
    loop,
    number_elements,
    number_logical_processors,
    subloop;

  struct lfds700_list_asu_element
    *lasue;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_stack_state
    ss;

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
  // TRD : memory_in_megabytes can be any value in its range

  /* TRD : we have two threads per CPU
           the threads loop for ten seconds
           the first thread pushes 10000 elements then pops 10000 elements
           the second thread pops 10000 elements then pushes 10000 elements
           all pushes and pops go onto the single main stack

           after time is up, all threads push what they have remaining onto
           the main stack

           we then validate the main stack
  */

  internal_display_test_name( "Popping and pushing (%d seconds)", TEST_DURATION_IN_SECONDS );

  lfds700_list_asu_query( list_of_logical_processors, LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, (void **) &number_logical_processors );

  lfds700_misc_prng_init( &ps );

  number_elements = ( memory_in_megabytes * ONE_MEGABYTE_IN_BYTES ) / ( sizeof(struct test_element) * number_logical_processors * 2 );

  lfds700_stack_init_valid_on_current_logical_core( &ss, NULL );

  te_array = util_aligned_malloc( sizeof(struct test_element) * number_elements * number_logical_processors, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  // TRD : some initial elements so the pushing threads can start immediately
  for( loop = 0 ; loop < number_elements * number_logical_processors ; loop++ )
  {
    (te_array+loop)->datum = loop;
    LFDS700_STACK_SET_VALUE_IN_ELEMENT( (te_array+loop)->se, te_array+loop );
    lfds700_stack_push( &ss, &(te_array+loop)->se, &ps );
  }

  ts = util_aligned_malloc( sizeof(struct test_state) * number_logical_processors * 2, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    // TRD : first set of threads
    (ts+loop)->ss = &ss;
    (ts+loop)->number_elements = number_elements;
    lfds700_stack_init_valid_on_current_logical_core( &(ts+loop)->ss_thread_local, NULL );

    // TRD : second set of threads
    (ts+loop+number_logical_processors)->ss = &ss;
    (ts+loop+number_logical_processors)->number_elements = number_elements;
    lfds700_stack_init_valid_on_current_logical_core( &(ts+loop+number_logical_processors)->ss_thread_local, NULL );

    // TRD : fill the pushing thread stacks
    (ts+loop+number_logical_processors)->ss_thread_local_te_array = util_aligned_malloc( sizeof(struct test_element) * number_elements, LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES );

    for( subloop = 0 ; subloop < number_elements ; subloop++ )
    {
      ((ts+loop+number_logical_processors)->ss_thread_local_te_array+subloop)->datum = loop;
      LFDS700_STACK_SET_VALUE_IN_ELEMENT( ((ts+loop+number_logical_processors)->ss_thread_local_te_array+subloop)->thread_local_se, (ts+loop+number_logical_processors)->ss_thread_local_te_array+subloop );
      lfds700_stack_push( &(ts+loop+number_logical_processors)->ss_thread_local, &((ts+loop+number_logical_processors)->ss_thread_local_te_array+subloop)->thread_local_se, &ps );
    }
  }

  thread_handles = util_malloc_wrapper( sizeof(test_pal_thread_state_t) * number_logical_processors * 2 );

  util_thread_starter_new( &tts, number_logical_processors * 2 );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  loop = 0;
  lasue = NULL;

  while( LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT(*list_of_logical_processors, lasue) )
  {
    lp = LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( *lasue );
    util_thread_starter_start( tts, &thread_handles[loop], loop, lp, thread_popping_and_pushing_start_popping, ts+loop );
    util_thread_starter_start( tts, &thread_handles[loop+number_logical_processors], loop+number_logical_processors, lp, thread_popping_and_pushing_start_pushing, ts+loop+number_logical_processors );
    loop++;
  }

  util_thread_starter_run( tts );

  for( loop = 0 ; loop < number_logical_processors * 2 ; loop++ )
    test_pal_thread_wait( thread_handles[loop] );

  util_thread_starter_delete( tts );

  free( thread_handles );

  LFDS700_MISC_BARRIER_LOAD;

  vi.min_elements = vi.max_elements = number_elements * number_logical_processors * 2;

  lfds700_stack_query( &ss, LFDS700_STACK_QUERY_SINGLETHREADED_VALIDATE, &vi, &dvs );

  lfds700_stack_cleanup( &ss, NULL );

  for( loop = 0 ; loop < number_logical_processors ; loop++ )
  {
    lfds700_stack_cleanup( &(ts+loop)->ss_thread_local, NULL );
    lfds700_stack_cleanup( &(ts+loop+number_logical_processors)->ss_thread_local, NULL );
    util_aligned_free( (ts+loop+number_logical_processors)->ss_thread_local_te_array );
  }

  util_aligned_free( ts );

  util_aligned_free( te_array );

  // TRD : print the test result
  internal_display_test_result( 1, "stack", dvs );

  return;
}






/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_popping_and_pushing_start_popping( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    count;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_stack_element
    *se;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  time_t
    start_time;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  util_thread_starter_ready_and_wait( tsts );

  start_time = time( NULL );

  while( time(NULL) < start_time + TEST_DURATION_IN_SECONDS )
  {
    count = 0;

    while( count < ts->number_elements )
      if( lfds700_stack_pop(ts->ss, &se, &ps) )
      {
        lfds700_stack_push( &ts->ss_thread_local, se, &ps );
        count++;
      }

    // TRD : return our local stack to the main stack
    while( lfds700_stack_pop(&ts->ss_thread_local, &se, &ps) )
      lfds700_stack_push( ts->ss, se, &ps );
  }

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}





/****************************************************************************/
static test_pal_thread_return_t TEST_PAL_CALLING_CONVENTION thread_popping_and_pushing_start_pushing( void *util_thread_starter_thread_state )
{
  lfds700_pal_uint_t
    count;

  struct lfds700_misc_prng_state
    ps;

  struct lfds700_stack_element
    *se;

  struct test_state
    *ts;

  struct util_thread_starter_thread_state
    *tsts;

  time_t
    start_time;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;

  assert( util_thread_starter_thread_state != NULL );

  tsts = (struct util_thread_starter_thread_state *) util_thread_starter_thread_state;
  ts = (struct test_state *) tsts->thread_user_state;

  lfds700_misc_prng_init( &ps );

  util_thread_starter_ready_and_wait( tsts );

  start_time = time( NULL );

  while( time(NULL) < start_time + TEST_DURATION_IN_SECONDS )
  {
    // TRD : return our local stack to the main stack
    while( lfds700_stack_pop(&ts->ss_thread_local, &se, &ps) )
      lfds700_stack_push( ts->ss, se, &ps );

    count = 0;

    while( count < ts->number_elements )
      if( lfds700_stack_pop(ts->ss, &se, &ps) )
      {
        lfds700_stack_push( &ts->ss_thread_local, se, &ps );
        count++;
      }
  }

  // TRD : now push whatever we have in our local stack
  while( lfds700_stack_pop(&ts->ss_thread_local, &se, &ps) )
    lfds700_stack_push( ts->ss, se, &ps );

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return( (test_pal_thread_return_t) EXIT_SUCCESS );
}

